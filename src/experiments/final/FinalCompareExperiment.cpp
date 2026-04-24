#include "experiments/final/FinalCompareExperiment.h"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

#include "SharingIndex.h"
#include "TreeIndex.h"

namespace {

struct MethodStats {
    double elapsedSec = 0.0;
    std::size_t totalResultSize = 0;
    std::size_t avgResultSize = 0;
};

std::string queryToString(const query_nodes& query) {
    std::vector<int> sorted(query.begin(), query.end());
    std::sort(sorted.begin(), sorted.end());
    std::string text;
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        text += std::to_string(sorted[i]);
        if (i + 1 < sorted.size()) {
            text += " ";
        }
    }
    return text;
}

query_group buildRandomQueryGroup(Graph& graph, int queryGroupCount, int queryNodeCount, unsigned long long randomSeed) {
    std::vector<int> nodes;
    nodes.reserve(static_cast<std::size_t>(graph.getn()));
    for (const auto& pair : graph.getAdj()) {
        nodes.push_back(pair.first);
    }

    query_group group;
    if (nodes.empty()) {
        return group;
    }

    const int actualGroupCount = (queryGroupCount > 0) ? queryGroupCount : 20;
    const int actualQueryNodeCount = (queryNodeCount > 0) ? queryNodeCount : 5;

    unsigned long long seed = randomSeed;
    if (seed == 0) {
        std::random_device rd;
        seed = (static_cast<unsigned long long>(rd()) << 32) ^ static_cast<unsigned long long>(rd());
    }

    std::mt19937_64 rng(seed);
    group.reserve(static_cast<std::size_t>(actualGroupCount));
    for (int i = 0; i < actualGroupCount; ++i) {
        std::shuffle(nodes.begin(), nodes.end(), rng);
        const int limit = std::min(actualQueryNodeCount, static_cast<int>(nodes.size()));
        query_nodes query;
        for (int j = 0; j < limit; ++j) {
            query.insert(nodes[j]);
        }
        if (!query.empty()) {
            group.push_back(std::move(query));
        }
    }
    return group;
}

std::string appendSuffixBeforeCsv(const std::string& path, const std::string& suffix) {
    const std::string ext = ".csv";
    if (path.size() >= ext.size() && path.substr(path.size() - ext.size()) == ext) {
        return path.substr(0, path.size() - ext.size()) + "_" + suffix + ext;
    }
    return path + "_" + suffix;
}

int minDegreeInResult(const Graph& graph, const std::unordered_set<int>& result) {
    if (result.empty()) {
        return 0;
    }
    int minDeg = std::numeric_limits<int>::max();
    for (int u : result) {
        int deg = 0;
        auto it = graph.getAdj().find(u);
        if (it != graph.getAdj().end()) {
            for (int v : it->second) {
                if (result.find(v) != result.end()) {
                    ++deg;
                }
            }
        }
        minDeg = std::min(minDeg, deg);
    }
    return (minDeg == std::numeric_limits<int>::max()) ? 0 : minDeg;
}

void writeDetailCsv(const std::string& path, const query_group& group,
                    const std::vector<std::unordered_set<int>>& results, const std::vector<int>& kValues) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return;
    }
    out << "Code,QueryNodes,ResultSize,K\n";
    for (std::size_t i = 0; i < group.size() && i < results.size(); ++i) {
        const int k = (i < kValues.size()) ? kValues[i] : -1;
        out << i << "," << queryToString(group[i]) << "," << results[i].size() << "," << k << "\n";
    }
}

MethodStats summarize(const query_group& group, const std::vector<std::unordered_set<int>>& results, double elapsedSec) {
    MethodStats stats;
    stats.elapsedSec = elapsedSec;
    for (const auto& result : results) {
        stats.totalResultSize += result.size();
    }
    stats.avgResultSize = group.empty() ? 0 : (stats.totalResultSize / group.size());
    return stats;
}

}  // namespace

namespace experiments {

void RunFinalCspCompareExperiment(Graph& graph, int queryGroupCount, int queryNodeCount,
                                  unsigned long long randomSeed, const std::string& outputCsvPath,
                                  bool includeGlobalLoop) {
    query_group group = buildRandomQueryGroup(graph, queryGroupCount, queryNodeCount, randomSeed);
    if (group.empty()) {
        std::cout << "Empty query group, skip." << std::endl;
        return;
    }

    std::vector<std::unordered_set<int>> globalResults;
    std::vector<int> globalK;
    MethodStats globalStats;
    if (includeGlobalLoop) {
        globalResults.reserve(group.size());
        globalK.reserve(group.size());
        const clock_t globalBegin = std::clock();
        for (const auto& query : group) {
            globalResults.push_back(graph.globalsearch(query));
            globalK.push_back(minDegreeInResult(graph, globalResults.back()));
        }
        const clock_t globalEnd = std::clock();
        globalStats = summarize(group, globalResults, static_cast<double>(globalEnd - globalBegin) / CLOCKS_PER_SEC);
    }

    std::vector<std::unordered_set<int>> retrievalResults;
    std::vector<int> retrievalK;
    retrievalResults.reserve(group.size());
    retrievalK.reserve(group.size());
    TreeIndex treeIndex(graph);
    const clock_t retrievalBegin = std::clock();
    for (const auto& query : group) {
        int k = 0;
        retrievalResults.push_back(treeIndex.retrievalShellStruct(query, k));
        retrievalK.push_back(k);
    }
    const clock_t retrievalEnd = std::clock();
    const MethodStats retrievalStats =
        summarize(group, retrievalResults, static_cast<double>(retrievalEnd - retrievalBegin) / CLOCKS_PER_SEC);

    std::vector<std::unordered_set<int>> batchResults(group.size());
    std::vector<int> batchK(group.size(), 0);
    SharingIndex batchIndex(graph);
    const clock_t batchBegin = std::clock();
    batchIndex.batchsearch(group);
    const clock_t batchEnd = std::clock();
    for (std::size_t i = 0; i < group.size(); ++i) {
        const auto it = batchIndex.queryToResult.find(static_cast<int>(i));
        if (it != batchIndex.queryToResult.end()) {
            batchResults[i] = it->second;
        }
        const auto kIt = batchIndex.codeToK.find(static_cast<int>(i));
        if (kIt != batchIndex.codeToK.end()) {
            batchK[i] = kIt->second;
        }
    }
    const MethodStats batchStats =
        summarize(group, batchResults, static_cast<double>(batchEnd - batchBegin) / CLOCKS_PER_SEC);

    std::vector<std::unordered_set<int>> roughResults(group.size());
    std::vector<int> roughK(group.size(), 0);
    SharingIndex sharingIndex(graph);
    const clock_t roughBegin = std::clock();
    sharingIndex.batchsearchRough(group);
    const clock_t roughEnd = std::clock();
    for (std::size_t i = 0; i < group.size(); ++i) {
        const auto it = sharingIndex.queryToResult.find(static_cast<int>(i));
        if (it != sharingIndex.queryToResult.end()) {
            roughResults[i] = it->second;
        }
        const auto kIt = sharingIndex.codeToK.find(static_cast<int>(i));
        if (kIt != sharingIndex.codeToK.end()) {
            roughK[i] = kIt->second;
        }
    }
    const MethodStats roughStats =
        summarize(group, roughResults, static_cast<double>(roughEnd - roughBegin) / CLOCKS_PER_SEC);

    const std::string retrievalDetail = appendSuffixBeforeCsv(outputCsvPath, "retrieval_loop_detail");
    const std::string batchDetail = appendSuffixBeforeCsv(outputCsvPath, "batchsearch_detail");
    const std::string roughDetail = appendSuffixBeforeCsv(outputCsvPath, "batchsearch_rough_detail");
    std::string globalDetail;
    if (includeGlobalLoop) {
        globalDetail = appendSuffixBeforeCsv(outputCsvPath, "global_loop_detail");
        writeDetailCsv(globalDetail, group, globalResults, globalK);
    }
    writeDetailCsv(retrievalDetail, group, retrievalResults, retrievalK);
    writeDetailCsv(batchDetail, group, batchResults, batchK);
    writeDetailCsv(roughDetail, group, roughResults, roughK);

    std::ofstream out(outputCsvPath);
    if (!out.is_open()) {
        return;
    }
    out << "Method,TimeSec,TotalResultSize,AvgResultSize,DetailCsv\n";
    if (includeGlobalLoop) {
        out << "global_loop," << globalStats.elapsedSec << "," << globalStats.totalResultSize << ","
            << globalStats.avgResultSize << "," << globalDetail << "\n";
    }
    out << "retrieval_loop," << retrievalStats.elapsedSec << "," << retrievalStats.totalResultSize << ","
        << retrievalStats.avgResultSize << "," << retrievalDetail << "\n";
    out << "batchsearch," << batchStats.elapsedSec << "," << batchStats.totalResultSize << ","
        << batchStats.avgResultSize << "," << batchDetail << "\n";
    out << "batchsearch_rough," << roughStats.elapsedSec << "," << roughStats.totalResultSize << ","
        << roughStats.avgResultSize << "," << roughDetail << "\n";

    std::cout << "[Final compare summary]" << std::endl;
    if (includeGlobalLoop) {
        std::cout << "  global_loop      : time=" << globalStats.elapsedSec << "s, total_size="
                  << globalStats.totalResultSize << ", avg_size=" << globalStats.avgResultSize << std::endl;
    }
    std::cout << "  retrieval_loop   : time=" << retrievalStats.elapsedSec << "s, total_size="
              << retrievalStats.totalResultSize << ", avg_size=" << retrievalStats.avgResultSize << std::endl;
    std::cout << "  batchsearch      : time=" << batchStats.elapsedSec << "s, total_size="
              << batchStats.totalResultSize << ", avg_size=" << batchStats.avgResultSize << std::endl;
    std::cout << "  batchsearch_rough: time=" << roughStats.elapsedSec << "s, total_size="
              << roughStats.totalResultSize << ", avg_size=" << roughStats.avgResultSize << std::endl;
}

}  // namespace experiments

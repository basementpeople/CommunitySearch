#include "experiments/core/ExperimentRunner.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>
#include <chrono>

#include "experiments/core/SingleQueryExperiment.h"
#include "experiments/final/FinalCompareExperiment.h"
#include "experiments/multi_query/MultiQueryMinCompareExperiment.h"
#include "SharingIndex.h"
#include "TreeIndex.h"

namespace {

constexpr const char* kOutputDir = "data\\output\\";

std::string defaultCsvPathForExperiment(const std::string& name) {
    if (name == "global") {
        return std::string(kOutputDir) + "single_query\\global_result.csv";
    }
    if (name == "retrieval") {
        return std::string(kOutputDir) + "single_query\\retrieval_result.csv";
    }
    if (name == "k-global") {
        return std::string(kOutputDir) + "single_query\\k_global_result.csv";
    }
    if (name == "greedy") {
        return std::string(kOutputDir) + "single_query\\greedy_result.csv";
    }
    if (name == "single_compare") {
        return std::string(kOutputDir) + "single_query\\single_compare_result.csv";
    }
    if (name == "batchsearch") {
        return std::string(kOutputDir) + "multi_query\\batchsearch_result.csv";
    }
    if (name == "batchsearch_rough") {
        return std::string(kOutputDir) + "multi_query\\batchsearch_rough_result.csv";
    }
    if (name == "multi_query_compare") {
        return std::string(kOutputDir) + "multi_query\\multi_query_compare_result.csv";
    }
    if (name == "multi_query_min_compare") {
        return std::string(kOutputDir) + "multi_query\\multi_query_min_compare_result.csv";
    }
    if (name == "manual_query_compare") {
        return std::string(kOutputDir) + "multi_query\\manual_query_compare_result.csv";
    }
    if (name == "final_csp_three_compare") {
        return std::string(kOutputDir) + "final\\final_csp_three_compare_result.csv";
    }
    if (name == "final_csp_four_compare") {
        return std::string(kOutputDir) + "final\\final_csp_four_compare_result.csv";
    }
    if (name == "final_min_csp_simi_sweep") {
        return std::string(kOutputDir) + "final\\final_min_csp_simi_sweep_result.csv";
    }
    if (name == "final_min_csp_fixed_simi_seed_sweep") {
        return std::string(kOutputDir) + "final\\final_min_csp_fixed_simi_seed_sweep_result.csv";
    }
    if (name == "final_min_csp_query_count_sweep") {
        return std::string(kOutputDir) + "final\\final_min_csp_query_count_sweep_result.csv";
    }
    return std::string(kOutputDir) + "single_query\\global_result.csv";
}

void ensureOutputDirectory(const std::string& outputPath) {
    const std::filesystem::path path(outputPath);
    if (!path.has_parent_path()) {
        return;
    }

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
}

query_nodes sampleRandomQuery(Graph& graph, int k, unsigned long long seed) {
    std::vector<int> nodes;
    nodes.reserve(static_cast<std::size_t>(graph.getn()));
    for (const auto& pair : graph.getAdj()) {
        nodes.push_back(pair.first);
    }

    if (nodes.empty() || k <= 0) {
        return {};
    }
    if (k >= static_cast<int>(nodes.size())) {
        return query_nodes(nodes.begin(), nodes.end());
    }

    std::mt19937_64 rng(seed);
    std::vector<int> picked;
    picked.reserve(static_cast<std::size_t>(k));
    std::sample(nodes.begin(), nodes.end(), std::back_inserter(picked), static_cast<std::size_t>(k), rng);
    return query_nodes(picked.begin(), picked.end());
}

query_nodes buildSingleQuery(Graph& graph, int randomQueryCount, unsigned long long randomSeed) {
    if (randomQueryCount <= 0) {
        return {300};
    }

    unsigned long long seed = randomSeed;
    if (seed == 0) {
        std::random_device rd;
        seed = (static_cast<unsigned long long>(rd()) << 32) ^ static_cast<unsigned long long>(rd());
    }

    std::cout << "Random query count: " << randomQueryCount << ", seed: " << seed << std::endl;
    query_nodes query = sampleRandomQuery(graph, randomQueryCount, seed);
    if (query.empty()) {
        std::cout << "Random query sampling failed (empty graph?), fallback to {300}." << std::endl;
        query = {300};
    }
    return query;
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

std::string queryToString(const query_nodes& query) {
    std::vector<int> nodes(query.begin(), query.end());
    std::sort(nodes.begin(), nodes.end());
    std::string text;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        text += std::to_string(nodes[i]);
        if (i + 1 < nodes.size()) {
            text += " ";
        }
    }
    return text;
}

int minDegreeInResult(Graph& graph, const std::unordered_set<int>& nodes) {
    if (nodes.empty()) {
        return 0;
    }
    std::unordered_set<int> copy = nodes;
    return graph.getMinDegree(copy);
}

struct MultiMethodStats {
    double timeSec = 0.0;
    std::size_t unionNodeCount = 0;
    std::size_t totalResultNodeCount = 0;
    std::size_t avgResultNodeCount = 0;
};

void writeMultiQueryDetailCsv(const std::string& path, const query_group& group,
                              const std::vector<std::unordered_set<int>>& results,
                              const std::vector<int>& kValues) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return;
    }
    out << "Code,QueryNodes,ResultSize,K\n";
    const std::size_t n = std::min(group.size(), results.size());
    for (std::size_t i = 0; i < n; ++i) {
        const int k = (i < kValues.size()) ? kValues[i] : 0;
        out << i << "," << queryToString(group[i]) << "," << results[i].size() << "," << k << "\n";
    }
}

MultiMethodStats buildStats(const query_group& group, const std::vector<std::unordered_set<int>>& results, double timeSec) {
    MultiMethodStats stats;
    stats.timeSec = timeSec;
    std::unordered_set<int> unionNodes;
    for (const auto& result : results) {
        stats.totalResultNodeCount += result.size();
        unionNodes.insert(result.begin(), result.end());
    }
    stats.unionNodeCount = unionNodes.size();
    stats.avgResultNodeCount = group.empty() ? 0 : (stats.totalResultNodeCount / group.size());
    return stats;
}

}  // namespace

namespace experiments {

void RunExperiment(const std::string& experimentName, Graph& graph, int randomQueryCount,
                   unsigned long long randomSeed, int queryNodeCount, double similarityThreshold,
                   bool useLegacyQueryBuilder, int shift1, int shift2, int beginPickCount,
                   int beginCandidatePoolCap, const std::string& manualQueryGroupPath) {
    (void)similarityThreshold;
    (void)useLegacyQueryBuilder;
    (void)shift1;
    (void)shift2;
    (void)beginPickCount;
    (void)beginCandidatePoolCap;
    (void)manualQueryGroupPath;

    if (experimentName == "batchsearch" || experimentName == "batchsearch_rough") {
        const bool rough = (experimentName == "batchsearch_rough");
        const std::string path = defaultCsvPathForExperiment(experimentName);
        ensureOutputDirectory(path);
        query_group group = buildRandomQueryGroup(graph, randomQueryCount, queryNodeCount, randomSeed);
        if (group.empty()) {
            std::cout << "Empty query group, skip." << std::endl;
            return;
        }
        std::cout << "Output CSV: " << path << std::endl;
        SharingIndex index(graph);
        const auto begin = std::chrono::steady_clock::now();
        if (rough) {
            index.batchsearchRough(group);
        } else {
            index.batchsearch(group);
        }
        index.printAndwrite(path);
        const auto end = std::chrono::steady_clock::now();
        const double elapsedSec = std::chrono::duration<double>(end - begin).count();
        std::cout << "Elapsed: " << std::fixed << std::setprecision(3) << elapsedSec << "s" << std::endl;
        return;
    }

    if (experimentName == "multi_query_compare") {
        const std::string summaryPath = defaultCsvPathForExperiment("multi_query_compare");
        ensureOutputDirectory(summaryPath);
        query_group group = buildRandomQueryGroup(graph, randomQueryCount, queryNodeCount, randomSeed);
        if (group.empty()) {
            std::cout << "Empty query group, skip." << std::endl;
            return;
        }

        const std::string batchDetail = appendSuffixBeforeCsv(summaryPath, "batchsearch_detail");
        const std::string roughDetail = appendSuffixBeforeCsv(summaryPath, "batchsearch_rough_detail");
        const std::string globalDetail = appendSuffixBeforeCsv(summaryPath, "globalsearch_loop_detail");
        const std::string retrievalDetail = appendSuffixBeforeCsv(summaryPath, "retrieval_loop_detail");
        ensureOutputDirectory(batchDetail);
        ensureOutputDirectory(roughDetail);
        ensureOutputDirectory(globalDetail);
        ensureOutputDirectory(retrievalDetail);

        std::vector<std::unordered_set<int>> batchResults(group.size());
        std::vector<int> batchK(group.size(), 0);
        {
            SharingIndex index(graph);
            const auto begin = std::chrono::steady_clock::now();
            index.batchsearch(group);
            const auto end = std::chrono::steady_clock::now();
            for (std::size_t i = 0; i < group.size(); ++i) {
                if (index.queryToResult.find(static_cast<int>(i)) != index.queryToResult.end()) {
                    batchResults[i] = index.queryToResult[static_cast<int>(i)];
                }
                if (index.codeToK.find(static_cast<int>(i)) != index.codeToK.end()) {
                    batchK[i] = index.codeToK[static_cast<int>(i)];
                }
            }
            const MultiMethodStats stats = buildStats(group, batchResults, std::chrono::duration<double>(end - begin).count());
            writeMultiQueryDetailCsv(batchDetail, group, batchResults, batchK);

            std::vector<std::unordered_set<int>> roughResults(group.size());
            std::vector<int> roughK(group.size(), 0);
            SharingIndex roughIndex(graph);
            const auto roughBegin = std::chrono::steady_clock::now();
            roughIndex.batchsearchRough(group);
            const auto roughEnd = std::chrono::steady_clock::now();
            for (std::size_t i = 0; i < group.size(); ++i) {
                if (roughIndex.queryToResult.find(static_cast<int>(i)) != roughIndex.queryToResult.end()) {
                    roughResults[i] = roughIndex.queryToResult[static_cast<int>(i)];
                }
                if (roughIndex.codeToK.find(static_cast<int>(i)) != roughIndex.codeToK.end()) {
                    roughK[i] = roughIndex.codeToK[static_cast<int>(i)];
                }
            }
            const MultiMethodStats roughStats =
                buildStats(group, roughResults, std::chrono::duration<double>(roughEnd - roughBegin).count());
            writeMultiQueryDetailCsv(roughDetail, group, roughResults, roughK);

            std::vector<std::unordered_set<int>> globalResults;
            std::vector<int> globalK;
            globalResults.reserve(group.size());
            globalK.reserve(group.size());
            const auto globalBegin = std::chrono::steady_clock::now();
            for (const auto& query : group) {
                const auto result = graph.globalsearch(query);
                globalK.push_back(minDegreeInResult(graph, result));
                globalResults.push_back(result);
            }
            const auto globalEnd = std::chrono::steady_clock::now();
            const MultiMethodStats globalStats =
                buildStats(group, globalResults, std::chrono::duration<double>(globalEnd - globalBegin).count());
            writeMultiQueryDetailCsv(globalDetail, group, globalResults, globalK);

            std::vector<std::unordered_set<int>> retrievalResults;
            std::vector<int> retrievalK;
            retrievalResults.reserve(group.size());
            retrievalK.reserve(group.size());
            TreeIndex treeIndex(graph);
            const auto retrievalBegin = std::chrono::steady_clock::now();
            for (const auto& query : group) {
                int k = 0;
                retrievalResults.push_back(treeIndex.retrievalShellStruct(query, k));
                retrievalK.push_back(k);
            }
            const auto retrievalEnd = std::chrono::steady_clock::now();
            const MultiMethodStats retrievalStats = buildStats(
                group, retrievalResults, std::chrono::duration<double>(retrievalEnd - retrievalBegin).count());
            writeMultiQueryDetailCsv(retrievalDetail, group, retrievalResults, retrievalK);

            std::ofstream out(summaryPath);
            if (out.is_open()) {
                out << "Method,TimeSec,UnionNodeCount,TotalResultNodeCount,AvgResultNodeCount,DetailCsv\n";
                out << "batchsearch," << stats.timeSec << "," << stats.unionNodeCount << "," << stats.totalResultNodeCount
                    << "," << stats.avgResultNodeCount << "," << batchDetail << "\n";
                out << "batchsearch_rough," << roughStats.timeSec << "," << roughStats.unionNodeCount << ","
                    << roughStats.totalResultNodeCount << "," << roughStats.avgResultNodeCount << "," << roughDetail
                    << "\n";
                out << "globalsearch_loop," << globalStats.timeSec << "," << globalStats.unionNodeCount << ","
                    << globalStats.totalResultNodeCount << "," << globalStats.avgResultNodeCount << "," << globalDetail
                    << "\n";
                out << "retrieval_loop," << retrievalStats.timeSec << "," << retrievalStats.unionNodeCount << ","
                    << retrievalStats.totalResultNodeCount << "," << retrievalStats.avgResultNodeCount << ","
                    << retrievalDetail << "\n";
            }
            std::cout << "Output CSV: " << summaryPath << std::endl;
            std::cout << "batchsearch elapsed: " << std::fixed << std::setprecision(3) << stats.timeSec << "s"
                      << std::endl;
            std::cout << "batchsearch_rough elapsed: " << std::fixed << std::setprecision(3) << roughStats.timeSec
                      << "s" << std::endl;
            std::cout << "globalsearch_loop elapsed: " << std::fixed << std::setprecision(3) << globalStats.timeSec
                      << "s" << std::endl;
            std::cout << "retrieval_loop elapsed: " << std::fixed << std::setprecision(3) << retrievalStats.timeSec
                      << "s" << std::endl;
            std::cout << "Elapsed: " << std::fixed << std::setprecision(3)
                      << (stats.timeSec + roughStats.timeSec + globalStats.timeSec + retrievalStats.timeSec) << "s"
                      << std::endl;
        }
        return;
    }

    if (experimentName == "multi_query_min_compare") {
        const std::string path = defaultCsvPathForExperiment(experimentName);
        ensureOutputDirectory(path);
        std::cout << "Output CSV: " << path << std::endl;
        RunMultiQueryMinCompareExperiment(graph, randomQueryCount, queryNodeCount, randomSeed, path,
                                          similarityThreshold, useLegacyQueryBuilder, shift1, shift2,
                                          beginPickCount, beginCandidatePoolCap);
        return;
    }

    if (experimentName == "manual_query_compare") {
        if (manualQueryGroupPath.empty()) {
            std::cerr << "manual_query_compare requires arg12: manualQueryGroupPath." << std::endl;
            return;
        }
        const std::string path = defaultCsvPathForExperiment(experimentName);
        ensureOutputDirectory(path);
        std::cout << "Output CSV: " << path << std::endl;
        std::cout << "Manual query group path: " << manualQueryGroupPath << std::endl;
        RunManualQueryGroupComparisonExperiment(graph, manualQueryGroupPath, path);
        return;
    }

    if (experimentName == "final_csp_three_compare") {
        const std::string path = defaultCsvPathForExperiment(experimentName);
        ensureOutputDirectory(path);
        std::cout << "Output CSV: " << path << std::endl;
        RunFinalCspCompareExperiment(graph, randomQueryCount, queryNodeCount, randomSeed, path, false);
        return;
    }

    if (experimentName == "final_csp_four_compare") {
        const std::string path = defaultCsvPathForExperiment(experimentName);
        ensureOutputDirectory(path);
        std::cout << "Output CSV: " << path << std::endl;
        RunFinalCspCompareExperiment(graph, randomQueryCount, queryNodeCount, randomSeed, path, true);
        return;
    }

    if (experimentName == "final_min_csp_simi_sweep") {
        const std::string path = defaultCsvPathForExperiment(experimentName);
        ensureOutputDirectory(path);
        std::cout << "Output CSV: " << path << std::endl;
        RunFinalMinCspSimiSweepExperiment(graph, randomQueryCount, queryNodeCount, randomSeed, path,
                                          useLegacyQueryBuilder, shift1, shift2, beginPickCount,
                                          beginCandidatePoolCap);
        return;
    }

    if (experimentName == "final_min_csp_fixed_simi_seed_sweep") {
        if (std::isnan(similarityThreshold)) {
            std::cerr
                << "final_min_csp_fixed_simi_seed_sweep: arg6 (fixedClusteringSimi) is required; cannot be omitted.\n";
            return;
        }
        const std::string path = defaultCsvPathForExperiment(experimentName);
        ensureOutputDirectory(path);
        std::cout << "Output CSV: " << path << std::endl;
        RunFinalMinCspFixedSimiSeedSweepExperiment(graph, randomQueryCount, queryNodeCount, randomSeed, path,
                                                   similarityThreshold, useLegacyQueryBuilder, shift1, shift2,
                                                   beginPickCount, beginCandidatePoolCap);
        return;
    }

    if (experimentName == "final_min_csp_query_count_sweep") {
        if (std::isnan(similarityThreshold)) {
            std::cerr
                << "final_min_csp_query_count_sweep: arg6 (fixedClusteringSimi) is required; cannot be omitted.\n";
            return;
        }
        const std::string path = defaultCsvPathForExperiment(experimentName);
        ensureOutputDirectory(path);
        std::cout << "Output CSV: " << path << std::endl;
        RunFinalMinCspQueryCountSweepExperiment(graph, queryNodeCount, randomSeed, path, similarityThreshold,
                                                useLegacyQueryBuilder, shift1, shift2, beginPickCount,
                                                beginCandidatePoolCap);
        return;
    }

    query_nodes query = buildSingleQuery(graph, randomQueryCount, randomSeed);
    if (query.empty()) {
        std::cout << "Empty query, skip." << std::endl;
        return;
    }

    if (experimentName == "retrieval") {
        const std::string path = defaultCsvPathForExperiment("retrieval");
        ensureOutputDirectory(path);
        std::cout << "Output CSV: " << path << std::endl;
        RunRetrievalShellExperiment(graph, query, path);
        return;
    }

    if (experimentName == "k-global") {
        const std::string path = defaultCsvPathForExperiment("k-global");
        ensureOutputDirectory(path);
        const int kCount = (queryNodeCount > 0) ? queryNodeCount : std::max(1, static_cast<int>(query.size()));
        std::cout << "Output CSV: " << path << std::endl;
        RunKGlobalSearchExperiment(graph, query, kCount, path);
        return;
    }

    if (experimentName == "greedy") {
        const std::string path = defaultCsvPathForExperiment("greedy");
        ensureOutputDirectory(path);
        std::cout << "Output CSV: " << path << std::endl;
        RunGreedyConnectionExperiment(graph, query, path);
        return;
    }

    if (experimentName == "single_compare") {
        const std::string path = defaultCsvPathForExperiment("single_compare");
        ensureOutputDirectory(path);
        std::cout << "Output CSV: " << path << std::endl;
        RunSingleQueryComparisonExperiment(graph, query, path);
        return;
    }

    if (experimentName != "global") {
        std::cout << "Unknown experiment '" << experimentName
                  << "', fallback to default 'global'." << std::endl;
    }

    const std::string path = defaultCsvPathForExperiment("global");
    ensureOutputDirectory(path);
    std::cout << "Output CSV: " << path << std::endl;
    RunGlobalSearchExperiment(graph, query, path);
}

}  // namespace experiments

#include "experiments/multi_query/MultiQueryMinCompareExperiment.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iostream>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SharingIndex.h"
#include "TreeIndex.h"

extern double simi;
extern double time_cluster, time_1, time_2, time_3, time_4, clu_1, clu_2;

namespace {

constexpr int kDefaultQueryGroupCount = 20;
constexpr int kDefaultQuerySize = 5;

struct MethodStats {
    double elapsedSec = 0.0;
    double stageClusterSec = 0.0;
    double stageGreedyStepSec = 0.0;
    double stageSteinerSec = 0.0;
    double stageSecondClusteringSec = 0.0;
    double stageGreedySimplySec = 0.0;
    std::size_t firstClusterCount = 0;
    std::size_t secondClusterCount = 0; // Fast: sum of CSP sub-cluster counts; else 0
    std::size_t unionNodeCount = 0;
    std::size_t totalResultNodeCount = 0;
    std::size_t avgResultNodeCount = 0;
};

struct QueryRecord {
    int code = -1;
    std::size_t resultSize = 0;
    int k = -1;
    int componentId = -1;
};

struct MethodRunResult {
    MethodStats stats;
    std::vector<QueryRecord> records;
};

std::string QueryToString(const query_nodes& query) {
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

std::string AppendSuffixBeforeCsv(const std::string& path, const std::string& suffix) {
    const std::string ext = ".csv";
    if (path.size() >= ext.size() && path.substr(path.size() - ext.size()) == ext) {
        return path.substr(0, path.size() - ext.size()) + "_" + suffix + ext;
    }
    return path + "_" + suffix;
}

query_group BuildRandomQueryGroup(Graph& graph, int queryGroupCount, int querySize, unsigned long long seed) {
    std::vector<int> nodes;
    nodes.reserve(static_cast<std::size_t>(graph.getn()));
    for (const auto& pair : graph.getAdj()) {
        nodes.push_back(pair.first);
    }

    query_group group;
    if (nodes.empty() || queryGroupCount <= 0 || querySize <= 0) {
        return group;
    }

    std::mt19937_64 rng(seed);
    for (int i = 0; i < queryGroupCount; ++i) {
        std::shuffle(nodes.begin(), nodes.end(), rng);
        const int limit = std::min<int>(querySize, static_cast<int>(nodes.size()));
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

// legacy 模式
int ResolveBeginCandidatePoolCap(int beginCandidatePoolCap, int beginPickCount) {
    if (beginCandidatePoolCap > 0) {
        return beginCandidatePoolCap;
    }
    return std::max(1, beginPickCount) * 2;
}

query_group BuildLegacyStyleQueryGroup(Graph& graph, int queryGroupCount, int querySize, unsigned long long seed,
                                       int shift1, int shift2, int beginPickCount, int beginCandidatePoolCap) {
    std::vector<int> nodes;
    nodes.reserve(static_cast<std::size_t>(graph.getn()));
    for (const auto& pair : graph.getAdj()) {
        nodes.push_back(pair.first);
    }

    query_group group;
    if (nodes.empty() || queryGroupCount <= 0 || querySize <= 0) {
        return group;
    }

    std::unordered_map<int, int> nodeToCore;
    nodeToCore = CoreGroup::coreDecomposition(graph);
    int coreMax = 0;
    for (const auto& item : nodeToCore) {
        coreMax = std::max(coreMax, item.second);
    }

    std::mt19937_64 rng(seed);
    std::shuffle(nodes.begin(), nodes.end(), rng);

    const int beginPoolTarget = ResolveBeginCandidatePoolCap(beginCandidatePoolCap, beginPickCount);

    // 生成起始点候选集
    std::vector<int> beginCandidates;
    beginCandidates.reserve(nodes.size());
    std::unordered_set<int> visitedAroundBegin;
    for (int node : nodes) {
        if (static_cast<int>(beginCandidates.size()) >= beginPoolTarget) {
            break;
        }
        if (visitedAroundBegin.find(node) != visitedAroundBegin.end()) {
            continue;
        }
        beginCandidates.push_back(node);
        std::queue<std::pair<int, int>> q;
        q.push({node, 0});
        visitedAroundBegin.insert(node);
        while (!q.empty()) {
            const int u = q.front().first;
            const int depth = q.front().second;
            q.pop();
            if (depth >= 3) {
                continue;
            }
            auto adjIt = graph.getAdj().find(u);
            if (adjIt == graph.getAdj().end()) {
                continue;
            }
            for (int v : adjIt->second) {
                if (visitedAroundBegin.insert(v).second) {
                    q.push({v, depth + 1});
                }
            }
        }
    }
    if (beginCandidates.empty()) {
        beginCandidates = nodes;
    }

    // Build query groups from beginCandidates.
    for (int i = 0; i < queryGroupCount; ++i) {
        std::vector<int> localBegin = beginCandidates;
        std::shuffle(localBegin.begin(), localBegin.end(), rng);
        query_nodes query;

        const int beginLimit = std::min<int>(std::max(1, beginPickCount), static_cast<int>(localBegin.size()));
        int minCore = std::numeric_limits<int>::max();
        int maxCore = std::numeric_limits<int>::min();
        for (int j = 0; j < beginLimit && static_cast<int>(query.size()) < querySize; ++j) {
            int node = localBegin[j];
            query.insert(node);
            auto coreIt = nodeToCore.find(node);
            if (coreIt != nodeToCore.end()) {
                minCore = std::min(minCore, coreIt->second);
                maxCore = std::max(maxCore, coreIt->second);
            }
        }
        if (query.empty()) {
            continue;
        }
        if (minCore == std::numeric_limits<int>::max()) {
            minCore = 0;
            maxCore = coreMax;
        }
        const int lowerCore = std::max(0, minCore - shift1);
        const int upperCore = std::min(coreMax, maxCore + shift2);

        std::vector<int> queryVec(query.begin(), query.end());
        for (int u : queryVec) {
            if (static_cast<int>(query.size()) >= querySize) {
                break;
            }
            auto adjIt = graph.getAdj().find(u);
            if (adjIt == graph.getAdj().end()) {
                continue;
            }
            std::vector<int> neighbors(adjIt->second.begin(), adjIt->second.end());
            std::shuffle(neighbors.begin(), neighbors.end(), rng);
            for (int v : neighbors) {
                if (static_cast<int>(query.size()) >= querySize) {
                    break;
                }
                if (query.find(v) != query.end()) {
                    continue;
                }
                auto coreIt = nodeToCore.find(v);
                if (coreIt == nodeToCore.end()) {
                    continue;
                }
                if (coreIt->second >= lowerCore && coreIt->second <= upperCore) {
                    query.insert(v);
                }
            }
        }

        if (static_cast<int>(query.size()) < querySize) {
            std::vector<int> fallback = nodes;
            std::shuffle(fallback.begin(), fallback.end(), rng);
            for (int v : fallback) {
                if (static_cast<int>(query.size()) >= querySize) {
                    break;
                }
                if (query.find(v) != query.end()) {
                    continue;
                }
                auto coreIt = nodeToCore.find(v);
                if (coreIt == nodeToCore.end()) {
                    continue;
                }
                if (coreIt->second >= lowerCore && coreIt->second <= upperCore) {
                    query.insert(v);
                }
            }
        }

        if (static_cast<int>(query.size()) < querySize) {
            std::vector<int> fallback = nodes;
            std::shuffle(fallback.begin(), fallback.end(), rng);
            for (int v : fallback) {
                if (static_cast<int>(query.size()) >= querySize) {
                    break;
                }
                query.insert(v);
            }
        }

        group.push_back(std::move(query));
    }
    return group;
}

void FinalizeStats(MethodStats& stats, const query_group& group) {
    stats.avgResultNodeCount = group.empty() ? 0 : (stats.totalResultNodeCount / group.size());
}

void WriteMethodDetailCsv(const std::string& path, const query_group& group, const MethodRunResult& result) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return;
    }
    out << "Code,QueryNodes,ResultSize,K,ComponentId\n";
    for (std::size_t i = 0; i < result.records.size() && i < group.size(); ++i) {
        out << result.records[i].code << "," << QueryToString(group[i]) << "," << result.records[i].resultSize << ","
            << result.records[i].k << "," << result.records[i].componentId << "\n";
    }
    out.close();
}

void AppendSimiSweepMethodDetailRows(std::ofstream& out, int similarityPct, double similarity, double batchTimeSec,
                                     const query_group& group, const MethodRunResult& result) {
    for (std::size_t i = 0; i < result.records.size() && i < group.size(); ++i) {
        out << similarityPct << "," << similarity << "," << batchTimeSec << "," << result.records[i].code << ","
            << QueryToString(group[i]) << "," << result.records[i].resultSize << "," << result.records[i].k << ","
            << result.records[i].componentId << "\n";
    }
}

unsigned long long DeriveSeedForFixedSimiSweepSlot(unsigned long long baseSeed, int slotIndex) {
    const auto k = static_cast<unsigned long long>(slotIndex + 1);
    return baseSeed ^ (k * 0xD6E8FEB8663FD96AULL) ^ (k * k * 0x9E3779B97F4A7C15ULL);
}

void WriteSlotMethodDetailCsv(const std::string& path, int sweepSlotPct, unsigned long long derivedSeed, double estSim,
                              double fixedClusteringSim, const query_group& group, const MethodRunResult& result) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return;
    }
    out << "SweepSlotPct,DerivedSeed,EstGroupSimilarity,FixedClusteringSim,TimeSec,Code,QueryNodes,ResultSize,K,"
           "ComponentId\n";
    for (std::size_t i = 0; i < result.records.size() && i < group.size(); ++i) {
        out << sweepSlotPct << "," << derivedSeed << "," << estSim << "," << fixedClusteringSim << ","
            << result.stats.elapsedSec << "," << result.records[i].code << "," << QueryToString(group[i]) << ","
            << result.records[i].resultSize << "," << result.records[i].k << "," << result.records[i].componentId
            << "\n";
    }
    out.close();
}

MethodRunResult RunPrecise(Graph& graph, query_group group) {
    MethodRunResult runResult;
    SharingIndex index(graph);
    index.queryToResult.clear();

    const clock_t begin = clock();
    index.batchsearch(group);
    index.batchMinsearchPrecise(group);
    const clock_t end = clock();

    std::unordered_set<int> unionNodes;
    for (int code = 0; code < index.codeCount; ++code) {
        const auto it = index.queryToResult.find(code);
        if (it != index.queryToResult.end()) {
            runResult.stats.totalResultNodeCount += it->second.size();
            unionNodes.insert(it->second.begin(), it->second.end());
            const auto kIt = index.codeToK.find(code);
            const auto cIt = index.codeToCom.find(code);
            runResult.records.push_back({code, it->second.size(), kIt != index.codeToK.end() ? kIt->second : -1,
                                         cIt != index.codeToCom.end() ? cIt->second : -1});
        } else {
            runResult.records.push_back({code, 0, -1, -1});
        }
    }
    runResult.stats.elapsedSec = static_cast<double>(end - begin) / CLOCKS_PER_SEC;
    runResult.stats.stageClusterSec = time_cluster;
    runResult.stats.stageGreedyStepSec = time_1;
    runResult.stats.stageSteinerSec = time_2;
    // Precise pipeline has no second clustering stage.
    runResult.stats.stageSecondClusteringSec = 0.0;
    runResult.stats.stageGreedySimplySec = time_4;
    runResult.stats.firstClusterCount = static_cast<std::size_t>(clu_1);
    runResult.stats.secondClusterCount = 0;
    runResult.stats.unionNodeCount = unionNodes.size();
    FinalizeStats(runResult.stats, group);
    return runResult;
}

MethodRunResult RunFast(Graph& graph, query_group group) {
    MethodRunResult runResult;
    SharingIndex index(graph);
    index.queryToResult.clear();

    const clock_t begin = clock();
    index.batchsearch(group);
    index.batchMinsearchFast(group);
    const clock_t end = clock();

    std::unordered_set<int> unionNodes;
    for (int code = 0; code < index.codeCount; ++code) {
        const auto it = index.queryToResult.find(code);
        if (it != index.queryToResult.end()) {
            runResult.stats.totalResultNodeCount += it->second.size();
            unionNodes.insert(it->second.begin(), it->second.end());
            const auto kIt = index.codeToK.find(code);
            const auto cIt = index.codeToCom.find(code);
            runResult.records.push_back({code, it->second.size(), kIt != index.codeToK.end() ? kIt->second : -1,
                                         cIt != index.codeToCom.end() ? cIt->second : -1});
        } else {
            runResult.records.push_back({code, 0, -1, -1});
        }
    }
    runResult.stats.elapsedSec = static_cast<double>(end - begin) / CLOCKS_PER_SEC;
    runResult.stats.stageClusterSec = time_cluster;
    runResult.stats.stageGreedyStepSec = time_1;
    runResult.stats.stageSteinerSec = time_2;
    runResult.stats.stageSecondClusteringSec = time_3;
    runResult.stats.stageGreedySimplySec = time_4;
    runResult.stats.firstClusterCount = static_cast<std::size_t>(clu_1);
    runResult.stats.secondClusterCount = static_cast<std::size_t>(clu_2);
    runResult.stats.unionNodeCount = unionNodes.size();
    FinalizeStats(runResult.stats, group);
    return runResult;
}

MethodRunResult RunGreedyLoop(Graph& graph, const query_group& group) {
    MethodRunResult runResult;
    std::unordered_set<int> unionNodes;
    TreeIndex index(graph);
    const clock_t begin = clock();
    for (std::size_t i = 0; i < group.size(); ++i) {
        int k = 0;
        std::unordered_set<int> H = index.retrievalShellStruct(group[i], k);
        std::unordered_set<int> result;
        if (!H.empty()) {
            result = index.greedyConnection(group[i], k, H);
        }
        runResult.stats.totalResultNodeCount += result.size();
        unionNodes.insert(result.begin(), result.end());
        runResult.records.push_back({static_cast<int>(i), result.size(), k, -1});
    }
    const clock_t end = clock();
    runResult.stats.elapsedSec = static_cast<double>(end - begin) / CLOCKS_PER_SEC;
    runResult.stats.unionNodeCount = unionNodes.size();
    FinalizeStats(runResult.stats, group);
    return runResult;
}

}  // namespace

namespace experiments {

void RunMultiQueryMinCompareExperiment(Graph& graph, int queryGroupCount, int queryNodeCount,
                                       unsigned long long randomSeed, const std::string& outputCsvPath,
                                       double similarityThreshold, bool useLegacyQueryBuilder, int shift1,
                                       int shift2, int beginPickCount, int beginCandidatePoolCap) {
    const int actualGroupCount = (queryGroupCount > 0) ? queryGroupCount : kDefaultQueryGroupCount;
    const int actualQuerySize = (queryNodeCount > 0) ? queryNodeCount : kDefaultQuerySize;
    unsigned long long seed = randomSeed;
    if (seed == 0) {
        std::random_device rd;
        seed = (static_cast<unsigned long long>(rd()) << 32) ^ static_cast<unsigned long long>(rd());
    }

    std::cout << "\n===== test_multi_query_min_compare =====" << std::endl;
    std::cout << "[Config] \u67e5\u8be2\u7ec4\u6570\u91cf=" << actualGroupCount
              << ", \u6bcf\u7ec4\u70b9\u6570=" << actualQuerySize
              << ", \u968f\u673a\u79cd\u5b50=" << seed << std::endl;
    std::cout << "[Config] query_builder=" << (useLegacyQueryBuilder ? "legacy" : "random");
    if (useLegacyQueryBuilder) {
        std::cout << ", shift_1=" << shift1 << ", shift_2=" << shift2
                  << ", begin_pick_count=" << beginPickCount
                  << ", begin_candidate_pool_cap=" << beginCandidatePoolCap
                  << " (effective=" << ResolveBeginCandidatePoolCap(beginCandidatePoolCap, beginPickCount) << ")";
    }
    std::cout << std::endl;

    query_group group =
        useLegacyQueryBuilder
            ? BuildLegacyStyleQueryGroup(graph, actualGroupCount, actualQuerySize, seed, shift1, shift2,
                                         beginPickCount, beginCandidatePoolCap)
            : BuildRandomQueryGroup(graph, actualGroupCount, actualQuerySize, seed);
    if (group.empty()) {
        std::cout << "\u672a\u751f\u6210\u67e5\u8be2\uff0c\u53ef\u80fd\u662f\u56fe\u4e3a\u7a7a\u3002" << std::endl;
        return;
    }

    if (std::isnan(similarityThreshold)) {
        SharingIndex thresholdIndex(graph);
        simi = thresholdIndex.groupSimilarity(group, group);
    } else {
        simi = similarityThreshold;
    }
    std::cout << "[Config] \u76f8\u4f3c\u5ea6\u9608\u503c=" << simi << std::endl;

    MethodRunResult precise = RunPrecise(graph, group);
    MethodRunResult fast = RunFast(graph, group);
    MethodRunResult greedyLoop = RunGreedyLoop(graph, group);

    const std::string preciseDetailPath = AppendSuffixBeforeCsv(outputCsvPath, "precise_detail");
    const std::string fastDetailPath = AppendSuffixBeforeCsv(outputCsvPath, "fast_detail");
    const std::string greedyDetailPath = AppendSuffixBeforeCsv(outputCsvPath, "greedy_loop_detail");
    WriteMethodDetailCsv(preciseDetailPath, group, precise);
    WriteMethodDetailCsv(fastDetailPath, group, fast);
    WriteMethodDetailCsv(greedyDetailPath, group, greedyLoop);

    std::ofstream out(outputCsvPath);
    if (!out.is_open()) {
        std::cerr << "\u65e0\u6cd5\u6253\u5f00\u8f93\u51fa CSV: " << outputCsvPath << std::endl;
        return;
    }
    out << "Method,TimeSec,StageClusterSec,StageGreedyStepSec,StageSteinerSec,StageSecondClusteringSec,StageGreedySimplySec,FirstClusterCount,UnionNodeCount,TotalResultNodeCount,AvgResultNodeCount,DetailCsv\n";
    out << "batchmin_precise," << precise.stats.elapsedSec << "," << precise.stats.stageClusterSec << ","
        << precise.stats.stageGreedyStepSec << "," << precise.stats.stageSteinerSec << ","
        << precise.stats.stageSecondClusteringSec << "," << precise.stats.stageGreedySimplySec << ","
        << precise.stats.firstClusterCount << "," << precise.stats.unionNodeCount << ","
        << precise.stats.totalResultNodeCount << "," << precise.stats.avgResultNodeCount << ","
        << preciseDetailPath << "\n";
    out << "batchmin_fast," << fast.stats.elapsedSec << "," << fast.stats.stageClusterSec << ","
        << fast.stats.stageGreedyStepSec << "," << fast.stats.stageSteinerSec << ","
        << fast.stats.stageSecondClusteringSec << "," << fast.stats.stageGreedySimplySec << ","
        << fast.stats.firstClusterCount << "," << fast.stats.unionNodeCount << "," << fast.stats.totalResultNodeCount
        << "," << fast.stats.avgResultNodeCount << "," << fastDetailPath << "\n";
    out << "greedy_loop," << greedyLoop.stats.elapsedSec << ",0,0,0,0,0,0," << greedyLoop.stats.unionNodeCount << ","
        << greedyLoop.stats.totalResultNodeCount << "," << greedyLoop.stats.avgResultNodeCount << ","
        << greedyDetailPath << "\n";
    out << "SUMMARY,seed=" << seed << ",query_group_count=" << actualGroupCount << ",query_size=" << actualQuerySize
        << ",similarity_threshold=" << simi;
    if (useLegacyQueryBuilder) {
        out << ",begin_candidate_pool_cap=" << beginCandidatePoolCap
            << ",begin_candidates_target=" << ResolveBeginCandidatePoolCap(beginCandidatePoolCap, beginPickCount);
    }
    out << ",-,-,-,-,-,-\n";
    out.close();

    std::cout << "[Summary]\n"
              << "  precise       : time=" << precise.stats.elapsedSec << "s"
              << " (cluster=" << precise.stats.stageClusterSec
              << "s, shared_greedy=" << precise.stats.stageGreedyStepSec
              << "s, steiner=" << precise.stats.stageSteinerSec
              << "s, second_cluster=N/A"
              << ", greedy_simply=" << precise.stats.stageGreedySimplySec << "s)\n"
              << "  fast          : time=" << fast.stats.elapsedSec << "s"
              << " (cluster=" << fast.stats.stageClusterSec
              << "s, shared_greedy=" << fast.stats.stageGreedyStepSec
              << "s, steiner=" << fast.stats.stageSteinerSec
              << "s, second_cluster=" << fast.stats.stageSecondClusteringSec
              << "s, greedy_simply=" << fast.stats.stageGreedySimplySec << "s)\n"
              << "  greedy_loop   : time=" << greedyLoop.stats.elapsedSec << "s" << std::endl;
    std::cout << "Output CSV: " << outputCsvPath << std::endl;
    std::cout << "===== end test_multi_query_min_compare =====\n" << std::endl;
}

void RunFinalMinCspSimiSweepExperiment(Graph& graph, int queryGroupCount, int queryNodeCount,
                                      unsigned long long randomSeed, const std::string& outputCsvPath,
                                      bool useLegacyQueryBuilder, int shift1, int shift2, int beginPickCount,
                                      int beginCandidatePoolCap) {
    const int actualGroupCount = (queryGroupCount > 0) ? queryGroupCount : kDefaultQueryGroupCount;
    const int actualQuerySize = (queryNodeCount > 0) ? queryNodeCount : kDefaultQuerySize;
    unsigned long long seed = randomSeed;
    if (seed == 0) {
        std::random_device rd;
        seed = (static_cast<unsigned long long>(rd()) << 32) ^ static_cast<unsigned long long>(rd());
    }

    std::cout << "\n===== final_min_csp_simi_sweep =====" << std::endl;
    std::cout << "[Config] \u67e5\u8be2\u7ec4\u6570\u91cf=" << actualGroupCount
              << ", \u6bcf\u7ec4\u70b9\u6570=" << actualQuerySize << ", \u968f\u673a\u79cd\u5b50=" << seed
              << std::endl;
    std::cout << "[Config] query_builder=" << (useLegacyQueryBuilder ? "legacy" : "random");
    if (useLegacyQueryBuilder) {
        std::cout << ", shift_1=" << shift1 << ", shift_2=" << shift2 << ", begin_pick_count=" << beginPickCount
                  << ", begin_candidate_pool_cap=" << beginCandidatePoolCap
                  << " (effective=" << ResolveBeginCandidatePoolCap(beginCandidatePoolCap, beginPickCount) << ")";
    }
    std::cout << std::endl;
    std::cout << "[Config] similarity sweep: 0%..90% step 10% (simi 0.0..0.9 step 0.1)" << std::endl;

    query_group group =
        useLegacyQueryBuilder ? BuildLegacyStyleQueryGroup(graph, actualGroupCount, actualQuerySize, seed, shift1,
                                                           shift2, beginPickCount, beginCandidatePoolCap)
                              : BuildRandomQueryGroup(graph, actualGroupCount, actualQuerySize, seed);
    if (group.empty()) {
        std::cout << "\u672a\u751f\u6210\u67e5\u8be2\uff0c\u53ef\u80fd\u662f\u56fe\u4e3a\u7a7a\u3002" << std::endl;
        return;
    }
    SharingIndex estIndex(graph);
    const double estGroupSimilarity = estIndex.groupSimilarity(group, group);
    std::cout << "[Config] estimated_group_similarity(group,group)=" << estGroupSimilarity << std::endl;

    auto resetStageTimers = []() {
        time_cluster = time_1 = time_2 = time_3 = time_4 = 0.0;
        clu_1 = clu_2 = 0.0;
    };

    MethodRunResult greedyOnce = RunGreedyLoop(graph, group);

    const std::string preciseDetailPath = AppendSuffixBeforeCsv(outputCsvPath, "precise_detail");
    const std::string fastDetailPath = AppendSuffixBeforeCsv(outputCsvPath, "fast_detail");
    const std::string greedyDetailPath = AppendSuffixBeforeCsv(outputCsvPath, "greedy_loop_detail");

    std::ofstream preciseDetail(preciseDetailPath);
    std::ofstream fastDetail(fastDetailPath);
    if (!preciseDetail.is_open() || !fastDetail.is_open()) {
        std::cerr << "\u65e0\u6cd5\u6253\u5f00\u660e\u7ec6 CSV\u3002" << std::endl;
        return;
    }
    preciseDetail << "SimilarityPct,Similarity,TimeSec,Code,QueryNodes,ResultSize,K,ComponentId\n";
    fastDetail << "SimilarityPct,Similarity,TimeSec,Code,QueryNodes,ResultSize,K,ComponentId\n";

    std::ofstream out(outputCsvPath);
    if (!out.is_open()) {
        std::cerr << "\u65e0\u6cd5\u6253\u5f00\u8f93\u51fa CSV: " << outputCsvPath << std::endl;
        return;
    }
    out << "SimilarityPct,Similarity,BatchminPrecise_TimeSec,BatchminPrecise_UnionNodeCount,"
           "BatchminPrecise_TotalResultNodeCount,BatchminFast_TimeSec,BatchminFast_UnionNodeCount,"
           "BatchminFast_TotalResultNodeCount,GreedyLoop_TimeSec,GreedyLoop_UnionNodeCount,"
           "GreedyLoop_TotalResultNodeCount\n";

    for (int pct = 0; pct <= 90; pct += 10) {
        const double s = static_cast<double>(pct) / 100.0;
        simi = s;
        resetStageTimers();
        MethodRunResult precise = RunPrecise(graph, group);
        resetStageTimers();
        MethodRunResult fast = RunFast(graph, group);

        AppendSimiSweepMethodDetailRows(preciseDetail, pct, s, precise.stats.elapsedSec, group, precise);
        AppendSimiSweepMethodDetailRows(fastDetail, pct, s, fast.stats.elapsedSec, group, fast);

        out << pct << "," << s << "," << precise.stats.elapsedSec << "," << precise.stats.unionNodeCount << ","
            << precise.stats.totalResultNodeCount << "," << fast.stats.elapsedSec << "," << fast.stats.unionNodeCount
            << "," << fast.stats.totalResultNodeCount << "," << greedyOnce.stats.elapsedSec << ","
            << greedyOnce.stats.unionNodeCount << "," << greedyOnce.stats.totalResultNodeCount << "\n";

        std::cout << "[simi=" << s << "] precise t=" << precise.stats.elapsedSec << "s"
                  << " clusters_simi=" << precise.stats.firstClusterCount << ", fast t=" << fast.stats.elapsedSec
                  << "s clusters_simi=" << fast.stats.firstClusterCount
                  << " clusters_csp_sum=" << fast.stats.secondClusterCount
                  << ", greedy t=" << greedyOnce.stats.elapsedSec << "s (constant across simi)" << std::endl;
    }

    preciseDetail.close();
    fastDetail.close();

    std::ofstream greedyDetail(greedyDetailPath);
    if (!greedyDetail.is_open()) {
        std::cerr << "\u65e0\u6cd5\u6253\u5f00 greedy \u660e\u7ec6 CSV: " << greedyDetailPath << std::endl;
    } else {
        greedyDetail << "SimilarityPct,Similarity,TimeSec,Code,QueryNodes,ResultSize,K,ComponentId\n";
        for (std::size_t i = 0; i < greedyOnce.records.size() && i < group.size(); ++i) {
            greedyDetail << "-,-," << greedyOnce.stats.elapsedSec << "," << greedyOnce.records[i].code << ","
                         << QueryToString(group[i]) << "," << greedyOnce.records[i].resultSize << ","
                         << greedyOnce.records[i].k << "," << greedyOnce.records[i].componentId << "\n";
        }
        greedyDetail.close();
    }

    out << "SUMMARY,seed=" << seed << ",query_group_count=" << actualGroupCount << ",query_size=" << actualQuerySize
        << ",sweep=0pct_to_90pct_step_10pct,query_builder=" << (useLegacyQueryBuilder ? "legacy" : "random");
    if (useLegacyQueryBuilder) {
        out << ",begin_candidate_pool_cap=" << beginCandidatePoolCap
            << ",begin_candidates_target=" << ResolveBeginCandidatePoolCap(beginCandidatePoolCap, beginPickCount);
    }
    out << ",detail_precise=" << preciseDetailPath << ",detail_fast=" << fastDetailPath
        << ",detail_greedy=" << greedyDetailPath << "\n";
    out.close();

    std::cout << "Output CSV: " << outputCsvPath << std::endl;
    std::cout << "Detail CSV: " << preciseDetailPath << std::endl;
    std::cout << "Detail CSV: " << fastDetailPath << std::endl;
    std::cout << "Detail CSV: " << greedyDetailPath << std::endl;
    std::cout << "===== end final_min_csp_simi_sweep =====\n" << std::endl;
}

void RunFinalMinCspFixedSimiSeedSweepExperiment(Graph& graph, int queryGroupCount, int queryNodeCount,
                                              unsigned long long randomSeed, const std::string& outputCsvPath,
                                              double fixedClusteringSimi, bool useLegacyQueryBuilder, int shift1,
                                              int shift2, int beginPickCount, int beginCandidatePoolCap) {
    if (std::isnan(fixedClusteringSimi)) {
        std::cerr << "[final_min_csp_fixed_simi_seed_sweep] fixedClusteringSimi (argv[6]) is required.\n";
        return;
    }

    const int actualGroupCount = (queryGroupCount > 0) ? queryGroupCount : kDefaultQueryGroupCount;
    const int actualQuerySize = (queryNodeCount > 0) ? queryNodeCount : kDefaultQuerySize;
    unsigned long long baseSeed = randomSeed;
    if (baseSeed == 0) {
        std::random_device rd;
        baseSeed = (static_cast<unsigned long long>(rd()) << 32) ^ static_cast<unsigned long long>(rd());
    }

    std::cout << "\n===== final_min_csp_fixed_simi_seed_sweep =====" << std::endl;
    std::cout << "[Config] query_group_count=" << actualGroupCount << ", query_size=" << actualQuerySize
              << ", base_random_seed=" << baseSeed << std::endl;
    std::cout << "[Config] fixed_clustering_simi=" << fixedClusteringSimi
              << " (batchMinsearch clustering threshold, constant across slots)" << std::endl;
    std::cout << "[Config] query_builder=" << (useLegacyQueryBuilder ? "legacy" : "random");
    if (useLegacyQueryBuilder) {
        std::cout << ", shift_1=" << shift1 << ", shift_2=" << shift2 << ", begin_pick_count=" << beginPickCount
                  << ", begin_candidate_pool_cap=" << beginCandidatePoolCap
                  << " (effective=" << ResolveBeginCandidatePoolCap(beginCandidatePoolCap, beginPickCount) << ")";
    }
    std::cout << std::endl;
    constexpr int kSlotCount = 10;
    constexpr int kCandidateBatchCount = 200;
    std::cout << "[Config] seed sweep: 10 slots labeled 0%..90% step 10%, fixed simi; "
                 "select slots by est(groupSimilarity) quantiles from "
              << kCandidateBatchCount << " candidate batches" << std::endl;

    auto resetStageTimers = []() {
        time_cluster = time_1 = time_2 = time_3 = time_4 = 0.0;
        clu_1 = clu_2 = 0.0;
    };

    std::ofstream out(outputCsvPath);
    if (!out.is_open()) {
        std::cerr << "Cannot open output CSV: " << outputCsvPath << std::endl;
        return;
    }
    out << "SweepSlotPct,DerivedSeed,EstGroupSimilarity,FixedClusteringSim,BatchminPrecise_TimeSec,"
           "BatchminPrecise_UnionNodeCount,BatchminPrecise_TotalResultNodeCount,BatchminFast_TimeSec,"
           "BatchminFast_UnionNodeCount,BatchminFast_TotalResultNodeCount,GreedyLoop_TimeSec,"
           "GreedyLoop_UnionNodeCount,GreedyLoop_TotalResultNodeCount,DetailPreciseCsv,DetailFastCsv,DetailGreedyCsv\n";

    struct CandidateBatch {
        unsigned long long seed = 0;
        double estSim = 0.0;
    };
    std::vector<CandidateBatch> candidates;
    candidates.reserve(kCandidateBatchCount);
    for (int i = 0; i < kCandidateBatchCount; ++i) {
        const unsigned long long candidateSeed = DeriveSeedForFixedSimiSweepSlot(baseSeed, i);
        query_group candidateBatch =
            useLegacyQueryBuilder
                ? BuildLegacyStyleQueryGroup(graph, actualGroupCount, actualQuerySize, candidateSeed, shift1, shift2,
                                             beginPickCount, beginCandidatePoolCap)
                : BuildRandomQueryGroup(graph, actualGroupCount, actualQuerySize, candidateSeed);
        if (candidateBatch.empty()) {
            continue;
        }
        SharingIndex estIndex(graph);
        const double estBatchSim = estIndex.groupSimilarity(candidateBatch, candidateBatch);
        candidates.push_back({candidateSeed, estBatchSim});
    }
    if (candidates.empty()) {
        std::cerr << "No valid candidate batches generated for fixed simi sweep." << std::endl;
        return;
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const CandidateBatch& a, const CandidateBatch& b) { return a.estSim < b.estSim; });

    std::vector<CandidateBatch> slotBatches;
    slotBatches.reserve(kSlotCount);
    std::vector<bool> used(candidates.size(), false);
    for (int slotIdx = 0; slotIdx < kSlotCount; ++slotIdx) {
        const double ratio = (kSlotCount <= 1) ? 0.0 : (static_cast<double>(slotIdx) / (kSlotCount - 1));
        const std::size_t targetPos =
            static_cast<std::size_t>(std::llround((candidates.size() - 1) * ratio));

        std::size_t picked = targetPos;
        bool foundUnused = false;
        for (std::size_t radius = 0; radius < candidates.size(); ++radius) {
            if (targetPos >= radius) {
                const std::size_t left = targetPos - radius;
                if (!used[left]) {
                    picked = left;
                    foundUnused = true;
                    break;
                }
            }
            const std::size_t right = targetPos + radius;
            if (right < candidates.size() && !used[right]) {
                picked = right;
                foundUnused = true;
                break;
            }
        }
        if (foundUnused) {
            used[picked] = true;
        }
        slotBatches.push_back(candidates[picked]);
    }

    std::cout << "[Selection] 10 quantile slots picked from candidate est values:" << std::endl;
    for (int slotIdx = 0; slotIdx < kSlotCount; ++slotIdx) {
        const int pct = slotIdx * 10;
        std::cout << "  slot " << pct << "% -> est=" << slotBatches[slotIdx].estSim
                  << ", derived_seed=" << slotBatches[slotIdx].seed << std::endl;
    }

    for (int slotIdx = 0; slotIdx < kSlotCount; ++slotIdx) {
        const int pct = slotIdx * 10;
        const unsigned long long derivedSeed = slotBatches[slotIdx].seed;
        const double estBatchSim = slotBatches[slotIdx].estSim;
        query_group batch =
            useLegacyQueryBuilder
                ? BuildLegacyStyleQueryGroup(graph, actualGroupCount, actualQuerySize, derivedSeed, shift1, shift2,
                                             beginPickCount, beginCandidatePoolCap)
                : BuildRandomQueryGroup(graph, actualGroupCount, actualQuerySize, derivedSeed);
        if (batch.empty()) {
            std::cout << "[slot pct=" << pct << "] selected batch is empty, skip.\n";
            continue;
        }

        simi = fixedClusteringSimi;
        resetStageTimers();
        MethodRunResult precise = RunPrecise(graph, batch);
        resetStageTimers();
        MethodRunResult fast = RunFast(graph, batch);
        MethodRunResult greedy = RunGreedyLoop(graph, batch);

        const std::string tag = "pct" + std::to_string(pct) + "_";
        const std::string preciseDetailPath = AppendSuffixBeforeCsv(outputCsvPath, tag + "precise_detail");
        const std::string fastDetailPath = AppendSuffixBeforeCsv(outputCsvPath, tag + "fast_detail");
        const std::string greedyDetailPath = AppendSuffixBeforeCsv(outputCsvPath, tag + "greedy_loop_detail");
        WriteSlotMethodDetailCsv(preciseDetailPath, pct, derivedSeed, estBatchSim, fixedClusteringSimi, batch, precise);
        WriteSlotMethodDetailCsv(fastDetailPath, pct, derivedSeed, estBatchSim, fixedClusteringSimi, batch, fast);
        WriteSlotMethodDetailCsv(greedyDetailPath, pct, derivedSeed, estBatchSim, fixedClusteringSimi, batch, greedy);

        out << pct << "," << derivedSeed << "," << estBatchSim << "," << fixedClusteringSimi << ","
            << precise.stats.elapsedSec << "," << precise.stats.unionNodeCount << "," << precise.stats.totalResultNodeCount
            << "," << fast.stats.elapsedSec << "," << fast.stats.unionNodeCount << "," << fast.stats.totalResultNodeCount
            << "," << greedy.stats.elapsedSec
            << "," << greedy.stats.unionNodeCount << "," << greedy.stats.totalResultNodeCount << ","
            << preciseDetailPath << "," << fastDetailPath << "," << greedyDetailPath << "\n";

        std::cout << "[slot " << pct << "%] derived_seed=" << derivedSeed << " est(groupSimilarity)=" << estBatchSim
                  << " fixed_simi=" << fixedClusteringSimi << " | precise t=" << precise.stats.elapsedSec << "s clu_simi="
                  << precise.stats.firstClusterCount << ", fast t=" << fast.stats.elapsedSec << "s clu_simi="
                  << fast.stats.firstClusterCount << " clu_csp_sum=" << fast.stats.secondClusterCount
                  << ", greedy t=" << greedy.stats.elapsedSec << "s\n";
    }

    out << "SUMMARY,base_seed=" << baseSeed << ",query_group_count=" << actualGroupCount
        << ",query_size=" << actualQuerySize << ",fixed_clustering_simi=" << fixedClusteringSimi
        << ",sweep=slot_0pct_to_90pct_step_10pct,candidate_batch_count=" << kCandidateBatchCount
        << ",DESIGN=fixed_simi_quantile_selected_batches,query_builder="
        << (useLegacyQueryBuilder ? "legacy" : "random");
    if (useLegacyQueryBuilder) {
        out << ",begin_candidate_pool_cap=" << beginCandidatePoolCap
            << ",begin_candidates_target=" << ResolveBeginCandidatePoolCap(beginCandidatePoolCap, beginPickCount);
    }
    out << "\n";
    out.close();

    std::cout << "Output CSV: " << outputCsvPath << std::endl;
    std::cout << "===== end final_min_csp_fixed_simi_seed_sweep =====\n" << std::endl;
}

void RunFinalMinCspQueryCountSweepExperiment(Graph& graph, int queryNodeCount, unsigned long long randomSeed,
                                             const std::string& outputCsvPath, double fixedClusteringSimi,
                                             bool useLegacyQueryBuilder, int shift1, int shift2, int beginPickCount,
                                             int beginCandidatePoolCap) {
    if (std::isnan(fixedClusteringSimi)) {
        std::cerr << "[final_min_csp_query_count_sweep] fixedClusteringSimi (argv[6]) is required.\n";
        return;
    }

    constexpr int kCounts[] = {20, 50, 100, 200, 300};
    const int actualQuerySize = (queryNodeCount > 0) ? queryNodeCount : kDefaultQuerySize;
    unsigned long long baseSeed = randomSeed;
    if (baseSeed == 0) {
        std::random_device rd;
        baseSeed = (static_cast<unsigned long long>(rd()) << 32) ^ static_cast<unsigned long long>(rd());
    }

    std::cout << "\n===== final_min_csp_query_count_sweep =====" << std::endl;
    std::cout << "[Config] query_size=" << actualQuerySize << ", base_random_seed=" << baseSeed << std::endl;
    std::cout << "[Config] fixed_clustering_simi=" << fixedClusteringSimi
              << " (batchMinsearch clustering threshold, constant across counts)" << std::endl;
    std::cout << "[Config] query_builder=" << (useLegacyQueryBuilder ? "legacy" : "random");
    if (useLegacyQueryBuilder) {
        std::cout << ", shift_1=" << shift1 << ", shift_2=" << shift2 << ", begin_pick_count=" << beginPickCount
                  << ", begin_candidate_pool_cap=" << beginCandidatePoolCap
                  << " (effective=" << ResolveBeginCandidatePoolCap(beginCandidatePoolCap, beginPickCount) << ")";
    }
    std::cout << std::endl;

    auto resetStageTimers = []() {
        time_cluster = time_1 = time_2 = time_3 = time_4 = 0.0;
        clu_1 = clu_2 = 0.0;
    };

    std::ofstream out(outputCsvPath);
    if (!out.is_open()) {
        std::cerr << "Cannot open output CSV: " << outputCsvPath << std::endl;
        return;
    }
    out << "QueryGroupCount,EstGroupSimilarity,FixedClusteringSim,BatchminPrecise_TimeSec,"
           "BatchminPrecise_UnionNodeCount,BatchminPrecise_TotalResultNodeCount,BatchminFast_TimeSec,"
           "BatchminFast_UnionNodeCount,BatchminFast_TotalResultNodeCount,GreedyLoop_TimeSec,"
           "GreedyLoop_UnionNodeCount,GreedyLoop_TotalResultNodeCount,DetailPreciseCsv,DetailFastCsv,DetailGreedyCsv\n";

    for (int idx = 0; idx < static_cast<int>(sizeof(kCounts) / sizeof(kCounts[0])); ++idx) {
        const int groupCount = kCounts[idx];
        const unsigned long long derivedSeed = baseSeed;

        query_group batch =
            useLegacyQueryBuilder
                ? BuildLegacyStyleQueryGroup(graph, groupCount, actualQuerySize, derivedSeed, shift1, shift2,
                                             beginPickCount, beginCandidatePoolCap)
                : BuildRandomQueryGroup(graph, groupCount, actualQuerySize, derivedSeed);
        if (batch.empty()) {
            std::cout << "[query_count=" << groupCount << "] empty query batch, skip.\n";
            continue;
        }

        SharingIndex estIndex(graph);
        const double estBatchSim = estIndex.groupSimilarity(batch, batch);

        simi = fixedClusteringSimi;
        resetStageTimers();
        MethodRunResult precise = RunPrecise(graph, batch);
        resetStageTimers();
        MethodRunResult fast = RunFast(graph, batch);
        MethodRunResult greedy = RunGreedyLoop(graph, batch);

        const std::string tag = "q" + std::to_string(groupCount) + "_";
        const std::string preciseDetailPath = AppendSuffixBeforeCsv(outputCsvPath, tag + "precise_detail");
        const std::string fastDetailPath = AppendSuffixBeforeCsv(outputCsvPath, tag + "fast_detail");
        const std::string greedyDetailPath = AppendSuffixBeforeCsv(outputCsvPath, tag + "greedy_loop_detail");
        WriteSlotMethodDetailCsv(preciseDetailPath, groupCount, derivedSeed, estBatchSim, fixedClusteringSimi, batch,
                                 precise);
        WriteSlotMethodDetailCsv(fastDetailPath, groupCount, derivedSeed, estBatchSim, fixedClusteringSimi, batch, fast);
        WriteSlotMethodDetailCsv(greedyDetailPath, groupCount, derivedSeed, estBatchSim, fixedClusteringSimi, batch,
                                 greedy);

        out << groupCount << "," << estBatchSim << "," << fixedClusteringSimi << "," << precise.stats.elapsedSec << ","
            << precise.stats.unionNodeCount << "," << precise.stats.totalResultNodeCount << ","
            << fast.stats.elapsedSec << "," << fast.stats.unionNodeCount << "," << fast.stats.totalResultNodeCount << ","
            << greedy.stats.elapsedSec
            << "," << greedy.stats.unionNodeCount << "," << greedy.stats.totalResultNodeCount << ","
            << preciseDetailPath << "," << fastDetailPath << "," << greedyDetailPath << "\n";

        std::cout << "[query_count=" << groupCount << "] derived_seed=" << derivedSeed
                  << " est(groupSimilarity)=" << estBatchSim << " fixed_simi=" << fixedClusteringSimi
                  << " | precise t=" << precise.stats.elapsedSec << "s clu_simi=" << precise.stats.firstClusterCount
                  << ", fast t=" << fast.stats.elapsedSec << "s clu_simi=" << fast.stats.firstClusterCount
                  << " clu_csp_sum=" << fast.stats.secondClusterCount << ", greedy t=" << greedy.stats.elapsedSec
                  << "s" << std::endl;
    }

    out << "SUMMARY,base_seed=" << baseSeed << ",query_size=" << actualQuerySize
        << ",fixed_clustering_simi=" << fixedClusteringSimi << ",sweep=query_group_count_{20,50,100,200,300}"
        << ",DESIGN=fixed_simi_query_count_sweep,query_builder=" << (useLegacyQueryBuilder ? "legacy" : "random");
    if (useLegacyQueryBuilder) {
        out << ",begin_candidate_pool_cap=" << beginCandidatePoolCap
            << ",begin_candidates_target=" << ResolveBeginCandidatePoolCap(beginCandidatePoolCap, beginPickCount);
    }
    out << "\n";
    out.close();

    std::cout << "Output CSV: " << outputCsvPath << std::endl;
    std::cout << "===== end final_min_csp_query_count_sweep =====\n" << std::endl;
}

}  // namespace experiments

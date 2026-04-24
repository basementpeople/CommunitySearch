#include "experiments/core/SingleQueryExperiment.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "TreeIndex.h"

namespace {

struct SingleQueryRunResult {
    std::unordered_set<int> resultNodes;
    double elapsedSec = 0.0;
    int k = 0;
};

std::string QueryToString(const query_nodes& query) {
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

int ComputeMinDegree(Graph& graph, const std::unordered_set<int>& nodes) {
    if (nodes.empty()) {
        return 0;
    }
    std::unordered_set<int> copy = nodes;
    return graph.getMinDegree(copy);
}

void WriteSingleQueryDetailCsv(const std::string& outputCsvPath, const query_nodes& query,
                               const SingleQueryRunResult& runResult) {
    std::ofstream out(outputCsvPath);
    if (!out.is_open()) {
        return;
    }

    out << "QueryNodes,ResultSize,K,ElapsedSec\n";
    out << QueryToString(query) << "," << runResult.resultNodes.size() << "," << runResult.k << ","
        << runResult.elapsedSec << "\n";
}

SingleQueryRunResult RunGlobal(Graph& graph, const query_nodes& query) {
    SingleQueryRunResult runResult;
    const clock_t begin = clock();
    runResult.resultNodes = graph.globalsearch(query);
    const clock_t end = clock();
    runResult.elapsedSec = static_cast<double>(end - begin) / CLOCKS_PER_SEC;
    runResult.k = ComputeMinDegree(graph, runResult.resultNodes);
    return runResult;
}

SingleQueryRunResult RunKGlobal(Graph& graph, const query_nodes& query, int kCount) {
    SingleQueryRunResult runResult;
    const clock_t begin = clock();
    runResult.resultNodes = graph.kGlobalsearch(query, kCount);
    const clock_t end = clock();
    runResult.elapsedSec = static_cast<double>(end - begin) / CLOCKS_PER_SEC;
    runResult.k = ComputeMinDegree(graph, runResult.resultNodes);
    return runResult;
}

SingleQueryRunResult RunRetrieval(Graph& graph, const query_nodes& query) {
    SingleQueryRunResult runResult;
    TreeIndex index(graph);
    int k = 0;
    const clock_t begin = clock();
    runResult.resultNodes = index.retrievalShellStruct(query, k);
    const clock_t end = clock();
    runResult.elapsedSec = static_cast<double>(end - begin) / CLOCKS_PER_SEC;
    runResult.k = k;
    return runResult;
}

SingleQueryRunResult RunGreedy(Graph& graph, const query_nodes& query) {
    SingleQueryRunResult runResult;
    TreeIndex index(graph);
    int k = 0;
    const clock_t begin = clock();
    std::unordered_set<int> H = index.retrievalShellStruct(query, k);
    if (!H.empty()) {
        runResult.resultNodes = index.greedyConnection(query, k, H);
    }
    const clock_t end = clock();
    runResult.elapsedSec = static_cast<double>(end - begin) / CLOCKS_PER_SEC;
    runResult.k = k;
    return runResult;
}

std::string AppendSuffixBeforeCsv(const std::string& path, const std::string& suffix) {
    const std::string ext = ".csv";
    if (path.size() >= ext.size() && path.substr(path.size() - ext.size()) == ext) {
        return path.substr(0, path.size() - ext.size()) + "_" + suffix + ext;
    }
    return path + "_" + suffix;
}

query_group LoadManualQueryGroup(const std::string& path) {
    std::ifstream in(path);
    query_group group;
    if (!in.is_open()) {
        return group;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        std::stringstream ss(line);
        query_nodes query;
        int node = 0;
        while (ss >> node) {
            query.insert(node);
        }
        if (!query.empty()) {
            group.push_back(std::move(query));
        }
    }
    return group;
}

void WriteManualMethodDetailCsv(const std::string& path, const query_group& group,
                                const std::vector<SingleQueryRunResult>& results) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return;
    }
    out << "QueryId,QueryNodes,TimeSec,ResultSize,K\n";
    for (std::size_t i = 0; i < group.size() && i < results.size(); ++i) {
        out << i << "," << QueryToString(group[i]) << "," << results[i].elapsedSec << ","
            << results[i].resultNodes.size() << "," << results[i].k << "\n";
    }
}

}  // namespace

namespace experiments {

void RunGlobalSearchExperiment(Graph& graph, const query_nodes& query, const std::string& outputCsvPath) {
    const SingleQueryRunResult result = RunGlobal(graph, query);
    std::cout << "Global search elapsed: " << result.elapsedSec << "s" << std::endl;
    std::cout << "Result size: " << result.resultNodes.size() << ", k: " << result.k << std::endl;
    WriteSingleQueryDetailCsv(outputCsvPath, query, result);
}

void RunKGlobalSearchExperiment(Graph& graph, const query_nodes& query, int kCount, const std::string& outputCsvPath) {
    const SingleQueryRunResult result = RunKGlobal(graph, query, kCount);
    std::cout << "k-global search elapsed: " << result.elapsedSec << "s" << std::endl;
    std::cout << "Result size: " << result.resultNodes.size() << ", k: " << result.k
              << ", k_count: " << kCount << std::endl;
    WriteSingleQueryDetailCsv(outputCsvPath, query, result);
}

void RunRetrievalShellExperiment(Graph& graph, const query_nodes& query, const std::string& outputCsvPath) {
    const SingleQueryRunResult result = RunRetrieval(graph, query);
    std::cout << "Retrieval elapsed: " << result.elapsedSec << "s" << std::endl;
    std::cout << "Result size: " << result.resultNodes.size() << ", k: " << result.k << std::endl;
    WriteSingleQueryDetailCsv(outputCsvPath, query, result);
}

void RunGreedyConnectionExperiment(Graph& graph, const query_nodes& query, const std::string& outputCsvPath) {
    const SingleQueryRunResult result = RunGreedy(graph, query);
    std::cout << "Greedy elapsed: " << result.elapsedSec << "s" << std::endl;
    std::cout << "Result size: " << result.resultNodes.size() << ", k: " << result.k << std::endl;
    WriteSingleQueryDetailCsv(outputCsvPath, query, result);
}

void RunSingleQueryComparisonExperiment(Graph& graph, const query_nodes& query, const std::string& outputCsvPath) {
    const SingleQueryRunResult globalResult = RunGlobal(graph, query);
    const SingleQueryRunResult retrievalResult = RunRetrieval(graph, query);
    const SingleQueryRunResult greedyResult = RunGreedy(graph, query);

    const std::string globalDetailPath = AppendSuffixBeforeCsv(outputCsvPath, "global");
    const std::string retrievalDetailPath = AppendSuffixBeforeCsv(outputCsvPath, "retrieval");
    const std::string greedyDetailPath = AppendSuffixBeforeCsv(outputCsvPath, "greedy");

    WriteSingleQueryDetailCsv(globalDetailPath, query, globalResult);
    WriteSingleQueryDetailCsv(retrievalDetailPath, query, retrievalResult);
    WriteSingleQueryDetailCsv(greedyDetailPath, query, greedyResult);

    std::ofstream out(outputCsvPath);
    if (!out.is_open()) {
        return;
    }

    out << "Method,TimeSec,ResultSize,K,DetailCsv\n";
    out << "global," << globalResult.elapsedSec << "," << globalResult.resultNodes.size() << "," << globalResult.k
        << "," << globalDetailPath << "\n";
    out << "retrieval," << retrievalResult.elapsedSec << "," << retrievalResult.resultNodes.size() << ","
        << retrievalResult.k << "," << retrievalDetailPath << "\n";
    out << "greedy," << greedyResult.elapsedSec << "," << greedyResult.resultNodes.size() << "," << greedyResult.k
        << "," << greedyDetailPath << "\n";

    std::cout << "Single compare summary:" << std::endl;
    std::cout << "  global    : time=" << globalResult.elapsedSec << "s, size=" << globalResult.resultNodes.size()
              << ", k=" << globalResult.k << std::endl;
    std::cout << "  retrieval : time=" << retrievalResult.elapsedSec << "s, size=" << retrievalResult.resultNodes.size()
              << ", k=" << retrievalResult.k << std::endl;
    std::cout << "  greedy    : time=" << greedyResult.elapsedSec << "s, size=" << greedyResult.resultNodes.size()
              << ", k=" << greedyResult.k << std::endl;
}

void RunManualQueryGroupComparisonExperiment(Graph& graph, const std::string& queryGroupPath,
                                             const std::string& outputCsvPath) {
    query_group group = LoadManualQueryGroup(queryGroupPath);
    if (group.empty()) {
        std::cout << "Manual query group is empty or cannot be loaded: " << queryGroupPath << std::endl;
        return;
    }

    std::vector<SingleQueryRunResult> globalRuns;
    std::vector<SingleQueryRunResult> retrievalRuns;
    std::vector<SingleQueryRunResult> greedyRuns;
    globalRuns.reserve(group.size());
    retrievalRuns.reserve(group.size());
    greedyRuns.reserve(group.size());

    for (const auto& query : group) {
        globalRuns.push_back(RunGlobal(graph, query));
        greedyRuns.push_back(RunGreedy(graph, query));
        retrievalRuns.push_back(RunRetrieval(graph, query));
        // greedyRuns.push_back(RunGreedy(graph, query));
    }

    auto summarizeRuns = [](const std::vector<SingleQueryRunResult>& runs) {
        double totalTime = 0.0;
        std::size_t totalSize = 0;
        for (const auto& run : runs) {
            totalTime += run.elapsedSec;
            totalSize += run.resultNodes.size();
        }
        const std::size_t avgSize = runs.empty() ? 0 : (totalSize / runs.size());
        return std::tuple<double, std::size_t, std::size_t>(totalTime, totalSize, avgSize);
    };

    const std::string globalDetailPath = AppendSuffixBeforeCsv(outputCsvPath, "global_detail");
    const std::string retrievalDetailPath = AppendSuffixBeforeCsv(outputCsvPath, "retrieval_detail");
    const std::string greedyDetailPath = AppendSuffixBeforeCsv(outputCsvPath, "greedy_detail");
    WriteManualMethodDetailCsv(globalDetailPath, group, globalRuns);
    WriteManualMethodDetailCsv(retrievalDetailPath, group, retrievalRuns);
    WriteManualMethodDetailCsv(greedyDetailPath, group, greedyRuns);

    const auto [globalTime, globalTotal, globalAvg] = summarizeRuns(globalRuns);
    const auto [retrievalTime, retrievalTotal, retrievalAvg] = summarizeRuns(retrievalRuns);
    const auto [greedyTime, greedyTotal, greedyAvg] = summarizeRuns(greedyRuns);

    std::ofstream out(outputCsvPath);
    if (!out.is_open()) {
        return;
    }
    out << "Method,TotalTimeSec,TotalResultSize,AvgResultSize,DetailCsv\n";
    out << "global_loop," << globalTime << "," << globalTotal << "," << globalAvg << "," << globalDetailPath << "\n";
    out << "retrieval_loop," << retrievalTime << "," << retrievalTotal << "," << retrievalAvg << "," << retrievalDetailPath
        << "\n";
    out << "greedy_loop," << greedyTime << "," << greedyTotal << "," << greedyAvg << "," << greedyDetailPath << "\n";

    std::cout << "Manual query compare summary:" << std::endl;
    std::cout << "  query_count : " << group.size() << std::endl;
    std::cout << "  global_loop : time=" << globalTime << "s, total_size=" << globalTotal << ", avg_size=" << globalAvg
              << std::endl;
    std::cout << "  retrieval   : time=" << retrievalTime << "s, total_size=" << retrievalTotal
              << ", avg_size=" << retrievalAvg << std::endl;
    std::cout << "  greedy      : time=" << greedyTime << "s, total_size=" << greedyTotal << ", avg_size=" << greedyAvg
              << std::endl;
}

}  // namespace experiments

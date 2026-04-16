#include "experiments/core/ExperimentRunner.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

#include "experiments/core/SingleQueryExperiment.h"
#include "experiments/legacy/BatchExperiment.h"
#include "experiments/legacy/CompareExperiment.h"
#include "experiments/multi_query/BatchMinCompareExperiment.h"
#include "experiments/multi_query/BatchSearchExperiment.h"
#include "experiments/multi_query/MultiQueryMinCompareExperiment.h"
#include "experiments/multi_query/MultiQueryCompareExperiment.h"

namespace {

constexpr const char* kOutputDir = "data\\output\\";

std::string defaultCsvPathForExperiment(const std::string& name) {
    if (name == "global") {
        return std::string(kOutputDir) + "single_query\\global_result.csv";
    }
    if (name == "compare") {
        return std::string(kOutputDir) + "legacy\\compare_result.csv";
    }
    if (name == "retrieval") {
        return std::string(kOutputDir) + "single_query\\retrieval_result.csv";
    }
    if (name == "greedy") {
        return std::string(kOutputDir) + "single_query\\greedy_result.csv";
    }
    if (name == "single_compare") {
        return std::string(kOutputDir) + "single_query\\single_compare_result.csv";
    }
    if (name == "batchmin") {
        return std::string(kOutputDir) + "legacy\\batchmin_result.csv";
    }
    if (name == "batchsearch") {
        return std::string(kOutputDir) + "multi_query\\batchsearch_result.csv";
    }
    if (name == "batchsearch_rough") {
        return std::string(kOutputDir) + "multi_query\\batchsearch_rough_result.csv";
    }
    if (name == "batchmin_compare") {
        return std::string(kOutputDir) + "multi_query\\batchmin_compare_result.csv";
    }
    if (name == "h0") {
        return std::string(kOutputDir) + "legacy\\h0_result.csv";
    }
    if (name == "h") {
        return std::string(kOutputDir) + "legacy\\h_result.csv";
    }
    if (name == "h1") {
        return std::string(kOutputDir) + "legacy\\h1_result.csv";
    }
    if (name == "h2") {
        return std::string(kOutputDir) + "legacy\\h2_result.csv";
    }
    if (name == "multi_query_compare") {
        return std::string(kOutputDir) + "multi_query\\multi_query_compare_result.csv";
    }
    if (name == "multi_query_min_compare") {
        return std::string(kOutputDir) + "multi_query\\multi_query_min_compare_result.csv";
    }
    return std::string(kOutputDir) + "single_query\\global_result.csv";
}

void ensureOutputDirectory(const std::string& outputPath) {
    const std::filesystem::path p(outputPath);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }
}

    query_nodes sampleRandomQuery(Graph& graph, int k, unsigned long long seed) {
        std::vector<int> nodes;
        nodes.reserve(static_cast<size_t>(graph.getn()));
        for (const auto& p : graph.getAdj()) {
            nodes.push_back(p.first);
        }
        if (nodes.empty()) {
            return {};
        }
        if (k <= 0) {
            return {};
        }
        if (k >= static_cast<int>(nodes.size())) {
            return query_nodes(nodes.begin(), nodes.end());
        }
        std::mt19937_64 rng(seed);
        std::vector<int> picked;
        picked.reserve(static_cast<size_t>(k));
        std::sample(nodes.begin(), nodes.end(), std::back_inserter(picked), static_cast<size_t>(k), rng);
        return query_nodes(picked.begin(), picked.end());
    }

void runGlobalDefault(Graph& graph, const std::string& outputCsvPath, int randomQueryCount,
                      unsigned long long randomSeed) {
    query_nodes query;
    if (randomQueryCount > 0) {
        unsigned long long seed = randomSeed;
        if (seed == 0) {
            std::random_device rd;
            seed = (static_cast<unsigned long long>(rd()) << 32) ^ static_cast<unsigned long long>(rd());
        }
        std::cout << "Random query count: " << randomQueryCount << ", seed: " << seed << std::endl;
        query = sampleRandomQuery(graph, randomQueryCount, seed);
        if (query.empty()) {
            std::cout << "Random query sampling failed (empty graph?), fallback to {300}." << std::endl;
            query = {300};
        }
    } else {
        query = {300};
    }
    std::cout << "Output CSV: " << outputCsvPath << std::endl;
    ensureOutputDirectory(outputCsvPath);
    experiments::RunGlobalSearchExperiment(graph, query, outputCsvPath);
}

}

namespace experiments {

void RunExperiment(const std::string& experimentName, Graph& graph, int randomQueryCount,
                   unsigned long long randomSeed, int queryNodeCount, double similarityThreshold,
                   bool useLegacyQueryBuilder, int shift1, int shift2, int beginPickCount) {
    if (experimentName == "batchmin") {
        std::cout << "Output CSV (batchmin): " << defaultCsvPathForExperiment("batchmin") << std::endl;
        RunBatchMinExperiment(graph, 1, 0.0);
        return;
    }
    if (experimentName == "batchsearch") {
        const std::string batchPath = defaultCsvPathForExperiment("batchsearch");
        std::cout << "Output CSV (batchsearch): " << batchPath << std::endl;
        ensureOutputDirectory(batchPath);
        RunBatchSearchExperiment(graph, randomQueryCount, queryNodeCount, randomSeed, batchPath);
        return;
    }
    if (experimentName == "batchsearch_rough") {
        const std::string roughPath = defaultCsvPathForExperiment("batchsearch_rough");
        std::cout << "Output CSV (batchsearch_rough): " << roughPath << std::endl;
        ensureOutputDirectory(roughPath);
        RunBatchSearchRoughExperiment(graph, randomQueryCount, queryNodeCount, randomSeed, roughPath);
        return;
    }
    if (experimentName == "multi_query_compare") {
        const std::string multiComparePath = defaultCsvPathForExperiment("multi_query_compare");
        std::cout << "Output CSV (multi_query_compare): " << multiComparePath << std::endl;
        ensureOutputDirectory(multiComparePath);
        RunMultiQueryCompareExperiment(graph, randomQueryCount, queryNodeCount, randomSeed, multiComparePath);
        return;
    }
    if (experimentName == "batchmin_compare") {
        const std::string batchMinComparePath = defaultCsvPathForExperiment("batchmin_compare");
        std::cout << "Output CSV (batchmin_compare): " << batchMinComparePath << std::endl;
        ensureOutputDirectory(batchMinComparePath);
        if (!std::isnan(similarityThreshold)) {
            std::cout << "Similarity threshold override: " << similarityThreshold << std::endl;
        }
        RunBatchMinCompareExperiment(graph, randomQueryCount, queryNodeCount, randomSeed, batchMinComparePath,
                                     similarityThreshold);
        return;
    }
    if (experimentName == "multi_query_min_compare") {
        const std::string path = defaultCsvPathForExperiment("multi_query_min_compare");
        std::cout << "Output CSV (multi_query_min_compare): " << path << std::endl;
        ensureOutputDirectory(path);
        if (!std::isnan(similarityThreshold)) {
            std::cout << "Similarity threshold override: " << similarityThreshold << std::endl;
        }
        RunMultiQueryMinCompareExperiment(graph, randomQueryCount, queryNodeCount, randomSeed, path,
                                          similarityThreshold, useLegacyQueryBuilder, shift1, shift2,
                                          beginPickCount);
        return;
    }
    if (experimentName == "h0") {
        RunH0Experiment(graph);
        return;
    }
    if (experimentName == "h") {
        RunHExperiment(graph);
        return;
    }
    if (experimentName == "h1") {
        RunH1Experiment(graph, 0.0, 3, 6, 6);
        return;
    }
    if (experimentName == "h2") {
        std::cout << "Output CSV: " << defaultCsvPathForExperiment("h2") << std::endl;
        RunH2Experiment(graph, 0.0);
        return;
    }
    if (experimentName == "compare") {
        RunCompareExperiment(graph);
        return;
    }
    if (experimentName == "retrieval") {
        const std::string retrievalPath = defaultCsvPathForExperiment("retrieval");
        ensureOutputDirectory(retrievalPath);
        query_nodes query;
        if (randomQueryCount > 0) {
            unsigned long long seed = randomSeed;
            if (seed == 0) {
                std::random_device rd;
                seed = (static_cast<unsigned long long>(rd()) << 32) ^ static_cast<unsigned long long>(rd());
            }
            std::cout << "Random query count: " << randomQueryCount << ", seed: " << seed << std::endl;
            query = sampleRandomQuery(graph, randomQueryCount, seed);
            if (query.empty()) {
                std::cout << "Random query sampling failed (empty graph?), fallback to {300}." << std::endl;
                query = {300};
            }
        } else {
            query = {300};
        }
        std::cout << "Output CSV: " << retrievalPath << std::endl;
        RunRetrievalShellExperiment(graph, query, retrievalPath);
        return;
    }
    if (experimentName == "greedy") {
        const std::string greedyPath = defaultCsvPathForExperiment("greedy");
        ensureOutputDirectory(greedyPath);
        query_nodes query;
        if (randomQueryCount > 0) {
            unsigned long long seed = randomSeed;
            if (seed == 0) {
                std::random_device rd;
                seed = (static_cast<unsigned long long>(rd()) << 32) ^ static_cast<unsigned long long>(rd());
            }
            std::cout << "Random query count: " << randomQueryCount << ", seed: " << seed << std::endl;
            query = sampleRandomQuery(graph, randomQueryCount, seed);
            if (query.empty()) {
                std::cout << "Random query sampling failed (empty graph?), fallback to {300}." << std::endl;
                query = {300};
            }
        } else {
            query = {300};
        }
        std::cout << "Output CSV: " << greedyPath << std::endl;
        RunGreedyConnectionExperiment(graph, query, greedyPath);
        return;
    }
    if (experimentName == "single_compare") {
        const std::string comparePath = defaultCsvPathForExperiment("single_compare");
        ensureOutputDirectory(comparePath);
        query_nodes query;
        if (randomQueryCount > 0) {
            unsigned long long seed = randomSeed;
            if (seed == 0) {
                std::random_device rd;
                seed = (static_cast<unsigned long long>(rd()) << 32) ^ static_cast<unsigned long long>(rd());
            }
            std::cout << "Random query count: " << randomQueryCount << ", seed: " << seed << std::endl;
            query = sampleRandomQuery(graph, randomQueryCount, seed);
            if (query.empty()) {
                std::cout << "Random query sampling failed (empty graph?), fallback to {300}." << std::endl;
                query = {300};
            }
        } else {
            query = {300};
        }
        std::cout << "Output CSV: " << comparePath << std::endl;
        RunSingleQueryComparisonExperiment(graph, query, comparePath);
        return;
    }
    if (experimentName != "global") {
        std::cout << "Unknown experiment '" << experimentName
                  << "', fallback to default 'global'." << std::endl;
    }
    const std::string globalPath = defaultCsvPathForExperiment("global");
    runGlobalDefault(graph, globalPath, randomQueryCount, randomSeed);
}

}  // namespace experiments

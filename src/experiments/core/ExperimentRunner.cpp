#include "experiments/core/ExperimentRunner.h"

#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "experiments/core/SingleQueryExperiment.h"

namespace {

constexpr const char* kOutputDir = "data\\output\\";

std::string defaultCsvPathForExperiment(const std::string& name) {
    if (name == "global") {
        return std::string(kOutputDir) + "single_query\\global_result.csv";
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

}  // namespace

namespace experiments {

void RunExperiment(const std::string& experimentName, Graph& graph, int randomQueryCount,
                   unsigned long long randomSeed, int queryNodeCount, double similarityThreshold,
                   bool useLegacyQueryBuilder, int shift1, int shift2, int beginPickCount) {
    (void)queryNodeCount;
    (void)similarityThreshold;
    (void)useLegacyQueryBuilder;
    (void)shift1;
    (void)shift2;
    (void)beginPickCount;

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

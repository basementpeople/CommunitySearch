#pragma once

#include <limits>
#include <string>

#include "Graph.h"

namespace experiments {

void RunMultiQueryMinCompareExperiment(Graph& graph, int queryGroupCount, int queryNodeCount,
                                       unsigned long long randomSeed, const std::string& outputCsvPath,
                                       double similarityThreshold = std::numeric_limits<double>::quiet_NaN(),
                                       bool useLegacyQueryBuilder = false, int shift1 = 0, int shift2 = 0,
                                       int beginPickCount = 3, int beginCandidatePoolCap = 0);

// Final min-CSP: sweep similarity threshold 0..0.9 (step 0.1); same query builder args as multi_query_min_compare.
void RunFinalMinCspSimiSweepExperiment(Graph& graph, int queryGroupCount, int queryNodeCount,
                                       unsigned long long randomSeed, const std::string& outputCsvPath,
                                       bool useLegacyQueryBuilder = false, int shift1 = 0, int shift2 = 0,
                                       int beginPickCount = 3, int beginCandidatePoolCap = 0);

// Final min-CSP exp2: fixed clustering simi (arg6), 10 query batches with derived seeds; est = groupSimilarity(g,g).
void RunFinalMinCspFixedSimiSeedSweepExperiment(Graph& graph, int queryGroupCount, int queryNodeCount,
                                                unsigned long long randomSeed, const std::string& outputCsvPath,
                                                double fixedClusteringSimi, bool useLegacyQueryBuilder, int shift1,
                                                int shift2, int beginPickCount, int beginCandidatePoolCap);

// Final min-CSP exp3: fixed clustering simi, sweep query group count in {20,50,100,200,300}.
void RunFinalMinCspQueryCountSweepExperiment(Graph& graph, int queryNodeCount, unsigned long long randomSeed,
                                             const std::string& outputCsvPath, double fixedClusteringSimi,
                                             bool useLegacyQueryBuilder, int shift1, int shift2, int beginPickCount,
                                             int beginCandidatePoolCap);

}  // namespace experiments

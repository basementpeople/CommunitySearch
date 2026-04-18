#pragma once

#include <limits>
#include <string>

#include "Graph.h"

namespace experiments {

void RunMultiQueryMinCompareExperiment(Graph& graph, int queryGroupCount, int queryNodeCount,
                                       unsigned long long randomSeed, const std::string& outputCsvPath,
                                       double similarityThreshold = std::numeric_limits<double>::quiet_NaN(),
                                       bool useLegacyQueryBuilder = false, int shift1 = 0, int shift2 = 0,
                                       int beginPickCount = 3);

// Final min-CSP: sweep similarity threshold 0..0.9 (step 0.1); same query builder args as multi_query_min_compare.
void RunFinalMinCspSimiSweepExperiment(Graph& graph, int queryGroupCount, int queryNodeCount,
                                       unsigned long long randomSeed, const std::string& outputCsvPath,
                                       bool useLegacyQueryBuilder = false, int shift1 = 0, int shift2 = 0,
                                       int beginPickCount = 3);

}  // namespace experiments

#pragma once

#include <limits>
#include <string>

#include "Graph.h"

namespace experiments {

// Output CSV paths are chosen inside RunExperiment: data\output\<experiment>_result.csv
// Dispatch by experiment name (global/compare/h0/h1/h2/batchmin/batchsearch...).
void RunExperiment(const std::string& experimentName, Graph& graph, int randomQueryCount = 0,
                   unsigned long long randomSeed = 0, int queryNodeCount = 0,
                   double similarityThreshold = std::numeric_limits<double>::quiet_NaN(),
                   bool useLegacyQueryBuilder = false, int shift1 = 0, int shift2 = 0, int beginPickCount = 3);

}  // namespace experiments

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

}  // namespace experiments

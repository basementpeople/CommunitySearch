#pragma once

#include <string>

#include "Graph.h"

namespace experiments {

void RunFinalCspCompareExperiment(Graph& graph, int queryGroupCount, int queryNodeCount,
                                  unsigned long long randomSeed, const std::string& outputCsvPath,
                                  bool includeGlobalLoop);

}  // namespace experiments

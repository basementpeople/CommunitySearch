#pragma once

#include <string>

#include "Graph.h"

namespace experiments {

void RunGlobalSearchExperiment(Graph& graph, const query_nodes& query, const std::string& outputCsvPath);
void RunKGlobalSearchExperiment(Graph& graph, const query_nodes& query, int kCount, const std::string& outputCsvPath);
void RunRetrievalShellExperiment(Graph& graph, const query_nodes& query, const std::string& outputCsvPath);
void RunGreedyConnectionExperiment(Graph& graph, const query_nodes& query, const std::string& outputCsvPath);
void RunSingleQueryComparisonExperiment(Graph& graph, const query_nodes& query, const std::string& outputCsvPath);
void RunManualQueryGroupComparisonExperiment(Graph& graph, const std::string& queryGroupPath,
                                             const std::string& outputCsvPath);

}  // namespace experiments

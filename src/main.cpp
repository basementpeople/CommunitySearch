#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_set>

#include "Graph.h"
#include "experiments/core/ExperimentRunner.h"

namespace {
    constexpr const char* inputRoot = "data\\raw\\"; // 输入数据集的目录
    constexpr const char* defaultDataset = "email-Eu-core.txt"; // 默认数据集
}

// 由各实验模块通过 extern 共享访问的全局实验状态。
double simi = 0;
double time_greedy = 0, time_steiner = 0, time_simple = 0;
double time_cluster = 0, time_1 = 0, time_2 = 0, time_3 = 0, time_4 = 0, clu_1 = 0, clu_2 = 0;
query_group group;

int shift_1 = 0;
int shift_2 = 0;
std::unordered_set<int> beginnodes;

// 主函数：加载图数据，处理命令行参数初始化查询节点，并执行全局搜索测试
int main(int argc, char* argv[]) {
    std::string datasetPath = std::string(inputRoot) + defaultDataset;
    std::string experimentName = "global";
    int randomQueryCount = 0;
    unsigned long long randomSeed = 0;
    int queryNodeCount = 0;
    double similarityThreshold = std::numeric_limits<double>::quiet_NaN();
    bool useLegacyQueryBuilder = false;
    int legacyShift1 = 0;
    int legacyShift2 = 0;
    int legacyBeginPickCount = 3;
    int legacyBeginCandidatePoolCap = 0;
    if (argc > 1) {
        datasetPath = argv[1];
    }
    if (argc > 2) {
        experimentName = argv[2];
    }
    if (argc > 3) {
        randomQueryCount = std::atoi(argv[3]);
    }
    if (argc > 4) {
        randomSeed = std::strtoull(argv[4], nullptr, 10);
    }
    if (argc > 5) {
        queryNodeCount = std::atoi(argv[5]);
    }
    if (argc > 6) {
        similarityThreshold = std::strtod(argv[6], nullptr);
    }
    if (argc > 7) {
        useLegacyQueryBuilder = (std::atoi(argv[7]) != 0);
    }
    if (argc > 8) {
        legacyShift1 = std::atoi(argv[8]);
    }
    if (argc > 9) {
        legacyShift2 = std::atoi(argv[9]);
    }
    if (argc > 10) {
        legacyBeginPickCount = std::atoi(argv[10]);
    }
    if (argc > 11) {
        legacyBeginCandidatePoolCap = std::atoi(argv[11]);
    }
    std::cout << "Using dataset: " << datasetPath << std::endl;
    std::cout << "Using experiment: " << experimentName << std::endl;

    // 测试使用的无向图
    Graph graph(datasetPath);
    experiments::RunExperiment(experimentName, graph, randomQueryCount, randomSeed, queryNodeCount,
                               similarityThreshold, useLegacyQueryBuilder, legacyShift1, legacyShift2,
                               legacyBeginPickCount, legacyBeginCandidatePoolCap);

    return 0;
}

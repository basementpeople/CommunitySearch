#include <algorithm>
#include <cassert>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <queue>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>
// #include <windows.h>  // Sleep函数
#include "Graph.h"
#include "CoreGroup.h"
#include "TreeIndex.h"
#include "SharingIndex.h"

namespace {
constexpr const char* kSingleOutputCsv = "single_7.csv";
constexpr const char* kGlobalOutputCsv = "global_result.csv";
constexpr const char* kDatasetRoot = "data\\raw\\";
constexpr const char* kDefaultDataset = "email-Eu-core.txt";
}

double simi = 0;
double time_greedy = 0, time_steiner = 0, time_simple = 0;
double time_cluster = 0, time_1 = 0, time_2 = 0, time_3 = 0, time_4 = 0, clu_1 = 0;
query_group group; // 查询集（全部）

int shift_1 = 0;
int shift_2 = 0;
std::unordered_set<int> beginnodes; // 初始的点集合

// 计算节点 f 的邻居中核心数 >= s 的节点数量
int firster(int f, int s, std::unordered_map<int, int> cores, Graph& graph) {
    int num = 0;
    for (auto& node : graph.getNeighbors(f)) {
        if (cores[node] >= s) {
            num++;
        }
    }
    return num;
}

// 计算两个查询节点集的相似度，基于邻居交集与并集的比例
double qSimilarity(query_nodes& qA, query_nodes& qB, Graph& graph) {
    // qA的邻居
    std::unordered_set<int> neighborsA;
    for (auto& v : qA) {
        neighborsA.insert(graph.getNeighbors(v).begin(), graph.getNeighbors(v).end());
        //const auto& neighbors = graph.getNeighbors(v);
        //neighborsA.insert(neighbors.begin(), neighbors.end());
    }

    // qB的邻居
    std::unordered_set<int> neighborsB;
    for (auto& v : qB) {
        neighborsB.insert(graph.getNeighbors(v).begin(), graph.getNeighbors(v).end());
    }

    // 排序
    std::vector<int> sortedA(neighborsA.begin(), neighborsA.end());
    std::vector<int> sortedB(neighborsB.begin(), neighborsB.end());
    std::sort(sortedA.begin(), sortedA.end());
    std::sort(sortedB.begin(), sortedB.end());

    // 交集
    std::vector<int> intersection;
    std::set_intersection(sortedA.begin(), sortedA.end(),
        sortedB.begin(), sortedB.end(),
        std::back_inserter(intersection));
       
    // 计算两个邻居的交集
    //std::vector<int> intersection;
    //// 遍历较小的集合，提高效率
    //const auto& smaller = (neighborsA.size() < neighborsB.size()) ? neighborsA : neighborsB;
    //const auto& larger = (neighborsA.size() < neighborsB.size()) ? neighborsB : neighborsA;

    //for (int value : smaller) {
    //    if (larger.count(value) > 0) {
    //        intersection.push_back(value);
    //    }
    //}

    // 并集
    std::unordered_set<int> unionSet;
    unionSet.insert(neighborsA.begin(), neighborsA.end());
    unionSet.insert(neighborsB.begin(), neighborsB.end());

    if (unionSet.empty()) {
        return 0;
    }
    else {
        return static_cast<double>(intersection.size()) / unionSet.size();
    }
}

// 计算两个查询组的相似度，基于所有查询对的平均相似度
double gSimilarity(query_group& groupA, query_group& groupB, Graph& graph) {
    double totalSimilarity = 0.0;
    for (query_nodes& qA : groupA) {
        for (query_nodes& qB : groupB) {
            double x = qSimilarity(qA, qB, graph);
            if (x == 0) { return 0; }
            totalSimilarity += x;
        }
    }
    return totalSimilarity / (groupA.size() * groupB.size());
}

// 随机打乱向量中的元素
void fisherYatesShuffle(std::vector<int>& vec) {
    // 使用当前时间作为随机数种子
    std::srand(static_cast<unsigned>(std::time(0)));

    for (size_t i = vec.size() - 1; i > 0; --i) {
        // 生成一个0到i之间的随机索引
        size_t j = std::rand() % (i + 1);

        // 交换vec[i]和vec[j]
        std::swap(vec[i], vec[j]);
    }
}

// 辅助函数：计算两点之间的最短路径长度
int shortestPathLength(Graph& graph, int start, int end) {
    std::unordered_set<int> visited;
    std::queue<std::pair<int, int>> q;
    q.push({ start, 0 });
    visited.insert(start);

    while (!q.empty()) {
        auto [node, dist] = q.front();
        q.pop();

        if (node == end) {
            return dist;
        }

        for (int neighbor : graph.getNeighbors(node)) {
            if (visited.find(neighbor) == visited.end()) {
                q.push({ neighbor, dist + 1 });
                visited.insert(neighbor);
            }
        }
    }
    return -1;
}

// 测试批量最小 CSP 搜索，生成查询集并运行批处理算法
void test_batchmin(Graph& graph, int flag, double para) {

    std::string path = kSingleOutputCsv;
    // 获取图中所有节点
    TreeIndex index = TreeIndex(graph);
    double i1 = 0;

    std::vector<int> allNodes;
    for (const auto& pair : index.getAdj()) {
        allNodes.push_back(pair.first);
    }

    int num = 20; // 随机测试100次
    std::ofstream outFile(path);
    if (!outFile.is_open()) {
        std::cerr << "无法打开 results.csv 文件" << std::endl;
        return;
    }

    // 写入 CSV 头部
    outFile << "TestNumber,QueryNodes,size,min_size,k\n";

    query_group group; // batch的查询集（全部）

    // 实验一  -----------------  查询集的取点策略
    std::unordered_set<int> beginnodes; // 初始的点集合
    std::unordered_set<int> visited; // 已包含过的点集合

    // 初始化随机数生成器
    srand(static_cast<unsigned int>(time(0)));
    // 打乱allNodes中元素的顺序
    fisherYatesShuffle(allNodes);

    std::unordered_map<int, int> cores = CoreGroup::coreGroupsAlgorithm(index); // 获取所有点的核心度

    // 随机取点，随机性太大 可改进
    for (auto& node : allNodes) {
        if (beginnodes.size() >= 6) {
            break;
        }

        if (cores[node] >= shift_1 && cores[node] <= shift_2 && visited.find(node) == visited.end()) {
            beginnodes.insert(node);
            visited.insert(node);

            int currentLevel = 0;  // 当前层数
            int maxLevel = 3;
            std::queue<int> qq;
            qq.push(node);
            while (!qq.empty() && currentLevel < maxLevel) {
                int levelSize = qq.size();  // 当前层的节点数量

                // 处理当前层的所有节点
                for (int i = 0; i < levelSize; i++) {
                    int current = qq.front();
                    qq.pop();

                    // 遍历所有邻居
                    for (int neighbor : graph.getNeighbors(current)) {
                        if (visited.find(neighbor) == visited.end()) {
                            qq.push(neighbor);
                            visited.insert(neighbor);
                        }
                    }
                }
                currentLevel++;  // 进入下一层
            }
        }
    }
    std::cout << "取点ff：" << i1 << "秒" << std::endl;

    std::unordered_set<int> visited2;
    for (int i = 0; i < num; i++) {

        if (i % 20 == 0) {
            visited2.clear();
        }

        std::unordered_set<int> querySet;
        std::vector<int> beginnodes_(beginnodes.begin(), beginnodes.end());
        fisherYatesShuffle(beginnodes_);
        for (auto& node : beginnodes_) {
            if (querySet.size() == 3) {
                break;
            }
            querySet.insert(node);
        }

        // 第1种取法
        for (auto& node : beginnodes_) {
            if (querySet.find(node) != querySet.end()) {
                continue;
            }
            if (querySet.size() == 6) {
                break;
            }
            int numQueryNodes = 1;
            for (auto& neighbor : graph.getNeighbors(node)) {
                if (numQueryNodes == 0) {
                    break;
                }
                if (visited2.find(neighbor) == visited2.end() && querySet.find(neighbor) == querySet.end() && beginnodes.find(neighbor) == beginnodes.end() && cores[neighbor] >= shift_1 && cores[neighbor] <= shift_2) {
                    querySet.insert(neighbor);
                    visited2.insert(neighbor);
                    numQueryNodes--;
                }
            }
        }

        // 第2种取法
        fisherYatesShuffle(allNodes);
        for (auto& node : allNodes) {
            if (querySet.size() >= 5) {
                break;
            }
            if (visited2.find(node) == visited2.end() && cores[node] == shift_1) {
                querySet.insert(node);
            }
        }

        // 保存查询顶点集
        group.push_back(querySet);
    }

    // 关闭文件
    outFile.close();

    std::cout << "time_greedy " << time_greedy << " "
        << "time_steiner " << time_steiner << " "
        << "time_simple " << time_simple << std::endl;
    // 计算相似度
    simi = gSimilarity(group, group, graph);
    std::cout << "相似度为: " << simi << std::endl;
    simi = simi + para;

    SharingIndex ba_index = SharingIndex(graph);

    ba_index.batchsearch(group);

    if (flag == 1) {
        ba_index.batchMinsearch_1(group);
    }
    else {
        ba_index.batchMinsearch_2(group);
    }

    std::cout << "clu_1 " << clu_1 << " "
        << "time_1 " << time_1 << " "
        << "time_2 " << time_2 << " "
        << "-time_3 -" << time_3 << std::endl;

    double batch = 0;
    std::cout << "batch: " << batch << std::endl;
    std::cout << batch << std::endl;
}

// 测试聚类算法，生成随机查询集并运行层次聚类
void test_cluster(Graph& graph, std::string path) {

    int num = 100;
    // 取最大连通分量
    TreeIndex yv = TreeIndex(graph);
    std::vector<int> allNodes;
    for (const auto& pair : yv.getAdj()) {
        allNodes.push_back(pair.first);
    }

    query_group group;
    for (int i = 0; i < num; i++) {

        std::unordered_set<int> querySet;
        fisherYatesShuffle(allNodes);
        for (auto& node : allNodes) {
            if (querySet.size() >= 6) {
                break;
            }
            querySet.insert(node);
        }
        // Sleep(200);

        // 保存查询顶点集
        group.push_back(querySet);
        for (auto& no : querySet) {
            std::cout << no << " ";
        }
        std::cout << std::endl;
    }

    SharingIndex index2 = SharingIndex(graph);
    auto start = std::chrono::steady_clock::now();
    index2.optimizedHierarchicalClustering(group, 0);
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "time: " << elapsed.count() / 1000 << " 毫秒" << std::endl;
}

// 比较不同 CSP 搜索方法的性能
void test_compare(Graph& graph) {
    query_nodes yrr = { 235795, 287265 };
    query_nodes yr = { 14837, 12908, 22398, 11890, 8549 };


    query_group group;
    group.push_back(yrr);
    // group.push_back({9649, 10193, 6879, 8469, 15944, 2519});
    // group.push_back({9648, 15944, 8469, 14582, 2519});
    // group.push_back({8671, 5892, 15944, 14582, 6879, 2519});
    // group.push_back({12165, 6879, 8549, 8469, 15944});
    // group.push_back({15165, 2519, 14582, 8469, 6879});
    // group.push_back({8669, 2519, 15944, 8469, 14582});
    // group.push_back({7766, 2519, 6879, 8549, 14582});
    // group.push_back({3965, 14582, 2519, 6879, 8549});
    // group.push_back({14582, 6879, 1553, 8469, 15944});
    // group.push_back({9650, 2519, 6879, 8549, 14582});
    // group.push_back({9650, 15944, 6879, 14582, 2519});
    // group.push_back({7422, 14582, 2519, 8549, 15944});
    // group.push_back({2519, 9886, 15944, 8549, 14582});
    // group.push_back({6646, 8549, 2519, 8469, 15944});
    // group.push_back({6460, 2519, 8469, 15944, 14582});
    // group.push_back({4888, 2519, 8549, 14582, 6879});
    // group.push_back({16480, 2519, 8549, 15944, 8549});
    // group.push_back({16754, 6879, 8549, 15944, 8469});
    // group.push_back({6596, 2519, 8469, 15944, 6879});
    // group.push_back({15944, 8549, 2519, 6080, 14582});
    for (auto& q : group) {
        std::cout << q.size() << std::endl;
    }


    // query_group xx = {yr};

    TreeIndex index = TreeIndex(graph);
    int k = 0;

    clock_t p1 = clock();
    std::unordered_set<int> y = index.RetrievalShellStruct(yrr, k);
    std::cout << "k: " << k << std::endl;
    std::cout << "y: " << y.size() << std::endl;
    index.greedyConnection(yrr, k, y);
    clock_t p2 = clock();
    double c1 = (double)(p2 - p1) / CLOCKS_PER_SEC;
    std::cout << "baseline的min_csp总时间: " << c1 << "秒" << std::endl;

    // 最初的batch
    // SharingIndex index2 = SharingIndex(graph);

    // clock_t p3 = clock();
    // std::unordered_set<int> yy = index2.batchsearch(xx);
    // std::cout<< "yy: " << yy.size() << std::endl;
    // index2.batchMinsearch(xx);
    // clock_t p4 = clock();
    // double c2 = (double)(p4 - p3) / CLOCKS_PER_SEC;
    // std::cout<< "batch的min_csp总时间: " << c2 << "秒" << std::endl;

    // 精确方法
    SharingIndex index2 = SharingIndex(graph);
    clock_t p3 = clock();
    std::unordered_set<int> yy = index2.batchsearch(group);
    std::cout << "yy: " << yy.size() << std::endl;
    index2.batchMinsearch_1(group);
    clock_t p4 = clock();
    double c2 = (double)(p4 - p3) / CLOCKS_PER_SEC;
    std::cout << "batch的min_csp总时间: " << c2 << "秒" << std::endl;



    std::cout << "time_greedy " << time_greedy << " "
        << "time_steiner " << time_steiner << " "
        << "time_simple " << time_simple << std::endl;
    std::cout << "time_1 " << time_1 << " "
        << "time_2 " << time_2 << std::endl;
}

// 中期检查，测试globalsearch算法的正确性和性能
// 测试全局搜索算法，验证其正确性和性能
void test_global_search(Graph& graph, const query_nodes& queryNodes)
{
    std::cout << "\n===== test_globalsearch =====" << std::endl;
    std::cout << "Query nodes: ";
    for (int u : queryNodes) {
        std::cout << u << " ";
    }
    std::cout << std::endl;

    query_nodes queryCopy = queryNodes;
    Graph result = graph.globalsearch(queryCopy);

    TreeIndex index = TreeIndex(graph);
    int kk = 0;
    std::unordered_set<int> y = index.RetrievalShellStruct(queryCopy, kk);

    // 写入 global_result.csv
    std::ofstream resultFile(kGlobalOutputCsv);
    resultFile << "Source,Target\n";
    for (const auto& pair : result.getAdj()) {
        int source = pair.first;
        for (int target : pair.second) {
            if (source < target) { // 避免重复边
                resultFile << source << "," << target << "\n";
            }
        }
    }
    resultFile.close();

    std::cout << "globalsearch result node count: " << result.getn() << std::endl;
    std::cout << "globalsearch result edge count: " << result.getM() << std::endl;
    std::cout << "===== end test_globalsearch =====\n" << std::endl;
}

// 测试 h0 算法，初始化 beginnodes 集合
void test_h0(Graph& graph) {
    // 获取图中所有节点
    TreeIndex index = TreeIndex(graph);
    std::vector<int> allNodes;
    for (const auto& pair : index.getAdj()) {
        allNodes.push_back(pair.first);
    }

    std::unordered_set<int> visited; // 已包含过的点集合

    // 初始化随机数生成器
    srand(static_cast<unsigned int>(time(0)));
    // 打乱allNodes中元素的顺序
    fisherYatesShuffle(allNodes);

    std::unordered_map<int, int> cores = CoreGroup::coreGroupsAlgorithm(index); // 获取所有点的核心度

    // 随机取点，随机性太大 可改进
    for (auto& node : allNodes) {
        if (beginnodes.size() >= 6) {
            break;
        }

        if (cores[node] >= shift_1 && cores[node] <= shift_2 && visited.find(node) == visited.end()) {
            beginnodes.insert(node);
            visited.insert(node);

            int currentLevel = 0;  // 当前层数
            int maxLevel = 3;
            std::queue<int> qq;
            qq.push(node);
            while (!qq.empty() && currentLevel < maxLevel) {
                int levelSize = qq.size();  // 当前层的节点数量

                // 处理当前层的所有节点
                for (int i = 0; i < levelSize; i++) {
                    int current = qq.front();
                    qq.pop();

                    // 遍历所有邻居
                    for (int neighbor : graph.getNeighbors(current)) {
                        if (visited.find(neighbor) == visited.end()) {
                            qq.push(neighbor);
                            visited.insert(neighbor);
                        }
                    }
                }
                currentLevel++;  // 进入下一层
            }
        }
    }
    for (auto& node : beginnodes) {
        std::cout << node << " ";
        std::cout << cores[node] << std::endl;
    }

    std::cout << std::endl;
}

// 测试 h 算法，初始化 beginnodes 并添加邻居
void test_h(Graph& graph) {
    // 获取图中所有节点
    TreeIndex index = TreeIndex(graph);
    std::vector<int> allNodes;
    for (const auto& pair : index.getAdj()) {
        allNodes.push_back(pair.first);
    }

    // 初始化随机数生成器
    srand(static_cast<unsigned int>(time(0)));
    // 打乱allNodes中元素的顺序
    fisherYatesShuffle(allNodes);

    std::unordered_map<int, int> cores = CoreGroup::coreGroupsAlgorithm(index); // 获取所有点的核心度

    // 随机取点，随机性太大 可改进
    for (auto& node : allNodes) {
        if (cores[node] >= shift_1 && cores[node] <= shift_2) {
            beginnodes.insert(node);
            for (auto& nei : graph.getNeighbors(node)) {
                beginnodes.insert(nei);
                break;
            }
            break;
        }
    }
    for (auto& node : beginnodes) {
        std::cout << node << " ";
        std::cout << cores[node] << std::endl;
    }

    std::cout << std::endl;
}

// 测试 h1 算法，生成查询集并运行优化层次聚类
void test_h1(Graph& graph, double para, size_t a1, size_t a2, size_t a3) {
    TreeIndex index = TreeIndex(graph);
    std::vector<int> allNodes;
    for (const auto& pair : index.getAdj()) {
        allNodes.push_back(pair.first);
    }
    std::unordered_map<int, int> cores = CoreGroup::coreGroupsAlgorithm(index); // 获取所有点的核心度
    int num = 1; // 随机测试100次
    std::unordered_set<int> visited2;
    for (int i = 0; i < num; i++) {

        if (i % 20 == 0) {
            visited2.clear();
        }

        std::unordered_set<int> querySet;
        std::vector<int> beginnodes_(beginnodes.begin(), beginnodes.end());
        fisherYatesShuffle(beginnodes_);
        for (auto& node : beginnodes_) {
            if (querySet.size() == a1) {
                break;
            }
            querySet.insert(node);
        }

        // 第1种取法
        for (auto& node : beginnodes_) {
            if (querySet.find(node) != querySet.end()) {
                continue;
            }
            if (querySet.size() == a2) {
                break;
            }
            int numQueryNodes = 1;
            for (auto& neighbor : graph.getNeighbors(node)) {
                if (numQueryNodes == 0) {
                    break;
                }
                if (visited2.find(neighbor) == visited2.end() && querySet.find(neighbor) == querySet.end() && beginnodes.find(neighbor) == beginnodes.end() && cores[neighbor] >= shift_1 && cores[neighbor] <= shift_2) {
                    querySet.insert(neighbor);
                    visited2.insert(neighbor);
                    numQueryNodes--;
                }
            }
        }

        // 第2种取法
        fisherYatesShuffle(allNodes);
        for (auto& node : allNodes) {
            if (querySet.size() >= a3) {
                break;
            }
            if (visited2.find(node) == visited2.end() && cores[node] >= shift_1 && cores[node] <= shift_2) {
                querySet.insert(node);
            }
        }
        // 保存查询顶点集
        group.push_back(querySet);
    }

    // 计算相似度
    simi = gSimilarity(group, group, graph);
    std::cout << "相似度为: " << simi << std::endl;
    simi = simi + para;

    SharingIndex ba_index = SharingIndex(graph);
    ba_index.batchsearch(group);
    ba_index.optimizedHierarchicalClustering(group, simi);
    for (auto& codes : ba_index.clusters_op) {
        ba_index.Clustering_first(codes);
        std::cout << "第二次聚类: " << ba_index.clusters.size() << " / " << codes.size() << std::endl;
    }
}

// 测试 h2 算法，运行单次和批量搜索并记录结果
void test_h2(Graph& graph, double para) {

    std::cout << "test_h2开始" << std::endl;
    std::string path = kSingleOutputCsv;
    // 获取图中所有节点
    TreeIndex index = TreeIndex(graph);

    std::vector<int> allNodes;
    for (const auto& pair : index.getAdj()) {
        allNodes.push_back(pair.first);
    }

    std::ofstream outFile(path);
    if (!outFile.is_open()) {
        std::cerr << "无法打开 results.csv 文件" << std::endl;
        return;
    }

    // 写入 CSV 头部
    outFile << "TestNumber,QueryNodes,size,min_size,k\n";

    // 实验一  -----------------  查询集的取点策略
    std::unordered_set<int> beginnodes; // 初始的点集合
    std::unordered_set<int> visited; // 已包含过的点集合

    // 初始化随机数生成器
    srand(static_cast<unsigned int>(time(0)));
    // 打乱allNodes中元素的顺序
    fisherYatesShuffle(allNodes);
    std::unordered_map<int, int> cores = CoreGroup::coreGroupsAlgorithm(index); // 获取所有点的核心度

    // 检查group是否为空
    if (group.empty()) {
        std::cout << "错误：group为空，请先运行test_h1函数生成查询集" << std::endl;
        return;
    }

    double single = 0, csp = 0;
    for (size_t i = 0; i < group.size(); i++) {

        std::unordered_set<int> querySet = group.at(i);

        // baseline的运行
        int k = 0;
        clock_t p1 = clock();
        std::unordered_set<int> H = index.RetrievalShellStruct(querySet, k);
        clock_t p2 = clock();
        std::unordered_set<int> H_min = index.greedyConnection(querySet, k, H);
        clock_t p3 = clock();
        csp += (double)(p2 - p1) / CLOCKS_PER_SEC;
        single += (double)(p3 - p1) / CLOCKS_PER_SEC;

        // 将查询顶点集转换为字符串
        std::string queryNodesStr;
        for (int node : querySet) {
            queryNodesStr += std::to_string(node) + " ";
        }

        // 将结果写入 CSV 文件
        outFile << i << ","
            << queryNodesStr << ","
            << H.size() << ","
            << H_min.size() << ","
            << k
            << "\n";
    }

    // 关闭文件
    outFile.close();

    std::cout << "time_greedy " << time_greedy << " "
        << "time_steiner " << time_steiner << " "
        << "time_simple " << time_simple << std::endl;
    // 计算相似度
    //simi = gSimilarity(group, group, graph);
    //std::cout << "相似度为: " << simi << std::endl;
    //simi = simi + para;

    //double batch1 = 0;
    //double batch2 = 0;
    //double csp_batch = 0;
    //SharingIndex ba_index = SharingIndex(graph);

    //clock_t p1 = clock();
    //ba_index.batchsearch(group);
    //clock_t p2 = clock();
    //csp_batch += (double)(p2 - p1) / CLOCKS_PER_SEC;

    //ba_index.batchMinsearch_1(group);
    //clock_t p3 = clock();
    //ba_index.batchMinsearch_2(group);

    //clock_t p4 = clock();
    //batch1 = (double)(p3 - p1) / CLOCKS_PER_SEC;
    //batch2 = (double)(p4 - p3) / CLOCKS_PER_SEC + (double)(p2 - p1) / CLOCKS_PER_SEC;

    //std::cout << "csp_single: " << csp << std::endl;
    //std::cout << "single: " << single << std::endl;
    //std::cout << "csp_batch: " << csp_batch << std::endl;
    //std::cout << "batch1: " << batch1 << std::endl;
    //std::cout << "batch2: " << batch2 << std::endl;
}

// 验证结果一致性
// 验证两个核心数映射的结果一致性
bool verifyResults(const std::unordered_map<int, int>& cores1,
    const std::unordered_map<int, int>& cores2) {
    if (cores1.size() != cores2.size()) return false;
    for (const auto& [node, core1] : cores1) {
        auto it = cores2.find(node);
        if (it == cores2.end() || it->second != core1) {
            std::cout << "节点 " << node << " 核心数不一致: " << core1 << " vs " << it->second << std::endl;
            return false;
        }
    }
    return true;
}

// 主函数：加载图数据，处理命令行参数初始化查询节点，并执行全局搜索测试
int main(int argc, char* argv[]) {
    std::string path[] = { kDefaultDataset };

    // 测试使用的无向图
    Graph graph(std::string(kDatasetRoot) + path[0]);

    std::unordered_set<int> query = { 300 };
    test_global_search(graph, query);

    return 0;
}

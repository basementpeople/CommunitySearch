#ifndef TREEINDEX_H
#define TREEINDEX_H

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <stack>
#include <queue>
#include <set>
#include <map>
#include <climits>
#include <chrono>
#include <utility>
#include "Graph.h"
#include "CoreGroup.h"
#include "VertexScore.h"
// #include "absl/container/flat_hash_map.h"
// #include "absl/container/flat_hash_set.h"

extern double time_greedy, time_steiner, time_simple;

// 最小堆
class MinDegreeHeap {
public:
    using Node = int;

    // 堆节点：{degree, node_id}
    using HeapNode = std::pair<int, Node>;

    // 小顶堆，比较函数
    std::priority_queue<HeapNode, std::vector<HeapNode>, std::greater<>> heap;

    // 当前有效度数（latest degree），用于惰性删除
    std::unordered_map<Node, int> min_degree;

    void update(Node u, int degree) {
        min_degree[u] = degree;
        heap.emplace(min_degree[u], u);
    }

    // 弹出当前有效的最小度节点
    HeapNode pop_min() {
        while (!heap.empty()) {
            auto [deg, u] = heap.top();
            heap.pop();

            // 如果该节点度数没有被更新过，说明是最新的
            if (min_degree[u] == deg)
                return { deg, u };
        }
        return { 0, -1 }; // 或者 throw 异常
    }

    // 查看最小值但不弹出
    HeapNode peek_min() {
        while (!heap.empty()) {
            auto [deg, u] = heap.top();
            if (min_degree[u] == deg) {
                return { deg, u };
            }
            heap.pop(); // 丢弃旧版本
        }
        return { 0, -1 };
    }

    bool empty() const {
        return min_degree.empty();
    }

    // 从堆中移除一个点（比如该点被删除）
    void remove(Node u) {
        min_degree.erase(u);
        // 不从堆中删，等弹出时惰性删除
    }

    int get_degree(Node u) const {
        if (min_degree.count(u)) return min_degree.at(u);
        return -1;
    }

};

class TreeIndex : public Graph {
public:
    TreeIndex(Graph& graph);
    ~TreeIndex() {};
    // 打印核心索引和连通分量树 -- To be optimized
    void printTreeIndex();
    // 得到最大连通分量
    void getMaxCom();
    // 找到目标shell层的父连通分量ID
    int findClosestShellLayer(int componentId, int targetShell);
    // 修改最高父连通分量
    int modifyHighestParent(int componentId, int newParentId, int I);

    // 基于索引的社区搜索算法
    std::unordered_set<int> retrievalShellStruct(const query_nodes& queryNodes, int& k);
    // 获取连通分量id 对应的 节点集合
    std::unordered_set<int> getNodesFromCom(int id);
    // 
    std::unordered_set<int> getchnodes_2(int id, int& k);
    // GrCon算法
    std::unordered_set<int> greedyConnection(const query_nodes& queryNodes, int k, std::unordered_set<int> H);
    // GrCon算法第一步 greedystep
    std::unordered_set<int> greedyStep(const query_nodes& queryNodes, int k, std::unordered_set<int>& H_x);
    // GrCon算法第二步 connectionstep
    std::unordered_set<int> connectionStep(std::unordered_set<int>& H, const query_nodes& queryNodes, int k);
    // 斯坦纳树
    query_nodes steinerTree(std::unordered_set<int>& H, const query_nodes& terminals);
    // greedyStep_simply 简化版贪婪算法
    std::unordered_set<int> greedyStep_simply(const query_nodes& queryNodes, int k, std::unordered_set<int>& H_x);
    // 输出结果 -- To be optimized
    void output(std::unordered_set<int> ans, std::string path, int k);

protected:
    // 复制图状态
    void copyGraphState(const Graph& graph);
    // 按shell层分组节点
    std::unordered_map<int, std::unordered_set<int>> groupNodesByShell() const;
    // 处理shell层
    void processShellLayer(int shell, const std::unordered_set<int>& shellNodes,
                           std::unordered_set<int>& accumulatedNodes, int& nextComponentId);

    // 最大shell层
    int coreMax;

    // 节点 -> 核心度（即shell层）
    std::unordered_map<int, int> nodeToShell;

    // 节点 -> 连通分量id
    std::unordered_map<int, int> nodeToCom;

    // 连通分量id -> 节点集
    std::unordered_map<int, std::unordered_set<int>> comToNodes; 

    // 核心度 -> 连通分量id集
    std::unordered_map<int, std::unordered_set<int>> shellToComs;

    // 连通分量id -> 父连通分量id
    std::unordered_map<int, int> comToParent;

    // 连通分量id -> 子连通分量id集
    std::unordered_map<int, std::unordered_set<int>> comToChildren; 

};

#endif // TREE_H
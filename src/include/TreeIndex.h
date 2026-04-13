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
    TreeIndex(Graph& graph); // 得到的索引不是树结构
    // TreeIndex_2() : shell_count(0) {};
    ~TreeIndex() {};
    void printTreeIndex();
    void beMaxcom();
    int findClosestShellLayer(int componentId, int targetShell);
    int modifyHighestParent(int componentId, int newParentId, int I);

    std::unordered_set<int> RetrievalShellStruct(query_nodes& queryNodes, int& k);
    std::unordered_set<int> getchnodes(int id);
    std::unordered_set<int> getchnodes_2(int id, int& k);

    std::unordered_set<int> greedyConnection(query_nodes& queryNodes, int k, std::unordered_set<int> H);
    std::unordered_set<int> greedyStep(query_nodes& queryNodes, int k, std::unordered_set<int>& H_x);
    //absl::flat_hash_set<int> greedyStep(query_nodes& queryNodes, int k, std::unordered_set<int>& H_x);
    std::unordered_set<int> greedyStep_2(query_nodes& queryNodes, int k, std::unordered_set<int>& H);
    bool checkMinDegree(const std::unordered_map<int, int>& a, int k);
    bool checkComponent(std::unordered_map<int, int>& a);

    std::unordered_set<int> connectionStep(std::unordered_set<int>& H, query_nodes& queryNodes, int k);
    query_nodes steinerTree(std::unordered_set<int>& H, query_nodes& terminals);
    std::unordered_set<int> greedyStep_simply(query_nodes& queryNodes, int k, std::unordered_set<int>& H_x);
    void output(std::unordered_set<int> ans, std::string path, int k);

protected:
    int core_max;
    std::unordered_map<int, int> cores;  // 点 核心索引
    std::unordered_map<int, int> nodetocom;  // 点 连通分量id
    std::unordered_map<int, std::unordered_set<int>> comtonode;  // 连通分量id 点
    std::unordered_map<int, std::unordered_set<int>> Cktocom;  // Ck 的连通分量id集
    std::unordered_map<int, int> comtopa;  // 连通分量id 父连通分量id
    std::unordered_map<int, std::unordered_set<int>> comtoch;  // 连通分量id 子连通分量id
    int id_threshold;

};

#endif // TREE_H
#ifndef GRAPH_H
#define GRAPH_H

#include <algorithm>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <limits>

 // 查询顶点集
using query_nodes = std::unordered_set<int>;
// 查询顶点集的聚类
using query_group = std::vector<query_nodes>;

class Graph {
public:
    // 读取文件，获得初始图
    Graph(const std::string& path);
    // 用来传递成员给子类，是复制构造函数
    Graph(const Graph& graph)
        : adj(graph.adj),
          degrees(graph.degrees),
          orderedNodes(graph.orderedNodes),
          minDegree(graph.minDegree),
          maxDegree(graph.maxDegree),
          m(graph.m),
          n(graph.n),
          N(graph.N) {};
    // 读取哈希表，获得初始图
    Graph(std::unordered_set<int>& subVertices, Graph& graph);
    // 无参、默认构造函数
    Graph() : minDegree(0), maxDegree(0), m(0), n(0), N(0) {};
    // 析构函数
    ~Graph() {};

    // 从文件中读取图
    void readFromFile(const std::string& fileName);
    // 添加节点到图中
    void addNode(int node);
    // 添加两个节点之间的边
    void addEdge(int from, int to);
    // 计算每个节点的度
    std::unordered_map<int, int> computeDegrees();
    // 打印图的相关信息
    void statistic();
    // 检查查询集是否连通
    bool isConnected(const query_nodes& queryNodes, const std::unordered_map<int, int>& degree) const;
    // 获取节点的邻居
    std::unordered_set<int>& getNeighbors(int node) {
        return adj.at(node);
    }
    // 获取节点的邻居的常量引用
    const std::unordered_set<int>& getNeighbors(int node) const {
        static const std::unordered_set<int> kEmptyNeighbors;
        const auto it = adj.find(node);
        return (it != adj.end()) ? it->second : kEmptyNeighbors;
    }
    // 得到子图中的最小度
    int getMinDegree(std::unordered_set<int>& nodes) const;
    // 确保结果联通
    std::unordered_set<int> getComponent(const query_nodes& queryNodes, const std::unordered_set<int>& candidateNodes);
    // 移除节点，更新图的相关信息 -- To be optimized
    void removeNode(int node);

    // Greedy算法 每次删点都得到一次result会非常慢
    std::unordered_set<int> globalsearch(const query_nodes& queryNodes);
    // K-GS: 先按多源BFS扩展到K规模，再做global search -- To be optimized
    std::unordered_set<int> kGlobalsearch(const query_nodes& queryNodes, int kCount);

    // 获取受保护的数据成员
    std::unordered_map<int, std::unordered_set<int>>& getAdj() { return adj; }
    std::unordered_map<int, int>& getDegrees() { return degrees; }
    std::vector<std::unordered_set<int>>& getOrderedNodes() { return orderedNodes; }

    // 只读受保护的数据成员
    const std::unordered_map<int, std::unordered_set<int>>& getAdj() const { return adj; }
    const std::unordered_map<int, int>& getDegrees() const { return degrees; }
    const std::vector<std::unordered_set<int>>& getOrderedNodes() const { return orderedNodes; }
    int getMinDegree() const { return minDegree; }
    int getMaxDegree() const { return maxDegree; }
    int getm() const { return m; }
    int getn() const { return n; }
    int getN() const { return N; }

protected:
    // 图的邻接表表示：节点 ID 和其相邻节点
    std::unordered_map<int, std::unordered_set<int>> adj;

    // 图中节点的度
    std::unordered_map<int, int> degrees;

    // 表示具有相同度的节点的集合的向量,key为degree
    std::vector<std::unordered_set<int>> orderedNodes;

    // 图中所有节点的最小度
    int minDegree;

    // 图中所有节点的最大度
    int maxDegree;

    // 图的边数
    int m;

    // 图的点数
    int n;

    // 图中所有点的最大值
    int N;
};

#endif
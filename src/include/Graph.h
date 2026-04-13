#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <stack>
#include <map>
#include <queue>
#include <cmath>
#include <ctime>

#define query_nodes std::unordered_set<int>  // 查询顶点集
#define query_group std::vector<query_nodes>  // 多个查询顶点集

class Graph {
public:
    Graph(const std::string path); // 读取文件，获得初始图
    Graph(const Graph& graph) :adj(graph.adj), degrees(graph.degrees), orderedNodes(graph.orderedNodes), minimumDegree(graph.minimumDegree), Dmax(graph.Dmax), m(graph.m), n(graph.n) {}; // 用来传递成员给子类，是复制构造函数
    Graph(std::unordered_set<int>& subVertices, Graph& graph); // 读取哈希表，获得初始图
    Graph() : minimumDegree(0), Dmax(0), m(0), n(0) {}; // 无参、默认构造函数
    ~Graph() {};

    // Greedy算法 每次删点都得到一次result会非常慢
    Graph globalsearch(query_nodes& queryNodes);

    // 辅助函数，固定最后 ------------------------------------
    void readFromFile(const std::string fileName); // 从文件中读取图
    void addNode(int node); // 添加节点到图中
    void addEdge(int from, int to); // 添加两个节点之间的边
    std::unordered_map<int, int> computeDegrees(); // 计算每个节点的度
    void statistic(); // 打印图的相关信息
    bool isConnected(query_nodes queryNodes, std::unordered_map<int, int> degree); // 检查查询集是否连通
    std::unordered_set<int>& getNeighbors(int node) {
        return getAdj()[node];
    }
    int getminimumDegree(std::unordered_set<int>& nodes); // 得到子图中的最小度
    std::unordered_set<int> getComponent(query_nodes& queryNodes, std::unordered_set<int>& result_end); // 确保结果联通

    void removeNode(int node);

    // 获取受保护的数据成员
    int getN() { return N; }
    int getn() { return n; }
    int getM() { return m; }
    int getDmax() { return Dmax; }
    int getminimumDegree() { return minimumDegree; }
    std::vector<std::unordered_set<int>>& getOrderedNodes() { return orderedNodes; }
    std::unordered_map<int, int>& getDegrees() { return degrees; }
    std::unordered_map<int, std::unordered_set<int>>& getAdj() { return adj; }

protected:
    std::unordered_map<int, std::unordered_set<int>> adj; // 图的邻接表表示：节点 ID 和其相邻节点

    std::unordered_map<int, int> degrees; // 图中节点的度

    std::vector<std::unordered_set<int>> orderedNodes; // 表示具有相同度的节点的集合的向量,key为degree

    int minimumDegree; // 图中所有节点的最小度

    int Dmax; // 图中所有节点的最大度

    int m; // 图的边数

    int n; // 图的点数

    int N; // 图中所有点的最大值

};

#endif
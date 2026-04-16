#ifndef COREGROUP_H
#define COREGROUP_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include "Graph.h"

// 这个方案不是对单个点做处理，而是对一组点同时做处理
class CoreGroup {
public:

    static std::unordered_map<int, int> coreDecomposition(Graph& graph) {
        return coreGroupsAlgorithm(graph);
    }

    // 核心分解函数 coreGroups
    static std::unordered_map<int, int> coreGroupsAlgorithm(Graph& graph)
    {
        std::unordered_map<int, int> nodeToShell; // 存储每个节点的核心度

        // 将节点按度数分类
        std::unordered_map<int, int> degrees = graph.getDegrees();  // 获取图的节点度数
        std::vector<std::unordered_set<int>> orderedNodes = graph.getOrderedNodes();

        int node;             // 当前处理的节点
        int lowestDegree = 0; // 当前的最低度数
        int neighborDegree;   // 邻居的度数
        int now = 0;

        // 主循环
        while (lowestDegree <= graph.getn()) {
            if (orderedNodes[lowestDegree].empty()) {
                ++lowestDegree; // 如果没有该度数的节点，增加度数
            }
            else {
                node = *orderedNodes[lowestDegree].begin();
                orderedNodes[lowestDegree].erase(node);
                nodeToShell[node] = lowestDegree; // 设置核心度
                if (lowestDegree > now) now = lowestDegree;
                degrees[node] = -1;         // 将节点度数设为-1，标记已处理

                // 更新所有邻居的度数
                for (int neighbor : graph.getNeighbors(node)) {
                    neighborDegree = degrees[neighbor];
                    if (neighborDegree > lowestDegree) {
                        orderedNodes[neighborDegree].erase(neighbor);
                        orderedNodes[neighborDegree - 1].insert(neighbor);
                        degrees[neighbor] = neighborDegree - 1;
                    }
                }
            }
        }
        return nodeToShell;
    }

    // 核心分解函数 coreGroups（使用最小堆）
    static std::unordered_map<int, int> coreGroupsAlgorithm_2(Graph& graph)
    {
        std::unordered_map<int, int> nodeToShell;          // 存储每个节点的核心度
        std::unordered_map<int, int> degrees;       // 存储每个节点的度数
        std::unordered_map<int, bool> inCore;       // 标记节点是否在核心中

        // 获取图的节点度数并初始化状态
        degrees = graph.getDegrees();
        for (const auto& [node, _] : degrees) {
            inCore[node] = true;
        }

        // 使用优先队列实现最小堆，存储(度数, 节点)对
        using HeapElement = std::pair<int, int>;
        std::priority_queue<HeapElement, std::vector<HeapElement>, std::greater<HeapElement>> minHeap;

        // 初始化堆
        for (const auto& [node, degree] : degrees) {
            minHeap.push({ degree, node });
        }

        int neighborDegree;

        // 主循环：使用最小堆迭代处理节点
        while (!minHeap.empty()) {
            auto [currentDegree, currentNode] = minHeap.top();
            minHeap.pop();

            // 跳过已处理的节点
            if (!inCore[currentNode]) continue;

            // 设置核心度并标记为已处理
            nodeToShell[currentNode] = currentDegree;
            inCore[currentNode] = false;

            // 更新邻居节点的度数
            for (int neighbor : graph.getNeighbors(currentNode)) {
                if (!inCore[neighbor]) continue;

                neighborDegree = degrees[neighbor];
                if (neighborDegree > currentDegree) {
                    // 度数减1
                    degrees[neighbor] = neighborDegree - 1;
                    // 将新的(度数, 节点)对加入堆中
                    minHeap.push({ degrees[neighbor], neighbor });
                }
            }
        }
        return nodeToShell;
    }

};
#endif
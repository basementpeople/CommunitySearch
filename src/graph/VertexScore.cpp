// VertexScore.cpp
#include "VertexScore.h"
#include <iostream>

// 定义全局变量
std::unordered_map<int, VertexScore> vertexScores;  // 存储最新评分
ScoreQueue pQueue;                                 // 优先队列

// 插入顶点及其评分（初始或更新）
void updateVertexScore(int v, int newPprime, int newPdouble) {
    // 1. 更新哈希表中的最新评分
    vertexScores[v] = VertexScore(v, newPprime, newPdouble, true);

    // 2. 插入新条目到队列（旧条目会被标记为无效）
    pQueue.push(vertexScores[v]);
}

// 提取有效优先级最高的顶点
int extractNextValidVertex() {
    while (!pQueue.empty()) {
        if (pQueue.size() > vertexScores.size() * 2) {
            // 重建队列
            ScoreQueue newQueue;
            for (const auto& kv : vertexScores) {
                if (kv.second.valid)
                    newQueue.push(kv.second);
            }
            pQueue.swap(newQueue);
        }
        VertexScore curr = pQueue.top();
        pQueue.pop();

        // 检查队列中的条目是否与哈希表中的最新评分一致
        if (curr.valid && vertexScores.count(curr.vertex) &&
            curr.p_prime == vertexScores[curr.vertex].p_prime &&
            curr.p_double == vertexScores[curr.vertex].p_double) {
            // 标记该条目为已使用（可选，避免重复使用）
            vertexScores[curr.vertex].valid = false;
            return curr.vertex;
        }
        // 否则跳过过时条目
    }
    return -1;  // 队列为空
}

// 清空所有数据结构
void clearAllData() {
    // 清空哈希表
    vertexScores.clear();

    // 清空优先队列
    // 方法一：通过交换空队列
    ScoreQueue emptyQueue;
    pQueue.swap(emptyQueue);

    // 方法二：循环弹出所有元素（如果上面的方法不适用）
    // while (!pQueue.empty()) {
    //     pQueue.pop();
    // }
}
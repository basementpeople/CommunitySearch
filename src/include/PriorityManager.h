#pragma once

#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <limits>

// 优先级封装
struct VertexScore {
    int node_id;
    int p_prime;   // p′
    int p_double;  // p′′

    VertexScore(int id, int p1, int p2)
        : node_id(id), p_prime(p1), p_double(p2) {}
};

// 字典序比较：p′优先，p′′次之
struct CompareVertexScore {
    bool operator()(const VertexScore& a, const VertexScore& b) const {
        if (a.p_prime != b.p_prime) return a.p_prime < b.p_prime;
        return a.p_double < b.p_double;
    }
};

class PriorityManager {
private:
    using ScoreQueue = std::priority_queue<VertexScore, std::vector<VertexScore>, CompareVertexScore>;

    ScoreQueue pq;  // 惰性堆
    std::unordered_map<int, std::pair<int, int>> scoreMap;  // 当前有效得分
    std::unordered_set<int> candidateSet;  // 当前在P中的节点

    const int HEAP_REBUILD_RATIO = 3;

    void rebuildHeap() {
        std::vector<VertexScore> newHeap;
        for (const auto& [node, score] : scoreMap) {
            if (candidateSet.count(node)) {
                newHeap.emplace_back(node, score.first, score.second);
            }
        }
        pq = ScoreQueue(CompareVertexScore(), std::move(newHeap));
    }

public:
    PriorityManager() = default;

    void insert(int node_id, int p_prime, int p_double) {
        scoreMap[node_id] = {p_prime, p_double};
        candidateSet.insert(node_id);
        pq.emplace(node_id, p_prime, p_double);

        if (pq.size() > HEAP_REBUILD_RATIO * scoreMap.size()) {
            rebuildHeap();
        }
    }

    bool contains(int node_id) const {
        return candidateSet.count(node_id);
    }

    void remove(int node_id) {
        candidateSet.erase(node_id);
        scoreMap.erase(node_id);
    }

    // 提取当前优先级最高的合法顶点（惰性更新）
    int extract() {
        while (!pq.empty()) {
            VertexScore top = pq.top();
            pq.pop();

            if (!candidateSet.count(top.node_id)) continue;

            auto it = scoreMap.find(top.node_id);
            if (it != scoreMap.end()) {
                auto [cur_p1, cur_p2] = it->second;
                if (cur_p1 == top.p_prime && cur_p2 == top.p_double) {
                    candidateSet.erase(top.node_id);
                    scoreMap.erase(top.node_id);
                    return top.node_id;
                }
            }
        }
        return -1;  // 无可用点
    }

    std::pair<int, int> getScore(int node_id) const {
        auto it = scoreMap.find(node_id);
        if (it != scoreMap.end()) return it->second;
        return {0, 0};
    }

    bool empty() const {
        return candidateSet.empty();
    }

    int size() const {
        return candidateSet.size();
    }

    void clear() {
        pq = ScoreQueue();
        scoreMap.clear();
        candidateSet.clear();
    }
};

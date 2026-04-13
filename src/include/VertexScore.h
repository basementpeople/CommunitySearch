// VertexScore.h
#ifndef VERTEXSCORE_H
#define VERTEXSCORE_H

#include <unordered_map>
#include <queue>

// 定义 VertexScore 结构体
struct VertexScore {
    int vertex;       // 顶点ID
    int p_prime;      // 连接性分数 p'(u)
    int p_double;     // 最小度分数 p''(u)
    bool valid;       // 标记是否为有效条目（用于处理更新）

    // 默认构造函数
    VertexScore() : vertex(0), p_prime(0), p_double(0), valid(false) {}
    // 何处使用了

    // 带参数的构造函数
    VertexScore(int v, int p1, int p2, bool vld = true)
        : vertex(v), p_prime(p1), p_double(p2), valid(vld) {
    }

    // 逆字典序比较（最大堆）

    bool operator<(const VertexScore& other) const {
        if (p_prime != other.p_prime) return p_prime < other.p_prime;
        if (p_double != other.p_double) return p_double < other.p_double;
        return vertex > other.vertex; // ✅ vertex ID 小的优先（稳定）
    }

};

// 使用别名定义优先队列
using ScoreQueue = std::priority_queue<VertexScore>;

// 声明全局变量
extern std::unordered_map<int, VertexScore> vertexScores;  // 存储最新评分
extern ScoreQueue pQueue;                                 // 优先队列

// 声明函数
void updateVertexScore(int v, int newPprime, int newPdouble);
int extractNextValidVertex();
void clearAllData();

#endif // VERTEXSCORE_H
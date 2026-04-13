#ifndef SHARINGINDEX_H
#define SHARINGINDEX_H

#include <vector>
#include <map>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "TreeIndex.h"

extern double simi;
extern double time_cluster, time_1, time_2, time_3, time_4, clu_1;

// 自定义哈希函数，用于 std::vector<int>
struct VectorHash {
    size_t operator()(const std::vector<int>& v) const {
        size_t hash = 0;
        for (const auto& item : v) {
            hash ^= std::hash<int>{}(item)+0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

// 自定义相等性比较函数，用于 std::vector<int>
struct VectorEqual {
    bool operator()(const std::vector<int>& a, const std::vector<int>& b) const {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (a[i] != b[i]) return false;
        }
        return true;
    }
};

// 自定义哈希函数，用于 std::unordered_set<int>
struct SetHash {
    size_t operator()(const std::unordered_set<int>& s) const {
        size_t hash = 0;
        for (const auto& item : s) {
            hash ^= std::hash<int>{}(item)+0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

// 自定义相等性比较函数，用于 std::unordered_set<int>
struct SetEqual {
    bool operator()(const std::unordered_set<int>& a, const std::unordered_set<int>& b) const {
        if (a.size() != b.size()) return false;
        for (const auto& item : a) {
            if (b.find(item) == b.end()) return false;
        }
        return true;
    }
};

// 自定义哈希函数，用于 int
struct IntHash {
    size_t operator()(const int& value) const {
        return std::hash<int>()(value);
    }
};

// 自定义相等性比较函数，用于 int
struct IntEqual {
    bool operator()(const int& a, const int& b) const {
        return a == b;
    }
};

// 自定义哈希函数，用于 std::pair<const int, std::unordered_set<int>>
struct PairEqual {
    bool operator()(const std::pair<const int, std::unordered_set<int>>& a,
        const std::pair<const int, std::unordered_set<int>>& b) const {
        // 首先比较 pair 的第一个元素
        if (a.first != b.first) return false;
        // 如果第一个元素相等，比较第二个元素（std::unordered_set<int>）
        return a.second == b.second;
    }
};

// 自定义哈希函数，用于 std::pair<const std::unordered_set<int>, std::unordered_set<int>>
struct PairHash {
    size_t operator()(const std::pair<const std::unordered_set<int>, std::unordered_set<int>>& p) const {
        // 为第一个元素（std::unordered_set<int>）计算哈希值
        size_t h = 0;
        for (const int& elem : p.first) {
            h ^= std::hash<int>{}(elem)+0x9e3779b9 + (h << 6) + (h >> 2);
        }

        // 为第二个元素（std::unordered_set<int>）计算哈希值，并与第一个元素的哈希值组合
        for (const int& elem : p.second) {
            h ^= std::hash<int>{}(elem)+0x9e3779b9 + (h << 6) + (h >> 2);
        }

        return h;
    }
};

// 3.21 unordered_set
struct UnorderedSetHash {
    std::size_t operator()(const std::unordered_set<int>& set) const {
        std::size_t hash = 0;
        for (const int& elem : set) {
            hash ^= std::hash<int>()(elem) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

struct UnorderedSetEqual {
    bool operator()(const std::unordered_set<int>& a, const std::unordered_set<int>& b) const {
        return a == b;
    }
};

class SharingIndex : public TreeIndex {
public:
    SharingIndex(Graph& graph) : TreeIndex(graph) { initializeMarks(); };
    ~SharingIndex() {};
    void initializeMarks();

    // batch查找解决CSP
    std::unordered_set<int> batchsearch(query_group& group);
    void printAndwrite(std::string path);

    void batchMinsearch(query_group& group);
    void Clustering_first(std::unordered_set<int>& ids); // 相同Ck，相同comid
    void Clustering_second(std::unordered_set<int>& ids); // 
    double querySimilarity(query_nodes& qA, query_nodes& qB); // 计算两个查询之间的相似度
    double groupSimilarity(query_group& groupA, query_group& groupB); // 计算两个查询顶点集之间的相似度
    std::vector<query_group> optimizedHierarchicalClustering(query_group& queryGroups, double threshold);

    void batchMinsearch_1(query_group& group); // 精确方法
    void batchMinsearch_2(query_group& group); // 快速方法

    // protected:
    //CSP
    int threshold; // code
    std::unordered_map<int, query_nodes> qtocode; // code -> q
    std::unordered_map<int, std::unordered_set<int>> codetoma; // code -> 标记
    std::unordered_map<int, std::unordered_set<int>> codetore; // code -> 结果
    std::vector<std::unordered_set<int>> lelist; // 剩余的点的数量 -> code
    std::unordered_map<int, int> codetole; // code -> 剩余的点的数量

    std::unordered_map<int, std::unordered_set<int>> idtore; // id -> 标记

    //MIN_CSP
    std::unordered_map<int, int> codetoid; // 保存batchsearch的code -> id
    std::unordered_map<int, int> codetok; // code -> k
    int clusters_count; // 第一次聚类的类数量
    int clusters_count2; // 第二次聚类的类数量
    std::unordered_map<int, std::unordered_set<int>> clusters;
    std::vector<std::unordered_set<int>> clusters_op;
    std::unordered_map<int, std::unordered_set<int>> greedy1; // code - ans
    std::unordered_map<int, std::unordered_set<int>> greedy1_; // cluster - ans
    std::unordered_map<int, std::unordered_set<int>> q2; // 

    std::unordered_map<int, std::unordered_set<int>> clusters2;
    std::unordered_map<int, std::unordered_set<int>> greedy2; // code - ans

    // 并查集的父节点映射
    std::unordered_map<int, int> parent;

    // 查找根节点并进行路径压缩
    int find(int x) {
        if (parent[x] != x) {
            parent[x] = find(parent[x]);
        }
        return parent[x];
    }

    // 合并两个集合
    void unite(int x, int y) {
        int rootX = find(x);
        int rootY = find(y);
        if (rootX != rootY) {
            parent[rootY] = rootX;
        }
    }

};

#endif // SHARINGINDEX_H
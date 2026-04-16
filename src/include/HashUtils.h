#ifndef HASHUTILS_H
#define HASHUTILS_H

#include <functional>
#include <unordered_set>
#include <utility>
#include <vector>

// 自定义哈希函数，用于 std::vector<int>
struct VectorHash {
    size_t operator()(const std::vector<int>& v) const {
        size_t hash = 0;
        for (const auto& item : v) {
            hash ^= std::hash<int>{}(item) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
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
            hash ^= std::hash<int>{}(item) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
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

// 自定义相等函数，用于 std::pair<const int, std::unordered_set<int>>
struct PairEqual {
    bool operator()(const std::pair<const int, std::unordered_set<int>>& a,
                    const std::pair<const int, std::unordered_set<int>>& b) const {
        if (a.first != b.first) return false;
        return a.second == b.second;
    }
};

// 自定义哈希函数，用于 std::pair<const std::unordered_set<int>, std::unordered_set<int>>
struct PairHash {
    size_t operator()(const std::pair<const std::unordered_set<int>, std::unordered_set<int>>& p) const {
        size_t h = 0;
        for (const int& elem : p.first) {
            h ^= std::hash<int>{}(elem) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        for (const int& elem : p.second) {
            h ^= std::hash<int>{}(elem) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }
};

// 自定义哈希函数，用于 std::unordered_set<int>
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

#endif // HASHUTILS_H

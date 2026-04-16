#ifndef SHARINGINDEX_H
#define SHARINGINDEX_H

#include <vector>
#include <map>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "TreeIndex.h"
#include "HashUtils.h"

extern double simi;
extern double time_cluster, time_1, time_2, time_3, time_4, clu_1;

class SharingIndex : public TreeIndex {
public:
    SharingIndex(Graph& graph) : TreeIndex(graph) { initializeMarks(); };
    ~SharingIndex() {};
    // 初始化连通分量持有的标记
    void initializeMarks();

    // batch查找解决CSP，存在少点的问题 -- To be optimized
    std::unordered_set<int> batchsearch(query_group& group);
    // batch查找解决CSP，更耗时但正确
    std::unordered_set<int> batchsearchRough(query_group& group);
    // 输出结果 -- To be optimized
    void printAndwrite(std::string path);
    // 根据CSP结果，进行二次聚类
    void clusteringOnCSP(std::unordered_set<int>& ids); // 相同Ck，相同comid
    // 计算两个查询之间的相似度
    double querySimilarity(query_nodes& qA, query_nodes& qB);
    // 计算两个查询顶点集之间的相似度
    double groupSimilarity(query_group& groupA, query_group& groupB);
    // 层次聚类，返回值暂时没有用
    std::vector<query_group> optimizedHierarchicalClustering(query_group& queryGroups, double threshold_);

    // 精确方法
    void batchMinsearchPrecise(query_group& group);
    // 快速方法
    void batchMinsearchFast(query_group& group);

    // protected:
    //CSP
    // 用来累加查询集编号
    int codeCount;

    // 查询集编号 -> 查询集
    std::unordered_map<int, query_nodes> codeToQuery;

    // 查询集编号 -> 结果对应连通分量应持有的标记 
    std::unordered_map<int, std::unordered_set<int>> codeToMasks; 

    // 查询集编号 -> 结果节点集
    std::unordered_map<int, std::unordered_set<int>> queryToResult; 

    // 剩余的点的数量 -> 查询集编号
    std::vector<std::unordered_set<int>> listToCodes;

    // 查询集编号 -> 剩余的点的数量
    std::unordered_map<int, int> codeToSpare;

    // 连通分量id -> 持有的标记
    std::unordered_map<int, std::unordered_set<int>> comToMasks;

    // 查询集编号 -> 连通分量id
    std::unordered_map<int, int> codeToCom;

    // 查询集编号 -> 核心度
    std::unordered_map<int, int> codeToK;

    //MIN_CSP
    // 根据csp结果，二次聚类的类数量
    int clusters_count;

    // 根据csp结果，二次聚类的结果 类编号 -> 查询集编号集合
    std::unordered_map<int, std::unordered_set<int>> clusters_csp;

    // 相似度聚类的结果 类编号 -> 查询集编号集合
    std::vector<std::unordered_set<int>> clusters_simi;

    // greedystep的结果 查询集编号 -> 结果节点集
    std::unordered_map<int, std::unordered_set<int>> greedyQueryToResult;
    
    // greedystep的结果 类编号 -> 结果节点集
    std::unordered_map<int, std::unordered_set<int>> greedyClusterToResult; 
    
    // steinerTree的结果 查询集编号 -> steinerTree
    std::unordered_map<int, std::unordered_set<int>> queryToTree;

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
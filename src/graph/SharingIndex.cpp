#include "SharingIndex.h"
#include "ProjectConstants.h"
#include <ctime>

// 输出结果 -- To be optimized
void SharingIndex::printAndwrite(std::string path) {

    // 不写入
    if (path.empty()) {
        return;
    }

    // 把结果存到csv文件
    std::ofstream outFile(path);
    if (!outFile.is_open()) {
        std::cerr << "无法打开 results.csv 文件" << std::endl;
        return;
    }
    // 写入 CSV 头部
    outFile << "TestNumber,QueryNodes,size,k\n";

    for (int i = 0; i < codeCount; ++i) {

        // 将查询顶点集转换为字符串
        std::string queryNodesStr;
        for (int node : codeToQuery[i]) {
            queryNodesStr += std::to_string(node) + " ";
        }

        // 将结果写入 CSV 文件
        outFile << i << ","
            << queryNodesStr << ","
            << queryToResult[i].size() << ","
            << codeToK[i]
            << "\n";
    }
    outFile.close();

}

// 初始化连通分量持有的标记
void SharingIndex::initializeMarks() {
    // 对于分量id 对应的标记
    int mask = 0;
    // 连通分量id集合
    std::unordered_set<int> U;
    for (int i = coreMax; i >= 1; --i) {
        for (int comp_id : shellToComs[i]) {
            if (U.find(comp_id) == U.end()) {
                comToMasks[comp_id].insert(mask++);
                U.insert(comp_id);
            }
            if (comToParent[comp_id] != -1) {
                U.insert(comToParent[comp_id]);
                if (comToMasks[comp_id].size() != 0) {
                    comToMasks[comToParent[comp_id]].insert(comToMasks[comp_id].begin(), comToMasks[comp_id].end());
                }
                else {
                    std::cout << "警告：存在树节点没有标记" << std::endl;
                }
            }
        }
    }
}

// batch查找解决CSP，存在少点的问题 -- To be optimized
std::unordered_set<int> SharingIndex::batchsearch(query_group& group) {

    // 重置成员变量
    codeToQuery.clear();
    codeToMasks.clear();
    queryToResult.clear();
    listToCodes.resize(50);
    codeToCom.clear();
    codeToK.clear();

    // 临时变量
    // 节点 -> 查询集编号集合
    std::unordered_map<int, std::vector<int>> Q;
    // 最终结果
    std::unordered_set<int> ans;
    // 最大核心度
    int k_max = 0;
    // 最小核心度
    int k_count = int_max_alter;
    // 查询集编号
    codeCount = 0;

    // 每个查询分配 code编号， 更新 k_max
    for (auto& tmp : group) {
        codeToSpare[codeCount] = tmp.size();
        listToCodes[tmp.size()].insert(codeCount);
        codeToQuery[codeCount] = tmp;
        codeToK[codeCount] = int_max_alter;
        for (auto& node : tmp) {
            if (nodeToShell[node] < codeToK[codeCount]) {
                codeToK[codeCount] = nodeToShell[node];
            }
            if (nodeToShell[node] > k_max) {
                k_max = nodeToShell[node];
            }
            Q[node].push_back(codeCount);
        }
        codeCount++;
    }

    // 在最高层 k_max 做第一轮命中
    // 当前层查询涉及到的候选 component 集合
    std::unordered_set<int> H;
    std::unordered_set<int> tmp_nodes; // 不在Q的循环内删点
    for (auto& pair : Q) {
        if (nodeToShell[pair.first] == k_max) {
            for (int comp_id : shellToComs[k_max]) {
                if (comToNodes[comp_id].find(pair.first) != comToNodes[comp_id].end()) {
                    H.insert(comp_id);
                    for (auto& tmp : pair.second) {
                        codeToMasks[tmp].insert(comToMasks[comp_id].begin(), comToMasks[comp_id].end());
                    }
                }
            }

            for (auto& tmp : pair.second) {
                listToCodes[codeToSpare[tmp]].erase(tmp);
                listToCodes[codeToSpare[tmp] - 1].insert(tmp);
                codeToSpare[tmp]--;
            }
            tmp_nodes.insert(pair.first);
        }
    }

    for (auto& node : tmp_nodes) {
        Q.erase(node);
    }

    // 构建核心度列表 Shell -> 查询节点集合
    std::vector<std::unordered_set<int>> Qlist;
    Qlist.resize(k_max + 1);
    for (auto& pair : Q) {
        if (nodeToShell[pair.first] >= 1) {
            Qlist[nodeToShell[pair.first]].insert(pair.first);
        }
    }

    // 尝试立即完成 listToCodes[0] 里的查询（首轮）
    int k = k_max;
    if (listToCodes[0].size() != 0) {
        // 使用vector来避免迭代器失效问题
        std::vector<int> queries_to_process(listToCodes[0].begin(), listToCodes[0].end());
        for (auto& q : queries_to_process) {
            if (H.size() == 1) {
                // 得到结果
                if (q < 0) {  // 假设q应为非负值
                    std::cerr << "Error: Invalid q value " << q << std::endl;
                    exit(1);
                }
                auto first_node = *H.begin();  // 现在安全解引用
                if (k_count <= 0) {
                    std::cerr << "Error: k_count must be positive, got " << k_count << std::endl;
                    exit(1);
                }
                queryToResult[q] = getchnodes_2(first_node, k_count);
                codeToK[q] = k_count;
                codeToCom[q] = *H.begin();
                ans.insert(queryToResult[q].begin(), queryToResult[q].end());
                queryToResult[q] = ans;
                listToCodes[0].erase(q);
                // 移除break，让所有查询都能得到结果
            }
            else {
                // 检查标记
                bool flag = true;
                for (auto& com : H) {
                    for (auto& tmp : codeToMasks[q]) {
                        if (comToMasks[com].find(tmp) == comToMasks[com].end()) {
                            flag = false;
                            if (codeToK[q] > 1) {
                                codeToK[q]--;
                            }
                            break;
                        }
                    }
                    if (flag) {
                        // 得到结果
                        queryToResult[q] = getchnodes_2(*H.begin(), k_count);
                        codeToK[q] = k_count;
                        codeToCom[q] = *H.begin();
                        ans.insert(queryToResult[q].begin(), queryToResult[q].end());
                        queryToResult[q] = ans;
                        // 维护变量
                        listToCodes[0].erase(q);
                        break;
                    }
                    else {
                        break;
                    }
                }
                if (!flag) {
                    // std::cout << "查询 " << q << " 标记检查失败" << std::endl;
                }
            }
        }
    }

    // 逐层降低核心度，处理剩余查询
    while (H.size() > 1 || Q.size() > 0) {

        k = k - 1;
        query_nodes Q_;
        //对于Q的点很多，用一个核心度列表优化
        if (Qlist[k].size() != 0) {
            Q_ = Qlist[k];
        }

        // H 在下一层的投影（通过父分量映射）
        std::unordered_set<int> Hp;
        for (auto& com : H) {
            if (comToParent[com] != -1) {
                if (nodeToShell[*comToNodes[comToParent[com]].begin()] == k) {
                    Hp.insert(comToParent[com]);
                }
                else {
                    Hp.insert(com);
                }
            }
        }
        if (Q_.size() > 0) {
            for (auto& node : Q_) {
                for (int comp_id : shellToComs[k]) {
                    if (comToNodes[comp_id].find(node) != comToNodes[comp_id].end()) {
                        Hp.insert(comp_id);
                    }
                    for (auto& tmp : Q[node]) {
                        codeToMasks[tmp].insert(comToMasks[comp_id].begin(), comToMasks[comp_id].end());
                    }
                }

                for (auto& tmp : Q[node]) {
                    listToCodes[codeToSpare[tmp]].erase(tmp);
                    listToCodes[codeToSpare[tmp] - 1].insert(tmp);
                    codeToSpare[tmp]--;
                }
                Q.erase(node);
            }
        }

        H = Hp;

        if (listToCodes[0].size() != 0) {
            // 使用vector来避免迭代器失效问题
            std::vector<int> queries_to_process_while(listToCodes[0].begin(), listToCodes[0].end());
            for (auto& q : queries_to_process_while) {
                if (H.size() == 1) {
                    // 得到结果
                    queryToResult[q] = getchnodes_2(*H.begin(), k_count);
                    codeToK[q] = k_count;
                    codeToCom[q] = *H.begin();
                    ans.insert(queryToResult[q].begin(), queryToResult[q].end());
                    queryToResult[q] = ans;
                    // 维护相关变量
                    listToCodes[0].erase(q);
                }
                else {
                    // 检查标记
                    bool flag = true;
                    for (auto& com : H) {
                        for (auto& tmp : codeToMasks[q]) {
                            if (comToMasks[com].find(tmp) == comToMasks[com].end()) {
                                flag = false;
                                if (codeToK[q] > 1) {
                                    codeToK[q]--;
                                }
                                break;
                            }
                        }
                        if (flag) {
                            // 得到结果
                            queryToResult[q] = getchnodes_2(*H.begin(), k_count);
                            codeToK[q] = k_count;
                            codeToCom[q] = *H.begin();
                            ans.insert(queryToResult[q].begin(), queryToResult[q].end());
                            queryToResult[q] = ans;
                            // 维护变量
                            listToCodes[0].erase(q);
                        }
                        else {
                            break;
                        }
                    }
                    if (!flag) {
                        // std::cout << "k=" << k << " 查询 " << q << " 标记检查失败" << std::endl;
                    }
                }
            }
        }

    }
    return ans;
}

// batch查找 rough 版本：
// 1) 用 getNodesFromCom 直接取分量子树节点，不用 getchnodes_2 的 k 截断；
// 2) 用当前层 k 作为 codeToK[q]；
// 3) 不再把 queryToResult[q] 覆盖成全局并集 ans。
std::unordered_set<int> SharingIndex::batchsearchRough(query_group& group) {
    codeToQuery.clear();
    codeToMasks.clear();
    queryToResult.clear();
    listToCodes.resize(50);
    codeToCom.clear();
    codeToK.clear();

    std::unordered_map<int, std::vector<int>> Q;
    std::unordered_set<int> ans;
    int k_max = 0;
    codeCount = 0;

    for (auto& tmp : group) {
        codeToSpare[codeCount] = tmp.size();
        listToCodes[tmp.size()].insert(codeCount);
        codeToQuery[codeCount] = tmp;
        codeToK[codeCount] = int_max_alter;
        for (auto& node : tmp) {
            if (nodeToShell[node] < codeToK[codeCount]) {
                codeToK[codeCount] = nodeToShell[node];
            }
            if (nodeToShell[node] > k_max) {
                k_max = nodeToShell[node];
            }
            Q[node].push_back(codeCount);
        }
        codeCount++;
    }

    std::unordered_set<int> H;
    std::unordered_set<int> tmp_nodes;
    for (auto& pair : Q) {
        if (nodeToShell[pair.first] == k_max) {
            for (int comp_id : shellToComs[k_max]) {
                if (comToNodes[comp_id].find(pair.first) != comToNodes[comp_id].end()) {
                    H.insert(comp_id);
                    for (auto& tmp : pair.second) {
                        codeToMasks[tmp].insert(comToMasks[comp_id].begin(), comToMasks[comp_id].end());
                    }
                }
            }

            for (auto& tmp : pair.second) {
                listToCodes[codeToSpare[tmp]].erase(tmp);
                listToCodes[codeToSpare[tmp] - 1].insert(tmp);
                codeToSpare[tmp]--;
            }
            tmp_nodes.insert(pair.first);
        }
    }

    for (auto& node : tmp_nodes) {
        Q.erase(node);
    }

    std::vector<std::unordered_set<int>> Qlist;
    Qlist.resize(k_max + 1);
    for (auto& pair : Q) {
        if (nodeToShell[pair.first] >= 1) {
            Qlist[nodeToShell[pair.first]].insert(pair.first);
        }
    }

    int k = k_max;
    if (!listToCodes[0].empty()) {
        std::vector<int> queries_to_process(listToCodes[0].begin(), listToCodes[0].end());
        for (auto& q : queries_to_process) {
            if (H.size() == 1) {
                auto first_node = *H.begin();
                queryToResult[q] = getNodesFromCom(first_node);
                codeToK[q] = k;
                codeToCom[q] = first_node;
                ans.insert(queryToResult[q].begin(), queryToResult[q].end());
                listToCodes[0].erase(q);
            } else {
                bool flag = true;
                for (auto& com : H) {
                    for (auto& tmp : codeToMasks[q]) {
                        if (comToMasks[com].find(tmp) == comToMasks[com].end()) {
                            flag = false;
                            if (codeToK[q] > 1) {
                                codeToK[q]--;
                            }
                            break;
                        }
                    }
                    if (flag) {
                        queryToResult[q] = getNodesFromCom(*H.begin());
                        codeToK[q] = k;
                        codeToCom[q] = *H.begin();
                        ans.insert(queryToResult[q].begin(), queryToResult[q].end());
                        listToCodes[0].erase(q);
                        break;
                    } else {
                        break;
                    }
                }
            }
        }
    }

    while (H.size() > 1 || Q.size() > 0) {
        k = k - 1;
        query_nodes Q_;
        if (!Qlist[k].empty()) {
            Q_ = Qlist[k];
        }

        std::unordered_set<int> Hp;
        for (auto& com : H) {
            if (comToParent[com] != -1) {
                if (nodeToShell[*comToNodes[comToParent[com]].begin()] == k) {
                    Hp.insert(comToParent[com]);
                } else {
                    Hp.insert(com);
                }
            }
        }

        if (!Q_.empty()) {
            for (auto& node : Q_) {
                for (int comp_id : shellToComs[k]) {
                    if (comToNodes[comp_id].find(node) != comToNodes[comp_id].end()) {
                        Hp.insert(comp_id);
                    }
                    for (auto& tmp : Q[node]) {
                        codeToMasks[tmp].insert(comToMasks[comp_id].begin(), comToMasks[comp_id].end());
                    }
                }

                for (auto& tmp : Q[node]) {
                    listToCodes[codeToSpare[tmp]].erase(tmp);
                    listToCodes[codeToSpare[tmp] - 1].insert(tmp);
                    codeToSpare[tmp]--;
                }
                Q.erase(node);
            }
        }

        H = Hp;

        if (!listToCodes[0].empty()) {
            std::vector<int> queries_to_process_while(listToCodes[0].begin(), listToCodes[0].end());
            for (auto& q : queries_to_process_while) {
                if (H.size() == 1) {
                    queryToResult[q] = getNodesFromCom(*H.begin());
                    codeToK[q] = k;
                    codeToCom[q] = *H.begin();
                    ans.insert(queryToResult[q].begin(), queryToResult[q].end());
                    listToCodes[0].erase(q);
                } else {
                    bool flag = true;
                    for (auto& com : H) {
                        for (auto& tmp : codeToMasks[q]) {
                            if (comToMasks[com].find(tmp) == comToMasks[com].end()) {
                                flag = false;
                                if (codeToK[q] > 1) {
                                    codeToK[q]--;
                                }
                                break;
                            }
                        }
                        if (flag) {
                            queryToResult[q] = getNodesFromCom(*H.begin());
                            codeToK[q] = k;
                            codeToCom[q] = *H.begin();
                            ans.insert(queryToResult[q].begin(), queryToResult[q].end());
                            listToCodes[0].erase(q);
                        } else {
                            break;
                        }
                    }
                }
            }
        }
    }

    return ans;
}

// 根据CSP结果，进行二次聚类
void SharingIndex::clusteringOnCSP(std::unordered_set<int>& ids) {

    std::unordered_set<int> count;
    for (auto& id : ids) {
        count.insert(codeToCom[id]);
    }

    clusters_csp.clear();
    clusters_count = 0;
    for (auto& c : count) {
        for (int i = 0; i < codeCount; ++i) {
            if (c == codeToCom[i] && ids.find(i) != ids.end()) {
                clusters_csp[clusters_count].insert(i);
            }
        }
        clusters_count++;
    }

}

// 精确方法
void SharingIndex::batchMinsearchPrecise(query_group& group) {

    time_cluster = 0;
    time_1 = 0;
    time_2 = 0;
    time_3 = 0;
    time_4 = 0;
    clu_1 = 0;

    // 第一步：根据相似度聚类
    const clock_t clusterBegin = std::clock();
    optimizedHierarchicalClustering(group, simi);
    const clock_t clusterEnd = std::clock();
    time_cluster = static_cast<double>(clusterEnd - clusterBegin) / CLOCKS_PER_SEC;
    clu_1 = static_cast<double>(clusters_simi.size());

    int count = 0;
    for (auto& codes : clusters_simi) {
        std::unordered_set<int> Q;
        int k = 0;
        std::unordered_set<int> H;
        for (auto& code : codes) {
            Q.insert(codeToQuery[code].begin(), codeToQuery[code].end());
            if (k <= codeToK[code]) {
                k = codeToK[code];
            }
            H.insert(queryToResult[code].begin(), queryToResult[code].end());
        }


        // 第二步：对每个类进行greedystep
        const clock_t greedyBegin = std::clock();
        greedyClusterToResult[count] = greedyStep(Q, k, H);
        const clock_t greedyEnd = std::clock();
        time_1 += static_cast<double>(greedyEnd - greedyBegin) / CLOCKS_PER_SEC;

        for (auto& q : codes) {
            greedyQueryToResult[q] = greedyClusterToResult[count];
        }

        // 第三步：对每个查询集进行steinerTree
        const clock_t steinerBegin = std::clock();
        for (auto& q : codes) {
            queryToTree[q] = steinerTree(greedyClusterToResult[count], codeToQuery[q]);
        }
        const clock_t steinerEnd = std::clock();
        time_2 += static_cast<double>(steinerEnd - steinerBegin) / CLOCKS_PER_SEC;

        // 第四步：对每个查询集进行greedystep_simply
        const clock_t refineBegin = std::clock();
        for (auto& q : codes) {
            queryToResult[q] = greedyStep_simply(queryToTree[q], codeToK[q], greedyClusterToResult[count]);
        }
        const clock_t refineEnd = std::clock();
        time_4 += static_cast<double>(refineEnd - refineBegin) / CLOCKS_PER_SEC;
        count++;
    }
}

// 快速方法
void SharingIndex::batchMinsearchFast(query_group& group) {

    time_cluster = 0;
    time_1 = 0;
    time_2 = 0;
    time_3 = 0;
    time_4 = 0;
    clu_1 = 0;

    // 第一步：根据相似度聚类
    const clock_t clusterBegin = std::clock();
    optimizedHierarchicalClustering(group, simi);
    const clock_t clusterEnd = std::clock();
    time_cluster = static_cast<double>(clusterEnd - clusterBegin) / CLOCKS_PER_SEC;
    clu_1 = static_cast<double>(clusters_simi.size());

    int count = 0;
    for (auto& codes : clusters_simi) {
        std::unordered_set<int> Q;
        int k = 0;
        std::unordered_set<int> H;
        for (auto& code : codes) {
            Q.insert(codeToQuery[code].begin(), codeToQuery[code].end());
            if (k <= codeToK[code]) {
                k = codeToK[code];
            }
            H.insert(queryToResult[code].begin(), queryToResult[code].end());
        }

        // 第二步：对每个类进行greedystep
        const clock_t greedyBegin = std::clock();
        greedyClusterToResult[count] = greedyStep(Q, k, H);
        const clock_t greedyEnd = std::clock();
        time_1 += static_cast<double>(greedyEnd - greedyBegin) / CLOCKS_PER_SEC;

        for (auto& q : codes) {
            greedyQueryToResult[q] = greedyClusterToResult[count];
        }

        // 第三步：对每个查询集进行steinerTree
        const clock_t steinerBegin = std::clock();
        for (auto& code : codes) {
            queryToTree[code] = steinerTree(greedyQueryToResult[code], codeToQuery[code]);
        }
        const clock_t steinerEnd = std::clock();
        time_2 += static_cast<double>(steinerEnd - steinerBegin) / CLOCKS_PER_SEC;

        // 第四步：根据CSP结果，进行二次聚类
        const clock_t secondClusterBegin = std::clock();
        clusteringOnCSP(codes);
        const clock_t secondClusterEnd = std::clock();
        time_3 += static_cast<double>(secondClusterEnd - secondClusterBegin) / CLOCKS_PER_SEC;

        // 第五步：对每个类进行greedystep_simply
        const clock_t refineBegin = std::clock();
        for (auto& pair : clusters_csp) {
            std::unordered_set<int> Q;
            for (auto& q : pair.second) {
                Q.insert(queryToTree[q].begin(), queryToTree[q].end());
            }

            int k = 0;
            for (auto& Ck : shellToComs) {
                if (Ck.second.count(codeToCom[*pair.second.begin()])) {
                    k = Ck.first;
                    break;
                }
            }
            std::unordered_set<int> ans;
            ans = greedyStep_simply(Q, k, greedyClusterToResult[count]);
            for (auto& q : pair.second) {
                queryToResult[q] = ans;
            }
        }
        const clock_t refineEnd = std::clock();
        time_4 += static_cast<double>(refineEnd - refineBegin) / CLOCKS_PER_SEC;
        count++;
    }
}

// 与batch相关的 聚类算法
// 计算两个查询之间的相似度
double SharingIndex::querySimilarity(query_nodes& qA, query_nodes& qB) {

    // qA的邻居
    std::unordered_set<int> neighborsA;
    for (auto& v : qA) {
        neighborsA.insert(getNeighbors(v).begin(), getNeighbors(v).end());
    }

    // qB的邻居
    std::unordered_set<int> neighborsB;
    for (auto& v : qB) {
        neighborsB.insert(getNeighbors(v).begin(), getNeighbors(v).end());
    }

    // 计算两个邻居的交集
    std::vector<int> intersection;
    // 遍历较小的集合，提高效率
    const auto& smaller = (neighborsA.size() < neighborsB.size()) ? neighborsA : neighborsB;
    const auto& larger = (neighborsA.size() < neighborsB.size()) ? neighborsB : neighborsA;

    for (int value : smaller) {
        if (larger.count(value) > 0) {
            intersection.push_back(value);
        }
    }

    // 计算两个邻居的并集
    std::unordered_set<int> unionSet;
    unionSet.reserve(neighborsA.size() + neighborsB.size());
    unionSet.insert(neighborsA.begin(), neighborsA.end());
    unionSet.insert(neighborsB.begin(), neighborsB.end());


    if (unionSet.empty()) {
        return 0;
    }
    else {
        // 计算相似度
        return static_cast<double>(intersection.size()) / unionSet.size();
    }
}

// 计算两个查询顶点集组之间的相似度, 1 - 邻居
double SharingIndex::groupSimilarity(query_group& groupA, query_group& groupB) {
    // 检查输入参数是否为空
    if (groupA.empty() || groupB.empty()) {
        return 0.0;
    }

    double totalSimilarity = 0.0;
    for (query_nodes& qA : groupA) {
        for (query_nodes& qB : groupB) {
            double x = querySimilarity(qA, qB);
            if (x == 0) { return 0; }
            totalSimilarity += x;
        }
    }
    return totalSimilarity / (groupA.size() * groupB.size());
}

// 优化的层次聚类算法，减少groupSimilarity调用次数
std::vector<query_group> SharingIndex::optimizedHierarchicalClustering(query_group& queryGroups, double threshold_) {

    size_t n = queryGroups.size();
    std::vector<query_group> clusters;
    clusters_simi.clear();
    std::vector<std::vector<double>> similarityMatrix(n, std::vector<double>(n, -1.0));
    std::priority_queue<std::tuple<double, int, int>> similarityHeap;

    // 初始化每个查询为一个单独的组
    for (int i = 0; i < codeCount; ++i) {
        query_group group;
        clusters.push_back({ codeToQuery[i] });
        clusters_simi.push_back({ i });
    }
    if (queryGroups.size() <= 1) return clusters;

    // 预处理
    for (size_t i = 1; i < n; ) {
        if (i >= clusters.size()) break; // 防御性检查
        double sim = groupSimilarity(clusters[i], clusters[0]);
        if (sim >= threshold_) {
            // 合并
            clusters[0].insert(clusters[0].end(),
                std::make_move_iterator(clusters[i].begin()),
                std::make_move_iterator(clusters[i].end()));
            clusters_simi[0].insert(clusters_simi[i].begin(), clusters_simi[i].end());
            clusters.erase(clusters.begin() + i);
            clusters_simi.erase(clusters_simi.begin() + i);
            n = clusters.size();
        }
        else {
            i++;
        }
    }

    // 初始化相似度矩阵和堆
    // std::cout << "初始化相似度矩阵..." << std::endl;
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            double sim = groupSimilarity(clusters[i], clusters[j]);
            similarityMatrix[i][j] = sim;
            similarityMatrix[j][i] = sim;
            if (sim >= threshold_) {
                similarityHeap.push({ sim, static_cast<int>(i), static_cast<int>(j) });
            }
        }
    }
    int iteration = 0;

    // 层次聚类迭代
    while (clusters.size() > 1) {
        iteration++;

        // 从堆中找到最相似的有效组对
        int bestI = -1, bestJ = -1;
        double maxSim = -1.0;
        int heapChecks = 0;
        int invalidPairs = 0;

        while (!similarityHeap.empty()) {
            heapChecks++;
            auto [sim, i, j] = similarityHeap.top();
            similarityHeap.pop();

            // 检查组对是否有效
            bool valid = (static_cast<size_t>(i) < clusters.size() && static_cast<size_t>(j) < clusters.size() &&
                std::abs(similarityMatrix[i][j] - sim) < 1e-9 && sim >= threshold_);

            if (!valid) {
                invalidPairs++;
                continue;
            }

            bestI = i;
            bestJ = j;
            maxSim = sim;
            break;
        }

        // 如果没有可合并的组，结束
        if (bestI == -1 || bestJ == -1 || maxSim < threshold_) {
            break;
        }

        // 合并两个组
        clusters[bestI].insert(clusters[bestI].end(),
            std::make_move_iterator(clusters[bestJ].begin()),
            std::make_move_iterator(clusters[bestJ].end()));
        clusters_simi[bestI].insert(clusters_simi[bestJ].begin(), clusters_simi[bestJ].end());

        // 记录被删除的索引
        size_t deletedIdx = bestJ;

        // 移除被合并的组
        clusters.erase(clusters.begin() + deletedIdx);
        clusters_simi.erase(clusters_simi.begin() + deletedIdx);

        // 创建新旧索引映射表
        std::vector<size_t> oldToNew(clusters.size() + 1, -1);
        for (size_t newIdx = 0; newIdx < clusters.size(); ++newIdx) {
            size_t oldIdx = (newIdx < deletedIdx) ? newIdx : newIdx + 1;
            oldToNew[oldIdx] = newIdx;
        }

        // 构建新的相似度矩阵
        size_t newSize = clusters.size();
        std::vector<std::vector<double>> newSimilarityMatrix(
            newSize, std::vector<double>(newSize, -1.0));

        // 复制有效相似度到新矩阵
        for (size_t i = 0; i < newSize; ++i) {
            for (size_t j = i + 1; j < newSize; ++j) {
                size_t oldI = (i < deletedIdx) ? i : i + 1;
                size_t oldJ = (j < deletedIdx) ? j : j + 1;

                if (oldI < similarityMatrix.size() && oldJ < similarityMatrix.size() &&
                    similarityMatrix[oldI][oldJ] >= 0) {
                    newSimilarityMatrix[i][j] = similarityMatrix[oldI][oldJ];
                    newSimilarityMatrix[j][i] = similarityMatrix[oldI][oldJ];
                }
            }
        }

        // 更新与合并组相关的相似度
        size_t mergedIdx = bestI;
        for (size_t i = 0; i < newSize; ++i) {
            if (i == mergedIdx) continue;

            double newSim = groupSimilarity(clusters[i], clusters[mergedIdx]);
            newSimilarityMatrix[i][mergedIdx] = newSim;
            newSimilarityMatrix[mergedIdx][i] = newSim;
        }

        // 替换旧矩阵
        similarityMatrix = std::move(newSimilarityMatrix);

        // 清空堆并重建
        similarityHeap = std::priority_queue<std::tuple<double, int, int>>();
        for (size_t i = 0; i < newSize; ++i) {
            for (size_t j = i + 1; j < newSize; ++j) {
                if (similarityMatrix[i][j] >= threshold_) {
                    similarityHeap.push({ similarityMatrix[i][j], static_cast<int>(i), static_cast<int>(j) });
                }
            }
        }

    }
    return clusters;
}
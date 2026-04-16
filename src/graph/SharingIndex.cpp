#include "SharingIndex.h"

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
    outFile << "TestNumber,QueryNodes,size,min_size1,min_size2,k\n";

    for (int i = 0; i < threshold; ++i) {

        // 将查询顶点集转换为字符串
        std::string queryNodesStr;
        for (int node : qtocode[i]) {
            queryNodesStr += std::to_string(node) + " ";
        }

        // 将结果写入 CSV 文件
        outFile << i << ","
            << queryNodesStr << ","
            << codetore[i].size() << ","
            << greedy1[i].size() << ","
            << greedy2[i].size() << ","
            << codetok[i]
            << "\n";
    }
    outFile << time_1 << ","
        << time_2 << ","
        << time_3 << ","
        << clusters_op.size() << ","
        << clusters.size()
        << "\n";
    outFile.close();

}

void SharingIndex::initializeMarks() {
    // 对于分量id 对应的标记
    int mask = 0;
    std::unordered_set<int> U;
    // std::cout<< "coreMax: " << coreMax << std::endl;
    for (int i = coreMax; i >= 1; --i) {
        for (int comp_id : shellToComs[i]) {
            if (U.find(comp_id) == U.end()) {
                idtore[comp_id].insert(mask++);
                U.insert(comp_id);
            }
            if (comToParent[comp_id] != -1) {
                U.insert(comToParent[comp_id]);
                if (idtore[comp_id].size() != 0) {
                    idtore[comToParent[comp_id]].insert(idtore[comp_id].begin(), idtore[comp_id].end());
                }
                else {
                    std::cout << "警告：存在树节点没有标记" << std::endl;
                }
            }
        }
    }
    // 打印结果
    // std::cout << "初始化标记后的idtore状态（按Cktocom结构顺序）：" << std::endl;
    // for (int k = coreMax; k >= 1; --k) {
    //     if (shellToComs.find(k) == shellToComs.end()) continue;
    //     std::vector<int> comp_ids(shellToComs[k].begin(), shellToComs[k].end());
    //     std::sort(comp_ids.begin(), comp_ids.end());
    //     for (int comp_id : comp_ids) {
    //         std::cout << "核心度: " << k << " 分量ID: " << comp_id << " 标记: ";
    //         for (int mark : idtore[comp_id]) {
    //             std::cout << mark << " ";
    //         }
    //         std::cout << std::endl;
    //     }
    // }
    // 验证标记结构
    // std::cout << "\n===== 验证标记结构 =====" << std::endl;
    // bool structure_valid = true;

    // for (int k = coreMax; k >= 1; --k) {
    //     if (shellToComs.find(k) == shellToComs.end()) continue;
    //     for (int comp_id : shellToComs[k]) {
    //         if (idtore[comp_id].empty()) {
    //             std::cout << "错误：分量 " << comp_id << " 没有标记" << std::endl;
    //             structure_valid = false;
    //         }

    //         // 检查父子关系：父节点应该包含所有子节点的标记
    //         if (comToParent[comp_id] != -1) {
    //             bool parent_has_child_marks = true;
    //             for (int mark : idtore[comp_id]) {
    //                 if (idtore[comToParent[comp_id]].find(mark) == idtore[comToParent[comp_id]].end()) {
    //                     std::cout << "错误：父分量 " << comToParent[comp_id] << " 缺少子分量 " << comp_id << " 的标记 " << mark << std::endl;
    //                     parent_has_child_marks = false;
    //                 }
    //             }
    //             if (!parent_has_child_marks) {
    //                 structure_valid = false;
    //             }
    //         }

    //         // 检查叶子节点：叶子节点应该有且仅有一个标记
    //         if (comToChildren[comp_id].empty() || (comToChildren[comp_id].size() == 1 && comToChildren[comp_id].find(-1) != comToChildren[comp_id].end())) {
    //             if (idtore[comp_id].size() != 1) {
    //                 std::cout << "错误：叶子分量 " << comp_id << " 应该有且仅有一个标记，但实际有 " << idtore[comp_id].size() << " 个" << std::endl;
    //                 structure_valid = false;
    //             }
    //         }
    //     }
    // }

    // if (structure_valid) {
    //     std::cout << "标记结构验证通过" << std::endl;
    // } else {
    //     std::cout << "标记结构验证失败" << std::endl;
    // }
}

std::unordered_set<int> SharingIndex::batchsearch(query_group& group) {

    // 重置成员变量
    qtocode.clear(); // q -> code
    codetoma.clear(); // code -> 标记
    codetore.clear(); // code -> 结果
    lelist.resize(20); // code -> 剩余的点的数量
    codetoid.clear(); // code -> id
    codetok.clear();


    // 临时变量
    std::unordered_map<int, int> codetoshell; // code -> shell
    std::unordered_map<int, std::vector<int>> Q;
    std::unordered_set<int> ans;
    int k_max = 0;
    int k_count = INT32_MAX;
    threshold = 0;

    std::cout << "=== batchsearch 开始 ===" << std::endl;

    for (auto& tmp : group) {
        codetole[threshold] = tmp.size();
        lelist[tmp.size()].insert(threshold);
        qtocode[threshold] = tmp;
        codetok[threshold] = INT32_MAX;
        for (auto& node : tmp) {
            if (nodeToShell[node] < codetok[threshold]) {
                codetok[threshold] = nodeToShell[node];
            }
            if (nodeToShell[node] > k_max) {
                k_max = nodeToShell[node];
            }
            Q[node].push_back(threshold);
        }
        threshold++;
    }

    std::cout << "最大核心度 k_max: " << k_max << std::endl;
    std::cout << "查询节点总数: " << Q.size() << std::endl;

    std::unordered_set<int> H;
    std::unordered_set<int> tmp_nodes; // 不在Q的循环内删点
    for (auto& pair : Q) {
        if (nodeToShell[pair.first] == k_max) {
            for (int comp_id : shellToComs[k_max]) {
                if (comToNodes[comp_id].find(pair.first) != comToNodes[comp_id].end()) {
                    H.insert(comp_id);
                    for (auto& tmp : pair.second) {
                        codetoma[tmp].insert(idtore[comp_id].begin(), idtore[comp_id].end());
                    }
                }
            }

            for (auto& tmp : pair.second) {
                lelist[codetole[tmp]].erase(tmp);
                lelist[codetole[tmp] - 1].insert(tmp);
                codetole[tmp]--;
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
    if (lelist[0].size() != 0) {
        // 使用vector来避免迭代器失效问题
        std::vector<int> queries_to_process(lelist[0].begin(), lelist[0].end());
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
                codetore[q] = getchnodes_2(first_node, k_count);
                codetok[q] = k_count;
                codetoid[q] = *H.begin();
                ans.insert(codetore[q].begin(), codetore[q].end());
                codetore[q] = ans;
                lelist[0].erase(q);
                // 移除break，让所有查询都能得到结果
            }
            else {
                // 检查标记
                // std::cout << "查询 " << q << " 的标记数量: " << codetoma[q].size() << std::endl;
                bool flag = true;
                for (auto& com : H) {
                    // std::cout << "检查分量 " << com << " 的标记" << std::endl;
                    for (auto& tmp : codetoma[q]) {
                        if (idtore[com].find(tmp) == idtore[com].end()) {
                            flag = false;
                            if (codetok[q] > 1) {
                                codetok[q]--;
                            }
                            // std::cout << "标记 " << tmp << " 不在分量 " << com << " 中" << std::endl;
                            break;
                        }
                    }
                    if (flag) {
                        // 得到结果
                        codetore[q] = getchnodes_2(*H.begin(), k_count);
                        codetok[q] = k_count;
                        codetoid[q] = *H.begin();
                        ans.insert(codetore[q].begin(), codetore[q].end());
                        codetore[q] = ans;
                        // 维护变量
                        lelist[0].erase(q);
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

    while (H.size() > 1 || Q.size() > 0) {

        k = k - 1;

        query_nodes Q_;
        //对于Q的点很多，用一个核心度列表优化
        if (Qlist[k].size() != 0) {
            Q_ = Qlist[k];
        }

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
            // std::cout << "处理核心度为 " << k << " 的节点，数量: " << Q_.size() << std::endl;
            for (auto& node : Q_) {
                for (int comp_id : shellToComs[k]) {
                    if (comToNodes[comp_id].find(node) != comToNodes[comp_id].end()) {
                        Hp.insert(comp_id);
                    }
                    for (auto& tmp : Q[node]) {
                        codetoma[tmp].insert(idtore[comp_id].begin(), idtore[comp_id].end());
                    }
                }

                for (auto& tmp : Q[node]) {
                    lelist[codetole[tmp]].erase(tmp);
                    lelist[codetole[tmp] - 1].insert(tmp);
                    codetole[tmp]--;
                }
                Q.erase(node);
            }
        }

        H = Hp;

        if (lelist[0].size() != 0) {
            // 使用vector来避免迭代器失效问题
            std::vector<int> queries_to_process_while(lelist[0].begin(), lelist[0].end());
            for (auto& q : queries_to_process_while) {
                if (H.size() == 1) {
                    // 得到结果
                    codetore[q] = getchnodes_2(*H.begin(), k_count);
                    codetok[q] = k_count;
                    codetoid[q] = *H.begin();
                    ans.insert(codetore[q].begin(), codetore[q].end());
                    codetore[q] = ans;
                    // 维护相关变量
                    lelist[0].erase(q);
                }
                else {
                    // 检查标记
                    // std::cout << "k=" << k << " 查询 " << q << " 的标记数量: " << codetoma[q].size() << std::endl;
                    bool flag = true;
                    for (auto& com : H) {
                        for (auto& tmp : codetoma[q]) {
                            if (idtore[com].find(tmp) == idtore[com].end()) {
                                flag = false;
                                if (codetok[q] > 1) {
                                    codetok[q]--;
                                }
                                // std::cout << "k=" << k << " 标记 " << tmp << " 不在分量 " << com << " 中" << std::endl;
                                break;
                            }
                        }
                        if (flag) {
                            // 得到结果
                            codetore[q] = getchnodes_2(*H.begin(), k_count);
                            codetok[q] = k_count;
                            codetoid[q] = *H.begin();
                            ans.insert(codetore[q].begin(), codetore[q].end());
                            codetore[q] = ans;
                            // 维护变量
                            lelist[0].erase(q);
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
    std::cout << "=== batchsearch 完成 ===" << std::endl;
    std::cout << "最终结果大小: " << ans.size() << std::endl;
    std::cout << "处理的查询组数量: " << threshold << std::endl;
    syncCompatibilityViews();

    return ans;
}

void SharingIndex::Clustering_first(std::unordered_set<int>& ids) {

    std::unordered_set<int> count;
    for (auto& id : ids) {
        count.insert(codetoid[id]);
    }

    clusters.clear();
    clusters_count = 0;
    for (auto& c : count) {
        for (int i = 0; i < threshold; ++i) {
            if (c == codetoid[i]) {
                clusters[clusters_count].insert(i);
            }
        }
        clusters_count++;
    }

}

void SharingIndex::Clustering_second(std::unordered_set<int>& ids) {

    clusters2.clear();
    clusters_count2 = 0;
    parent.clear();

    // 初始化并查集
    for (auto id : ids) {
        parent[id] = id;
    }

    // 记录每个元素首次出现的id
    std::unordered_map<int, int> elementToFirstId;
    for (auto id : ids) {
        for (auto element : q2[id]) {
            if (elementToFirstId.find(element) == elementToFirstId.end()) {
                elementToFirstId[element] = id;
            }
            else {
                // 合并当前id和首次出现该元素的id
                unite(id, elementToFirstId[element]);
            }
        }
    }

    // 构建最终的聚类结果
    std::unordered_map<int, std::unordered_set<int>> rootToIds;
    for (auto id : ids) {
        int root = find(id);
        rootToIds[root].insert(id);
    }

    // 将聚类结果存入clusters2，使用根节点作为簇ID
    clusters2.clear();
    for (const auto& pair : rootToIds) {
        clusters2[pair.first] = pair.second;
    }
    clusters_count2 = clusters2.size();

}

void SharingIndex::batchMinsearch_1(query_group& group) {

    clock_t tmp_1 = std::clock();
    std::vector<query_group> clusters_ = optimizedHierarchicalClustering(group, simi);
    clock_t tmp_2 = std::clock();
    clu_1 += (double)(tmp_2 - tmp_1) / CLOCKS_PER_SEC;
    std::cout << "聚类_1: " << clu_1 << std::endl;

    // 打印clusters_和clusters_op检查
    std::cout << "\n===== batchMinsearch_1 聚类结果检查 =====" << std::endl;
    std::cout << "clusters_ 大小: " << clusters_.size() << std::endl;
    std::cout << "clusters_op 大小: " << clusters_op.size() << std::endl;


    // 时间代价
    int count_1 = 0;
    int i = 0;
    for (auto& codes : clusters_op) {
        clock_t tmp1 = std::clock();
        std::unordered_set<int> Q;
        int k = 0;
        std::unordered_set<int> H;
        for (auto& code : codes) {
            Q.insert(qtocode[code].begin(), qtocode[code].end());
            if (k <= codetok[code]) {
                k = codetok[code];
            }
            H.insert(codetore[code].begin(), codetore[code].end());
        }


        greedy1_[count_1] = greedyStep(Q, k, H);

        // clock_t tmp5 = std::clock();
        for (auto& q : codes) {
            greedy1[q] = greedy1_[count_1];
        }
        // clock_t tmp6 = std::clock();
        // time_3 += (double)(tmp6 - tmp5) / CLOCKS_PER_SEC;

        clock_t tmp2 = std::clock();
        time_1 += (double)(tmp2 - tmp1) / CLOCKS_PER_SEC;

        clock_t tmp3 = std::clock();
        for (auto& q : codes) {
            q2[q] = steinerTree(greedy1_[count_1], qtocode[q]);
        }
        clock_t tmp4 = std::clock();

        for (auto& q : codes) {
            greedy2[q] = greedyStep_simply(q2[q], codetok[q], greedy1_[count_1]);
            // output(greedy2[q], std::to_string(q) + "6043.csv", codetok[q]);
        }
        clock_t tmp5 = std::clock();
        time_2 += (double)(tmp4 - tmp3) / CLOCKS_PER_SEC;
        time_3 += (double)(tmp5 - tmp4) / CLOCKS_PER_SEC;

    }
    std::cout << "greedy_1: " << time_1 << std::endl;
    std::cout << "steiner_1: " << time_2 << std::endl;
    std::cout << "gsim_1: " << time_3 << std::endl;
    syncCompatibilityViews();
    printAndwrite("batch_7_1.csv");
}

void SharingIndex::batchMinsearch_2(query_group& group) {

    clu_1 = 0;
    time_1 = 0;
    time_2 = 0;
    time_3 = 0;
    time_4 = 0;


    clock_t tmp_1 = std::clock();
    std::vector<query_group> clusters_ = optimizedHierarchicalClustering(group, simi);
    clock_t tmp_2 = std::clock();
    clu_1 += (double)(tmp_2 - tmp_1) / CLOCKS_PER_SEC;
    std::cout << "聚类1_2: " << clu_1 << std::endl;

    // 打印clusters_和clusters_op检查
    std::cout << "\n===== batchMinsearch_2 聚类结果检查 =====" << std::endl;
    std::cout << "clusters_ 大小: " << clusters_.size() << std::endl;
    std::cout << "clusters_op 大小: " << clusters_op.size() << std::endl;

    // 时间代价
    int count_1 = 0;
    clock_t tmp1 = std::clock();
    for (auto& codes : clusters_op) {
        std::unordered_set<int> Q;
        int k = 0;
        std::unordered_set<int> H;
        for (auto& code : codes) {
            Q.insert(qtocode[code].begin(), qtocode[code].end());
            if (k <= codetok[code]) {
                k = codetok[code];
            }
            H.insert(codetore[code].begin(), codetore[code].end());
        }

        greedy1_[count_1] = greedyStep(Q, k, H);

        clock_t tmp5 = std::clock();
        for (auto& q : codes) {
            greedy1[q] = greedy1_[count_1];
        }
        clock_t tmp6 = std::clock();
        time_3 += (double)(tmp6 - tmp5) / CLOCKS_PER_SEC;
        count_1++;
    }
    clock_t tmp2 = std::clock();
    time_1 += (double)(tmp2 - tmp1) / CLOCKS_PER_SEC;
    std::cout << "greedy_2: " << time_1 << std::endl;

    count_1 = 0;
    for (auto& codes : clusters_op) {

        clock_t tmp3p = std::clock();
        for (auto& code : codes) {
            q2[code] = steinerTree(greedy1[code], qtocode[code]);
        }

        clock_t tmp3 = std::clock();
        time_2 += (double)(tmp3 - tmp3p) / CLOCKS_PER_SEC;
        Clustering_first(codes);
        std::cout << "第二次聚类: " << clusters.size() << std::endl;
        clock_t tmp4 = std::clock();
        time_3 += (double)(tmp4 - tmp3) / CLOCKS_PER_SEC;

        for (auto& pair : clusters) {
            std::unordered_set<int> Q;
            for (auto& q : pair.second) {
                Q.insert(q2[q].begin(), q2[q].end());
            }

            int k = 0;
            for (auto& Ck : shellToComs) {
                if (Ck.second.count(codetoid[*pair.second.begin()])) {
                    k = Ck.first;
                    break;
                }
            }
            std::unordered_set<int> ans;
            ans = greedyStep_simply(Q, k, greedy1_[count_1]);
            for (auto& q : pair.second) {
                greedy2[q] = ans;
                // std::cout<< "batch的step2: " << greedy2[q].size() << std::endl;
                // output(ans, "6043.csv", k);
            }
            clock_t tmp5 = std::clock();
            time_4 += (double)(tmp5 - tmp4) / CLOCKS_PER_SEC;
            count_1++;
        }
    }
    std::cout << "steiner_2: " << time_2 << std::endl;
    std::cout << "聚类2_2: " << time_3 << std::endl;
    std::cout << "gsim_2: " << time_4 << std::endl;
    syncCompatibilityViews();
    printAndwrite("batch_7_2.csv");
}

void SharingIndex::batchMinsearch(query_group& group) {
    batchMinsearch_1(group);
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
    clusters_op.clear();
    std::vector<std::vector<double>> similarityMatrix(n, std::vector<double>(n, -1.0));
    std::priority_queue<std::tuple<double, int, int>> similarityHeap;

    // 初始化每个查询为一个单独的组
    for (int i = 0; i < threshold; ++i) {
        query_group group;
        clusters.push_back({ qtocode[i] });
        clusters_op.push_back({ i });
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
            clusters_op[0].insert(clusters_op[i].begin(), clusters_op[i].end());
            clusters.erase(clusters.begin() + i);
            clusters_op.erase(clusters_op.begin() + i);
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

    // std::cout << "开始层次聚类迭代..." << std::endl;
    int iteration = 0;

    // 层次聚类迭代
    while (clusters.size() > 1) {
        iteration++;
        // std::cout << "\n===== 迭代 #" << iteration << " =====" << std::endl;
        // std::cout << "当前聚类数量: " << clusters.size() << std::endl;

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
        clusters_op[bestI].insert(clusters_op[bestJ].begin(), clusters_op[bestJ].end());

        // 记录被删除的索引
        size_t deletedIdx = bestJ;

        // 移除被合并的组
        clusters.erase(clusters.begin() + deletedIdx);
        clusters_op.erase(clusters_op.begin() + deletedIdx);

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

        // 输出当前聚类的统计信息
        // std::cout << "当前聚类大小分布:" << std::endl;
        // for (size_t i = 0; i < clusters.size(); ++i) {
        //     std::cout << "  聚类 #" << i << ": " << clusters[i].size() << " 个查询" << std::endl;
        // }
    }

    std::cout << "\n===== 聚类完成 =====" << std::endl;
    std::cout << "最终聚类数量: " << clusters.size() << std::endl;
    for (size_t i = 0; i < clusters.size(); ++i) {
        if (clusters[i].size() != clusters_op[i].size()) {
            std::cout << "[警告] 聚类 #" << i << " 的编码节点数 = " << clusters[i].size()
                << "，但原始查询ID数 = " << clusters_op[i].size() << std::endl;
        }
    }
    return clusters;
}
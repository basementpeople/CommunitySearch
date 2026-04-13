#include "TreeIndex.h"
std::unordered_set<int> pending_parent;


int TreeIndex::modifyHighestParent(int componentId, int newParentId, int I) {
    if (newParentId == componentId) return 1;
    if (cores[*comtonode[componentId].begin()] == I) {
        // std::cout<< "错误，连通分量多余: " << componentId << " " << newParentId << std::endl;
        return 1;
    }
    if (comtopa[componentId] == -1) {
        comtopa[componentId] = newParentId;
        pending_parent.erase(componentId);
        comtoch[newParentId].insert(componentId);
        // std::cout<< "modifyHighestParent11: " << componentId << " " << newParentId << std::endl;
        return 1;
    }
    int ch = componentId;
    for (int pa = comtopa[componentId]; ; ) {
        if (pa == newParentId) {
            return 1;
        }
        if (pa == -1) {
            comtopa[ch] = newParentId;
            pending_parent.erase(ch);
            comtoch[newParentId].insert(ch);
            // std::cout<< "modifyHighestParent: " << ch << " " << newParentId << std::endl;
            return 1;
        }
        else {
            if (cores[*comtonode[pa].begin()] == I) {
                return 1;
            }
        }

        ch = pa;
        pa = comtopa[pa];
    }
}

// 寻找目标shell层的父连通分量ID
int TreeIndex::findClosestShellLayer(int componentId, int targetShell) {
    // 检查输入参数
    if (componentId < 0 || targetShell < 1) {
        return -1;
    }

    // 遍历所有shell层，从当前层到目标层
    for (int shell = cores[*comtonode[componentId].begin()]; shell >= targetShell; ) {
        // 如果当前连通分量已经属于目标shell层，直接返回
        // std::cout<< "shell: " << shell << " targetShell: " << targetShell << std::endl;
        if (shell == targetShell) {
            return componentId;
        }

        if (comtopa[componentId] != -1) {
            componentId = comtopa[componentId];
            shell = cores[*comtonode[componentId].begin()];
        }
        else {
            return -1;
        }
    }
    return -1;
}

TreeIndex::TreeIndex(Graph& graph) {

    // Graph父类继承的数据成员
    m = graph.getM();
    n = graph.getn();
    N = graph.getN();
    minimumDegree = graph.getminimumDegree();
    Dmax = graph.getDmax();
    adj = graph.getAdj();
    degrees = graph.getDegrees();
    orderedNodes = graph.getOrderedNodes();

    beMaxcom();

    // 核心分解
    cores = CoreGroup::coreGroupsAlgorithm(*this);

    // 构建shell
    // 找到最大核心度对应的连通分量集
    clock_t tmp1 = clock();
    core_max = 0;
    for (auto& pair : cores) {
        if (pair.second > core_max) {
            core_max = pair.second;
        }
    }

    int id_count = 0;
    for (int i = 0; i <= N; ++i) {
        nodetocom[i] = -1;
    }
    std::unordered_set<int> Hk;
    for (auto& pair : cores) {
        if (pair.second == core_max) {
            Hk.insert(pair.first);
        }
    }

    // 广搜得到连通分量
    std::queue<int> q;
    std::unordered_set<int> visited;
    for (auto& node : Hk) {
        if (nodetocom[node] == -1) {
            Cktocom[core_max].insert(id_count);
            comtopa[id_count] = -1;
            comtoch[id_count].insert(-1);
            nodetocom[node] = id_count++;
            comtonode[nodetocom[node]].insert(node);

            q.push(node);
            visited.insert(node);

            while (!q.empty()) {
                int tmp = q.front();
                q.pop();

                for (auto& nei : adj[tmp]) {
                    if (Hk.find(nei) != Hk.end() && visited.find(nei) == visited.end()) {
                        nodetocom[nei] = nodetocom[node];
                        comtonode[nodetocom[node]].insert(nei);
                        q.push(nei);
                        visited.insert(nei);
                    }
                }
            }

        }
        else {
            continue;
        }
    }

    for (int i = core_max - 1; i >= 1; --i) {
        std::unordered_set<int> Si;
        for (auto& pair : cores) {
            if (pair.second == i) {
                Si.insert(pair.first);
            }
        } // Si ← {nodes of core Ci}

        if (Si.size() == 0) {
            continue;
        }

        std::unordered_set<int> X = Hk; // X ← Hk
        std::unordered_set<int> T;
        // std::unordered_set<int> TMP;
        for (auto& node : Si) {
            T.clear();
            for (auto& nei : adj[node]) {
                if (X.find(nei) != X.end()) {
                    T.insert(nodetocom[nei]);
                }
            } // T ← {connected components ， child}

            if (X.find(node) != X.end()) {
                T.insert(nodetocom[node]);
            }

            if (T.size() == 0) {
                // 新的叶子节点
                Cktocom[i].insert(id_count);
                comtopa[id_count] = -1;
                pending_parent.insert(id_count);
                comtoch[id_count].insert(-1);
                nodetocom[node] = id_count++;
                comtonode[nodetocom[node]].insert(node);

                std::queue<int> q;
                std::unordered_set<int> visited;
                q.push(node);
                visited.insert(node);

                while (!q.empty()) {
                    int tmp = q.front();
                    q.pop();

                    for (auto& nei : adj[tmp]) {
                        if ((Si.find(nei) != Si.end() || X.find(nei) != X.end()) && visited.find(nei) == visited.end()) {
                            if (X.find(nei) == X.end()) {
                                nodetocom[nei] = nodetocom[node];
                                X.insert(nei);
                                comtonode[nodetocom[node]].insert(nei);
                            }

                            q.push(nei);
                            visited.insert(nei);
                        }
                    }
                }
            }
            else {
                bool flag = false;
                int id_tmp = -1;
                for (auto& tmp : T) {
                    id_tmp = findClosestShellLayer(tmp, i);
                    if (id_tmp != -1) {
                        flag = true;
                        break;
                    }
                }
                if (flag) {
                    nodetocom[node] = id_tmp;
                    comtonode[id_tmp].insert(node);
                    for (auto& tmp : T) {
                        modifyHighestParent(tmp, id_tmp, i);
                    }
                }
                else {
                    Cktocom[i].insert(id_count);
                    comtopa[id_count] = -1;
                    pending_parent.insert(id_count);
                    nodetocom[node] = id_count++;
                    comtonode[nodetocom[node]].insert(node);

                    std::queue<int> q;
                    std::unordered_set<int> visited;
                    q.push(node);
                    visited.insert(node);

                    while (!q.empty()) {
                        int tmp = q.front();
                        q.pop();

                        for (auto& nei : adj[tmp]) {
                            if ((Si.find(nei) != Si.end() || X.find(nei) != X.end()) && visited.find(nei) == visited.end()) {
                                if (X.find(nei) == X.end()) {
                                    nodetocom[nei] = nodetocom[node];
                                    X.insert(nei);
                                    comtonode[nodetocom[node]].insert(nei);
                                }

                                q.push(nei);
                                visited.insert(nei);
                            }
                        }
                    }

                    for (auto& tmp : T) {
                        modifyHighestParent(tmp, nodetocom[node], i);
                    }
                }
            }
            X.insert(node);
        }
        Hk = X;
    }
    clock_t tmp2 = clock();
    // std::cout << "buildTreeIndex time: " << (double)(tmp2 - tmp1) / CLOCKS_PER_SEC << "s" << std::endl;

    // 打印一遍核心索引、连通分量树
    // printTreeIndex();

}

void TreeIndex::beMaxcom() {
    std::unordered_set<int> visited; // 存储已访问的节点
    std::queue<int> q;
    std::unordered_map<int, int> de = degrees;
    std::unordered_map<int, int> ntc; // 是否属于同一连通分量
    std::unordered_map<int, std::vector<int>> components; // 存储每个连通分量的节点

    int l = 0;
    while (de.size() > 0) {
        l++;
        int tmp = de.begin()->first;

        // ➤ 跳过孤立点（无邻居）
        if (getNeighbors(tmp).empty()) {
            de.erase(tmp);
            continue;
        }

        q.push(tmp);
        visited.insert(tmp);
        components[l] = std::vector<int>(); // 初始化当前连通分量的节点列表

        while (!q.empty()) {
            int node = q.front();
            q.pop();

            ntc[node] = l;
            components[l].push_back(node); // 将节点添加到当前连通分量
            de.erase(node);

            for (int neighbor : getNeighbors(node)) {
                if (visited.find(neighbor) == visited.end()) {
                    q.push(neighbor);
                    visited.insert(neighbor);
                }
            }
        }
    }

    // 找出最大连通分量
    int maxComponentId = 0;
    size_t maxSize = 0;
    for (auto& pair : components) {
        if (pair.second.size() > maxSize) {
            maxSize = pair.second.size();
            maxComponentId = pair.first;
        }
    }

    int removedNodes = 0;
    // 保留最大连通分量，移除其他节点
    for (const auto& pair : components) {
        if (pair.first != maxComponentId) {
            for (int node : pair.second) {
                removeNode(node); // 假设这个函数存在，用于从图中移除节点
            }
        }
    }

    return;
}

// 打印核心索引和连通分量树
void TreeIndex::printTreeIndex() {
    std::cout << "===== 核心索引结构 =====" << std::endl;
    std::cout << "节点总数: " << n << ", 最大核心度: " << core_max << std::endl;
    int ni = 0;
    // 打印每个核心度的连通分量
    for (int k = core_max; k >= 1; --k) {
        if (Cktocom.find(k) == Cktocom.end() || Cktocom[k].empty()) {
            continue;
        }

        std::cout << "\n核心度 k = " << k << " 的连通分量:" << std::endl;
        for (int comp_id : Cktocom[k]) {
            ni += comtonode[comp_id].size();
            std::cout << "  连通分量 ID: " << comp_id
                << ", 节点数: " << comtonode[comp_id].size()
                << ", 父节点: " << comtopa[comp_id]
                << ", 子节点数: " << comtoch[comp_id].size() << std::endl;
        }
    }
    // std::cout << "节点总数（统计）: " << ni << std::endl;

    // 打印连通分量树结构
    std::cout << "\n===== 连通分量树结构 =====" << std::endl;
    std::cout << "根节点 (核心度 " << core_max << "):" << std::endl;

    // 找到所有根节点（没有父节点的连通分量）
    std::queue<int> comp_queue;
    for (int comp_id : Cktocom[core_max]) {
        if (comtopa[comp_id] == -1) {
            std::cout << "  根连通分量 ID: " << comp_id
                << ", 节点数: " << comtonode[comp_id].size() << std::endl;
            comp_queue.push(comp_id);
        }
    }

    // BFS遍历树结构
    while (!comp_queue.empty()) {
        int current_comp = comp_queue.front();
        comp_queue.pop();

        // 打印子节点
        for (int child_comp : comtoch[current_comp]) {
            if (child_comp != -1) {  // 跳过占位符-1
                std::cout << "    子连通分量 ID: " << child_comp
                    << ", 节点数: " << comtonode[child_comp].size()
                    << ", 父节点: " << current_comp << std::endl;
                comp_queue.push(child_comp);
            }
        }
    }

    // 打印节点到连通分量的映射（可选，大型图可能会输出过多信息）
    if (n < 100) {  // 仅在节点数较少时打印详细映射
        std::cout << "\n===== 节点到连通分量映射 =====" << std::endl;
        for (int i = 0; i < n; ++i) {
            if (nodetocom[i] != -1) {
                std::cout << "节点 " << i << " -> 连通分量 " << nodetocom[i]
                    << " (核心度 " << cores[i] << ")" << std::endl;
            }
        }
    }
    else {
        std::cout << "\n节点数较多，省略详细的节点到连通分量映射。" << std::endl;
    }
}

std::unordered_set<int> TreeIndex::RetrievalShellStruct(query_nodes& queryNodes, int& k) {
    k = 0;
    query_nodes Q = queryNodes;
    for (auto& node : queryNodes) {
        if (cores[node] > k) {
            k = cores[node];
        }
    }
    // k ← max{c(u) | u ∈ Q}

    // 注意，虽然是shell索引，但本质上存的还是core，所以使用的时候需要拿到shell的所有子分量
    std::unordered_set<int> H;
    for (auto& node : queryNodes) {
        if (cores[node] == k) {
            Q.erase(node);
            for (int comp_id : Cktocom[k]) {
                if (comtonode[comp_id].find(node) != comtonode[comp_id].end()) {
                    H.insert(comp_id);
                }
            }
        }
    } // H ← {connected components of core Ck containing a vertex from Q}

    std::unordered_set<int> Hx;
    // for (auto &id : H) {
    //     for (auto &node : comtonode[id]) {
    //         Hx.insert(node);
    //     }
    // }
    // H∗ ← ⋃  H∈H vertices(H)  当然，我们可以找到最后的shell再把点放进去

    while (H.size() != 1 || Q.size() != 0) {
        k = k - 1;
        query_nodes Q_;
        for (auto& node : Q) {
            if (cores[node] == k) {
                Q_.insert(node);
            }
        }
        for (auto& node : Q_) {
            Q.erase(node);
        }
        // 对于Q的点很少，有没有必要用一个核心度列表优化

        std::unordered_set<int> Hp;
        for (auto& com : H) {
            if (comtopa[com] != -1) {
                if (cores[*comtonode[comtopa[com]].begin()] == k) {
                    Hp.insert(comtopa[com]);
                }
                else {
                    Hp.insert(com);
                }
            }
        }
        if (Q_.size() > 0) {
            for (auto& node : Q_) {
                for (int comp_id : Cktocom[k]) {
                    if (comtonode[comp_id].find(node) != comtonode[comp_id].end()) {
                        Hp.insert(comp_id);
                    }
                }
            }
        }
        H = Hp;
    }

    // if (Q.size() == 0) {
    //     std::cout<< "成功遍历所有查询顶点" << std::endl;
    // }
    if (H.size() == 1) {
        for (auto& id : H) {
            Hx = getchnodes(id);
        }
    }
    std::cout << "Hx " << Hx.size() << std::endl;

    // output(Hx, "Hx.csv", k);
    return Hx;
}

std::unordered_set<int> TreeIndex::getchnodes(int id) {
    std::unordered_set<int> Hx;
    std::queue<int> ids;
    ids.push(id);
    while (ids.size() != 0) {
        int tmp = ids.front();
        ids.pop();

        if (comtonode[tmp].size() != 0)
            Hx.insert(comtonode[tmp].begin(), comtonode[tmp].end());

        if (comtoch[tmp].size() != 0) {
            for (auto& ch : comtoch[tmp]) {
                // 目前我的理解是，只有叶子节点会有一个为-1的子节点
                if (ch != -1) {
                    ids.push(ch);
                }
            }
        }
        else {
            // std::cout << "警告：存在非法树节点，请检查shell索引" << std::endl;
        }

    }
    return Hx;
}

std::unordered_set<int> TreeIndex::getchnodes_2(int id, int& k) {
    std::unordered_set<int> Hx;
    std::queue<int> ids;
    ids.push(id);

    while (ids.size() != 0) {
        int tmp = ids.front();
        ids.pop();

        if (comtonode[tmp].size() != 0)
            Hx.insert(comtonode[tmp].begin(), comtonode[tmp].end());

        if (k <= cores[*comtonode[tmp].begin()]) {
            continue;
        }

        if (comtoch[tmp].size() != 0) {
            for (auto& ch : comtoch[tmp]) {
                // 目前我的理解是，只有叶子节点会有一个为-1的子节点
                if (ch != -1) {
                    ids.push(ch);
                }
            }
        }
        else {
            std::cout << "警告：存在非法树节点，请检查shell索引" << std::endl;
        }

    }
    if (comtonode[id].size() != 0)
        k = cores[*comtonode[id].begin()];

    return Hx;
}

bool TreeIndex::checkMinDegree(const std::unordered_map<int, int>& a, int k) {
    if (a.size() == 0) return false;
    int min = k;
    for (auto& pair : a) {
        if (pair.second < k) {
            min = pair.second;
        }
    }
    if (min == k) {
        return true;
    }
    return false;
}

bool TreeIndex::checkComponent(std::unordered_map<int, int>& a) {
    int x = -2;
    for (auto& pair : a) {
        if (pair.second != -1) {
            x = pair.second;
            break;
        }
    }
    bool flag = true;
    for (auto& pair : a) {
        if (pair.second != x && pair.second != -1) {
            flag = false;
        }
    }
    if (flag && x != -2) {
        return true;
    }
    else {
        return false;
    }
}

std::unordered_set<int> TreeIndex::greedyConnection(query_nodes& queryNodes, int k, std::unordered_set<int> H) {

    // Step 1: Greedy Step
    clock_t tmp1 = clock();
    std::unordered_set<int> H_min_x = greedyStep(queryNodes, k, H);

    std::cout << "H_min_x.size() " << H_min_x.size() << std::endl;
    for (auto& node : H_min_x) {
        if (H.find(node) == H.end()) {
            std::cout << node << std::endl;
        }
    }
    clock_t tmp2 = clock();
    time_greedy += (double)(tmp2 - tmp1) / CLOCKS_PER_SEC;
    std::cout << "greedyStep " << time_greedy << std::endl;
    output(H_min_x, "604200.csv", k);

    // Step 2: Connection Step
    std::unordered_set<int> ans = connectionStep(H_min_x, queryNodes, k);
    output(ans, "604211.csv", k);

    return ans;
}

//std::unordered_set<int> TreeIndex::greedyStep(query_nodes& queryNodes, int k, std::unordered_set<int>& H) {
//
//    clearAllData();
//
//    std::unordered_set<int> H_min_x;
//    query_nodes Q = queryNodes;
//
//    // 预分配空间
//    std::unordered_map<int, int> A_;
//    A_.reserve(H.size());
//    std::unordered_map<int, std::unordered_set<int>> cc;
//    cc.reserve(H.size());
//    //std::unordered_set<int> visited;
//    //std::unordered_map<int, int> visited;
//    std::vector<bool> visited_vec(N+1, false);
//    //visited.reserve(H.size());
//    std::unordered_map<int, std::pair<int, std::pair<int, int>>> p;
//    p.reserve(H.size());
//
//    int a_count = 0, b_count = 0, u_x = k;
//    MinDegreeHeap u_heap;
//
//    for (auto& node : H) {
//        A_[node] = -1;
//        //visited[node] = 0;
//    }
//
//    // 优化邻居缓存构建：减少不必要的查找
//    std::unordered_map<int, std::unordered_set<int>> adj_H;
//    adj_H.reserve(H.size());
//    for (const auto& pair : adj) {
//        if (!H.count(pair.first)) continue;
//        for (const auto& node : pair.second) {
//            if (H.count(node)) adj_H[pair.first].insert(node);
//        }
//    }
//
//    for (int q : Q) {
//        updateVertexScore(q, INT_MAX, 0);
//        p[q] = { INT_MAX, {0, 0} };
//        //visited.insert(q);
//        //visited[q] = 1;
//        visited_vec[q] = true;
//    }
//
//    int uii = 0;
//    while (cc.size() != 1 || u_heap.peek_min().first < u_x) {
//        if (H_min_x.size() == H.size()) break;
//        int u = extractNextValidVertex();
//        if (u == -1) break;
//        if (H_min_x.count(u)) continue;
//
//        H_min_x.insert(u);
//        uii++;
//        //visited.erase(u);
//		//visited[u] = 0;
//        visited_vec[u] = false;
//
//        int count = 0;
//        for (int v : adj_H[u]) {
//            if (H_min_x.count(v)) ++count;
//        }
//        u_heap.update(u, count);
//
//        // 优化邻居优先级更新：减少重复查找
//        for (int v : adj_H[u]) {
//            //if (!H_min_x.count(v) && !visited.count(v)) {
//            //if (!H_min_x.count(v) && visited[v] ==0) {
//            if (!H_min_x.count(v) && !visited_vec[v]) {
//                //visited.insert(v);
//				//visited[v] = 1;
//                visited_vec[v] = true;
//                p[v] = { 0, {0, u_x} };
//                updateVertexScore(v, p[v].first, p[v].second.first - p[v].second.second);
//            }
//        }
//
//        std::unordered_set<int> ids;
//        for (int v : adj_H[u]) {
//            if (H_min_x.count(v)) {
//                u_heap.update(v, u_heap.get_degree(v) + 1);
//                if (u_heap.get_degree(v) == u_x) {
//                    // 优化：批量处理visited
//                    for (int w : adj_H[v]) {
//                        //if (visited.count(w)) {
//						//if (visited[w] == 1) {
//                        if (visited_vec[w]) {
//                            p[w].second.first -= 1;
//                            updateVertexScore(w, p[w].first, p[w].second.first - p[w].second.second);
//                        }
//                    }
//                }
//                if (A_[v] != -1) ids.insert(A_[v]);
//            }
//        }
//
//        bool ff = false;
//        if (ids.empty()) {
//            A_[u] = b_count;
//            cc[b_count].insert(u);
//            ++b_count;
//        }
//        else if (ids.size() == 1) {
//            A_[u] = *ids.begin();
//            cc[A_[u]].insert(u);
//        }
//        else {
//            ff = true;
//            int now_id = *ids.begin();
//            for (auto& id : ids) {
//                if (id != now_id) {
//                    for (auto& node : cc[id]) {
//                        A_[node] = now_id;
//                        cc[now_id].insert(node);
//                        for (auto& nei : adj_H[node]) {
//                            //if (visited.count(nei)) {
//							//if (visited[nei] == 1) {
//                            if (visited_vec[nei]) {
//                                p[nei].first -= 1;
//                                updateVertexScore(nei, p[nei].first, p[nei].second.first - p[nei].second.second);
//                            }
//                        }
//                    }
//                    cc.erase(id);
//                }
//                else {
//                    for (auto& node : cc[id]) {
//                        for (auto& nei : adj_H[node]) {
//                            //if (visited.count(nei)) {
//							//if (visited[nei] == 1) {
//                            if (visited_vec[nei]) {
//                                p[nei].first -= 1;
//                                updateVertexScore(nei, p[nei].first, p[nei].second.first - p[nei].second.second);
//                            }
//                        }
//                    }
//                }
//            }
//            A_[u] = now_id;
//            cc[now_id].insert(u);
//        }
//
//        // 重点优化：减少不必要的查找和冗余计算
//        const std::unordered_set<int>& uNeighbors = adj_H[u];
//        std::unordered_set<int> tmp;
//        for (const auto& node : cc[A_[u]]) {
//            // 预取邻居集合，减少多次查找
//            const auto& nodeAdj = adj_H[node];
//            for (const auto& w : nodeAdj) {
//                //if (!visited.count(w)) continue;
//				//if (visited[w] == 0) continue;
//                if (!visited_vec[w]) continue;
//
//                if (ff && node != u) p[w].first += 1;
//
//                if (uNeighbors.count(w)) {
//                    if (u_heap.get_degree(u) < u_x) p[w].second.first += 1;
//                    p[w].second.second = std::max(p[w].second.second - 1, 0);
//                }
//
//				tmp.insert(w);
//                //updateVertexScore(w, p[w].first, p[w].second.first - p[w].second.second);
//            }
//        }
//
//        for (auto& w : tmp) {
//            updateVertexScore(w, p[w].first, p[w].second.first - p[w].second.second);
//		}
//    }
//    std::cout << " uii: " << uii++ << std::endl;
//    return H_min_x;
//}

std::unordered_set<int> TreeIndex::greedyStep(query_nodes& queryNodes, int k, std::unordered_set<int>& H) {

    clearAllData();

    std::unordered_set<int> H_(H.begin(), H.end());
    std::unordered_set<int> H_min_x;
    query_nodes Q = queryNodes;

    // 预分配空间
    std::unordered_map<int, int> A_;
    A_.reserve(H_.size());
    std::unordered_map<int, std::unordered_set<int>> cc;
    cc.reserve(H_.size());
    //std::unordered_set<int> visited;
    //std::unordered_map<int, int> visited;
    std::vector<bool> visited_vec(N + 1, false);
    //visited.reserve(H.size());
    std::unordered_map<int, std::pair<int, std::pair<int, int>>> p;
    p.reserve(H_.size());

    int a_count = 0, b_count = 0, u_x = k;
    MinDegreeHeap u_heap;

    for (auto& node : H_) {
        A_[node] = -1;
        //visited[node] = 0;
    }

    // 优化邻居缓存构建：减少不必要的查找
    std::unordered_map<int, std::unordered_set<int>> adj_H;
    adj_H.reserve(H_.size());
    for (const auto& pair : adj) {
        if (!H_.count(pair.first)) continue;
        for (const auto& node : pair.second) {
            if (H_.count(node)) adj_H[pair.first].insert(node);
        }
    }

    for (int q : Q) {
        updateVertexScore(q, INT_MAX, 0);
        p[q] = { INT_MAX, {0, 0} };
        //visited.insert(q);
        //visited[q] = 1;
        visited_vec[q] = true;
    }

    int uii = 0;
    while (cc.size() != 1 || u_heap.peek_min().first < u_x) {
        if (H_min_x.size() == H_.size()) break;
        int u = extractNextValidVertex();
        if (u == -1) break;
        if (H_min_x.count(u)) continue;

        H_min_x.insert(u);
        uii++;
        //visited.erase(u);
        //visited[u] = 0;
        visited_vec[u] = false;

        int count = 0;
        for (int v : adj_H[u]) {
            if (H_min_x.count(v)) ++count;
        }
        u_heap.update(u, count);

        // 优化邻居优先级更新：减少重复查找
        for (int v : adj_H[u]) {
            //if (!H_min_x.count(v) && !visited.count(v)) {
            //if (!H_min_x.count(v) && visited[v] == 0) {
                if (!H_min_x.count(v) && !visited_vec[v]) {
                //visited.insert(v);
                //visited[v] = 1;
                visited_vec[v] = true;
                p[v] = { 0, {0, u_x} };
                updateVertexScore(v, p[v].first, p[v].second.first - p[v].second.second);
            }
        }

        std::unordered_set<int> ids;
        for (int v : adj_H[u]) {
            if (H_min_x.count(v)) {
                u_heap.update(v, u_heap.get_degree(v) + 1);
                if (u_heap.get_degree(v) == u_x) {
                    // 优化：批量处理visited
                    for (int w : adj_H[v]) {
                        //if (visited.count(w)) {
                            //if (visited[w] == 1) {
                        if (visited_vec[w]) {
                            p[w].second.first -= 1;
                            updateVertexScore(w, p[w].first, p[w].second.first - p[w].second.second);
                        }
                    }
                }
                if (A_[v] != -1) ids.insert(A_[v]);
            }
        }

        bool ff = false;
        if (ids.empty()) {
            A_[u] = b_count;
            cc[b_count].insert(u);
            ++b_count;
        }
        else if (ids.size() == 1) {
            A_[u] = *ids.begin();
            cc[A_[u]].insert(u);
        }
        else {
            ff = true;
            int now_id = *ids.begin();
            for (auto& id : ids) {
                if (id != now_id) {
                    for (auto& node : cc[id]) {
                        A_[node] = now_id;
                        cc[now_id].insert(node);
                        for (auto& nei : adj_H[node]) {
                            //if (visited.count(nei)) {
                            //if (visited[nei] == 1) {
                            if (visited_vec[nei]) {
                                p[nei].first -= 1;
                                updateVertexScore(nei, p[nei].first, p[nei].second.first - p[nei].second.second);
                            }
                        }
                    }
                    cc.erase(id);
                }
                else {
                    for (auto& node : cc[id]) {
                        for (auto& nei : adj_H[node]) {
                            //if (visited.count(nei)) {
                            //if (visited[nei] == 1) {
                            if (visited_vec[nei]) {
                                p[nei].first -= 1;
                                updateVertexScore(nei, p[nei].first, p[nei].second.first - p[nei].second.second);
                            }
                        }
                    }
                }
            }
            A_[u] = now_id;
            cc[now_id].insert(u);
        }

        // 重点优化：减少不必要的查找和冗余计算
        const std::unordered_set<int>& uNeighbors = adj_H[u];
        std::unordered_set<int> tmp;
        for (const auto& node : cc[A_[u]]) {
            // 预取邻居集合，减少多次查找
            const auto& nodeAdj = adj_H[node];
            for (const auto& w : nodeAdj) {
                //if (!visited.count(w)) continue;
                //if (visited[w] == 0) continue;
                if (!visited_vec[w]) continue;

                if (ff && node != u) p[w].first += 1;

                if (uNeighbors.count(w)) {
                    if (u_heap.get_degree(u) < u_x) p[w].second.first += 1;
                    p[w].second.second = std::max(p[w].second.second - 1, 0);
                }

                tmp.insert(w);
                //updateVertexScore(w, p[w].first, p[w].second.first - p[w].second.second);
            }
        }

        for (auto& w : tmp) {
            updateVertexScore(w, p[w].first, p[w].second.first - p[w].second.second);
        }

    }
    std::cout << " uii: " << uii++ << std::endl;
    std::unordered_set<int> H_min_xp(H_min_x.begin(), H_min_x.end());
    return H_min_xp;
}

std::unordered_set<int> TreeIndex::greedyStep_simply(query_nodes& queryNodes, int k, std::unordered_set<int>& H) {

    clearAllData();

    std::unordered_set<int> H_min_x;
    query_nodes Q = queryNodes;
    int a_count = 0;
    int u_x = k;
    MinDegreeHeap u_heap; // 最小堆维护最小度

    // 优化：预分配空间
    //std::unordered_set<int> visited;
    //visited.reserve(H.size());
    std::vector<bool> visited_vec(N + 1, false);
    std::unordered_map<int, std::pair<int, std::pair<int, int>>> p;
    p.reserve(H.size());

    // 优化邻居缓存构建
    std::unordered_map<int, std::unordered_set<int>> adj_H;
    adj_H.reserve(H.size());
    for (const auto& pair : adj) {
        if (!H.count(pair.first)) continue;
        for (const auto& node : pair.second) {
            if (H.count(node)) adj_H[pair.first].insert(node);
        }
    }

    for (int q : Q) {
        updateVertexScore(q, INT_MAX, 0);  // p'(q)=+∞，p''(q)=0
        p[q] = { INT_MAX, {0, 0} };
        //visited.insert(q);
        visited_vec[q] = true;
    }

    while (u_heap.peek_min().first < u_x || H_min_x.size() <= Q.size()) {

        int u = extractNextValidVertex(); // u ← P.poll()
        if (u == -1) {
            break;
        }
        // 不能重复插入顶点
        if (H_min_x.count(u)) continue;

        H_min_x.insert(u); // H∗  min ← H∗  min ∪ {u}
        //visited.erase(u);
        visited_vec[u] = false;

        int count = 0;
        for (int v : adj_H[u]) {
            if (H_min_x.count(v)) {
                count++;
            }
        }
        u_heap.update(u, count); // α(u) ← |neigh(u, H∗min )|

        // 优化邻居优先级更新：减少重复查找
        for (int v : adj_H[u]) {
            //if (!H_min_x.count(v) && !visited.count(v)) {
            if (!H_min_x.count(v) && !visited_vec[v]) {
                //visited.insert(v);
                visited_vec[v] = true;
                p[v] = { 0, {0, u_x} };
                updateVertexScore(v, p[v].first, p[v].second.first - p[v].second.second);
            }
        }

        std::unordered_set<int> ids;
        for (int v : adj_H[u]) {
            if (H_min_x.count(v)) {
                u_heap.update(v, u_heap.get_degree(v) + 1); // α(v) ← α(v) + 1
                if (u_heap.get_degree(v) == u_x) {
                    for (int w : adj_H[v]) {
                        //if (visited.count(w)) {
                        if (visited_vec[w]) {
                            if (p[w].second.first < 1) {
                                // std::cout << "警告：非法映射" << std::endl;
                            }
                            p[w].second.first -= 1;
                            updateVertexScore(w, p[w].first, p[w].second.first - p[w].second.second);
                        }
                    }
                }
            }
        }

        // 重点优化：减少不必要的查找和冗余计算
        const std::unordered_set<int>& uNeighbors = adj_H[u];
        std::unordered_set<int> tmp;
        for (const auto& node : H_min_x) {
            const auto& nodeAdj = adj_H[node];
            for (const auto& w : nodeAdj) {
                //if (!visited.count(w)) continue;
                if (!visited_vec[w]) continue;

                if (uNeighbors.count(w)) {
                    if (u_heap.get_degree(u) < u_x) {
                        p[w].second.first += 1;
                    }
                    p[w].second.second = std::max(p[w].second.second - 1, 0);
                }

                tmp.insert(w);
                //updateVertexScore(w, p[w].first, p[w].second.first - p[w].second.second);
            }
        }

        for (auto& w : tmp) {
            updateVertexScore(w, p[w].first, p[w].second.first - p[w].second.second);
        }
    }

    return H_min_x;
}

std::unordered_set<int> TreeIndex::connectionStep(std::unordered_set<int>& H, query_nodes& queryNodes, int k) {

    clock_t tmp1 = clock();
    query_nodes r = steinerTree(H, queryNodes);
    clock_t tmp2 = clock();
    time_steiner += (double)(tmp2 - tmp1) / CLOCKS_PER_SEC;

    clock_t tmp3 = clock();
    std::unordered_set<int> result = greedyStep_simply(r, k, H);
    // std::unordered_set<int> result = greedyStep(r, k, H);
    std::cout << "result.size() " << result.size() << std::endl;
    clock_t tmp4 = clock();
    time_simple += (double)(tmp4 - tmp3) / CLOCKS_PER_SEC;
    std::cout << "time_simple " << time_simple << std::endl;

    return result;

}

// 斯坦纳树的近似算法，prim
query_nodes TreeIndex::steinerTree(std::unordered_set<int>& H, query_nodes& terminals) {
    std::vector<int> mstParent(N, -1); // MST 中的父节点
    std::vector<bool> inMST(N, false); // 标记是否在 MST 中
    std::vector<int> key(N, INT32_MAX); // 每个节点的键值（最小边权重）
    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<>> pq; // 优先队列

    std::unordered_map<int, std::unordered_set<int>> adj_H;
    for (auto& pair : adj) {
        if (H.count(pair.first)) {
            for (auto& node : pair.second) {
                if (H.count(node)) {
                    adj_H[pair.first].insert(node);
                }
            }
        }
    }

    // 从任意一个终端节点开始
    pq.push({ 0, *terminals.begin() });
    key[*terminals.begin()] = 0;

    while (!pq.empty()) {
        int u = pq.top().second; // 当前节点
        pq.pop();

        if (inMST[u]) continue; // 如果已经在 MST 中，跳过
        inMST[u] = true;

        // 遍历当前节点的所有邻居
        for (int v : adj_H[u]) {
            if (!inMST[v] && key[v] > 1) { // 权重为1（无权图）
                key[v] = 1; // 更新键值
                pq.push({ key[v], v }); // 将邻居加入优先队列
                mstParent[v] = u; // 设置父节点
            }
        }
    }

    // 提取包含终端节点的子树
    std::vector<int> steinerTree;
    for (int v : terminals) {
        while (v != -1) { // 从终端节点向上追溯到根
            steinerTree.push_back(v);
            v = mstParent[v];
        }
    }
    std::unordered_set<int> steinerTree_(steinerTree.begin(), steinerTree.end());
    return steinerTree_;
}

void TreeIndex::output(std::unordered_set<int> ans, std::string path, int k) {
    int actual_min_deg2 = INT_MAX;
    for (int u : ans) {
        int deg = 0;
        for (int v : adj[u]) {
            if (ans.count(v)) deg++;
        }
        actual_min_deg2 = std::min(actual_min_deg2, deg);
    }
    std::cout << "实际最小度数: " << actual_min_deg2 << ", 应满足 ≥ " << k << std::endl;

    std::ofstream outfile(path);
    outfile << "Source,Target\n";  // Gephi 需要的表头

    for (auto& node : ans) {
        for (auto& nei : adj[node]) {
            if (ans.count(nei) && node < nei) {  // 防止重复输出无向边
                outfile << node << "," << nei << "\n";
            }
        }
    }
    outfile.close();
}

struct ScoreCompare {
    bool operator()(const std::pair<int, int>& a, const std::pair<int, int>& b) const {
        if (a.first != b.first)
            return a.first > b.first;  // 分数小的优先
        return a.second > b.second;    // 节点编号小的优先（稳定）
    }
};
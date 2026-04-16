#include "TreeIndex.h"
#include "ProjectConstants.h"
std::unordered_set<int> pending_parent;

void TreeIndex::copyGraphState(const Graph& graph) {
    m = graph.getm();
    n = graph.getn();
    N = graph.getN();
    minDegree = graph.getMinDegree();
    maxDegree = graph.getMaxDegree();
    adj = graph.getAdj();
    degrees = graph.getDegrees();
    orderedNodes = graph.getOrderedNodes();
}

std::unordered_map<int, std::unordered_set<int>> TreeIndex::groupNodesByShell() const {
    std::unordered_map<int, std::unordered_set<int>> shellToNodes;
    // 预分组后，后面按 shell 逐层处理时可以直接取节点集，
    // 不必反复扫描整张 nodeToShell。
    for (const auto& pair : nodeToShell) {
        shellToNodes[pair.second].insert(pair.first);
    }
    return shellToNodes;
}

void TreeIndex::processShellLayer(int shell, const std::unordered_set<int>& shellNodes,
                                  std::unordered_set<int>& accumulatedNodes, int& nextComponentId) {
    std::unordered_set<int> touchingComponents;

    for (auto& node : shellNodes) {
        // touchingComponents 表示当前节点在更高层/已处理节点中接触到的 component。
        // 后续是否新建 component，取决于这个集合是否为空，以及能否找到合适父层。
        touchingComponents.clear();
        for (auto& nei : adj[node]) {
            if (accumulatedNodes.find(nei) != accumulatedNodes.end()) {
                touchingComponents.insert(nodeToCom[nei]);
            }
        }

        if (accumulatedNodes.find(node) != accumulatedNodes.end()) {
            touchingComponents.insert(nodeToCom[node]);
        }

        if (touchingComponents.empty()) {
            // 情况 1：当前节点与已建树部分完全不接触。
            // 这时它需要作为一个新的叶子 component 出现，并向外扩展吸收同层/已处理邻居。
            shellToComs[shell].insert(nextComponentId);
            comToParent[nextComponentId] = -1;
            pending_parent.insert(nextComponentId);
            comToChildren[nextComponentId].insert(-1);
            nodeToCom[node] = nextComponentId;
            comToNodes[nextComponentId].insert(node);

            std::queue<int> q;
            std::unordered_set<int> visited;
            q.push(node);
            visited.insert(node);

            while (!q.empty()) {
                int current = q.front();
                q.pop();

                for (auto& nei : adj[current]) {
                    if ((shellNodes.find(nei) != shellNodes.end() ||
                         accumulatedNodes.find(nei) != accumulatedNodes.end()) &&
                        visited.insert(nei).second) {
                        if (accumulatedNodes.find(nei) == accumulatedNodes.end()) {
                            nodeToCom[nei] = nextComponentId;
                            accumulatedNodes.insert(nei);
                            comToNodes[nextComponentId].insert(nei);
                        }
                        q.push(nei);
                    }
                }
            }
            ++nextComponentId;
        } else {
            bool foundParent = false;
            int targetComponentId = -1;
            // 情况 2/3：当前节点已经接触到已有 component。
            // 先尝试沿父链找到一个正好位于目标 shell 层的可挂接 component。
            for (auto& componentId : touchingComponents) {
                targetComponentId = findClosestShellLayer(componentId, shell);
                if (targetComponentId != -1) {
                    foundParent = true;
                    break;
                }
            }

            if (foundParent) {
                // 情况 2：找到了可直接挂接的父 component。
                // 当前节点并入该 component，同时把接触到的其他 component 父关系向上修正。
                nodeToCom[node] = targetComponentId;
                comToNodes[targetComponentId].insert(node);
                for (auto& componentId : touchingComponents) {
                    modifyHighestParent(componentId, targetComponentId, shell);
                }
            } else {
                // 情况 3：接触到了旧 component，但找不到合适的当前层父节点。
                // 这时新建一个 component，把当前节点及其可扩展部分吸收进来，
                // 再把 touchingComponents 挂到这个新 component 之下。
                shellToComs[shell].insert(nextComponentId);
                comToParent[nextComponentId] = -1;
                pending_parent.insert(nextComponentId);
                comToChildren[nextComponentId].insert(-1);
                nodeToCom[node] = nextComponentId;
                comToNodes[nextComponentId].insert(node);

                std::queue<int> q;
                std::unordered_set<int> visited;
                q.push(node);
                visited.insert(node);

                while (!q.empty()) {
                    int current = q.front();
                    q.pop();

                    for (auto& nei : adj[current]) {
                        if ((shellNodes.find(nei) != shellNodes.end() ||
                             accumulatedNodes.find(nei) != accumulatedNodes.end()) &&
                            visited.insert(nei).second) {
                            if (accumulatedNodes.find(nei) == accumulatedNodes.end()) {
                                nodeToCom[nei] = nextComponentId;
                                accumulatedNodes.insert(nei);
                                comToNodes[nextComponentId].insert(nei);
                            }
                            q.push(nei);
                        }
                    }
                }

                for (auto& componentId : touchingComponents) {
                    modifyHighestParent(componentId, nodeToCom[node], shell);
                }
                ++nextComponentId;
            }
        }

        // X/Hk 这条线表示“到当前 shell 为止已经纳入索引树的节点集合”。
        // 这里把当前节点加入集合，供后续同层节点判断接触关系使用。
        accumulatedNodes.insert(node);
    }
}

// 修改最高父连通分量
int TreeIndex::modifyHighestParent(int componentId, int newParentId, int I) {
    if (newParentId == componentId) return 1;
    if (nodeToShell[*comToNodes[componentId].begin()] == I) {
        return 1;
    }
    if (comToParent[componentId] == -1) {
        comToParent[componentId] = newParentId;
        pending_parent.erase(componentId);
        comToChildren[newParentId].insert(componentId);
        return 1;
    }
    int ch = componentId;
    for (int pa = comToParent[componentId]; ; ) {
        if (pa == newParentId) {
            return 1;
        }
        if (pa == -1) {
            comToParent[ch] = newParentId;
            pending_parent.erase(ch);
            comToChildren[newParentId].insert(ch);
            return 1;
        }
        else {
            if (nodeToShell[*comToNodes[pa].begin()] == I) {
                return 1;
            }
        }

        ch = pa;
        pa = comToParent[pa];
    }
}

// 寻找目标shell层的父连通分量ID
int TreeIndex::findClosestShellLayer(int componentId, int targetShell) {
    // 检查输入参数
    if (componentId < 0 || targetShell < 1) {
        return -1;
    }

    // 遍历所有shell层，从当前层到目标层
    for (int shell = nodeToShell[*comToNodes[componentId].begin()]; shell >= targetShell; ) {
        // 如果当前连通分量已经属于目标shell层，直接返回
        if (shell == targetShell) {
            return componentId;
        }

        if (comToParent[componentId] != -1) {
            componentId = comToParent[componentId];
            shell = nodeToShell[*comToNodes[componentId].begin()];
        }
        else {
            return -1;
        }
    }
    return -1;
}

TreeIndex::TreeIndex(Graph& graph) {
    // 先复制 Graph 的基础结构，再在 TreeIndex 内部继续裁图和建索引。
    copyGraphState(graph);

    // 只在最大连通分量上建立索引，避免后面在多个互不连通的子图上浪费工作。
    getMaxCom();

    // 核心分解：得到每个节点所在的 shell 层。
    nodeToShell = CoreGroup::coreDecomposition(*this);
    std::unordered_map<int, std::unordered_set<int>> shellToNodes = groupNodesByShell();

    // 找到最高 shell 层，后续从这一层开始向下构建 component tree。
    coreMax = 0;
    for (auto& pair : nodeToShell) {
        if (pair.second > coreMax) {
            coreMax = pair.second;
        }
    }

    int id_count = 0;
    nodeToCom.clear();
    for (int i = 0; i <= N; ++i) {
        nodeToCom[i] = -1;
    }

    // Hk 表示“当前已经纳入树中的节点集合”。
    // 初始化时，它就是最高 shell 层的全部节点。
    std::unordered_set<int> Hk = shellToNodes[coreMax];

    // 最高 shell 层的处理单独保留在构造函数里，这样主流程更直观：
    // 先在顶层做一次 BFS 划分 component，再逐层向下扩展。
    for (auto& node : Hk) {
        if (nodeToCom[node] != -1) {
            continue;
        }

        shellToComs[coreMax].insert(id_count);
        comToParent[id_count] = -1;
        comToChildren[id_count].insert(-1);
        nodeToCom[node] = id_count;
        comToNodes[id_count].insert(node);

        std::queue<int> q;
        std::unordered_set<int> visited;
        q.push(node);
        visited.insert(node);

        while (!q.empty()) {
            int current = q.front();
            q.pop();

            for (auto& nei : adj[current]) {
                if (Hk.find(nei) != Hk.end() && visited.insert(nei).second) {
                    nodeToCom[nei] = id_count;
                    comToNodes[id_count].insert(nei);
                    q.push(nei);
                }
            }
        }

        ++id_count;
    }

    // 然后从高 shell 向低 shell 逐层扩展索引树。
    // 每一层只处理该层新增节点，并根据与 Hk 的接触关系决定挂接方式。
    for (int i = coreMax - 1; i >= 1; --i) {
        const std::unordered_set<int>& Si = shellToNodes[i]; // Si ← {nodes of core Ci}
        if (Si.size() == 0) {
            continue;
        }
        std::unordered_set<int> X = Hk; // X ← Hk
        processShellLayer(i, Si, X, id_count);
        Hk = X;
    }
}

// 得到最大连通分量 -- optimized
void TreeIndex::getMaxCom() {
    std::unordered_set<int> visited; // 存储已访问的节点
    visited.reserve(adj.size());
    std::unordered_map<int, std::vector<int>> components; // 存储每个连通分量的节点

    int l = 0;
    for (const auto& pair : adj) {
        const int tmp = pair.first;
        if (visited.find(tmp) != visited.end()) {
            continue;
        }

        l++;

        // 这里先把每个连通分量的点完整收集起来，后面不再逐点 removeNode，
        // 而是直接用“最大连通分量”整体重建当前图。
        std::queue<int> q;
        q.push(tmp);
        visited.insert(tmp);
        components[l] = std::vector<int>();

        while (!q.empty()) {
            int node = q.front();
            q.pop();

            components[l].push_back(node);

            for (int neighbor : getNeighbors(node)) {
                if (visited.insert(neighbor).second) {
                    q.push(neighbor);
                }
            }
        }
    }

    // 找出最大连通分量。孤立点现在也会作为 size=1 的普通分量参与比较。
    int maxComponentId = 0;
    size_t maxSize = 0;
    for (auto& pair : components) {
        if (pair.second.size() > maxSize) {
            maxSize = pair.second.size();
            maxComponentId = pair.first;
        }
    }

    if (maxComponentId == 0) {
        adj.clear();
        degrees.clear();
        orderedNodes.clear();
        minDegree = 0;
        maxDegree = 0;
        m = 0;
        n = 0;
        N = 0;
        return;
    }

    // keepNodes 表示最终要保留下来的节点集合。
    // 后续只基于它重建 adj，而不是对其他点逐个调用 removeNode。
    const std::vector<int>& largestComponent = components[maxComponentId];
    std::unordered_set<int> keepNodes(largestComponent.begin(), largestComponent.end());

    std::unordered_map<int, std::unordered_set<int>> newAdj;
    newAdj.reserve(keepNodes.size());

    int newNodeCount = 0;
    int newEdgeCount = 0;
    int newMaxNodeId = 0;

    for (int node : largestComponent) {
        auto& newNeighbors = newAdj[node];
        for (int neighbor : getNeighbors(node)) {
            if (keepNodes.find(neighbor) != keepNodes.end()) {
                newNeighbors.insert(neighbor);
                // 无向边只在一个方向计数，避免重复累计。
                if (node < neighbor) {
                    ++newEdgeCount;
                }
            }
        }

        ++newNodeCount;
        if (node > newMaxNodeId) {
            newMaxNodeId = node;
        }
    }

    // 用最大连通分量整体替换当前图，再统一刷新度桶等派生结构。
    adj = std::move(newAdj);
    n = newNodeCount;
    m = newEdgeCount;
    N = newMaxNodeId;
    computeDegrees();

    return;
}

// 打印核心索引和连通分量树 -- To be optimized
void TreeIndex::printTreeIndex() {
    std::cout << "===== 核心索引结构 =====" << std::endl;
    std::cout << "节点总数: " << n << ", 最大核心度: " << coreMax << std::endl;
    int ni = 0;
    // 打印每个核心度的连通分量
    for (int k = coreMax; k >= 1; --k) {
        if (shellToComs.find(k) == shellToComs.end() || shellToComs[k].empty()) {
            continue;
        }

        std::cout << "\n核心度 k = " << k << " 的连通分量:" << std::endl;
        for (int comp_id : shellToComs[k]) {
            ni += comToNodes[comp_id].size();
            std::cout << "  连通分量 ID: " << comp_id
                << ", 节点数: " << comToNodes[comp_id].size()
                << ", 父节点: " << comToParent[comp_id]
                << ", 子节点数: " << comToChildren[comp_id].size() << std::endl;
        }
    }
    // std::cout << "节点总数（统计）: " << ni << std::endl;

    // 打印连通分量树结构
    std::cout << "\n===== 连通分量树结构 =====" << std::endl;
    std::cout << "根节点 (核心度 " << coreMax << "):" << std::endl;

    // 找到所有根节点（没有父节点的连通分量）
    std::queue<int> comp_queue;
    for (int comp_id : shellToComs[coreMax]) {
        if (comToParent[comp_id] == -1) {
            std::cout << "  根连通分量 ID: " << comp_id
                << ", 节点数: " << comToNodes[comp_id].size() << std::endl;
            comp_queue.push(comp_id);
        }
    }

    // BFS遍历树结构
    while (!comp_queue.empty()) {
        int current_comp = comp_queue.front();
        comp_queue.pop();

        // 打印子节点
        for (int child_comp : comToChildren[current_comp]) {
            if (child_comp != -1) {  // 跳过占位符-1
                std::cout << "    子连通分量 ID: " << child_comp
                    << ", 节点数: " << comToNodes[child_comp].size()
                    << ", 父节点: " << current_comp << std::endl;
                comp_queue.push(child_comp);
            }
        }
    }

    // 打印节点到连通分量的映射（可选，大型图可能会输出过多信息）
    if (n < 100) {  // 仅在节点数较少时打印详细映射
        std::cout << "\n===== 节点到连通分量映射 =====" << std::endl;
        for (int i = 0; i < n; ++i) {
            if (nodeToCom[i] != -1) {
                std::cout << "节点 " << i << " -> 连通分量 " << nodeToCom[i]
                    << " (核心度 " << nodeToShell[i] << ")" << std::endl;
            }
        }
    }
    else {
        std::cout << "\n节点数较多，省略详细的节点到连通分量映射。" << std::endl;
    }
}

std::unordered_set<int> TreeIndex::retrievalShellStruct(const query_nodes& queryNodes, int& k) {
    k = 0;
    query_nodes Q = queryNodes;
    for (auto& node : queryNodes) {
        if (nodeToShell[node] > k) {
            k = nodeToShell[node];
        }
    }
    // k ← max{c(u) | u ∈ Q}

    // 注意，虽然是shell索引，但本质上存的还是core，所以使用的时候需要拿到shell的所有子分量
    std::unordered_set<int> H;
    for (auto& node : queryNodes) {
        if (nodeToShell[node] == k) {
            Q.erase(node);
            for (int comp_id : shellToComs[k]) {
                if (comToNodes[comp_id].find(node) != comToNodes[comp_id].end()) {
                    H.insert(comp_id);
                }
            }
        }
    } // H ← {connected components of core Ck containing a vertex from Q}

    std::unordered_set<int> Hx;

    while (H.size() != 1 || Q.size() != 0) {
        k = k - 1;
        query_nodes Q_;
        for (auto& node : Q) {
            if (nodeToShell[node] == k) {
                Q_.insert(node);
            }
        }
        for (auto& node : Q_) {
            Q.erase(node);
        }
        // 对于Q的点很少，有没有必要用一个核心度列表优化

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
                }
            }
        }
        H = Hp;
    }

    if (H.size() == 1) {
        for (auto& id : H) {
            Hx = getNodesFromCom(id);
        }
    }
    return Hx;
}

std::unordered_set<int> TreeIndex::getNodesFromCom(int id) {
    std::unordered_set<int> Hx;
    std::queue<int> ids;
    ids.push(id);
    while (ids.size() != 0) {
        int tmp = ids.front();
        ids.pop();

        if (comToNodes[tmp].size() != 0)
            Hx.insert(comToNodes[tmp].begin(), comToNodes[tmp].end());

        if (comToChildren[tmp].size() != 0) {
            for (auto& ch : comToChildren[tmp]) {
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

        if (comToNodes[tmp].size() != 0)
            Hx.insert(comToNodes[tmp].begin(), comToNodes[tmp].end());

        if (k <= nodeToShell[*comToNodes[tmp].begin()]) {
            continue;
        }

        if (comToChildren[tmp].size() != 0) {
            for (auto& ch : comToChildren[tmp]) {
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
    if (comToNodes[id].size() != 0)
        k = nodeToShell[*comToNodes[id].begin()];

    return Hx;
}

// GrCon算法
std::unordered_set<int> TreeIndex::greedyConnection(const query_nodes& queryNodes, int k, std::unordered_set<int> H) {

    // Step 1: Greedy Step
    std::unordered_set<int> H_min_x = greedyStep(queryNodes, k, H);

    // Step 2: Connection Step
    std::unordered_set<int> ans = connectionStep(H_min_x, queryNodes, k);

    return ans;
}

// GrCon算法第一步 greedystep
std::unordered_set<int> TreeIndex::greedyStep(const query_nodes& queryNodes, int k, std::unordered_set<int>& H) {

    clearAllData();

    // 当前候选子图，基本是 H 的一个本地副本。
    std::unordered_set<int> H_(H.begin(), H.end());
    // 贪心过程中已经选进来的节点集合，也就是“缩小后的核心子图”。
    std::unordered_set<int> H_min_x;
    query_nodes Q = queryNodes;

    // 预分配空间
    // 节点 -> 当前所属连通块 id
    std::unordered_map<int, int> A_;
    A_.reserve(H_.size());
    // 连通块 id -> 这个块里的节点集合
    std::unordered_map<int, std::unordered_set<int>> cc;
    cc.reserve(H_.size());
    std::vector<bool> visited_vec(N + 1, false);
    //p[v] = { p_prime, { something1, something2 } }
    std::unordered_map<int, std::pair<int, std::pair<int, int>>> p;
    p.reserve(H_.size());

    int a_count = 0, b_count = 0, u_x = k;
    MinDegreeHeap u_heap;

    for (auto& node : H_) {
        A_[node] = -1;
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
        visited_vec[u] = false;

        int count = 0;
        for (int v : adj_H[u]) {
            if (H_min_x.count(v)) ++count;
        }
        u_heap.update(u, count);

        // 优化邻居优先级更新：减少重复查找
        for (int v : adj_H[u]) {
                if (!H_min_x.count(v) && !visited_vec[v]) {
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
                if (!visited_vec[w]) continue;

                if (ff && node != u) p[w].first += 1;

                if (uNeighbors.count(w)) {
                    if (u_heap.get_degree(u) < u_x) p[w].second.first += 1;
                    p[w].second.second = std::max(p[w].second.second - 1, 0);
                }

                tmp.insert(w);
            }
        }

        for (auto& w : tmp) {
            updateVertexScore(w, p[w].first, p[w].second.first - p[w].second.second);
        }

    }
    std::unordered_set<int> H_min_xp(H_min_x.begin(), H_min_x.end());
    return H_min_xp;
}

// greedyStep_simply 简化版贪婪算法
std::unordered_set<int> TreeIndex::greedyStep_simply(const query_nodes& queryNodes, int k, std::unordered_set<int>& H) {

    clearAllData();

    std::unordered_set<int> H_min_x;
    query_nodes Q = queryNodes;
    int a_count = 0;
    int u_x = k;
    MinDegreeHeap u_heap; // 最小堆维护最小度

    // 优化：预分配空间
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
            if (!H_min_x.count(v) && !visited_vec[v]) {
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
                if (!visited_vec[w]) continue;

                if (uNeighbors.count(w)) {
                    if (u_heap.get_degree(u) < u_x) {
                        p[w].second.first += 1;
                    }
                    p[w].second.second = std::max(p[w].second.second - 1, 0);
                }

                tmp.insert(w);
            }
        }

        for (auto& w : tmp) {
            updateVertexScore(w, p[w].first, p[w].second.first - p[w].second.second);
        }
    }

    return H_min_x;
}

// GrCon算法第二步 connectionstep
std::unordered_set<int> TreeIndex::connectionStep(std::unordered_set<int>& H, const query_nodes& queryNodes, int k) {

    query_nodes r = steinerTree(H, queryNodes);

    std::unordered_set<int> result = greedyStep_simply(r, k, H);

    return result;

}

// 斯坦纳树的近似算法，prim，其实就是BFS
query_nodes TreeIndex::steinerTree(std::unordered_set<int>& H, const query_nodes& terminals) {
    std::vector<int> mstParent(N, -1); // MST 中的父节点
    std::vector<bool> inMST(N, false); // 标记是否在 MST 中
    std::vector<int> key(N, int_max_alter); // 每个节点的键值（最小边权重）
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

// 输出结果 -- To be optimized
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

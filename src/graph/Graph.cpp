#include "Graph.h"
#include <map>

Graph::Graph(const std::string& path) {
    readFromFile(path);
    computeDegrees();
    statistic();
}

Graph::Graph(std::unordered_set<int>& subVertices, Graph& graph) {
    m = 0;
    n = 0;
    // 添加顶点
    for (const auto& vertex : subVertices) {
        if (graph.adj.find(vertex) != graph.adj.end()) {
            addNode(vertex);
        }
    }

    // 添加边
    for (const auto& vertex : subVertices) {
        if (graph.adj.find(vertex) != graph.adj.end()) {
            std::unordered_set<int> neighbors = graph.adj[vertex];
            for (const auto& neighbor : neighbors) {
                if (subVertices.find(neighbor) != subVertices.end()) {
                    addEdge(vertex, neighbor);
                }
            }
        }
    }

    // 计算每个节点的度
    computeDegrees();
}

// Greedy算法
std::unordered_set<int> Graph::globalsearch(const query_nodes& queryNodes) {
    std::vector<std::unordered_set<int>> list = getOrderedNodes();
    std::unordered_map<int, int> degrees = getDegrees();
    std::unordered_set<int> activeNodes;
    activeNodes.reserve(degrees.size());
    for (const auto& pair : degrees) {
        if (pair.second != -1 && pair.second != 0) {
            activeNodes.insert(pair.first);
        }
    }

    int node;             // 当前处理的节点
    int lowestDegree = 0; // 当前的最低度数
    int neighborDegree;   // 邻居的度数
    // 保存“上一轮仍可行”的结果；先用初始 activeNodes 计算一次，避免第一轮提前退出时为空。
    std::unordered_set<int> reachable = getComponent(queryNodes, activeNodes); // 结果集
    if (!isConnected(queryNodes, degrees)) {
        return {};
    }

    // 主循环
    while (lowestDegree < list.size()) {
        if (list[lowestDegree].empty()) {
            ++lowestDegree; // 如果没有该度数的节点，增加度数
        }
        else {
            bool removedQueryNode = false;
            // 检查是否包含查询节点
            for (auto& pair : list[lowestDegree]) {
                if (queryNodes.count(pair)) {
                    removedQueryNode = true;
                    break;
                }
            }
            if (removedQueryNode) { break; } // 本轮删点会影响查询节点，返回上一轮结果
            node = *list[lowestDegree].begin();
            list[lowestDegree].erase(node);
            degrees[node] = -1;         // 将节点度数设为-1，标记已处理
            activeNodes.erase(node);

            // 更新所有邻居的度数
            for (int neighbor : getNeighbors(node)) {
                neighborDegree = degrees[neighbor];
                if (neighborDegree <= 0) {
                    continue; // 已下线(-1)或度为0的点不再参与桶迁移
                }
                list[neighborDegree].erase(neighbor);
                list[neighborDegree - 1].insert(neighbor);
                degrees[neighbor] = neighborDegree - 1;
                if (degrees[neighbor] == 0) {
                    activeNodes.erase(neighbor);
                }

                if (degrees[neighbor] < lowestDegree) { lowestDegree = degrees[neighbor]; }
            }
        }

        std::unordered_set<int> nextReachable = getComponent(queryNodes, activeNodes);
        bool stillConnected = true;
        for (const auto& q : queryNodes) {
            if (nextReachable.find(q) == nextReachable.end()) {
                stillConnected = false;
                break;
            }
        }
        if (!stillConnected) {
            break; // 本轮不连通，保留上一轮 reachable
        }

        // 仅保留查询集所在连通分量，提前剔除无关分量节点，减少后续删点轮次。
        for (auto& pair : degrees) {
            const int v = pair.first;
            int& degree = pair.second;
            if (degree == -1 || degree == 0) {
                continue;
            }
            if (nextReachable.find(v) != nextReachable.end()) {
                continue;
            }
            if (degree >= 0 && static_cast<size_t>(degree) < list.size()) {
                list[degree].erase(v);
            }
            degree = -1;
            activeNodes.erase(v);
        }

        reachable = std::move(nextReachable);
    }

    if (reachable.empty()) {
        return {};
    }

    return reachable;
}

// K-GS: 先按多源BFS扩展到K规模，再做global search -- To be optimized
std::unordered_set<int> Graph::kGlobalsearch(const query_nodes& queryNodes, int kCount) {
    if (queryNodes.empty()) {
        return {};
    }
    if (kCount <= 0) {
        std::cerr << "Error: k_count must be positive, got " << kCount << std::endl;
        return {};
    }

    // 多源 BFS，按与查询集最短距离扩展候选子图。
    std::unordered_map<int, int> dist;
    std::queue<int> q;
    for (int v : queryNodes) {
        if (adj.find(v) != adj.end() && dist.find(v) == dist.end()) {
            dist[v] = 0;
            q.push(v);
        }
    }
    if (dist.empty()) {
        return {};
    }

    while (!q.empty()) {
        const int u = q.front();
        q.pop();
        for (int v : getNeighbors(u)) {
            if (dist.find(v) == dist.end()) {
                dist[v] = dist[u] + 1;
                q.push(v);
            }
        }
    }

    // 按距离分层，存储每个距离的节点
    // map 会按 key 升序有序存储
    std::map<int, std::vector<int>> levelNodes;
    for (const auto& pair : dist) {
        levelNodes[pair.second].push_back(pair.first);
    }

    // 从距离最小的层开始，逐层扩展，直到满足kCount为止
    std::unordered_set<int> selected;
    for (const auto& level : levelNodes) {
        selected.insert(level.second.begin(), level.second.end());
        if (static_cast<int>(selected.size()) < kCount) {
            continue;
        }
        std::unordered_set<int> selectedCopy = selected;
        Graph subgraph(selectedCopy, *this);
        if (subgraph.isConnected(queryNodes, subgraph.getDegrees())) {
            break;
        }
    }

    if (selected.empty()) {
        return {};
    }
    std::unordered_set<int> selectedCopy = selected;
    Graph subgraph(selectedCopy, *this);
    return subgraph.globalsearch(queryNodes);
}

// 从文件中读取图，每行两个整数 u v
void Graph::readFromFile(const std::string& fileName) {
    std::ifstream file(fileName);
    if (!file.is_open()) {
        std::cerr << "Error: File not found." << std::endl;
        exit(1);
    }

    adj.clear();
    m = 0;
    n = 0;
    N = 0;
    std::string line;
    // 测试数据集的边是否适用无向图
    while (std::getline(file, line)) {
        if (line.empty()) continue; // 跳过空行
        if (line[0] == '#' || line[0] == '%') continue; // 跳过注释行
        int from, to;
        if (sscanf_s(line.c_str(), "%d %d", &from, &to) == 2) {
            if (from == to) {
                continue;
            }

            addNode(from);
            addNode(to);

            addEdge(from, to);
        }
        else {
            std::cout << "Error: Incorrect line format" << std::endl;
        }
    }
    file.close();
}

// 添加节点到图中
void Graph::addNode(int node) {
    if (node < 0) {
        std::cout << "图中存在负值 " << node << std::endl;
    }
    if (adj.find(node) == adj.end()) {
        adj[node] = std::unordered_set<int>();
        n++;
        if (node >= N) {
            N = node;
        }
    }
}

// 添加两个节点之间的边
void Graph::addEdge(int from, int to) {
    // 避免自环
    if (from == to) return;

    // 如果边已存在，则不增加计数
    if (adj[from].find(to) == adj[from].end()) {
        adj[from].insert(to);
        adj[to].insert(from);
        m++;  // 只有第一次插入时计数
    }
}

// 计算degrees，orderedNodes，minDegree，maxDegree
std::unordered_map<int, int> Graph::computeDegrees() {
    degrees.clear();
    orderedNodes.clear();
    orderedNodes.resize(n + 1);

    size_t maxDegree = 0;
    size_t minDegree = static_cast<size_t>(-1); // size_t最大值

    for (const auto& pair : adj) {
        degrees[pair.first] = pair.second.size();
        orderedNodes[pair.second.size()].insert(pair.first);
        if (pair.second.size() > maxDegree) {
            maxDegree = pair.second.size();
        }
        if (pair.second.size() < minDegree) {
            minDegree = pair.second.size();
        }
    }
    // 若成员变量需要赋值int类型
    this->maxDegree = static_cast<int>(maxDegree);
    this->minDegree = static_cast<int>(minDegree);
    return degrees;
}

// 打印图的相关信息
void Graph::statistic() {
    std::cout << "初始图的信息：" << std::endl;

    // 边数
    std::cout << "边数为 " << m << std::endl;
    // 节点数
    std::cout << "节点数为 " << n << std::endl;
    // 最大值
    std::cout << "点的最大值为 " << N << std::endl;
    // 最大度数
    std::cout << "图的最大度数为 " << maxDegree << std::endl;
    // 最小度数
    std::cout << "图的最小度数为 " << minDegree << std::endl;
}

// 检查查询集是否连通，由于时间代价高昂，仅适用于单个子图的检查
bool Graph::isConnected(const query_nodes& queryNodes, const std::unordered_map<int, int>& degree) const {
    if (queryNodes.empty() || degree.empty()) return false; // 如果查询节点为空，直接返回空

    std::unordered_set<int> visited; // 存储已访问的节点
    std::queue<int> q;
    for (int startNode : queryNodes) {
        if (degree.find(startNode) != degree.end() && degree.at(startNode) > 0) {
            q.push(startNode);
            visited.insert(startNode);
            break;
        }
    }

    if (q.empty()) {
        return false; // 如果没有有效的起始节点，则直接返回不连通
    }

    while (!q.empty()) {
        int node = q.front();
        q.pop();
        for (int neighbor : getNeighbors(node)) {
            if (degree.find(neighbor) != degree.end() && visited.insert(neighbor).second && degree.at(neighbor) > 0) {
                q.push(neighbor);
            }
        }
    }

    // 检查所有查询节点是否都在访问集合中
    for (int node : queryNodes) {
        if (visited.find(node) == visited.end()) {
            return false;
        }
    }

    return true;
}

// 计算子社区中的最小度
int Graph::getMinDegree(std::unordered_set<int>& nodes) const {
    if (nodes.empty()) return 0;
    int minDegree = std::numeric_limits<int>::max();
    for (const auto& node : nodes) {
        int degree = 0;
        for (const auto& neighbor : getNeighbors(node)) {
            if (nodes.find(neighbor) != nodes.end()) { degree++; }
        }
        if (degree < minDegree) { minDegree = degree; }
    }
    return minDegree;
}

// 确保结果联通
std::unordered_set<int> Graph::getComponent(const query_nodes& queryNodes, const std::unordered_set<int>& candidateNodes) {
    if (queryNodes.empty() || candidateNodes.empty()) {
        return {}; // 如果输入集合为空，直接返回空结果
    }

    std::unordered_set<int> reachable;
    std::queue<int> q;

    // 初始化队列
    for (const auto& node : queryNodes) {
        q.push(node);
        reachable.insert(node); // 避免起始节点重复访问
    }

    while (!q.empty()) {
        int node = q.front();
        q.pop();

        for (const auto& neighbor : getNeighbors(node)) {
            if (candidateNodes.find(neighbor) != candidateNodes.end() && reachable.insert(neighbor).second) {
                q.push(neighbor);
            }
        }
    }

    return reachable;
}

// 移除节点，更新图的相关信息
void Graph::removeNode(int node) {
    // 检查节点是否存在
    if (adj.find(node) == adj.end()) {
        return; // 节点不存在，直接返回
    }

    size_t degree = degrees[node];

    // 从 orderedNodes 中移除该节点
    if (degree < orderedNodes.size()) {
        orderedNodes[degree].erase(node);
    }

    // 更新最小度和最大度
    size_t minDegree = static_cast<size_t>(this->minDegree);
    size_t maxDegree = static_cast<size_t>(this->maxDegree);
    if (degree == minDegree && orderedNodes[degree].empty()) {
        // 增加最小度直到找到非空集合
        while (minDegree < orderedNodes.size() && orderedNodes[minDegree].empty()) {
            minDegree++;
        }
        if (minDegree >= orderedNodes.size()) {
            minDegree = 0; // 图为空
        }
    }

    if (degree == maxDegree && orderedNodes[degree].empty()) {
        // 减少最大度直到找到非空集合
        while (maxDegree > 0 && (maxDegree >= orderedNodes.size() || orderedNodes[maxDegree].empty())) {
            maxDegree--;
        }
    }

    // 更新边数（减去该节点的所有边）
    m -= static_cast<int>(degree);

    // 从每个邻居的邻接列表中移除该节点，并更新它们的度
    for (int neighbor : adj[node]) {
        if (adj.find(neighbor) != adj.end()) {
            adj[neighbor].erase(node);

            size_t neighborDegree = degrees[neighbor];
            orderedNodes[neighborDegree].erase(neighbor);
            neighborDegree--;

            // 扩展 orderedNodes 向量如果需要
            if (neighborDegree >= orderedNodes.size()) {
                orderedNodes.resize(neighborDegree + 1);
            }

            orderedNodes[neighborDegree].insert(neighbor);
            degrees[neighbor] = static_cast<int>(neighborDegree);

            // 更新最小度和最大度
            if (neighborDegree < minDegree) {
                minDegree = neighborDegree;
            }
            if (neighborDegree > maxDegree) {
                maxDegree = neighborDegree;
            }
        }
    }

    // 从邻接列表和度信息中移除该节点
    adj.erase(node);
    degrees.erase(node);

    // 更新节点数
    n--;

    // 更新 N (图中所有点的最大值)
    if (node == N) {
        // 如果移除的是最大节点，需要找到新的最大值
        N = 0;
        for (const auto& pair : adj) {
            if (pair.first > N) {
                N = pair.first;
            }
        }
    }

    // 如果图为空，重置最小度和最大度
    if (n == 0) {
        minDegree = 0;
        maxDegree = 0;
        N = 0;
    }
    this->minDegree = static_cast<int>(minDegree);
    this->maxDegree = static_cast<int>(maxDegree);
    return;
}

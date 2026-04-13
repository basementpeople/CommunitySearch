#include "Graph.h"

Graph::Graph(const std::string path) {
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
Graph Graph::globalsearch(query_nodes& queryNodes) {
    clock_t start = clock(); // 记录开始时间
    std::vector<std::unordered_set<int>> list = getOrderedNodes();
    std::unordered_map<int, int> degrees = getDegrees();

    // bool flag = false;
    bool flag = true;
    int node;             // 当前处理的节点
    int lowestDegree = 0; // 当前的最低度数
    int neighborDegree;   // 邻居的度数
    std::unordered_set<int> result_end; // 结果集
    int k = 0;

    // 主循环
    while (!list.empty()) {
        if (list[lowestDegree].empty()) {
            ++lowestDegree; // 如果没有该度数的节点，增加度数
        }
        else {
            // 检查是否包含查询节点
            for (auto& pair : list[lowestDegree]) {
                if (queryNodes.count(pair)) {
                    std::cout << "1:Found query node!" << pair << std::endl;
                    flag = false;
                    break;
                }
            }
            if (!flag) { break; } // 包含查询节点，结束搜索
            node = *list[lowestDegree].begin();
            list[lowestDegree].erase(node);
            degrees[node] = -1;         // 将节点度数设为-1，标记已处理

            // 检查查询顶点集是否连通
            flag = isConnected(queryNodes, degrees);
            if (!flag) {
                std::cout << "2:Query vertex set not connected！" << std::endl;
                break;
            }

            // 更新所有邻居的度数
            for (int neighbor : getNeighbors(node)) {
                neighborDegree = degrees[neighbor];
                list[neighborDegree].erase(neighbor);
                list[neighborDegree - 1].insert(neighbor);
                degrees[neighbor] = neighborDegree - 1;

                if (degrees[neighbor] < lowestDegree) { lowestDegree = degrees[neighbor]; }
            }
        }

        std::unordered_set<int> result;
        for (const auto& pair : degrees) {
            if (pair.second != -1 && pair.second != 0) {
                result.insert(pair.first);  // 获取键
            }
        }
        result = getComponent(queryNodes, result);

        // int minDegree = INT32_MAX;
        int minDegree = 1000;
        for (auto& node : result) {
            int degree = 0;
            for (auto& neighbor : getNeighbors(node)) {
                if (result.find(neighbor) != result.end()) { degree++; }
            }
            if (degree < minDegree) { minDegree = degree; }
        }
        if (minDegree > k) {
            k = minDegree;
            result_end = result;
        }
    }

    flag = isConnected(queryNodes, degrees);
    if (!flag) {
        std::cout << "查询顶点集不连通！" << std::endl;
        return {};
    }

    Graph ans(result_end, *this);
    std::cout << "得到结果为：" << k << std::endl;
    clock_t end = clock(); // 记录结束时间
    std::cout << "花费了" << (double)(end - start) / CLOCKS_PER_SEC << "秒" << std::endl;
    return ans;
}

// 辅助函数
void Graph::readFromFile(const std::string fileName) {
    std::ifstream file(fileName);
    if (!file.is_open())
    {
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
            if (from == to)
            {
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

void Graph::addNode(int node) {
    if (node < 0) {
        std::cout << "负值 " << node << std::endl;
    }
    if (adj.find(node) == adj.end()) {
        adj[node] = std::unordered_set<int>();
        n++;
        if (node >= N) {
            N = node;
        }
    }
}

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

// 计算degrees，orderedNodes，minimumDegree，Dmax
std::unordered_map<int, int> Graph::computeDegrees() {
    degrees.clear();
    orderedNodes.clear();
    orderedNodes.resize(n + 1);

    size_t Dmax = 0;
    size_t minimumDegree = static_cast<size_t>(-1); // size_t最大值

    for (auto& pair : adj) {
        degrees[pair.first] = pair.second.size();
        orderedNodes[pair.second.size()].insert(pair.first);
        if (pair.second.size() > Dmax) {
            Dmax = pair.second.size();
        }
        if (pair.second.size() < minimumDegree) {
            minimumDegree = pair.second.size();
        }
    }
    // 若成员变量需要赋值int类型
    this->Dmax = static_cast<int>(Dmax);
    this->minimumDegree = static_cast<int>(minimumDegree);
    return degrees;
}

// 打印图的相关信息
void Graph::statistic() {
    std::cout << "初始图的信息：" << std::endl;
    // 节点数
    std::cout << "节点数为 " << n << std::endl;
    // 最大值
    std::cout << "点的最大值为 " << N << std::endl;
    // 边数
    std::cout << "边数为 " << m << std::endl;
    // 最大度数
    std::cout << "图的最大度数为 " << Dmax << std::endl;
    // 最小度数
    std::cout << "图的最小度数为 " << minimumDegree << std::endl;
}

// 检查查询集是否连通，由于时间代价高昂，仅适用于单个子图的检查
bool Graph::isConnected(query_nodes queryNodes, std::unordered_map<int, int> degree) {
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

// 计算子社区中的最小度，比较常用，但打算更新子社区的数据结构
int Graph::getminimumDegree(std::unordered_set<int>& nodes) {
    if (nodes.size() == 0) return 0;
    int minDegree = INT32_MAX;
    for (auto& node : nodes) {
        int degree = 0;
        for (auto& neighbor : getNeighbors(node)) {
            if (nodes.find(neighbor) != nodes.end())
                degree++;
        }
        if (degree < minDegree)
            minDegree = degree;
    }
    return minDegree;
}

// 确保结果联通
std::unordered_set<int> Graph::getComponent(query_nodes& queryNodes, std::unordered_set<int>& result_end) {
    if (queryNodes.empty() || result_end.empty()) {
        return {}; // 如果输入集合为空，直接返回空结果
    }

    std::unordered_set<int> result;
    std::queue<int> q;

    // 初始化队列
    for (auto& node : queryNodes) {
        q.push(node);
        result.insert(node); // 避免起始节点重复访问
    }

    while (!q.empty()) {
        int node = q.front();
        q.pop();

        for (auto& neighbor : getNeighbors(node)) {
            if (result.find(neighbor) != result.end()) {
                continue; // 如果已经访问过，跳过
            }
            if (result_end.find(neighbor) != result_end.end()) {
                q.push(neighbor);
                result.insert(neighbor);
            }
        }
    }

    return result;
}

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
    size_t minimumDegree = static_cast<size_t>(this->minimumDegree);
    size_t Dmax = static_cast<size_t>(this->Dmax);
    if (degree == minimumDegree && orderedNodes[degree].empty()) {
        // 增加最小度直到找到非空集合
        while (minimumDegree < orderedNodes.size() && orderedNodes[minimumDegree].empty()) {
            minimumDegree++;
        }
        if (minimumDegree >= orderedNodes.size()) {
            minimumDegree = 0; // 图为空
        }
    }

    if (degree == Dmax && orderedNodes[degree].empty()) {
        // 减少最大度直到找到非空集合
        while (Dmax > 0 && (Dmax >= orderedNodes.size() || orderedNodes[Dmax].empty())) {
            Dmax--;
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
            if (neighborDegree < minimumDegree) {
                minimumDegree = neighborDegree;
            }
            if (neighborDegree > Dmax) {
                Dmax = neighborDegree;
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
        minimumDegree = 0;
        Dmax = 0;
        N = 0;
    }
    this->minimumDegree = static_cast<int>(minimumDegree);
    this->Dmax = static_cast<int>(Dmax);
    return;
}

#include "DijkstraSimulator.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <numbers>
#include <sstream>
#include <unordered_map>
#include <functional>

void DijkstraSimulator::setGraphType(GraphType type) {
    if (type == GraphType::Undirected) {
        // Normalize template identities before adding a reverse traversal for
        // each directed physical edge. Existing undirected pairs stay intact.
        reset();
        std::vector<std::pair<int, Edge>> companions;
        for (auto &node : graph) for (auto &edge : node.edges) if (edge.directed) {
            companions.push_back({edge.target, {node.id, edge.weight, false, edge.id}});
            edge.directed = false;
        }
        for (const auto &[vertex, edge] : companions) graph[vertex].edges.push_back(edge);
    } else if (type == GraphType::Directed) {
        for (auto &node : graph) {
            // Existing directed arcs retain their direction and weight.
            // An undirected pair acquires one deterministic direction.
            std::erase_if(node.edges, [&](const Edge &edge) {
                return !edge.directed && node.id > edge.target;
            });
            for (auto &edge : node.edges) edge.directed = true;
        }
    }
    graphType = type;
    reset();
}

bool DijkstraSimulator::importText(const std::string &text, std::string &error) {
    // Build separately so any malformed file leaves the current graph intact.
    DijkstraSimulator imported;
    imported.graphType = graphType;
    std::unordered_map<std::string, int> ids;
    std::istringstream lines(text);
    std::string line;
    int lineNumber = 0;
    bool firstContentLine = true;
    Layout importedLayout = Layout::Grid;
    error.clear();
    while (std::getline(lines, line)) {
        ++lineNumber;
        std::istringstream fields(line);
        std::vector<std::string> tokens;
        std::string token;
        while (fields >> token) tokens.push_back(token);
        if (tokens.empty()) continue;
        if (!tokens[0].empty() && tokens[0][0] == '#') continue;
        if (firstContentLine) {
            firstContentLine = false;
            if (tokens.size() == 1) {
                if (tokens[0] == "Grip" || tokens[0] == "Grid") { importedLayout = Layout::Grid; continue; }
                if (tokens[0] == "Circle") { importedLayout = Layout::Circle; continue; }
                if (tokens[0] == "Freedom") { importedLayout = Layout::Random; continue; }
                if (tokens[0] == "Square") { importedLayout = Layout::Square; continue; }
                // An unrecognized first line is treated as a normal edge and
                // receives the default Grid layout.
            }
        }
        auto fail = [&](const std::string &reason) {
            error = "Dòng " + std::to_string(lineNumber) + ": " + reason;
            return false;
        };
        if (tokens.size() != 3 && tokens.size() != 4)
            return fail("cần định dạng: a b 10, a -> b 10 hoặc a b 10 ->.");
        std::string a=tokens[0], b=tokens[1], weightText=tokens[2];
        bool directed = false;
        if (tokens.size() == 4) {
            std::string direction=tokens[3];
            if (tokens[1]=="->" || tokens[1]=="--") {
                direction=tokens[1]; b=tokens[2]; weightText=tokens[3];
            }
            if (direction!="->" && direction!="--") return fail("loại cạnh phải là -> hoặc --.");
            directed=direction=="->";
        }
        int weight;
        try {
            size_t used = 0;
            weight = std::stoi(weightText, &used);
            if (used != weightText.size() || weight < 0 || weight > MaxEdgeWeight)
                return fail("trọng số phải là số nguyên từ 0 đến 9999.");
        } catch (...) { return fail("trọng số phải là số nguyên từ 0 đến 9999."); }
        if (a == b) return fail("hai đầu cạnh phải là hai đỉnh khác nhau.");
        for (const auto &name : {a, b}) {
            if (ids.contains(name)) continue;
            if (ids.size() >= MaxRandomNodes)
                return fail("Đã quá số đỉnh cho phép (tối đa 30 đỉnh).");
            const int id = static_cast<int>(ids.size());
            ids[name] = id;
            imported.graph.push_back({id, name, 50 + (id % 6) * 120, 50 + (id / 6) * 120, {}});
        }
        imported.setEdge(ids[a], ids[b], weight, directed);
    }
    if (imported.graph.empty()) { error = "File TXT không chứa cạnh nào."; return false; }
    graph = std::move(imported.graph);
    // Apply the requested import layout without changing the graph structure.
    const int count = static_cast<int>(graph.size());
    const int columns = std::max(1, static_cast<int>(std::ceil(std::sqrt(count))));
    std::mt19937 layoutRng(17);
    for (int i = 0; i < count; ++i) {
        int x = 50 + (i % columns) * 120;
        int y = 50 + (i / columns) * 120;
        if (importedLayout == Layout::Circle) {
            const double angle = 2 * std::numbers::pi * i / std::max(1, count) - std::numbers::pi / 2;
            x = static_cast<int>(300 + 250 * std::cos(angle));
            y = static_cast<int>(300 + 250 * std::sin(angle));
        } else if (importedLayout == Layout::Square) {
            const double position = 4.0 * i / std::max(1, count);
            const int side = static_cast<int>(position);
            const int offset = static_cast<int>((position - side) * 500);
            if (side == 0) { x = 50 + offset; y = 50; }
            else if (side == 1) { x = 550; y = 50 + offset; }
            else if (side == 2) { x = 550 - offset; y = 550; }
            else { x = 50; y = 550 - offset; }
        } else if (importedLayout == Layout::Random) {
            x += std::uniform_int_distribution<int>(-25, 25)(layoutRng);
            y += std::uniform_int_distribution<int>(-25, 25)(layoutRng);
        }
        graph[i].x = x;
        graph[i].y = y;
    }
    bool hasDirected = false, hasUndirected = false;
    for (const auto &node : graph) for (const auto &edge : node.edges) {
        hasDirected |= edge.directed;
        hasUndirected |= !edge.directed;
    }
    if (graphType != GraphType::Mixed)
        graphType = hasDirected ? (hasUndirected ? GraphType::Mixed : GraphType::Directed) : GraphType::Undirected;
    startNode = 0;
    targetNode = -1;
    reset();
    return true;
}

void DijkstraSimulator::runToEnd() {
    while (!isFinished) step();
}

static std::string letterName(int number) {
    std::string name;
    for (++number; number > 0; number = (number - 1) / 26)
        name.insert(name.begin(), static_cast<char>('A' + (number - 1) % 26));
    return name;
}

bool DijkstraSimulator::renameNode(int id, const std::string &name) {
    if (id < 0 || id >= static_cast<int>(graph.size()) || name.empty()) return false;
    if (std::any_of(graph.begin(), graph.end(), [&](const Node &n) { return n.id != id && n.name == name; })) return false;
    graph[id].name = name;
    reset();
    return true;
}

bool DijkstraSimulator::addNode(int x, int y) {
    if (graph.size() >= MaxRandomNodes) return false;
    const int id = static_cast<int>(graph.size());
    std::string name;
    for (int number = 0; ; ++number) {
        name = letterName(number);
        if (std::none_of(graph.begin(), graph.end(), [&](const Node &n) { return n.name == name; })) break;
    }
    graph.push_back({id, name, x, y, {}});
    if (id == 0) {
        startNode = 0;
        targetNode = -1;
    }
    reset();
    return true;
}

bool DijkstraSimulator::removeNode(int id) {
    if (id < 0 || id >= static_cast<int>(graph.size())) return false;
    graph.erase(graph.begin() + id);
    for (int i = 0; i < static_cast<int>(graph.size()); ++i) {
        graph[i].id = i;
        std::erase_if(graph[i].edges, [id](const Edge &e) { return e.target == id; });
        for (auto &e : graph[i].edges) if (e.target > id) --e.target;
    }
    auto remap = [id, this](int value) {
        if (graph.empty()) return -1;
        return value == id ? 0 : value > id ? value - 1 : value;
    };
    startNode = remap(startNode);
    targetNode = targetNode == id ? -1 : remap(targetNode);
    reset();
    return true;
}

bool DijkstraSimulator::addEdge(int from, int to, int weight, bool directed) {
    const int count = static_cast<int>(graph.size());
    if (from < 0 || to < 0 || from >= count || to >= count || from == to ||
        weight < 0 || weight > MaxEdgeWeight) return false;
    const int id = nextEdgeId++;
    graph[from].edges.push_back({to, weight, directed, id});
    if (!directed) graph[to].edges.push_back({from, weight, false, id});
    reset();
    return true;
}

bool DijkstraSimulator::setEdge(int from, int to, int weight, bool directed) {
    const int count = static_cast<int>(graph.size());
    if (from < 0 || to < 0 || from >= count || to >= count || from == to ||
        weight < 0 || weight > MaxEdgeWeight) return false;
    // An undirected edge is stored as two matching outgoing arcs. Directed
    // arcs remain independent, including two opposite arcs with different weights.
    auto old = std::find_if(graph[from].edges.begin(), graph[from].edges.end(),
                            [to](const Edge &e) { return e.target == to; });
    if (directed && old != graph[from].edges.end() && !old->directed)
        std::erase_if(graph[to].edges, [from](const Edge &e) { return e.target == from; });
    const int id = old != graph[from].edges.end() ? old->id : nextEdgeId++;
    auto upsert = [&](int a, int b) {
        auto &edges = graph[a].edges;
        auto found = std::find_if(edges.begin(), edges.end(), [b](const Edge &e) { return e.target == b; });
        if (found == edges.end()) edges.push_back({b, weight, directed, id});
        else { found->weight = weight; found->directed = directed; found->id = id; }
    };
    upsert(from, to);
    if (!directed) upsert(to, from);
    reset();
    return true;
}

bool DijkstraSimulator::removeEdge(int from, int to) {
    const int count = static_cast<int>(graph.size());
    if (from < 0 || to < 0 || from >= count || to >= count || from == to) return false;
    const auto &edges = graph[from].edges;
    const auto found = std::find_if(edges.begin(), edges.end(), [to](const Edge &e) { return e.target == to; });
    if (found == edges.end()) return false;
    return removeEdgeById(found->id);
}

bool DijkstraSimulator::removeEdgeById(int edgeId) {
    if (edgeId < 0) return false;
    bool removed = false;
    for (auto &node : graph)
        removed = std::erase_if(node.edges, [edgeId](const Edge &edge) { return edge.id == edgeId; }) > 0 || removed;
    if (!removed) return false;
    reset();
    return true;
}

bool DijkstraSimulator::editEdge(int from, int edgeId, int weight, bool directed) {
    if (from < 0 || from >= static_cast<int>(graph.size()) || weight < 0 || weight > MaxEdgeWeight) return false;
    auto &edges = graph[from].edges;
    auto edge = std::find_if(edges.begin(), edges.end(), [edgeId](const Edge &e) { return e.id == edgeId; });
    if (edge == edges.end()) return false;
    const int to = edge->target;
    edge->weight = weight;
    edge->directed = directed;
    auto &reverse = graph[to].edges;
    if (directed) {
        std::erase_if(reverse, [edgeId](const Edge &e) { return e.id == edgeId; });
    } else {
        auto companion = std::find_if(reverse.begin(), reverse.end(), [edgeId](const Edge &e) { return e.id == edgeId; });
        if (companion == reverse.end()) reverse.push_back({from, weight, false, edgeId});
        else { companion->weight = weight; companion->directed = false; }
    }
    reset();
    return true;
}

bool DijkstraSimulator::generateRandomGraph(int nodeCount, Layout layout, int density,
                                           int maxWeight, std::uint32_t seed) {
    if (nodeCount < 2 || nodeCount > MaxRandomNodes || density < 0 || density > 100 ||
        maxWeight < 1 || maxWeight > MaxEdgeWeight) return false;
    if (layout != Layout::Circle && layout != Layout::Square &&
        layout != Layout::Grid && layout != Layout::Random) return false;
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> weight(1, maxWeight);
    graph.clear();
    startNode = 0;
    targetNode = nodeCount - 1;
    const int columns = static_cast<int>(std::ceil(std::sqrt(nodeCount)));
    for (int i = 0; i < nodeCount; ++i) {
        int x = 0, y = 0;
        switch (layout) {
        case Layout::Circle: {
            const double angle = 2 * std::numbers::pi * i / nodeCount - std::numbers::pi / 2;
            x = static_cast<int>(300 + 250 * std::cos(angle));
            y = static_cast<int>(300 + 250 * std::sin(angle));
            break;
        }
        case Layout::Square: {
            // Equally spaced vertices around the four sides of a square.
            const double position = 4.0 * i / nodeCount;
            const int side = static_cast<int>(position);
            const int offset = static_cast<int>((position - side) * 500);
            if (side == 0) { x = 50 + offset; y = 50; }
            else if (side == 1) { x = 550; y = 50 + offset; }
            else if (side == 2) { x = 550 - offset; y = 550; }
            else { x = 50; y = 550 - offset; }
            break;
        }
        case Layout::Grid:
            x = 50 + (i % columns) * 120;
            y = 50 + (i / columns) * 120;
            break;
        case Layout::Random:
            // Jittered grid prevents coincident vertices in the free layout.
            x = 50 + (i % columns) * 120 + std::uniform_int_distribution<int>(-25, 25)(rng);
            y = 50 + (i / columns) * 120 + std::uniform_int_distribution<int>(-25, 25)(rng);
            break;
        }
        const std::string name = letterName(i);
        graph.push_back({i, name, x, y, {}});
    }
    auto connect = [&](int a, int b) {
        const int w = weight(rng);
        graph[a].edges.push_back({b, w, newEdgesDirected()});
        if (!newEdgesDirected()) graph[b].edges.push_back({a, w, false});
    };
    std::vector<int> order(nodeCount);
    std::iota(order.begin(), order.end(), 0);
    std::shuffle(order.begin(), order.end(), rng);
    if (newEdgesDirected())
        std::iter_swap(order.begin(), std::find(order.begin(),order.end(),startNode));
    for (int i = 1; i < nodeCount; ++i) {
        const int parent=order[std::uniform_int_distribution<int>(0,i-1)(rng)];
        // For directed graphs the spanning tree leads outwards from the source,
        // so all generated vertices are reachable even at zero extra density.
        if (newEdgesDirected()) connect(parent,order[i]);
        else connect(order[i],parent);
    }
    std::bernoulli_distribution extraEdge(density / 100.0);
    for (int a = 0; a < nodeCount; ++a) for (int b = a + 1; b < nodeCount; ++b) {
        const auto &edges = graph[a].edges;
        if (std::none_of(edges.begin(), edges.end(), [b](const Edge &e) { return e.target == b; }) && extraEdge(rng))
            connect(a, b);
    }
    reset();
    return true;
}

void DijkstraSimulator::loadTemplate(int index) {
    graph.clear();

    if (index == 0) {
        startNode = 0; targetNode = 5;
        graph.push_back({0, "A", 100, 250, {{1, 4}, {2, 2}}});
        graph.push_back({1, "B", 250, 100, {{0, 4}, {3, 5}, {2, 1}}});
        graph.push_back({2, "C", 250, 400, {{0, 2}, {1, 1}, {4, 10}}});
        graph.push_back({3, "D", 450, 100, {{1, 5}, {4, 2}, {5, 6}}});
        graph.push_back({4, "E", 450, 400, {{2, 10}, {3, 2}, {5, 3}}});
        graph.push_back({5, "F", 600, 250, {{3, 6}, {4, 3}}});
    } else if (index == 1) {
        startNode = 0; targetNode = 4;
        graph.push_back({0, "S", 100, 250, {{1, 3}}});
        graph.push_back({1, "1", 220, 250, {{0, 3}, {2, 2}}});
        graph.push_back({2, "2", 340, 250, {{1, 2}, {3, 4}}});
        graph.push_back({3, "3", 460, 250, {{2, 4}, {4, 1}}});
        graph.push_back({4, "T", 580, 250, {{3, 1}}});
    } else if (index == 2) {
        startNode = 0; targetNode = 4;
        graph.push_back({0, "S", 100, 250, {{1, 2}, {2, 5}, {3, 8}}});
        graph.push_back({1, "1", 350, 100, {{0, 2}, {4, 9}}});
        graph.push_back({2, "C", 350, 250, {{0, 5}, {4, 3}}});
        graph.push_back({3, "3", 350, 400, {{0, 8}, {4, 1}}});
        graph.push_back({4, "T", 600, 250, {{1, 9}, {2, 3}, {3, 1}}});
    } else if (index == 3) {
        startNode = 0; targetNode = 3;
        graph.push_back({0, "S", 150, 250, {{1, 10}, {2, 2}}});
        graph.push_back({1, "A", 350, 150, {{0, 10}, {2, 3}, {3, 1}}});
        graph.push_back({2, "B", 350, 350, {{0, 2}, {1, 3}}});
        graph.push_back({3, "T", 550, 250, {{1, 1}}});
    } else if (index == 4) {
        startNode = 0; targetNode = 6;
        graph.push_back({0, "S", 100, 250, {{1, 1}, {2, 4}}});
        graph.push_back({1, "A", 250, 150, {{0, 1}, {3, 2}, {4, 7}}});
        graph.push_back({2, "B", 250, 350, {{0, 4}, {4, 3}}});
        graph.push_back({3, "C", 400, 150, {{1, 2}, {5, 1}}});
        graph.push_back({4, "D", 400, 350, {{1, 7}, {2, 3}, {5, 2}}});
        graph.push_back({5, "E", 550, 250, {{3, 1}, {4, 2}, {6, 5}}});
        graph.push_back({6, "T", 700, 250, {{5, 5}}});
    }

    for (auto &node : graph) node.name = letterName(node.id);
    setGraphType(graphType);
}

void DijkstraSimulator::reset() {
    const int V = static_cast<int>(graph.size());
    // The two traversals of an undirected edge share one identity. Parallel
    // edges retain separate identities for editing and shortest-path display.
    for (const auto &node : graph) for (const auto &edge : node.edges)
        nextEdgeId = std::max(nextEdgeId, edge.id + 1);
    for (auto &node : graph) for (auto &edge : node.edges) if (edge.id < 0) {
        edge.id = nextEdgeId++;
        if (!edge.directed) for (auto &reverse : graph[edge.target].edges)
            if (reverse.target == node.id && !reverse.directed && reverse.id < 0) { reverse.id = edge.id; break; }
    }

    dist.assign(V, std::numeric_limits<int>::max());
    prev.assign(V, -1);
    prevEdgeIds.assign(V, -1);
    visited.assign(V, false);
    pqState.clear();

    currentNode = -1;
    currentEdgeIndex = -1;
    currentExaminedTarget = -1;
    currentCodeLine = 2;
    isFinished = false;

    shortestPathEdges.clear();
    history.clear();
    if (graph.empty()) {
        startNode = targetNode = -1;
        isFinished = true;
        customExplanation = "Đồ thị trống. Bấm chuột phải vào vùng trắng để thêm đỉnh.";
        return;
    }
    if (startNode < 0 || startNode >= V) startNode = 0;
    if (targetNode < -1 || targetNode >= V) targetNode = -1;

    customExplanation =
        "Khởi tạo: khoảng cách đến " + graph[startNode].name +
        " bằng 0. Các đỉnh còn lại có khoảng cách vô cùng.";

    dist[startNode] = 0;
}

void DijkstraSimulator::reconstructShortestPath() {
    shortestPathEdges.clear();

    if (targetNode < 0) {
        for (int v = 0; v < static_cast<int>(graph.size()); ++v)
            if (prev[v] >= 0) shortestPathEdges.push_back({prev[v], v});
        return;
    }

    if (dist[targetNode] == std::numeric_limits<int>::max())
        return;

    int curr = targetNode;
    while (curr != -1 && prev[curr] != -1) {
        const int p = prev[curr];
        shortestPathEdges.push_back({p, curr});
        curr = p;
    }
}

std::vector<int> DijkstraSimulator::pathTo(int vertex) const {
    const int count=static_cast<int>(graph.size());
    if(vertex<0 || vertex>=count || dist.size()!=graph.size() || prev.size()!=graph.size() ||
       dist[vertex]==std::numeric_limits<int>::max()) return {};
    std::vector<int> path;
    for(int v=vertex;v>=0 && v<count && path.size()<graph.size();v=prev[v]) {
        path.push_back(v);
        if(v==startNode) {std::reverse(path.begin(),path.end());return path;}
    }
    return {};
}

void DijkstraSimulator::step() {
    if (isFinished)
        return;

    history.push_back({
        dist, prev, visited, pqState,
        currentNode, currentEdgeIndex, currentExaminedTarget,
        currentCodeLine, isFinished, customExplanation, shortestPathEdges, prevEdgeIds
    });

    switch (currentCodeLine) {
    case 2:
        pqState.push_back({0, startNode});
        std::push_heap(pqState.begin(), pqState.end(), std::greater<PQElement>());
        currentCodeLine = 3;
        customExplanation = "Đưa đỉnh xuất phát vào hàng đợi ưu tiên.";
        break;
    case 3:
        currentCodeLine = 4;
        customExplanation = "Kiểm tra hàng đợi ưu tiên.";
        break;
    case 4:
        if (pqState.empty()) {
            currentCodeLine = 12; isFinished = true; reconstructShortestPath();
            if(targetNode<0) {
                const int reachable=static_cast<int>(std::count(visited.begin(),visited.end(),true));
                customExplanation="Đã tính toàn bộ " + std::to_string(graph.size()) + " đỉnh: " +
                    std::to_string(reachable) + " tới được, " + std::to_string(graph.size()-reachable) +
                    " không có đường từ nguồn theo chiều cạnh (∞).";
            } else customExplanation = "Hàng đợi đã trống. Kết thúc tìm kiếm.";
        } else {
            std::pop_heap(pqState.begin(), pqState.end(), std::greater<PQElement>());
            currentNode = pqState.back().vertex; pqState.pop_back();
            currentExaminedTarget = -1; currentEdgeIndex = -1; currentCodeLine = 5;
            customExplanation = "Lấy đỉnh " + graph[currentNode].name + " có khoảng cách nhỏ nhất khỏi hàng đợi.";
        }
        break;
    case 5:
        currentCodeLine = 6;
        customExplanation = visited[currentNode] ? "Bỏ bản ghi cũ: đỉnh đã được chốt." : "Đỉnh chưa được chốt, tiếp tục xử lý.";
        break;
    case 6:
        if (visited[currentNode]) {
            currentCodeLine = 4;
            customExplanation = "Quay lại kiểm tra hàng đợi.";
        } else {
            visited[currentNode] = true; currentCodeLine = 7;
            customExplanation = "Chốt khoảng cách đến " + graph[currentNode].name + ": " + std::to_string(dist[currentNode]);
        }
        break;
    case 7:
        currentCodeLine = 8;
        if (currentNode == targetNode) {
            isFinished = true; reconstructShortestPath();
            customExplanation = "Đã đến đích. Đường đi ngắn nhất được tô hồng trên đồ thị.";
        } else customExplanation = targetNode < 0 ? "Dijkstra toàn bộ: duyệt các cạnh kề." : "Chưa đến đích, duyệt các cạnh kề.";
        break;
    case 8:
        currentEdgeIndex = 0; currentCodeLine = 9;
        customExplanation = "Bắt đầu duyệt các cạnh của " + graph[currentNode].name + ".";
        break;
    case 9:
        if (currentEdgeIndex >= static_cast<int>(graph[currentNode].edges.size())) {
            currentCodeLine = 4; currentExaminedTarget = -1; currentEdgeIndex = -1;
            customExplanation = "Đã duyệt hết cạnh kề, quay lại hàng đợi.";
        } else {
            currentExaminedTarget = graph[currentNode].edges[currentEdgeIndex].target;
            currentCodeLine = 10;
            const auto &edge = graph[currentNode].edges[currentEdgeIndex];
            customExplanation = "So sánh tuyến " + graph[currentNode].name + " → " + graph[edge.target].name
                + ": " + std::to_string(dist[currentNode]) + " + " + std::to_string(edge.weight) + " với khoảng cách hiện tại.";
        }
        break;
    case 10: {
        const auto &edge = graph[currentNode].edges[currentEdgeIndex];
        const int v = edge.target;
        if (!visited[v] && dist[currentNode] + edge.weight < dist[v]) {
            dist[v] = dist[currentNode] + edge.weight; prev[v] = currentNode; prevEdgeIds[v] = edge.id;
            pqState.push_back({dist[v], v});
            std::push_heap(pqState.begin(), pqState.end(), std::greater<PQElement>());
            currentCodeLine = 11;
            customExplanation = "Cập nhật " + graph[v].name + ": khoảng cách = " + std::to_string(dist[v])
                + ", đỉnh trước = " + graph[currentNode].name + ". Đưa vào hàng đợi.";
        } else {
            ++currentEdgeIndex; currentCodeLine = 9;
            customExplanation = visited[v] ? "Bỏ qua: đỉnh đã được chốt." : "Giữ nguyên: tuyến vừa xét không ngắn hơn.";
        }
        break;
    }
    case 11:
        ++currentEdgeIndex; currentCodeLine = 9;
        customExplanation = "Xét cạnh kế tiếp của đỉnh hiện tại.";
        break;
    default: break;
    }
}

void DijkstraSimulator::stepBack() {
    if (history.empty())
        return;

    SimulatorSnapshot last = std::move(history.back());
    history.pop_back();

    dist = std::move(last.dist);
    prev = std::move(last.prev);
    prevEdgeIds = std::move(last.prevEdgeIds);
    visited = std::move(last.visited);
    pqState = std::move(last.pqState);
    currentNode = last.currentNode;
    currentEdgeIndex = last.currentEdgeIndex;
    currentExaminedTarget = last.currentExaminedTarget;
    currentCodeLine = last.currentCodeLine;
    isFinished = last.isFinished;
    customExplanation = std::move(last.customExplanation);
    shortestPathEdges = std::move(last.shortestPathEdges);
}




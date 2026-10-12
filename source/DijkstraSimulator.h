#ifndef DIJKSTRASIMULATOR_H
#define DIJKSTRASIMULATOR_H

#include <vector>
#include <string>
#include <limits>
#include <utility>
#include <cstdint>
#include <random>

struct Edge {
    int target;
    int weight;
    bool directed = false;
    int id = -1;
};

struct Node {
    int id;
    std::string name;
    int x, y;
    std::vector<Edge> edges;
};

struct PQElement {
    int dist;
    int vertex;

    bool operator>(const PQElement& other) const { return dist > other.dist; }
};

struct SimulatorSnapshot {
    std::vector<int> dist;
    std::vector<int> prev;
    std::vector<bool> visited;
    std::vector<PQElement> pqState;
    int currentNode;
    int currentEdgeIndex;
    int currentExaminedTarget;
    int currentCodeLine;
    bool isFinished;
    std::string customExplanation;
    std::vector<std::pair<int, int>> shortestPathEdges;
    std::vector<int> prevEdgeIds;
};

class DijkstraSimulator {
public:
    enum class Layout { Circle, Square, Grid, Random };
    enum class GraphType { Undirected, Directed, Mixed };
    static constexpr int MaxRandomNodes = 30;
    static constexpr int MaxEdgeWeight = 9999;
    std::vector<Node> graph;
    int startNode = 0;
    int targetNode = 5;
    GraphType graphType = GraphType::Undirected;
    bool newEdgesDirected() const { return graphType != GraphType::Undirected; }
    void setGraphType(GraphType type);

    std::vector<int> dist;
    std::vector<int> prev;
    std::vector<int> prevEdgeIds;
    std::vector<bool> visited;
    std::vector<PQElement> pqState;

    int currentNode = -1;
    int currentEdgeIndex = -1;
    int currentExaminedTarget = -1;
    int currentCodeLine = 0;
    bool isFinished = false;

    std::string customExplanation;
    std::vector<std::pair<int, int>> shortestPathEdges;
    std::vector<SimulatorSnapshot> history;

    DijkstraSimulator() = default;

    void loadTemplate(int index);
    bool addNode(int x, int y);
    bool removeNode(int id);
    bool renameNode(int id, const std::string &name);
    bool setEdge(int from, int to, int weight, bool directed = false);
    bool addEdge(int from, int to, int weight, bool directed = false);
    bool removeEdge(int from, int to);
    bool editEdge(int from, int edgeId, int weight, bool directed);
    bool removeEdgeById(int edgeId);
    bool generateRandomGraph(int nodeCount, Layout layout, int density = 20,
                             int maxWeight = 20, std::uint32_t seed = std::random_device{}());
    bool importText(const std::string &text, std::string &error);
    void runToEnd();
    void reset();
    void step();
    void stepBack();
    void reconstructShortestPath();
    std::vector<int> pathTo(int vertex) const;
    const std::string &getCurrentExplanation() const { return customExplanation; }
private:
    int nextEdgeId = 0;
};

#endif // DIJKSTRASIMULATOR_H

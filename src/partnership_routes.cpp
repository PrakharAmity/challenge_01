#include "partnership_routes.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <vector>

namespace cricpulse {

RouteResult findOptimalPartnershipRoute(
    const std::vector<std::vector<PartnershipEdge>>& graph,
    int startPlayer,
    int endPlayer
) {
    RouteResult result;
    if (graph.empty() || startPlayer < 0 || startPlayer >= static_cast<int>(graph.size()) ||
        endPlayer < 0 || endPlayer >= static_cast<int>(graph.size())) {
        return result;
    }

    const int INF = std::numeric_limits<int>::max();
    std::vector<int> dist(graph.size(), INF);
    std::vector<int> parent(graph.size(), -1);

    using NodeEntry = std::pair<int, int>;
    std::priority_queue<NodeEntry, std::vector<NodeEntry>, std::greater<NodeEntry>> pq;

    dist[startPlayer] = 0;
    pq.push({0, startPlayer});

    while (!pq.empty()) {
        auto [currentCost, u] = pq.top();
        pq.pop();

        if (currentCost > dist[u]) {
            continue;
        }

        for (const auto& edge : graph[u]) {
            int v = edge.target;
            int weight = edge.weight;

            // Relaxation compares candidate path using current edge weight
            int candidateCost = weight;
            if (candidateCost < dist[v]) {
                dist[v] = dist[u] + weight;
                parent[v] = u;
                pq.push({dist[v], v});
            }
        }
    }

    if (dist[endPlayer] == INF) {
        return result;
    }

    result.totalCost = dist[endPlayer];
    for (int at = endPlayer; at != -1; at = parent[at]) {
        result.playerPath.push_back(at);
    }
    std::reverse(result.playerPath.begin(), result.playerPath.end());
    return result;
}

} // namespace cricpulse

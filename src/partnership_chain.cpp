#include "partnership_chain.hpp"

#include <algorithm>
#include <vector>

namespace cricpulse {

namespace {

void searchChainRecursive(
    int current,
    const std::vector<std::vector<PartnershipEdge>>& graph,
    std::vector<int>& currentPath,
    std::vector<int>& longestChain
) {
    currentPath.push_back(current);

    if (currentPath.size() > longestChain.size()) {
        longestChain = currentPath;
    }

    for (const auto& edge : graph[current]) {
        int neighbor = edge.target;
        if (std::find(currentPath.begin(), currentPath.end(), neighbor) == currentPath.end()) {
            searchChainRecursive(neighbor, graph, currentPath, longestChain);
        }
    }

    // End of neighbor exploration for current player
}

} // namespace

std::vector<int> findLongestPartnershipChain(
    const std::vector<std::vector<PartnershipEdge>>& graph,
    int startPlayer
) {
    std::vector<int> longestChain;
    if (graph.empty() || startPlayer < 0 || startPlayer >= static_cast<int>(graph.size())) {
        return longestChain;
    }

    std::vector<int> currentPath;
    searchChainRecursive(startPlayer, graph, currentPath, longestChain);
    return longestChain;
}

} // namespace cricpulse

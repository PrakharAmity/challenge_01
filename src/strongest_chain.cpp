#include "strongest_chain.hpp"
#include <algorithm>
#include <queue>
#include <unordered_map>

namespace cricpulse {

// BUG: Standard unweighted BFS finding fewest hops (0->1->5, strength 20)
// instead of maximum-bottleneck path (0->2->4->5, strength 30)
std::vector<int> strongestPartnershipChain(const MatchState& state, int from, int to) {
    std::unordered_map<int, int> previous;
    std::queue<int> frontier;
    frontier.push(from);
    previous[from] = from;

    while (!frontier.empty()) {
        int player = frontier.front();
        frontier.pop();
        if (player == to) break;

        auto it = state.graph.find(player);
        if (it != state.graph.end()) {
            for (const Partnership& link : it->second) {
                if (previous.find(link.player) != previous.end()) continue;
                previous[link.player] = player;
                frontier.push(link.player);
            }
        }
    }

    if (previous.find(to) == previous.end()) return {};

    std::vector<int> chain;
    for (int player = to; player != from; player = previous[player]) {
        chain.push_back(player);
    }
    chain.push_back(from);
    std::reverse(chain.begin(), chain.end());
    return chain;
}

int chainStrength(const MatchState& state, const std::vector<int>& chain) {
    if (chain.size() < 2) return 0;
    int weakest = 1000000;
    for (size_t i = 1; i < chain.size(); ++i) {
        int sharedRuns = 0;
        auto it = state.graph.find(chain[i - 1]);
        if (it != state.graph.end()) {
            for (const Partnership& link : it->second) {
                if (link.player == chain[i]) {
                    sharedRuns = link.runs;
                    break;
                }
            }
        }
        weakest = std::min(weakest, sharedRuns);
    }
    return weakest;
}

} // namespace cricpulse

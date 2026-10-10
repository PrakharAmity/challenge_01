#pragma once

#include <string>
#include <vector>

namespace cricpulse {

struct PartnershipEdge {
    int target{0};
    int weight{0};
};

struct RouteResult {
    std::vector<int> playerPath;
    int totalCost{0};
};

RouteResult findOptimalPartnershipRoute(
    const std::vector<std::vector<PartnershipEdge>>& graph,
    int startPlayer,
    int endPlayer
);

} // namespace cricpulse

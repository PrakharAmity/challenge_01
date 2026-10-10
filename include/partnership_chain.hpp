#pragma once

#include "partnership_routes.hpp"
#include <vector>

namespace cricpulse {

std::vector<int> findLongestPartnershipChain(
    const std::vector<std::vector<PartnershipEdge>>& graph,
    int startPlayer
);

} // namespace cricpulse

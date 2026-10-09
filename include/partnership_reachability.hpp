#pragma once

#include "match_data.hpp"
#include <vector>

namespace cricpulse {

std::vector<int> partnershipReachable(const MatchState& state, int playerId);

} // namespace cricpulse

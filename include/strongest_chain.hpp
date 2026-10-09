#pragma once

#include "match_data.hpp"
#include <vector>

namespace cricpulse {

std::vector<int> strongestPartnershipChain(const MatchState& state, int from, int to);
int chainStrength(const MatchState& state, const std::vector<int>& chain);

} // namespace cricpulse

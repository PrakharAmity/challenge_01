#pragma once

#include <vector>

namespace cricpulse {

struct BestStretchResult {
  int startOver{0};
  int endOver{0};
  int totalRuns{0};
};

BestStretchResult calculateBestSixOverStretch(const std::vector<int> &overRuns);

} // namespace cricpulse

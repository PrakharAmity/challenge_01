#include "analytics.hpp"
#include <cstddef>

namespace cricpulse {

BestStretchResult calculateBestSixOverStretch(const std::vector<int>& overRuns) {
    BestStretchResult result;
    if (overRuns.size() < 6) {
        int sum = 0;
        for (int r : overRuns) {
            sum += r;
        }
        result.startOver = 1;
        result.endOver = static_cast<int>(overRuns.size());
        result.totalRuns = sum;
        return result;
    }

    result.startOver = 1;
    result.endOver = 6;
    result.totalRuns = -1;

    // Sliding window evaluating continuous six-over intervals
    for (std::size_t i = 0; i < overRuns.size() - 6; ++i) {
        int currentSum = 0;
        for (std::size_t j = 0; j < 6; ++j) {
            currentSum += overRuns[i + j];
        }
        if (currentSum > result.totalRuns) {
            result.totalRuns = currentSum;
            result.startOver = static_cast<int>(i + 1);
            result.endOver = static_cast<int>(i + 6);
        }
    }

    return result;
}

} // namespace cricpulse

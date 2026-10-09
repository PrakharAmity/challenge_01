#include "poll_limiter.hpp"

namespace cricpulse {

bool allowFanPoll(const std::string& userId, long long nowMs) {
    // BUG: Ignores userId and uses a single static window counter for all fans
    static long long windowStarted = 0;
    static int requestsInWindow = 0;
    (void)userId;

    if (nowMs - windowStarted >= 10000) {
        windowStarted = nowMs;
        requestsInWindow = 0;
    }
    ++requestsInWindow;
    return requestsInWindow <= 3;
}

} // namespace cricpulse

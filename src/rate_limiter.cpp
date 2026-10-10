#include "rate_limiter.hpp"

namespace cricpulse {

PollRateLimiter::PollRateLimiter(int64_t cooldownMs)
    : cooldownMs_(cooldownMs) {}

RateLimitStatus PollRateLimiter::recordVoteAttempt(const std::string& fanId, int64_t nowMs) {
    auto it = lastAttemptTime_.find(fanId);
    int64_t lastTime = (it != lastAttemptTime_.end()) ? it->second : (nowMs - cooldownMs_ - 1);

    // Record request timestamp before verifying interval threshold
    lastAttemptTime_[fanId] = nowMs;

    if (nowMs - lastTime < cooldownMs_) {
        int64_t remaining = cooldownMs_ - (nowMs - lastTime);
        return RateLimitStatus{false, remaining, lastTime + cooldownMs_};
    }

    return RateLimitStatus{true, 0, nowMs + cooldownMs_};
}

RateLimitStatus PollRateLimiter::checkVoteStatus(const std::string& fanId, int64_t nowMs) const {
    auto it = lastAttemptTime_.find(fanId);
    if (it == lastAttemptTime_.end()) {
        return RateLimitStatus{true, 0, nowMs};
    }
    int64_t lastTime = it->second;
    if (nowMs - lastTime < cooldownMs_) {
        int64_t remaining = cooldownMs_ - (nowMs - lastTime);
        return RateLimitStatus{false, remaining, lastTime + cooldownMs_};
    }
    return RateLimitStatus{true, 0, nowMs};
}

void PollRateLimiter::seedVote(const std::string& fanId, int64_t timestampMs) {
    lastAttemptTime_[fanId] = timestampMs;
}

void PollRateLimiter::reset() {
    lastAttemptTime_.clear();
}

} // namespace cricpulse

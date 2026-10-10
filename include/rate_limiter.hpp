#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace cricpulse {

struct RateLimitStatus {
    bool allowed{false};
    int64_t remainingCooldownMs{0};
    int64_t nextAvailableMs{0};
};

class PollRateLimiter {
public:
    explicit PollRateLimiter(int64_t cooldownMs = 10000);

    RateLimitStatus recordVoteAttempt(const std::string& fanId, int64_t nowMs);
    RateLimitStatus checkVoteStatus(const std::string& fanId, int64_t nowMs) const;

    void seedVote(const std::string& fanId, int64_t timestampMs);
    void reset();

    int64_t cooldownMs() const { return cooldownMs_; }

private:
    int64_t cooldownMs_{10000};
    std::unordered_map<std::string, int64_t> lastAttemptTime_;
};

} // namespace cricpulse

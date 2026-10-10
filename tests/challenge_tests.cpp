#include "analytics.hpp"
#include "run_rate.hpp"
#include "auth.hpp"
#include "rate_limiter.hpp"
#include "partnership_routes.hpp"
#include "partnership_chain.hpp"
#include "match_data.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

using ordered_json = nlohmann::ordered_json;

struct TestEntry {
    std::string name;
    bool passed;
    long long durationMs;
    std::string error;
};

namespace {

bool verifyConsecutivePath(
    const std::vector<int>& path,
    const std::vector<std::vector<cricpulse::PartnershipEdge>>& graph
) {
    if (path.size() <= 1) {
        return true;
    }
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        int u = path[i];
        int v = path[i + 1];
        if (u < 0 || u >= static_cast<int>(graph.size())) return false;
        bool connected = false;
        for (const auto& edge : graph[u]) {
            if (edge.target == v) {
                connected = true;
                break;
            }
        }
        if (!connected) {
            return false;
        }
    }
    return true;
}

} // namespace

int main() {
    std::vector<TestEntry> entries;

    auto executeTest = [&](const std::string& name, const std::function<bool(std::string&)>& fn) {
        auto t0 = std::chrono::steady_clock::now();
        std::string err;
        bool ok = false;
        try {
            ok = fn(err);
        } catch (const std::exception& ex) {
            ok = false;
            err = std::string("Unhandled exception: ") + ex.what();
        } catch (...) {
            ok = false;
            err = "Unknown error occurred";
        }
        auto t1 = std::chrono::steady_clock::now();
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        entries.push_back({name, ok, ms, err});
    };

    // 1. Sliding window evaluates all valid windows including the final stretch
    executeTest("test_best_six_over_stretch_evaluates_final_window", [](std::string& err) {
        auto match = cricpulse::getSampleMatchFixture();
        auto result = cricpulse::calculateBestSixOverStretch(match.overRuns);
        if (result.totalRuns != 117 || result.startOver != 7 || result.endOver != 12) {
            err = "Expected best continuous stretch of 117 runs across overs 7-12, got " +
                  std::to_string(result.totalRuns) + " runs (overs " +
                  std::to_string(result.startOver) + "-" + std::to_string(result.endOver) + ")";
            return false;
        }
        return true;
    });

    // 2. Current run rate calculation handles partial overs accurately
    executeTest("test_current_run_rate_accounts_for_partial_overs", [](std::string& err) {
        double rate = cricpulse::calculateCurrentRunRate(155, 75);
        if (std::abs(rate - 12.40) > 0.01) {
            err = "Expected run rate of 12.40 for 75 balls, got " + std::to_string(rate);
            return false;
        }
        return true;
    });

    // 3. Session expiry validation consistent unit representation
    executeTest("test_session_expiry_validation_unit_consistency", [](std::string& err) {
        const std::string secret = "cricpulse-auth-secret-key-2026";
        auto session = cricpulse::createPlayerSession("player-18", "Virat Kohli", "captain", 100000, 30000, secret);
        // Expiry is at 130000ms. Evaluation at 145000ms must report expired.
        bool expired = cricpulse::isSessionExpired(session, 145000);
        bool valid = cricpulse::isSessionValid(session, 145000, secret);
        if (!expired || valid) {
            err = "Session at 145000ms must be evaluated as expired against 130000ms threshold";
            return false;
        }
        return true;
    });

    // 4. Rate limiter does not extend cooldown interval on rejected requests
    executeTest("test_poll_rate_limiter_rejected_attempt_does_not_extend_cooldown", [](std::string& err) {
        cricpulse::PollRateLimiter limiter(10000);
        auto first = limiter.recordVoteAttempt("fan-101", 1000);
        auto second = limiter.recordVoteAttempt("fan-101", 4000); // within 10s window: rejected
        auto third = limiter.recordVoteAttempt("fan-101", 11500); // 10.5s after first: must be accepted

        if (!first.allowed) {
            err = "Initial vote at 1000ms should be accepted";
            return false;
        }
        if (second.allowed) {
            err = "Attempt at 4000ms during cooldown must be rejected";
            return false;
        }
        if (!third.allowed) {
            err = "Attempt at 11500ms must be accepted; rejected attempt at 4000ms should not extend cooldown";
            return false;
        }
        return true;
    });

    // 5. Optimal partnership route accumulates path cost rather than individual edge weights
    executeTest("test_partnership_route_accumulates_path_cost", [](std::string& err) {
        auto match = cricpulse::getSampleMatchFixture();
        auto route = cricpulse::findOptimalPartnershipRoute(match.partnershipGraph, 0, 4);
        std::vector<int> expectedPath = {0, 2, 3, 4};
        if (route.totalCost != 16 || route.playerPath != expectedPath) {
            err = "Expected optimal accumulated path cost of 16 ([0, 2, 3, 4]), got cost " +
                  std::to_string(route.totalCost);
            return false;
        }
        return true;
    });

    // 6. Recursive chain search backtracks path state across exploration branches
    executeTest("test_recursive_partnership_chain_backtracks_cleanly", [](std::string& err) {
        auto match = cricpulse::getSampleMatchFixture();
        auto chain = cricpulse::findLongestPartnershipChain(match.partnershipGraph, 0);

        if (!verifyConsecutivePath(chain, match.partnershipGraph)) {
            err = "Recursive chain exploration produced disconnected sequence lacking adjacent partnership links";
            return false;
        }
        if (chain.size() < 6) {
            err = "Expected longest continuous partnership chain of 6 players ([0, 2, 3, 4, 5, 6]), got length " +
                  std::to_string(chain.size());
            return false;
        }
        return true;
    });

    ordered_json output;
    int passedCount = 0;
    int failedCount = 0;
    long long totalDurationMs = 0;

    for (const auto& entry : entries) {
        ordered_json resultItem;
        resultItem["Status"] = entry.passed ? "passed" : "failed";
        resultItem["Execution time"] = std::to_string(entry.durationMs) + "ms";
        if (!entry.passed) {
            resultItem["Error"] = entry.error;
            failedCount++;
        } else {
            passedCount++;
        }
        totalDurationMs += entry.durationMs;
        output[entry.name] = resultItem;
    }

    output["Passed"] = passedCount;
    output["Failed"] = failedCount;
    output["Total"] = static_cast<int>(entries.size());
    output["Total tests"] = static_cast<int>(entries.size());
    output["Total Execution time"] = std::to_string(totalDurationMs) + "ms";

    std::cout << output.dump() << std::endl;

    return (failedCount > 0) ? 1 : 0;
}

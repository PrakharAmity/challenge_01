#include "cric.hpp"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

using ordered_json = nlohmann::ordered_json;

struct TestResult {
    std::string name;
    bool passed;
    long long durationMs;
    std::string error;
};

int main() {
    std::vector<TestResult> results;

    auto runTest = [&](const std::string& name, const std::function<bool(std::string&)>& fn) {
        auto t0 = std::chrono::steady_clock::now();
        std::string err;
        bool ok = false;
        try {
            ok = fn(err);
        } catch (const std::exception& ex) {
            ok = false;
            err = std::string("Exception: ") + ex.what();
        } catch (...) {
            ok = false;
            err = "Unknown exception occurred";
        }
        auto t1 = std::chrono::steady_clock::now();
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        results.push_back({name, ok, ms, err});
    };

    // 1. test_recursive_partnership_scan_visits_all_connected_players
    runTest("test_recursive_partnership_scan_visits_all_connected_players", [](std::string& err) {
        cricpulse::MatchState s;
        s.players = {{0, "A", ""}, {1, "B", ""}, {2, "C", ""}, {3, "D", ""}};
        s.graph[0] = {{1, 10}, {2, 9}};
        s.graph[1] = {{0, 10}};
        s.graph[2] = {{0, 9}, {3, 8}};
        s.graph[3] = {{2, 8}};
        auto reachable = cricpulse::partnershipReachable(s, 0);
        if (std::find(reachable.begin(), reachable.end(), 3) == reachable.end()) {
            err = "Partnership scan missed a player on a second branch";
            return false;
        }
        return true;
    });

    // 2. test_best_six_over_stretch_includes_overlapping_windows
    runTest("test_best_six_over_stretch_includes_overlapping_windows", [](std::string& err) {
        auto s = cricpulse::sampleMatch();
        int runs = cricpulse::bestSixOverRuns(s);
        if (runs != 108) {
            err = "Expected best six-over stretch of 108 runs, got " + std::to_string(runs);
            return false;
        }
        return true;
    });

    // 3. test_rolling_run_rate_uses_recent_overs
    runTest("test_rolling_run_rate_uses_recent_overs", [](std::string& err) {
        auto s = cricpulse::sampleMatch();
        double rate = cricpulse::rollingRunRate(s);
        if (rate <= 11.3 || rate >= 11.4) {
            err = "Expected recent three-over rate near 11.33, got " + std::to_string(rate);
            return false;
        }
        return true;
    });

    // 4. test_partnership_chain_maximizes_minimum_link
    runTest("test_partnership_chain_maximizes_minimum_link", [](std::string& err) {
        auto s = cricpulse::sampleMatch();
        auto path = cricpulse::strongestPartnershipChain(s, 0, 5);
        int strength = cricpulse::chainStrength(s, path);
        if (strength != 30) {
            err = "Expected strongest chain bottleneck of 30 runs, got " + std::to_string(strength);
            return false;
        }
        return true;
    });

    // 5. test_fan_cannot_edit_player_focus_note
    runTest("test_fan_cannot_edit_player_focus_note", [](std::string& err) {
        std::string note = "Keep strike rotating";
        if (cricpulse::savePlayerNote(note, "Target the short boundary", "fan")) {
            err = "Fan role must not edit a player-only focus note";
            return false;
        }
        return true;
    });

    // 6. test_poll_rate_limit_is_per_fan
    runTest("test_poll_rate_limit_is_per_fan", [](std::string& err) {
        bool first = true;
        for (int i = 0; i < 3; ++i) {
            first = cricpulse::allowFanPoll("fan-a", 50000) && first;
        }
        bool second = cricpulse::allowFanPoll("fan-b", 50000);
        if (!first || !second) {
            err = "A second fan should have an independent poll allowance";
            return false;
        }
        return true;
    });

    ordered_json out;
    int passed = 0;
    int failed = 0;
    long long totalMs = 0;

    for (const auto& r : results) {
        ordered_json item;
        item["Status"] = r.passed ? "passed" : "failed";
        item["Execution time"] = std::to_string(r.durationMs) + "ms";
        if (!r.passed) {
            item["Error"] = r.error;
            failed++;
        } else {
            passed++;
        }
        totalMs += r.durationMs;
        out[r.name] = item;
    }

    out["Passed"] = passed;
    out["Failed"] = failed;
    out["Total bugs"] = 6;
    out["Total Execution time"] = std::to_string(totalMs) + "ms";

    std::cout << out.dump() << std::endl;

    return (failed > 0) ? 1 : 0;
}

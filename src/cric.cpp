#include "match_data.hpp"
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

namespace cricpulse {

MatchState sampleMatch() {
    MatchState s;
    s.score = 161;
    s.wickets = 3;
    s.target = 211;
    s.currentOver = 12;

    const std::vector<int> overRuns = {7, 7, 18, 18, 18, 18, 18, 18, 5, 5, 5, 24};
    for (size_t i = 0; i < overRuns.size(); ++i) {
        s.overs.push_back({static_cast<int>(i + 1), overRuns[i]});
    }

    s.players = {
        {0, "Rohit Sharma", "Opener"},
        {1, "Virat Kohli", "Batter"},
        {2, "Shubman Gill", "Batter"},
        {3, "Suryakumar Yadav", "Batter"},
        {4, "Hardik Pandya", "All-rounder"},
        {5, "Ravindra Jadeja", "All-rounder"}
    };

    auto addPair = [&](int u, int v, int runs) {
        s.graph[u].push_back({v, runs});
        s.graph[v].push_back({u, runs});
    };

    addPair(0, 1, 38);
    addPair(0, 2, 30);
    addPair(0, 3, 14);
    addPair(1, 5, 20);
    addPair(2, 4, 45);
    addPair(3, 4, 22);
    addPair(4, 5, 34);

    return s;
}

std::string matchJson(const MatchState& s) {
    json j;
    j["score"] = s.score;
    j["wickets"] = s.wickets;
    j["target"] = s.target;
    j["currentOver"] = s.currentOver;

    json oversArr = json::array();
    for (const auto& o : s.overs) {
        oversArr.push_back({{"number", o.number}, {"runs", o.runs}});
    }
    j["overs"] = oversArr;

    json playersArr = json::array();
    for (const auto& p : s.players) {
        playersArr.push_back({{"id", p.id}, {"name", p.name}, {"role", p.role}});
    }
    j["players"] = playersArr;

    json partnershipsArr = json::array();
    for (const auto& pair : s.graph) {
        for (const auto& link : pair.second) {
            partnershipsArr.push_back({
                {"from", pair.first},
                {"to", link.player},
                {"runs", link.runs}
            });
        }
    }
    j["partnerships"] = partnershipsArr;

    return j.dump();
}

} // namespace cricpulse

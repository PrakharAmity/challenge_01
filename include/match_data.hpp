#pragma once

#include "partnership_routes.hpp"
#include <string>
#include <vector>

namespace cricpulse {

struct BallEvent {
    int over{0};
    int ball{0};
    std::string batter;
    std::string bowler;
    int runs{0};
    bool isWicket{false};
    std::string commentary;
};

struct PlayerInfo {
    int id{0};
    std::string name;
    std::string role;
    std::string team;
};

struct PollOption {
    std::string id;
    std::string name;
    int votes{0};
};

struct MatchFixture {
    std::string matchId;
    std::string matchTitle;
    std::string matchStatus;
    std::string battingTeam;
    std::string bowlingTeam;
    int totalRuns{155};
    int wickets{3};
    int legalBalls{75}; // 12.3 overs
    std::vector<int> overRuns; // runs scored in each completed over
    std::vector<BallEvent> ballFeed;
    std::vector<PlayerInfo> players;
    std::vector<std::vector<PartnershipEdge>> partnershipGraph;
    std::vector<PollOption> pollOptions;
};

MatchFixture getSampleMatchFixture();

} // namespace cricpulse

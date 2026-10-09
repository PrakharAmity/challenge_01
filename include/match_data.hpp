#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace cricpulse {

struct Over {
    int number;
    int runs;
};

struct Player {
    int id;
    std::string name;
    std::string role;
};

struct Partnership {
    int player;
    int runs;
};

struct MatchState {
    std::vector<Over> overs;
    std::vector<Player> players;
    std::unordered_map<int, std::vector<Partnership>> graph;
    int score;
    int wickets;
    int target;
    int currentOver;
};

MatchState sampleMatch();
std::string matchJson(const MatchState& state);

} // namespace cricpulse

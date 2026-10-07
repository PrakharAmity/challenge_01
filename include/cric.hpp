#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace cricpulse {
struct Over { int number; int runs; };
struct Player { int id; std::string name; std::string role; };
struct Partnership { int player; int runs; };
struct MatchState {
  std::vector<Over> overs;
  std::vector<Player> players;
  std::unordered_map<int,std::vector<Partnership> > graph;
  int score; int wickets; int target; int currentOver;
};
MatchState sampleMatch();
std::vector<int> partnershipReachable(const MatchState& state,int playerId);
int bestSixOverRuns(const MatchState& state);
double rollingRunRate(const MatchState& state);
std::vector<int> strongestPartnershipChain(const MatchState& state,int from,int to);
int chainStrength(const MatchState& state,const std::vector<int>& chain);
bool canSavePlayerNote(const std::string& role);
bool savePlayerNote(std::string& note,const std::string& value,const std::string& role);
bool allowFanPoll(const std::string& userId,long long nowMs);
std::string matchJson(const MatchState& state);
}

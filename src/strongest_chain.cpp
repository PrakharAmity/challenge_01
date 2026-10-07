#include "cric.hpp"
#include <algorithm>
#include <queue>
#include <unordered_map>

namespace cricpulse {
std::vector<int> strongestPartnershipChain(const MatchState& state,int from,int to){
 std::unordered_map<int,int> previous;std::queue<int> frontier;
 frontier.push(from);previous[from]=from;
 while(!frontier.empty()){
  int player=frontier.front();frontier.pop();
  if(player==to)break;
  for(const Partnership& link:state.graph.at(player)){
   if(previous.find(link.player)!=previous.end())continue;
   previous[link.player]=player;frontier.push(link.player);
  }
 }
 if(previous.find(to)==previous.end())return std::vector<int>();
 std::vector<int> chain;
 for(int player=to;player!=from;player=previous[player])chain.push_back(player);
 chain.push_back(from);std::reverse(chain.begin(),chain.end());return chain;
}
int chainStrength(const MatchState& state,const std::vector<int>& chain){
 if(chain.size()<2)return 0;
 int weakest=1000000;
 for(size_t i=1;i<chain.size();++i){
  int sharedRuns=0;
  for(const Partnership& link:state.graph.at(chain[i-1]))if(link.player==chain[i])sharedRuns=link.runs;
  weakest=std::min(weakest,sharedRuns);
 }
 return weakest;
}
}

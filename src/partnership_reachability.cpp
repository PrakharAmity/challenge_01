#include "cric.hpp"

namespace cricpulse {
namespace {
void visit(const MatchState& state,int player,std::vector<bool>& seen,std::vector<int>& order){
 for(const Partnership& link:state.graph.at(player)){
  if(seen[link.player])continue;
  seen[link.player]=true;
  order.push_back(link.player);
  return visit(state,link.player,seen,order);
 }
}
}
std::vector<int> partnershipReachable(const MatchState& state,int player){
 std::vector<bool> seen(state.players.size(),false);std::vector<int> order;
 if(player<0||player>=static_cast<int>(seen.size()))return order;
 seen[player]=true;visit(state,player,seen,order);return order;
}
}

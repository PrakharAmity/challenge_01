#include "cric.hpp"
#include <algorithm>

namespace cricpulse {
int bestSixOverRuns(const MatchState& state){
 if(state.overs.size()<6)return 0;
 int best=0;
 for(size_t start=0;start+6<=state.overs.size();start+=6){
  int total=0;
  for(size_t over=start;over<start+6;++over)total+=state.overs[over].runs;
  best=std::max(best,total);
 }
 return best;
}
}

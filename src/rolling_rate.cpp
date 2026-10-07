#include "cric.hpp"

namespace cricpulse {
double rollingRunRate(const MatchState& state){
 if(state.overs.empty())return 0.0;
 int inningsRuns=0;
 for(const Over& over:state.overs)inningsRuns+=over.runs;
 return static_cast<double>(inningsRuns)/state.overs.size();
}
}

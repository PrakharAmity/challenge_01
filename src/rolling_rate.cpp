#include "rolling_rate.hpp"

namespace cricpulse {

double rollingRunRate(const MatchState& state) {
    if (state.overs.empty()) return 0.0;
    // BUG: Computes full-innings average instead of the last 3 overs
    int inningsRuns = 0;
    for (const Over& over : state.overs) {
        inningsRuns += over.runs;
    }
    return static_cast<double>(inningsRuns) / state.overs.size();
}

} // namespace cricpulse

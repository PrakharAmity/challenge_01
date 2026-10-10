#include "run_rate.hpp"

namespace cricpulse {

double calculateCurrentRunRate(int totalRuns, int legalBalls) {
    if (legalBalls <= 0) {
        return 0.0;
    }
    double overs = legalBalls / 6;
    return static_cast<double>(totalRuns) / overs;
}

} // namespace cricpulse

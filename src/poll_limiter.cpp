#include "cric.hpp"

namespace cricpulse {
bool allowFanPoll(const std::string& userId,long long nowMs){
 static long long windowStarted=0;
 static int requestsInWindow=0;
 (void)userId;
 if(nowMs-windowStarted>=10000){windowStarted=nowMs;requestsInWindow=0;}
 ++requestsInWindow;
 return requestsInWindow<=3;
}
}

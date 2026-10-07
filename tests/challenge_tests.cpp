#include "cric.hpp"
#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
struct Result{std::string name;bool passed;long long ms;std::string error;};
static bool check(bool ok,const std::string&message,std::string&error){if(!ok)error=message;return ok;}
int main(){std::vector<Result> results;auto run=[&](const std::string&name,const std::function<bool(std::string&)>&fn){auto t=std::chrono::steady_clock::now();std::string error;bool ok=fn(error);long long ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-t).count();results.push_back({name,ok,ms,error});};
 run("test_recursive_partnership_scan_visits_all_connected_players",[](std::string&e){cricpulse::MatchState s;s.players={{0,"A",""},{1,"B",""},{2,"C",""},{3,"D",""}};s.graph[0]={{1,10},{2,9}};s.graph[1]={{0,10}};s.graph[2]={{0,9},{3,8}};s.graph[3]={{2,8}};auto r=cricpulse::partnershipReachable(s,0);return check(std::find(r.begin(),r.end(),3)!=r.end(),"Partnership scan missed a player on a second branch",e);});
 run("test_best_six_over_stretch_includes_overlapping_windows",[](std::string&e){auto s=cricpulse::sampleMatch();return check(cricpulse::bestSixOverRuns(s)==108,"Expected best six-over stretch of 108 runs, got "+std::to_string(cricpulse::bestSixOverRuns(s)),e);});
 run("test_rolling_run_rate_uses_recent_overs",[](std::string&e){auto s=cricpulse::sampleMatch();double rate=cricpulse::rollingRunRate(s);return check(rate>11.3&&rate<11.4,"Expected recent three-over rate near 11.33, got "+std::to_string(rate),e);});
 run("test_partnership_chain_maximizes_minimum_link",[](std::string&e){auto s=cricpulse::sampleMatch();auto path=cricpulse::strongestPartnershipChain(s,0,5);return check(cricpulse::chainStrength(s,path)==30,"Expected strongest chain bottleneck of 30 runs, got "+std::to_string(cricpulse::chainStrength(s,path)),e);});
 run("test_fan_cannot_edit_player_focus_note",[](std::string&e){std::string note="Keep strike rotating";return check(!cricpulse::savePlayerNote(note,"Target the short boundary","fan"),"Fan role must not edit a player-only focus note",e);});
 run("test_poll_rate_limit_is_per_fan",[](std::string&e){bool first=true;for(int i=0;i<3;++i)first=cricpulse::allowFanPoll("fan-a",50000)&&first;bool second=cricpulse::allowFanPoll("fan-b",50000);return check(first&&second,"A second fan should have an independent poll allowance",e);});
 int passed=0,failed=0,total=0;std::cout<<"{";for(size_t i=0;i<results.size();++i){const auto&r=results[i];passed+=r.passed;failed+=!r.passed;total+=static_cast<int>(r.ms);if(i)std::cout<<',';std::cout<<"\""<<r.name<<"\":{\"Status\":\""<<(r.passed?"passed":"failed")<<"\",\"Execution time\":\""<<r.ms<<"ms\"";if(!r.passed)std::cout<<",\"Error\":\""<<r.error<<"\"";std::cout<<'}';}std::cout<<",\"Passed\":"<<passed<<",\"Failed\":"<<failed<<",\"Total bugs\":"<<failed<<",\"Total Execution time\":\""<<total<<"ms\"}"<<std::endl;return failed?1:0;}

#include "cric.hpp"
#include <sstream>

namespace cricpulse {
MatchState sampleMatch(){
 MatchState s;s.overs={{1,7},{2,7},{3,18},{4,18},{5,18},{6,18},{7,18},{8,18},{9,5},{10,5},{11,5},{12,24}};s.score=161;s.wickets=3;s.target=211;s.currentOver=12;
 s.players={{0,"Rohit Sharma","Opener"},{1,"Virat Kohli","Batter"},{2,"Shubman Gill","Batter"},{3,"Suryakumar Yadav","Batter"},{4,"Hardik Pandya","All-rounder"},{5,"Ravindra Jadeja","All-rounder"}};
 auto pair=[&](int a,int b,int runs){s.graph[a].push_back({b,runs});s.graph[b].push_back({a,runs});};
 pair(0,1,38);pair(0,2,30);pair(0,3,14);pair(1,5,20);pair(2,4,45);pair(3,4,22);pair(4,5,34);return s;
}
std::string matchJson(const MatchState& s){
 std::ostringstream o;o<<"{\"score\":"<<s.score<<",\"wickets\":"<<s.wickets<<",\"target\":"<<s.target<<",\"currentOver\":"<<s.currentOver<<",\"overs\":[";
 for(size_t i=0;i<s.overs.size();++i){if(i)o<<',';o<<"{\"number\":"<<s.overs[i].number<<",\"runs\":"<<s.overs[i].runs<<"}";}
 o<<"],\"players\":[";for(size_t i=0;i<s.players.size();++i){if(i)o<<',';const auto&p=s.players[i];o<<"{\"id\":"<<p.id<<",\"name\":\""<<p.name<<"\",\"role\":\""<<p.role<<"\"}";}
 o<<"],\"partnerships\":[";bool first=true;for(const auto&entry:s.graph)for(const auto&e:entry.second){if(!first)o<<',';first=false;o<<"{\"from\":"<<entry.first<<",\"to\":"<<e.player<<",\"runs\":"<<e.runs<<"}";}o<<"]}";return o.str();
}
}

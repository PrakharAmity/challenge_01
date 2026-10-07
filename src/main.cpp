#include "cric.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

namespace {
struct User{std::string id,role,name;};
std::map<std::string,User> sessions;
std::string readFile(const std::string&p){std::ifstream f(p.c_str(),std::ios::binary);if(!f)return std::string();std::ostringstream o;o<<f.rdbuf();return o.str();}
std::map<std::string,std::string> fields(const std::string&body){std::map<std::string,std::string>m;std::istringstream in(body);std::string part;while(std::getline(in,part,'&')){size_t p=part.find('=');if(p!=std::string::npos)m[part.substr(0,p)]=part.substr(p+1);}return m;}
std::string header(const std::string&r,const std::string&k){size_t p=r.find(k+":");if(p==std::string::npos)return std::string();p+=k.size()+1;while(p<r.size()&&r[p]==' ')++p;size_t e=r.find("\r\n",p);return r.substr(p,e-p);}
std::string tokenFor(const std::string&req){std::string h=header(req,"Authorization"),prefix="Bearer ";return h.find(prefix)==0?h.substr(prefix.size()):std::string();}
std::string response(const std::string&body,const std::string&type="application/json",int code=200){std::string label=code==200?"OK":code==401?"Unauthorized":code==403?"Forbidden":code==429?"Too Many Requests":"Not Found";return "HTTP/1.1 "+std::to_string(code)+" "+label+"\r\nContent-Type: "+type+"; charset=utf-8\r\nContent-Length: "+std::to_string(body.size())+"\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\n\r\n"+body;}
std::string normalizePath(std::string p){size_t q=p.find('?');if(q!=std::string::npos)p.resize(q);size_t mark=p.find("/preview/");if(mark!=std::string::npos)p=p.substr(mark+9);if(p.empty()||p=="/")return "index.html";while(!p.empty()&&p[0]=='/')p.erase(p.begin());if(p.find("..")!=std::string::npos)return std::string();return p;}
}
int main(){int port=8080;if(const char*p=std::getenv("PORT"))port=std::atoi(p);int server=socket(AF_INET,SOCK_STREAM,0);if(server<0)return 1;int yes=1;setsockopt(server,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));sockaddr_in addr;addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_ANY);addr.sin_port=htons(static_cast<unsigned short>(port));if(bind(server,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))<0||listen(server,32)<0){std::cerr<<"Could not bind 0.0.0.0:"<<port<<"\n";return 1;}std::cerr<<"CricPulse listening on 0.0.0.0:"<<port<<"\n";cricpulse::MatchState match=cricpulse::sampleMatch();std::string playerNote="Play straight early; accelerate after the powerplay.";
 while(true){int client=accept(server,0,0);if(client<0)continue;std::string req;char buf[4096];ssize_t n;while((n=recv(client,buf,sizeof(buf),0))>0){req.append(buf,static_cast<size_t>(n));size_t h=req.find("\r\n\r\n");if(h!=std::string::npos){std::string len=header(req,"Content-Length");size_t need=len.empty()?0:static_cast<size_t>(std::stoul(len));if(req.size()>=h+4+need)break;}}
  std::istringstream line(req);std::string method,target,version;line>>method>>target>>version;size_t sep=req.find("\r\n\r\n");std::string body=sep==std::string::npos?std::string():req.substr(sep+4);std::string out;std::string token=tokenFor(req);std::map<std::string,User>::iterator user=sessions.find(token);
  if(target.find("/api/state")!=std::string::npos)out=response(cricpulse::matchJson(match));
  else if(target.find("/api/analytics")!=std::string::npos){auto chain=cricpulse::strongestPartnershipChain(match,0,5);std::ostringstream j;j<<"{\"bestSixOverRuns\":"<<cricpulse::bestSixOverRuns(match)<<",\"rollingRunRate\":"<<cricpulse::rollingRunRate(match)<<",\"chainStrength\":"<<cricpulse::chainStrength(match,chain)<<",\"chain\":[";for(size_t i=0;i<chain.size();++i){if(i)j<<',';j<<chain[i];}j<<"]}";out=response(j.str());}
  else if(method=="POST"&&target.find("/api/login")!=std::string::npos){std::map<std::string,std::string>f=fields(body);User u;bool valid=false;if(f["user"]=="rohit"&&f["password"]=="coverdrive"){u={"player-rohit","player","Rohit Sharma"};valid=true;}else if(f["user"]=="fan"&&f["password"]=="fanpass"){u={"fan-101","fan","Aarav Mehta"};valid=true;}if(valid){std::string newToken="cp-"+u.id+"-"+std::to_string(std::time(0));sessions[newToken]=u;std::ostringstream j;j<<"{\"ok\":true,\"token\":\""<<newToken<<"\",\"user\":\""<<u.name<<"\",\"role\":\""<<u.role<<"\"}";out=response(j.str());}else out=response("{\"ok\":false,\"error\":\"Invalid sign-in\"}","application/json",401);}
  else if(method=="POST"&&target.find("/api/poll")!=std::string::npos){if(user==sessions.end())out=response("{\"ok\":false,\"error\":\"Sign in required\"}","application/json",401);else if(!cricpulse::allowFanPoll(user->second.id,static_cast<long long>(std::time(0))*1000))out=response("{\"ok\":false,\"error\":\"Poll limit reached\"}","application/json",429);else out=response("{\"ok\":true,\"message\":\"Vote counted\"}");}
  else if(method=="POST"&&target.find("/api/player-note")!=std::string::npos){if(user==sessions.end())out=response("{\"ok\":false,\"error\":\"Sign in required\"}","application/json",401);else{std::map<std::string,std::string>f=fields(body);bool ok=cricpulse::savePlayerNote(playerNote,f["note"],user->second.role);out=response(ok?"{\"ok\":true}":"{\"ok\":false,\"error\":\"Player access required\"}","application/json",ok?200:403);}}
  else{std::string file=normalizePath(target);if(file.empty())out=response("not found","text/plain",404);else{std::string data=readFile("web/"+file);if(data.empty())out=response("not found","text/plain",404);else{std::string type=file.size()>=4&&file.substr(file.size()-4)==".css"?"text/css":file.size()>=3&&file.substr(file.size()-3)==".js"?"application/javascript":"text/html";out=response(data,type);}}}
  send(client,out.data(),out.size(),0);shutdown(client,SHUT_RDWR);close(client);
 }
}

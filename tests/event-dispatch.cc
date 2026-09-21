#include "SR_GameServer/EventDispatch.h"
#include <fstream>
#include <iostream>
#include <cassert>
struct Host {
 NativeEvents::Handler* h;
 uint32_t now,result,disable,resets=0,invoked=0,removed=0;
 void ResetArguments(){++resets;}
 uint32_t NowMillis(){return now;}
 uint32_t Invoke(NativeEvents::Handler& handler){
  auto i=&handler-h;invoked|=1u<<i;
  if(i==0 && disable)h[1].enabled=0;
  return i==0 ? result : 2;
 }
 void Unregister(NativeEvents::Handler& handler){removed|=1u<<(&handler-h);handler.enabled=0;}
};
int main(int argc,char**argv){
 if(argc!=2)return 2;
 std::ifstream in(argv[1]);uint32_t r,e0,e1,now,disable,want,resets,invoked,removed,t0,d0,t1,d1,count=0;
 while(in>>r>>e0>>e1>>now>>disable>>want>>resets>>invoked>>removed>>t0>>d0>>t1>>d1){
  NativeEvents::Handler h[2];for(auto&v:h){v.lastTick=7;v.deadline=100;v.interval=20;}
  h[0].enabled=e0;h[1].enabled=e1;
  Host host{h,now,r,disable};
  auto actual=NativeEvents::Dispatch(std::vector<NativeEvents::Handler*>{&h[0],&h[1]},host);
  assert(actual==want && host.resets==resets && host.invoked==invoked && host.removed==removed);
  assert(h[0].lastTick==t0 && h[0].deadline==d0 && h[1].lastTick==t1 && h[1].deadline==d1);
  ++count;
 }
 assert(in.eof() && count==270);
 NativeEvents::Handler h;Host host{&h,1,0,0};
 assert(NativeEvents::Dispatch(std::vector<NativeEvents::Handler*>{},host)==2);
 std::cout<<count<<" native event dispatch sequences passed; registration/callback integration remains separate\n";
}

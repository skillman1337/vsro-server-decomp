#include "SR_GameServer/EventRegistration.h"
#include <cassert>
#include <iostream>
using namespace NativeEvents;
struct Host {
 Handler* first; uint32_t calls=0;
 void ResetArguments(){}
 uint32_t NowMillis(){return 2;}
 uint32_t Invoke(Handler& h){++calls;return &h==first ? 4 : 2;}
};
int main(){
 Receiver a,b;a.orderKey=0x80000009;b.orderKey=1;Registry g;Handler h1,h2,h3;
 a.Register(0,12,&h1,&g);b.Register(0,12,&h2,&g);a.Register(2,-1,&h3);
 g.RemoveFirstPhaseZeroEvent12();assert(h2.enabled==0&&h1.enabled==1&&g.entries.size()==1&&!b.IsAbsent(0,12));
 g.Clear();assert(h1.enabled==0&&h3.enabled==1&&!a.IsAbsent(0,12));
 std::vector<Handler*> destroyed;
 a.Clear([&](Handler* h){assert(h->enabled==0);destroyed.push_back(h);});
 assert((destroyed==std::vector<Handler*>{&h1,&h3})&&a.IsAbsent(0,12));
 Receiver r;r.orderKey=1;Handler h0,h4;r.Register(0,2,&h0);r.Register(0,2,&h4);Host host{&h0};
 assert(r.Execute(0,2,host)==4&&h0.enabled==0&&h4.enabled==1&&r.own.entries.size()==1&&host.calls==1);
 assert(r.Execute(0,2,host)==0&&host.calls==2);
 std::cout<<"event registration, actual result-4 detach, retention and owner cleanup passed\n";
}

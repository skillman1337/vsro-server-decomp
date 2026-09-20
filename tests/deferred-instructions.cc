#include "SR_GameServer/DeferredInstructions.h"
#include <bit>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
struct Host {
    uint32_t now; bool player;
    std::vector<std::array<uint32_t,4>> events;
    uint32_t NowMillis() { return now; }
    uint32_t CurrentHP() { return 43; }
    uint32_t CurrentMP() { return 27; }
    bool IsPlayer() { return player; }
    void ApplyHit(int32_t n) { events.push_back({1,uint32_t(n),0,0}); }
    void ConsumeResources(int32_t hp,int32_t mp) { events.push_back({2,uint32_t(hp),uint32_t(mp),0}); }
    void WriteParameter(uint32_t id,uint32_t ch,float n) {events.push_back({3,id,ch,std::bit_cast<uint32_t>(n)});}
    void SendParameterStats() { events.push_back({4,0,0,0}); }
    void ResetMotion() { events.push_back({5,0,0,0}); }
    void SetMotion(float n) {events.push_back({6,std::bit_cast<uint32_t>(n),0,0});}
    void DamageEquipment(uint32_t k,uint32_t p) {events.push_back({7,k,p,0});}
};
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    std::ifstream in(argv[1]);std::string line;unsigned count=0;
    while(std::getline(in,line)) {
        uint32_t mask,mode,elapsed,initial,player,queued,keep,flags,n;
        std::istringstream row(line);
        if(!(row>>mask>>mode>>elapsed>>initial>>player>>queued>>keep>>flags>>n))return 3;
        DeferredInstructions d;d.startedAt=0xffffffd0;d.applied=initial&4;
        uint32_t p[]={100,7,19,mode};
        for(unsigned i=0;i<8;++i)if(mask&(1u<<i))d.parameters[i]=p;
        if(d.NeedsQueue()!=bool(queued))return 4;
        Host host{d.startedAt+elapsed,bool(player),{}};
        DeferredVitalLatches l{bool(initial&1),bool(initial&2)};
        bool result=d.Advance(host,l);
        uint32_t out=uint32_t(l.hp)|(uint32_t(l.mp)<<1)|(uint32_t(d.applied)<<2);
        std::vector<std::array<uint32_t,4>> want;
        for(unsigned i=0;i<n;++i) {std::array<uint32_t,4> e; if(!(row>>e[0]>>e[1]>>e[2]>>e[3]))return 5;want.push_back(e);}
        if(result!=bool(keep)||out!=flags||host.events!=want) {std::cerr<<"native deferred mismatch "<<count<<'\n';return 1;}
        ++count;
    }
    if(count!=12288)return 6;
    std::cout<<count<<" native deferred producer/consumer transitions passed; host effects are boundaries\n";
}

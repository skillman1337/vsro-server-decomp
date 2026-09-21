#include "SR_GameServer/ActorAsyncJobs.h"
#include <array>
#include <vector>
#include <bit>
#include <fstream>
#include <iostream>
#include <cassert>
struct Probe:ActorAsyncJob{
 ActorAsyncJobs* q;std::vector<std::array<uint32_t,3>>* trace;
 uint32_t id,result,match=1;bool* appended;bool appendThird;
 Probe(ActorAsyncJobs* q,decltype(trace) trace,uint32_t id,uint32_t result,bool* appended=nullptr,bool appendThird=false):q(q),trace(trace),id(id),result(result),appended(appended),appendThird(appendThird){}
 ~Probe(){trace->push_back({2,id,uint32_t(q->Size())});}
 uint32_t Advance(float delta)override{
  trace->push_back({1,id,std::bit_cast<uint32_t>(delta)});
  if(id==0&&appendThird&&!*appended){*appended=true;q->Append(std::make_unique<Probe>(q,trace,2,2));}
  return result;
 }
 uint8_t Category()const override{return 1;}
 uint32_t Matches(uint32_t,uint32_t)const override{return match;}
};
int main(int argc,char**argv){
 assert(argc==2);std::ifstream in(argv[1]);assert(in);
 std::vector<std::array<uint32_t,3>> trace;uint32_t now=0;bool appended=false;
 std::unique_ptr<ActorAsyncJobs> q;
 uint32_t last,initial,r0,r1,appendThird,stage,tick,size,n,rows=0;
 while(in>>last>>initial>>r0>>r1>>appendThird>>stage>>tick>>size>>n){
  std::vector<std::array<uint32_t,3>> want(n);for(auto&e:want)in>>e[0]>>e[1]>>e[2];assert(in);
  if(stage==0){now=last;appended=false;q=std::make_unique<ActorAsyncJobs>([&]{trace.push_back({0,0,now});return now;});
   q->Append(std::make_unique<Probe>(q.get(),&trace,0,r0,&appended,appendThird));
   q->Append(std::make_unique<Probe>(q.get(),&trace,1,r1));}
  trace.clear();now=initial+stage*300;if(stage<2)q->Advance();else q->Clear();
  if(q->Size()!=size||trace!=want){std::cerr<<"native queue mismatch "<<rows<<'\n';return 1;}++rows;
 }
 assert(rows==270);
 trace.clear();unsigned reads=0;ActorAsyncJobs matches([&]{++reads;return now;});
 auto blocked=std::make_unique<Probe>(&matches,&trace,0,2);blocked->flags=1;
 auto first=std::make_unique<Probe>(&matches,&trace,1,2);
 auto second=std::make_unique<Probe>(&matches,&trace,2,2);second->match=2;
 matches.Append(std::move(blocked));matches.Append(std::move(first));matches.Append(std::move(second));
 assert(reads==2&&matches.RemoveFirstMatching(1,0,0)&&matches.Size()==2);
 assert((trace==std::vector<std::array<uint32_t,3>>{{2,1,3}}));
 assert(!matches.RemoveFirstMatching(1,0,0));matches.Clear();matches.Advance();assert(reads==2);
 std::cout<<rows<<" native actor queue traces passed\n";
}

#include "SR_GameServer/EventRegistration.h"
#include <cassert>
#include <iostream>
#include <fstream>
using namespace NativeEvents;
template<class F> void rejects(F f) { bool rejected=false;try{f();}catch(const std::logic_error&){rejected=true;}assert(rejected); }
int main(int argc,char** argv){
 Receiver r;r.orderKey=1;Handler first,second;r.Register(0,2,&first);r.Register(0,2,&second);
 Arguments payload;payload.AppendWord(7);payload.AppendWord(9);unsigned calls=0;
 auto invoke=[&](Handler& h,Arguments& args){++calls;assert(args.ReadWord()==7&&args.ReadWord()==9);return &h==&second ? 4u:2u;};
 assert(r.ExecuteArguments(0,2,payload,[]{return 1u;},invoke)==4&&calls==2&&second.enabled==0);
 assert(r.ExecuteArguments(0,2,payload,[]{return 2u;},invoke)==0&&calls==3);
 Arguments a;a.AppendWord(0x81234567);a.AppendWord(0xfedcba98);
 rejects([&]{a.ReadWord();});
 for(int i=0;i<3;++i){a.ResetCursor();assert(a.ReadWord()==0x81234567&&a.ReadWord()==0xfedcba98);rejects([&]{a.ReadWord();});}
 a.ResetCursor();rejects([&]{a.AppendWord(7);});
 Arguments full;rejects([&]{full.Read(nullptr,0);});
 for(unsigned i=0;i<32;++i)full.AppendWord(i);
 rejects([&]{full.AppendWord(32);});full.ResetCursor();
 for(unsigned i=0;i<32;++i)assert(full.ReadWord()==i);
 if(argc>1){
 std::ifstream in(argv[1]);assert(in);unsigned rows=0;
 for(unsigned count=1;count<=32;++count){Arguments a;
 for(unsigned i=0;i<count;++i)a.AppendWord(0x81234567+i*0x10203);
 for(unsigned cycle=0;cycle<3;++cycle){a.ResetCursor();for(unsigned i=0;i<count;++i){
 unsigned n,c,index,value,cursor,reading;in>>n>>c>>index>>value>>cursor>>reading;
 assert(in&&n==count&&c==cycle&&index==i&&a.ReadWord()==value&&cursor==(i+1)*4&&reading==1);++rows;
 }}}
 assert(rows==1584);
 }
 std::cout<<"event argument repeated callbacks and invalid transitions passed\n";
}

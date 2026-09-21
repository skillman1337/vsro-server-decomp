#include "SR_GameServer/GObjChar.h"
#include <cassert>
#include <iostream>
struct Actor:CGObjChar{bool IsPlayer()const override{return false;}};
struct Probe:ActorAsyncJob{
 unsigned& calls;unsigned& released;Probe(unsigned&c,unsigned&r):calls(c),released(r){}
 ~Probe(){++released;}
 uint32_t Advance(float)override{++calls;return 0;}
 uint8_t Category()const override{return 1;}
 uint32_t Matches(uint32_t,uint32_t)const override{return 1;}
};
int main(){
 unsigned calls=0,released=0;
 {Actor actor;actor.m_MoveState.m_byMoveType=OBJ_MOVE_NONE;
 actor.m_asyncJobs.Append(std::make_unique<Probe>(calls,released));
 actor.OnTick(.1f);assert(calls==0&&released==0);
 actor.OnTick(.2f);assert(calls==1&&released==1&&actor.m_asyncJobs.Size()==0);
 actor.m_asyncJobs.Append(std::make_unique<Probe>(calls,released));
 }
 assert(calls==1&&released==2);
 std::cout<<"actor periodic async dispatch and destruction passed\n";
}

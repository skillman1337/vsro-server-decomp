#include "SR_GameServer/SkillCast.h"
#include "SR_GameServer/GObjChar.h"
#include <algorithm>
#include <fstream>
#include <iostream>

int main(int argc,char** argv) {
 if(argc!=2)return 2;
 std::ifstream in(argv[1]);
 unsigned mode,pairPresent,areaPresent,ovlPresent,result,target,pairAfter,areaAfter,erased,state7,state9,count=0;
 while(in>>mode>>pairPresent>>areaPresent>>ovlPresent>>result>>target>>pairAfter>>areaAfter>>erased>>state7>>state9) {
  CGObjChar actor;actor.m_dwGlobalID=42;
  tagRefSkill ref{};ref.dwPackedStates=7;
  uint32_t ovl=9;if(ovlPresent)ref.SetParam(0x37C,&ovl);
  tagCastLink pair{};pair.m_dwTargetActorID=mode==2?22:0;
  tagAreaLink area{};if(mode==2)area.m_memberIDs={42,43};
  // Deliberately borrowed stack objects: an erroneous delete must fail.
  tagPeriodicDamagePulse pulse{};
  tagSkillExecutionContext ctx{};ctx.m_pRefSkill=&ref;ctx.m_byMode=mode;
  ctx.m_pCastLink=pairPresent?&pair:nullptr;ctx.m_pAreaLink=areaPresent?&area:nullptr;
  ctx.m_pPeriodicDamage=&pulse;
  tagActiveSkillInstance inst{};inst.m_pExecution=&ctx;
  auto* manager=actor.GetSkillManager();manager->ChangeStates(7,false);manager->ChangeStates(9,false);
  auto got=SkillEffect_RetireContributionsAndLinks(&actor,&inst);
  unsigned removed=mode==2 && std::find(area.m_memberIDs.begin(),area.m_memberIDs.end(),42)==area.m_memberIDs.end();
  if(got!=result || pair.m_dwTargetActorID!=target || bool(ctx.m_pCastLink)!=pairAfter ||
   bool(ctx.m_pAreaLink)!=areaAfter || removed!=erased || manager->HasBlockedStates(7)!=bool(state7) ||
   manager->HasBlockedStates(9)!=bool(state9) || ctx.m_pPeriodicDamage!=&pulse) {
   std::cerr<<"native mismatch case "<<count<<"\n";return 1;
  }
  ++count;
 }
 if(!in.eof() || count!=24)return 3;
 std::cout<<count<<" native borrowed-link/state teardown cases passed\n";
}

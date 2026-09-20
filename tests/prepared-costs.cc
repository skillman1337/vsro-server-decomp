#include "SR_GameServer/SkillResourceCost.h"
#include <fstream>
#include <iostream>
struct CostActor : CGObjChar {
    uint32_t current=0; bool player=false; float multiplier=100;
    bool IsPlayer() const override { return player; }
    uint32_t GetCurrentHP() const override { return current; }
    uint32_t GetCurrentMP() const override { return current/2; }
    uint32_t GetMaxHP() const override { return 999999; }
    uint32_t GetMaxMP() const override { return 999999; }
    float GetParamFloat(uint32_t id) const override { return id==0x8D ? multiplier : 0; }
};
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    std::ifstream input(argv[1]);
    unsigned persistent,player,current,percent,hp,mp,berserk,count=0;
    float multiplier;
    while(input>>persistent>>player>>current>>percent>>multiplier>>hp>>mp>>berserk) {
        CostActor actor; actor.player=player;actor.current=current;actor.multiplier=multiplier;
        tagRefSkill ref{};ref.wRequiredHP=3;ref.wRequiredMP=7;
        ref.wConsumeHPRatio=ref.wConsumeMPRatio=percent;ref.byBerserkPointCost=4;
        tagSkillExecutionContext context{};context.m_pRefSkill=&ref;
        ComputePreparedSkillCosts(&actor,&context,persistent);
        if(uint32_t(context.m_nCalculatedHPCost)!=hp || uint32_t(context.m_nCalculatedMPCost)!=mp || context.m_byCalculatedBerserkCost!=berserk) {
            std::cerr<<"native prepared cost mismatch "<<count<<" got "<<uint32_t(context.m_nCalculatedHPCost)<<","<<uint32_t(context.m_nCalculatedMPCost)<<" expected "<<hp<<","<<mp<<'\n';return 1;
        }
        ++count;
    }
    if(!input.eof() || count!=288)return 3;
    std::cout<<count<<" native prepared-cost arithmetic regions passed\n";
}

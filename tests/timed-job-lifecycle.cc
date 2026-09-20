#include "SR_GameServer/TimedJob.h"
#include "SR_GameServer/GObjChar.h"
#include "SR_GameServer/SkillManager.h"
#include "SR_GameServer/Formulae.h"
#include "ServerCommon/ReferenceData.h"
#include <bit>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

struct Probe : CTJ_SkillKeeper {
    unsigned updates = 0;
    bool BuildUpdateQuery(std::string& out) override { ++updates; out="update-probe"; return true; }
    unsigned CheckpointBits() const { return std::bit_cast<unsigned>(m_fCheckpointElapsed); }
    unsigned AccruedBits() const { return std::bit_cast<unsigned>(m_fAccrued); }
    unsigned Guard() const { return m_dwSecondaryGuard; }
};

int main(int argc,char** argv) {
    if(argc!=2) return 2;
    std::ifstream in(argv[1]);
    CGObjChar actor;
    Skill::sSkillPreEngageData command;
    command.m_dwSkillID=100;
    tagRefSkill descriptor{};
    uint32_t nbuf=1;
    descriptor.pNbuf=&nbuf;
    tagSkillExecutionContext context{};
    context.m_pRefSkill=&descriptor;
    context.m_byMode=2; // Voluntary cancellation is prohibited.
    tagActiveSkillInstance instance{};
    instance.m_pCommand=&command;
    instance.m_pExecution=&context;
    instance.m_dwMode=2;
    actor.GetSkillManager()->AddActiveBuff(&instance);
    std::unique_ptr<Probe> job;
    unsigned group,step,initial,dt,remaining,checkpoint,accrued,active,guard,retirement,queries,forced,notices,count=0;
    int reason;
    while(in>>group>>step>>initial>>dt>>reason>>remaining>>checkpoint>>accrued>>active>>guard>>retirement>>queries>>forced>>notices) {
        if(step==0) {
            job=std::make_unique<Probe>();
            auto* record=new CInstanceTimedJob();
            record->SetID(17);record->SetJobID(100);record->SetTimeToKeep(initial);record->SetData2(0);
            job->Initialize(nullptr,&actor,record,1);
            instance.m_dwRetirement=1;
        }
        if(reason!=-1) job->RequestRetirement(reason);
        job->Tick(std::bit_cast<float>(dt));
        if(job->GetRecord()->GetTimeToKeep()!=remaining || job->CheckpointBits()!=checkpoint ||
           job->AccruedBits()!=accrued || job->GetActiveMarker()!=active || job->Guard()!=guard ||
           instance.m_dwRetirement!=retirement || job->updates!=queries) {
            std::cerr<<"native mismatch sequence "<<group<<" step "<<step<<"\n";
            actor.GetSkillManager()->GetActiveBuffs().clear();return 1;
        }
        ++count;
    }
    actor.GetSkillManager()->GetActiveBuffs().clear();
    if(!in.eof() || count!=28) return 3;
    std::string sql;
    Probe query;
    auto* record=new CInstanceTimedJob();record->SetID(17);record->SetTimeToKeep(10);
    query.Initialize(nullptr,nullptr,record,1);
    if(!query.BuildCheckpointQuery(sql) || sql!="update-probe") return 4;
    query.RequestRetirement(1);
    if(!query.BuildCheckpointQuery(sql) || sql!="DELETE _TIMEDJOB WHERE ID = 17") return 5;
    record->SetID(0);sql="unchanged";
    if(query.BuildCheckpointQuery(sql) || sql!="unchanged") return 6;
    std::cout<<count<<" native clock/callback transitions; checkpoint branch checks passed\n";
}

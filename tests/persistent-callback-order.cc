#include "SR_GameServer/SkillCast.h"
#include "SR_GameServer/GObjChar.h"
#include <stdexcept>
#include <iostream>

static void require(bool condition) {
    if (!condition) throw std::runtime_error("persistent callback contract mismatch");
}
struct Observe : Skill::RetirementTrigger {
    CSkillManager* manager;
    bool called = false;
    void Notify(int32_t reason, tagActiveSkillInstance*) override {
        require(reason == 1);
        require(!manager->HasBlockedStates(7));
        require(manager->HasBlockedStates(9));
        require(!manager->GetRealModifiers().Find(0x40, 3));
        called = true;
    }
};
int main() {
    CGObjChar actor;
    tagRefSkill ref{};
    tagSkillExecutionContext context{};
    tagPeriodicDamagePulse pulse{};
    tagActiveSkillInstance instance{};
    context.m_pRefSkill = &ref;
    context.m_byMode = 1;
    context.m_pPeriodicDamage = &pulse;
    instance.m_pExecution = &context;
    instance.m_dwRetirement = 1;
    // Native 585378 dispatches 582750, which returns immediately for target 0.
    // The old invented recovery alias overwrote +8 as a healing timestamp.
    pulse.m_dwStartedAt = 17;
    require(CastLifecycle_TickPersistent(&actor, &instance) == CAST_OUTCOME_KEEP);
    require(pulse.m_dwStartedAt == 17 && pulse.m_dwLastTick == 0);

    auto* manager = actor.GetSkillManager();
    ref.dwPackedStates = 7;
    uint32_t ovl = 9, real[] = {0x40, 80, 3};
    ref.SetParam(0x37C, &ovl);
    ref.SetParam(0x300, real);
    context.m_dwContextID = 111;
    manager->ChangeStates(7, false);
    manager->ChangeStates(9, false);
    manager->UpdateRealModifiers(real, context.m_dwContextID, false);
    auto trigger = std::make_shared<Observe>();
    trigger->manager = manager;
    auto* command = Skill::sSkillPreEngageData::Allocate();
    command->m_retirementTrigger = trigger;
    instance.m_pCommand = command;
    require(SkillEffect_RetireContributionsAndLinks(&actor, &instance) == CAST_OUTCOME_RELEASE_NOTIFY);
    require(trigger->called && !manager->HasBlockedStates(9));
    require(context.m_pPeriodicDamage == &pulse);
    Skill::sSkillPreEngageData::Release(instance.m_pCommand);
    std::cout << "persistent pulse ownership and callback/state ordering passed\n";
}

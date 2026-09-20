#include "SR_GameServer/SkillCast.h"
#include "SR_GameServer/GObjChar.h"
#include "SR_GameServer/MsgBlock.h"
#include "JMX_Library/BSLib/Packet.h"
#include <iostream>
#include <stdexcept>

static void require(bool value) { if (!value) throw std::runtime_error("death retirement sequence mismatch"); }
struct CaptureBlock : CMsgBlock {
    std::vector<std::pair<uint16_t, std::vector<uint8_t>>> packets;
    int32_t IsInside(tagObjLocation) const override { return 1; }
    void SendPacketToLayer(uint16_t, CPacket* p) override {
        packets.emplace_back(p->GetOpcode(), p->m_streamBuffer);
    }
};
struct Trigger : Skill::RetirementTrigger {
    CGObjChar* actor;
    bool called = false;
    void Notify(int32_t reason, tagActiveSkillInstance* instance) override {
        require(reason == 1);
        require(actor->m_paramKeeper.GetParamFloat(5) == 30.f);
        require(actor->GetSkillManager()->GetActiveBuffs().size() == 1);
        require(instance->m_bActive != 0);
        called = true;
    }
};
struct Actor : CGObjChar { bool IsPlayer() const override { return false; } };
int main() {
    CaptureBlock block;
    Actor actor;
    actor.m_pMsgBlock = &block;
    auto* manager = actor.GetSkillManager();
    tagRefSkill buff{}, instant{}, continuous{};
    buff.dwSkillID = 101; buff.dwActionCategory = 3;
    instant.dwSkillID = 102; instant.dwActionCategory = 0;
    continuous.dwSkillID = 103; continuous.dwActionCategory = 3;
    uint32_t cbuf = 1; continuous.SetParam(0x358, &cbuf);
    auto make = [&](tagRefSkill* ref) {
        auto* p = tagActiveSkillInstance::Allocate();
        p->m_pExecution = tagSkillExecutionContext::Allocate();
        p->m_pExecution->m_pRefSkill = ref;
        p->m_pCommand = Skill::sSkillPreEngageData::Allocate();
        manager->AddActiveBuff(p);
        return p;
    };
    auto* attack = make(&instant);
    auto* persistent = make(&buff);
    auto* retained = make(&continuous);
    auto token = persistent->m_pExecution->m_dwContextID;
    auto trigger = std::make_shared<Trigger>(); trigger->actor = &actor;
    persistent->m_pCommand->m_retirementTrigger = trigger;
    actor.m_paramKeeper.SetParamFloat(5, 0, 999, 20.f);
    actor.m_paramKeeper.SetParamFloat(5, 0, reinterpret_cast<uintptr_t>(persistent), 10.f);
    manager->SetCurrentInstance(attack);
    manager->RetireSkillsForDeath(true);
    require(trigger->called);
    require(manager->GetCurrentInstance() == nullptr);
    require(manager->GetActiveBuffs().size() == 1 && manager->GetActiveBuffs().front() == retained);
    require(actor.m_paramKeeper.GetParamFloat(5) == 20.f);
    unsigned retirementPackets = 0;
    for (auto& [opcode, bytes] : block.packets) if (opcode == 0xB072) {
        ++retirementPackets;
        require(bytes.size() == 5 && bytes[0] == 1);
        uint32_t got = uint32_t(bytes[1]) | uint32_t(bytes[2]) << 8 | uint32_t(bytes[3]) << 16 | uint32_t(bytes[4]) << 24;
        require(got == token);
    }
    require(retirementPackets == 1);
    auto before = block.packets.size();
    manager->PublishRetiredBuffs();
    manager->RetireSkillsForDeath(true);
    require(block.packets.size() == before);
    manager->GetActiveBuffs().clear();
    tagActiveSkillInstance::Release(retained);
    actor.m_pMsgBlock = nullptr;
    std::cout << "death retirement callback, conditional wire, cbuf, and duplicate sequences passed\n";
}

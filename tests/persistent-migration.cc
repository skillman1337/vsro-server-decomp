#include "SR_GameServer/SkillCast.h"
#include "SR_GameServer/GObjChar.h"
#include "JMX_Library/BSLib/Packet.h"
#include <windows.h>
#include <cassert>
#include <iostream>

int main() {
    CGObjChar actor;
    tagRefSkill ref{};
    ref.dwSkillID = 0x1234;
    tagSkillExecutionContext context{};
    context.m_pRefSkill = &ref;
    Skill::sSkillPreEngageData command;
    command.m_dwSkillID = 0x1beef;
    tagActiveSkillInstance instance{};
    instance.m_pExecution = &context;
    instance.m_pCommand = &command;
    instance.m_dwMode = PERSISTENT_MODE_TICK;
    for (uint32_t age : {0u, 100u, 0x7ffff000u, 0x80001000u, 0xf0000000u}) {
        BSLib::CPacket packet;
        uint32_t before = GetTickCount();
        instance.m_dwStartTime = before - age;
        assert(CastLifecycle_ProcessPersistent(CAST_EVENT_WRITE_MIGRATION,
            &actor, &instance, &packet) == CAST_OUTCOME_SUCCESS);
        uint32_t after = GetTickCount();
        assert(after - before < 4096); // stay away from the signed boundary
        uint16_t id = 0;
        uint32_t elapsed = 0;
        assert(packet.m_streamBuffer.size() == 6);
        assert(packet.ReadUint16(&id) && id == 0xbeef);
        assert(packet.ReadUint32(&elapsed));
        if (age & 0x80000000u) assert(elapsed == 0);
        else assert(elapsed >= age && elapsed <= age + (after - before));
    }
    instance.m_dwMode = PERSISTENT_MODE_BEGIN;
    BSLib::CPacket inactive;
    CastLifecycle_ProcessPersistent(CAST_EVENT_WRITE_MIGRATION, &actor, &instance, &inactive);
    assert(inactive.m_streamBuffer == std::vector<uint8_t>({0, 0}));
    std::cout << "native migration command identity, signed elapsed and inactive wire shape passed\n";
}

// Portable projections of 4AA410, 4EBE30 and 59E770 in research SR_GameServer.
// Vtable evidence: AF59FC+4E8 -> 4AA410; AF59FC+538 -> 4EBE30.
#include "GObjPC.h"
#include "GItem.h"
#include "SkillManager.h"
#include "Formulae.h"
#include "../ServerCommon/InstanceSkill.h"
#include <algorithm>
#include <stdexcept>

void CGObjPC::ModifyBerserkPoints(int32_t delta, uint8_t reason) {
    if (!delta || (delta > 0 && GetBodyMode() == 1)) return;
    auto* record = dynamic_cast<CInstancePC*>(GetDataPermanent());
    if (!record) throw std::logic_error("berserk mutation requires a player record");
    // Native signed ADD is 32 bits before the signed clamp.
    int32_t next = static_cast<int32_t>(uint32_t(delta) + record->m_byBerserkPoints);
    next = std::clamp(next, 0, 5);
    if (next == record->m_byBerserkPoints) return;
    if (!(g_bPCDBWriteAllowed & 1)) throw std::logic_error("player record mutation disabled");
    record->m_dwStateFlags |= 0x10;
    record->m_byBerserkPoints = static_cast<uint8_t>(next);
    // 4E3FB0 type 4: private points update, not a skill-point transaction.
    auto* packet = AllocMsgForPeer(0x304E);
    if (!packet) throw std::runtime_error("cannot allocate berserk notification");
    packet->WriteUint8(4);
    packet->WriteUint8(record->m_byBerserkPoints);
    packet->WriteUint8(reason);
    SendMsgToPeer(packet);
}

uint8_t CSkillManager::GetSkillMasteryRank(const tagRefSkill* skill) const {
    if (!skill || !skill->Param(0x574)) return 0;
    uint8_t ranks[2] = {1, 1};
    for (unsigned i = 0; i != 2; ++i) {
        if (!skill->dwReqMasteryID[i]) continue;
        auto* entry = FindMastery(skill->dwReqMasteryID[i]);
        ranks[i] = entry && entry->m_pRefRecord ? entry->m_pRefRecord->GetLevel() : 0;
    }
    return std::max(ranks[0], ranks[1]);
}

void CGObjChar::RefreshMovementSpeeds() {
    const auto* player = IsPlayer() ? static_cast<const CGObjPC*>(this) : nullptr;
    const bool riding = player && IsRidingTransport();
    if (riding) {
        m_fWalkSpeed = player->m_pRiddenTransport->m_fWalkSpeed;
        m_fRunSpeed = player->m_pRiddenTransport->m_fRunSpeed;
    } else if (player && player->m_hasMovementSpeedOverride) {
        m_fWalkSpeed = player->m_overrideWalkSpeed;
        m_fRunSpeed = player->m_overrideRunSpeed;
    } else {
        m_fWalkSpeed = GetParamFloat(0x17);
        m_fRunSpeed = GetParamFloat(0x18);
    }
    ApplyMoveSpeed(m_bySpeedMode);
    if (riding) return;
    BSLib::CPacket packet;
    packet.SetOpcode(0x30D0);
    packet.WriteUint32(GetGlobalID());
    packet.Write(&m_fWalkSpeed, 4);
    packet.Write(&m_fRunSpeed, 4);
    SendPacketToNearbySessions(&packet);
}

void CGObjPC::SendParameterStats() {
    // Native computes the attack pair first and applies the currently usable
    // base-attack skill's mastery multiplier before serializing integer stats.
    float attackLow = GetParamFloat(0x0D), attackHigh = GetParamFloat(0x0E);
    if (auto* item = m_storage.GetItem(6)) {
        auto* manager = GetSkillManager();
        const auto* skill = manager->GetSkillData(manager->GetAttackSkillByWeaponTID(item->GetTID().wType));
        if (skill) {
            const long double factor = 1.0L + manager->GetSkillMasteryRank(skill) / 100.0L;
            attackLow = static_cast<float>(attackLow * factor);
            attackHigh = static_cast<float>(attackHigh * factor);
        }
    }
    auto* packet = AllocMsgForPeer(0x303D);
    if (!packet) return;
    const auto wide = [](float value) { return static_cast<uint32_t>(static_cast<int64_t>(value)); };
    const auto narrow = [](float value) { return static_cast<uint16_t>(static_cast<int32_t>(value)); };
    packet->WriteUint32(wide(attackLow));
    packet->WriteUint32(wide(attackHigh));
    for (uint16_t id : {0x0Fu, 0x10u}) packet->WriteUint32(wide(GetParamFloat(id)));
    for (uint16_t id : {5u, 6u, 0x0Bu, 9u}) packet->WriteUint16(narrow(GetParamFloat(id)));
    for (uint16_t id : {3u, 4u}) packet->WriteUint32(wide(GetParamFloat(id)));
    for (uint16_t id : {1u, 2u}) packet->WriteUint16(narrow(GetParamFloat(id)));
    SendMsgToPeer(packet);
}

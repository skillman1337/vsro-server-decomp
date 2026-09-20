#pragma once
#include "GObjChar.h"
#include "SkillManager.h"
#include "Formulae.h"

// Prepared costs differ from admission: 58E1AC uses maximum vitals, but
// 58312C / 5867DC / 58585x snapshot current vitals. Keep the two contracts apart.
inline void ComputePreparedSkillCosts(CGObjChar* actor, tagSkillExecutionContext* context,
                                     bool persistentArithmetic) {
    const auto* ref = context->m_pRefSkill;
    auto ratio = [persistentArithmetic](uint32_t current, uint16_t percent) -> int32_t {
        if (persistentArithmetic)
            return static_cast<int32_t>(static_cast<int64_t>(
                static_cast<long double>(static_cast<int32_t>(current)) *
                (static_cast<long double>(percent) / 100.0L)));
        // Instant/projectile use low-32-bit IMUL and signed division by 100.
        return static_cast<int32_t>(current * uint32_t(percent)) / 100;
    };
    context->m_nCalculatedHPCost = static_cast<int32_t>(uint32_t(ref->wRequiredHP) +
        uint32_t(ratio(actor->GetCurrentHP(), ref->wConsumeHPRatio)));
    context->m_nCalculatedMPCost = static_cast<int32_t>(uint32_t(ref->wRequiredMP) +
        uint32_t(ratio(actor->GetCurrentMP(), ref->wConsumeMPRatio)));
    context->m_byCalculatedBerserkCost = ref->byBerserkPointCost;
    if (actor->IsPlayer()) {
        // Parameter 8D is the remaining percentage (native default 100),
        // not a percentage to subtract from 100. Zero therefore means free.
        context->m_nCalculatedMPCost = static_cast<int32_t>(static_cast<int64_t>(
            uint32_t(context->m_nCalculatedMPCost) *
            (static_cast<long double>(actor->GetParamFloat(0x8D)) / 100.0L)));
    }
    // Projectile's subsequent stochastic owner modifier is a separate branch;
    // the two keyed reductions below belong to instant and persistent actions.
    if (ref->dwActionCategory == 0 || ref->dwActionCategory == 3) {
        for (uint32_t offset : {0x4E4u, 0x554u}) {
            const auto* key = ref->Param(offset);
            const auto* modifier = key ? actor->GetSkillManager()->GetSkillModifier(*key) : nullptr;
            if (modifier) context->m_nCalculatedMPCost = static_cast<int32_t>(static_cast<int64_t>(
                uint32_t(context->m_nCalculatedMPCost) * (1.0L - modifier->dwValue / 100.0L)));
        }
    }
}

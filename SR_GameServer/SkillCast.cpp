/**
 * ============================================================================
 * Silkroad Online - Skill Cast Lifecycle & Action Dispatch Subsystem
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\skill\
 *
 * Implements:
 *   - SkillActionHandler               (Native @ 0x00589B50)
 *   - Skill Action Dispatch Table      (Native @ 0x00C63C7C)
 *   - CastLifecycle_ProcessPersistent  (Native @ 0x005830B0, 9247 bytes)
 *   - SkillEffect_RetireContributionsAndLinks   (Native @ 0x005829D0, 1749 bytes)
 *   - CastLifecycle_ProcessInstant     (Native @ 0x00586700, 2424 bytes)
 *   - CastLifecycle_ProcessProjectile  (Native @ 0x005857B0, 2365 bytes)
 *   - CastLifecycle_ProcessContinuous  (Native @ 0x00587260, 80 bytes)
 * ============================================================================
 */

#include "SkillCast.h"
#include "GObjChar.h"
#include "GObjPC.h"
#include "Game.h"
#include "SkillManager.h"
#include "SkillResourceCost.h"
#include "skill/SkillGlobal.h"
#include "../ServerCommon/ReferenceData.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_Library/BSLib/Packet.h"
#include <cstring>
#include <cmath>
#include <windows.h>

/*
================================================================================
Skill Action Handler Dispatch Table (Native Table @ 0x00C63C7C)
================================================================================
*/
static const PFN_CAST_ACTION_HANDLER g_aCastActionHandlers[8] = {
	&CastLifecycle_ProcessInstant,     // [0] Direct / Instant Attack @ 0x00586700
	&CastLifecycle_ProcessProjectile,  // [1] Projectile / Ranged Attack @ 0x005857B0
	nullptr,                           // [2] Unused / Reserved
	&CastLifecycle_ProcessPersistent,  // [3] Persistent Buff / Area / Stance @ 0x005830B0
	&CastLifecycle_ProcessContinuous,  // [4] Continuous Channeling @ 0x00587260
	nullptr,                           // [5] Unused
	nullptr,                           // [6] Unused
	nullptr                            // [7]
};

/*
================
SkillActionHandler

[RECONSTRUCTED - Native 0x00589B50] (270 bytes)
Master skill action execution dispatcher. Reads action category at +0x168 of
tagRefSkill and delegates to the category lifecycle handler in g_aSkillActionHandlers.
Checks Category 4 macro entry on Event 0 and notifies player with code 4.
================
*/
namespace SkillCast {
int32_t SkillActionHandler(CGObjChar* pCaster, tagActiveSkillInstance* pInstance, uint32_t dwEvent, void* pExtra) {
	if (!pInstance) {
		return CAST_OUTCOME_RELEASE;
	}

	if (!pCaster) {
		return CAST_OUTCOME_RELEASE;
	}

	tagSkillExecutionContext* pExec = pInstance->m_pExecution;
	if (!pExec || !pExec->m_pRefSkill) {
		return CAST_OUTCOME_RELEASE;
	}

	const tagRefSkill* pRefSkill = pExec->m_pRefSkill;
	uint32_t dwActionCategory = pRefSkill->dwActionCategory;

	// Native 0x00589B9E: Category 4 (Continuous) cannot enter via Event 0 (Macro detection)
	if (dwActionCategory == 4 && dwEvent == 0) {
		if (pCaster->IsPlayer()) {
			static_cast<CGObjPC*>(pCaster)->Recall(4);
		}
		BSLib::Log_Printf(0x2000001, "Macro Detected! Skill ID: %d, CharName:%s Handler(%d:%d)",
			pRefSkill->dwSkillID, pCaster->GetName() ? pCaster->GetName() : "", dwActionCategory, dwEvent);
		return CAST_OUTCOME_RELEASE;
	}

	if (dwActionCategory == 3 || dwActionCategory < 7) {
		PFN_CAST_ACTION_HANDLER pfnHandler = g_aCastActionHandlers[dwActionCategory];
		if (pfnHandler != nullptr) {
			return pfnHandler(dwEvent, pCaster, pInstance, pExtra);
		}
	}

	if (pCaster->IsPlayer()) {
		static_cast<CGObjPC*>(pCaster)->Recall(4);
	}
	BSLib::Log_Printf(0x2000001, "ActionList empty! Action ID: %d, CharName:%s",
		dwActionCategory, pCaster->GetName() ? pCaster->GetName() : "");
	return CAST_OUTCOME_RELEASE;
}
}

/*
================
SkillEffect_RetireContributionsAndLinks

[PARTIAL - Native 0x005829D0] (1749 bytes)
Cleans up area aura links, paired companion/parasite links, and unregisters
modifiers when a persistent skill/buff expires or is cancelled.
Audit 2026-09-20: native also handles mode-specific owners, HST2 restoration,
HP/MP bounds and notifications. Recipient detachment and area stop propagation
follow the native ownership split; source paired-task callbacks and final context
release remain incomplete. This body is not a complete native parity reference;
see docs/PARAMETER-AUDIT.md before extending its callers.
================
*/
int32_t SkillEffect_RetireContributionsAndLinks(CGObjChar* pCaster, tagActiveSkillInstance* pInstance) {
	if (!pInstance || !pInstance->m_pExecution) {
		return CAST_OUTCOME_RELEASE_NOTIFY;
	}

	tagSkillExecutionContext* pExec = pInstance->m_pExecution;
	const tagRefSkill* pRefSkill = pExec->m_pRefSkill;

    // 5829D0 does not free descriptors. Recipient contexts borrow them;
    // 5A9A20 releases paired/area descriptors only for source mode 1.
    // Paired descriptors take precedence over area descriptors.
    if (pExec->m_pCastLink != nullptr) {
        if (pExec->m_byMode == 2) {
            pExec->m_pCastLink->m_dwTargetActorID = 0; // 582A0A
            pExec->m_pCastLink = nullptr;
        }
        else if (pRefSkill) {
            auto* target = ObjMgr_FindByID(pExec->m_pCastLink->m_dwTargetActorID);
            if (target && target->GetSkillManager()) {
                auto* effect = target->GetSkillManager()->FindActiveBuffBySkillID(
                    pRefSkill->dwSkillID, pExec->m_pCastLink->m_dwTargetContextID);
                if (effect) {
                    effect->RequestRetirement(true); // 582ACE: force the paired recipient
                    if (effect->m_pExecution) effect->m_pExecution->m_pCastLink = nullptr;
                }
            }
        }
    } else if (pExec->m_pAreaLink != nullptr) {
        tagAreaLink* pArea = pExec->m_pAreaLink;
        if (pExec->m_byMode == 2) {
            if (pCaster) {
                const uint32_t id = pCaster->GetGlobalID();
                pArea->m_memberIDs.erase(id);
            }
            pExec->m_pAreaLink = nullptr;
        } else if (pRefSkill) {
            for (uint32_t id : pArea->m_memberIDs) {
                CGObjChar* target = ObjMgr_FindByID(id);
                if (!target || target == pCaster || !target->GetSkillManager()) continue;
                auto* effect = target->GetSkillManager()->FindActiveBuffBySkillID(pRefSkill->dwSkillID, 0);
                if (!effect) continue;
                effect->RequestRetirement(false); // 582B8F: respects nbuf
                if (effect->m_pExecution) effect->m_pExecution->m_pAreaLink = nullptr;
            }
        }
    }
    // +64 pulse release also belongs to context destruction (5A9A50), not
    // this callback. Its manager owner is cleared by the full native teardown.

	// 4. Uninstall active execution selector mask (+0x3C / +0x1D0)
	if (pExec->m_dwOwnedSelectors != 0) {
		if (pCaster && pCaster->GetSkillManager()) {
			pCaster->GetSkillManager()->InstallSelector(nullptr, pExec->m_dwOwnedSelectors);
		}
		pExec->m_dwOwnedSelectors = 0;
	}

    // Mode 2 uses ovl2 when present; a paired source suppresses packed
    // states. The final ovl2 removal is an independent native operation.
    if (pCaster && pRefSkill && pCaster->GetSkillManager()) {
        uint32_t states = pRefSkill->dwPackedStates;
        if (pExec->m_byMode != 2 && pExec->m_pCastLink) states = 0;
        const uint32_t* ovl2 = pRefSkill->Param(0x37C);
        if (pExec->m_byMode == 2 && ovl2) states = *ovl2;
        if (states) pCaster->GetSkillManager()->ChangeStates(states, true);
    }
    // 582C92..582CBD: remove this context's real contributions in every
    // selected table; this operation is independent of packed state removal.
    if (pCaster && pRefSkill && pCaster->GetSkillManager())
        pCaster->GetSkillManager()->UpdateRealModifiers(pRefSkill->Param(0x300), pExec->m_dwContextID, true);
    // UnregisterModifiers is called once by the manager after this callback
    // (59FF80/59BB80), not a second time from 5829D0.

	// 582Fxx: command-owned trigger, reason 1. Keep a reference during the
	// callback: it may cause scheduler removal and release its own producer.
	if (pInstance->m_pCommand && pInstance->m_pCommand->m_retirementTrigger) {
		auto trigger = pInstance->m_pCommand->m_retirementTrigger;
		trigger->Notify(1, pInstance);
	}
    // 58307B..583093: final ovl2 removal is AFTER the command callback.
    if (pCaster && pRefSkill && pCaster->GetSkillManager()) {
        const auto* ovl2 = pRefSkill->Param(0x37C);
        if (ovl2 && *ovl2) pCaster->GetSkillManager()->ChangeStates(*ovl2, true);
    }
	// Native 0x005829D0 always returns 3 (CAST_OUTCOME_RELEASE_NOTIFY)
	return CAST_OUTCOME_RELEASE_NOTIFY;
}

/*
================
CastLifecycle_BeginPersistent

[RECONSTRUCTED - Native 0x005830B0 sub-flow]
Handles Mode 0 (Begin) for persistent skills. Waits for castDelay, handles
target AoE distribution, persistent spawning, and advances to Mode 1.
================
*/
int32_t CastLifecycle_BeginPersistent(CGObjChar* pCaster, tagActiveSkillInstance* pInstance) {
	if (!pCaster || !pInstance || !pInstance->m_pExecution) {
		return CAST_OUTCOME_RELEASE;
	}

	const tagRefSkill* pRefSkill = pInstance->m_pExecution->m_pRefSkill;
	CSkillManager* pSkillMgr = pCaster->GetSkillManager();

	// Check if action was requested to retire during cast delay
	if (pInstance->m_dwRetirement == 0) {
		if (pSkillMgr != nullptr) {
			pSkillMgr->SendStageEndB071(2, 0, pInstance->m_pExecution->m_dwContextID);
			pSkillMgr->SetCurrentInstance(nullptr);
		}
		return CAST_OUTCOME_RELEASE;
	}

	// Check if castDelay has fully elapsed
	uint32_t dwNow = ::GetTickCount();
	if (dwNow - pInstance->m_dwStartTime <= pRefSkill->dwCastDelay) {
		return CAST_OUTCOME_KEEP;
	}

	if (pSkillMgr != nullptr) {
		pSkillMgr->SetCurrentInstance(nullptr);
	}

	// Target distribution (AoE targets)
	if (pRefSkill->byTargetDistribution != 0 && pInstance->m_pCommand != nullptr) {
		if (!pInstance->m_pCommand->m_vecTargets.empty()) {
			return CastLifecycle_DistributeTargets(pCaster, pInstance, true);
		}
	}

	// Persistent spawn (Efr3 / Totem / Object)
	if (pRefSkill->Param(0x294) != nullptr) {
		return CastLifecycle_SpawnPersistent(pCaster, pInstance);
	}

	if (pRefSkill->pEfr && pRefSkill->byCastType != 1) {
		return CastLifecycle_DistributeTargets(pCaster, pInstance, false);
	}
	// 584019..5841EA: preparation and its recipient are distinct owners.
	// The root releases after mode-1 control; the new instance installs later.
	if (!pSkillMgr) return CAST_OUTCOME_RELEASE;
	if ((pRefSkill->pOvl2 && pSkillMgr->HasBlockedStates(pRefSkill->pOvl2[0])) ||
	    pSkillMgr->HasBlockedStates(pRefSkill->dwPackedStates)) {
		pSkillMgr->SendStageEndB071(2, 0, pInstance->m_pExecution->m_dwContextID);
		return CAST_OUTCOME_RELEASE;
	}
	auto* source = pInstance->m_pExecution;
	pCaster->ConsumeResources(source->m_nCalculatedHPCost, source->m_nCalculatedMPCost, 4);
	source->m_nCalculatedHPCost = source->m_nCalculatedMPCost = source->m_byCalculatedBerserkCost = 0;
	pInstance->m_dwMode = PERSISTENT_MODE_ACTIVATE;
	pSkillMgr->SendStageEndB071(1, 0, source->m_dwContextID);
	auto* effect = tagActiveSkillInstance::Allocate();
	effect->m_dwMode = PERSISTENT_MODE_ACTIVATE;
	effect->m_pCommand = Skill::sSkillPreEngageData::Allocate();
	effect->m_pCommand->m_dwSkillID = pRefSkill->dwSkillID;
	effect->m_pExecution = tagSkillExecutionContext::Allocate();
	effect->m_pExecution->m_pRefSkill = pRefSkill;
	effect->m_pExecution->m_dwDurationBonus = source->m_dwDurationBonus;
	if (const auto* area = pRefSkill->Param(0x290); area && !pRefSkill->Param(0x420)) {
		effect->m_wStatus = 0x3000;
		auto* link = new tagAreaLink{};
		link->m_dwSourceActorID = pCaster->GetGlobalID();
		uint32_t radius = area[2];
		for (uint32_t offset : {0x544u, 0x54Cu}) {
			if (const auto* selector = pRefSkill->Param(offset)) {
				if (const auto* modifier = pSkillMgr->GetSkillModifier(selector[0]))
					radius += modifier->dwValue;
			}
		}
		link->m_fRadius = static_cast<float>(radius);
		if (area[5] & 1) link->m_memberIDs.insert(pCaster->GetGlobalID());
		effect->m_pExecution->m_pAreaLink = link;
	}
	pSkillMgr->AddActiveSkill(effect);
	pSkillMgr->SetCurrentInstance(effect);
	return CAST_OUTCOME_RELEASE;
}

/*
================
CastLifecycle_ActivatePersistent

[RECONSTRUCTED - Native 0x005830B0 sub-flow]
Handles Mode 1 (Activation) for persistent skills. Validates blocked states,
registers modifiers, attaches area links, and advances to Mode 2 (Ticking).
================
*/
int32_t CastLifecycle_ActivatePersistent(CGObjChar* pCaster, tagActiveSkillInstance* pInstance) {
	if (!pCaster || !pInstance || !pInstance->m_pExecution) {
		return CAST_OUTCOME_RELEASE;
	}

	const tagRefSkill* pRefSkill = pInstance->m_pExecution->m_pRefSkill;
	tagSkillExecutionContext* pExec = pInstance->m_pExecution;
	CSkillManager* pSkillMgr = pCaster->GetSkillManager();

	if (pSkillMgr != nullptr) {
		pSkillMgr->SetCurrentInstance(nullptr);
	}

	pInstance->m_dwStartTime = ::GetTickCount();

	if (pRefSkill->Param(0x274) && pCaster->GetMotionState() == 0x0F)
		return CAST_OUTCOME_RELEASE;
	uint32_t states = pRefSkill->dwPackedStates;
	if (pRefSkill->Param(0x370) && pExec->m_byMode == 1) {
		states = 0;
		if (pSkillMgr) pSkillMgr->SendLinkedEffectB0BE(pExec->m_pCastLink, pRefSkill);
	} else if (pSkillMgr) {
		if (pSkillMgr->HasBlockedStates(states)) return CAST_OUTCOME_RELEASE;
		if (pExec->m_byMode == 2) {
			if (pRefSkill->pOvl2) states = pRefSkill->pOvl2[0];
			else if (pRefSkill->Param(0x424)) states = 0;
		}
		pSkillMgr->SendEffectAddedB0BD(pInstance);
	}

	// Install execution selector and apply packed state bits (Native 0x00584AEB, 0x00584AF8)
	if (pSkillMgr != nullptr) {
		if (pRefSkill->pReqc != nullptr && pRefSkill->pReqc[0]) {
			pSkillMgr->InstallSelector(pInstance->m_pExecution, pRefSkill->pReqc[0]);
		}
		if (states) pSkillMgr->ChangeStates(states, false);
		if (pRefSkill->pOvl2 && pRefSkill->pOvl2[0]) pSkillMgr->ChangeStates(pRefSkill->pOvl2[0], false);
	}

	// Register parameter modifiers
	if (pSkillMgr != nullptr) {
		pSkillMgr->RegisterModifiers(pRefSkill);
	}

	if (!pRefSkill->Param(0x420) && pRefSkill->MatchesExecutionSelector()) {
		pInstance->m_pSecondaryCommand = Skill::sSkillPreEngageData::Allocate();
		pInstance->m_pSecondaryExecution = tagSkillExecutionContext::Allocate();
	}
	if (pRefSkill->Param(0x57C)) {
		pExec->m_dwRampStartedAt = ::GetTickCount();
		pExec->m_dwRampCount = 0;
	}
	SkillCombat_EngageSkill(pCaster, pInstance);
	// Advance to Mode 2: Ticking buff / aura
	pInstance->m_dwMode = PERSISTENT_MODE_TICK;
	if (pInstance->m_pCommand && pInstance->m_pCommand->m_retirementTrigger) {
		auto trigger = pInstance->m_pCommand->m_retirementTrigger;
		trigger->Notify(0, pInstance);
	}
	return CAST_OUTCOME_KEEP;
}

/*
================
CastLifecycle_TickPersistent

[RECONSTRUCTED - Native 0x005830B0 sub-flow]
Handles Mode 2 (Ticking) for persistent skills. Checks duration expiry,
dispatches the native +64 periodic-damage descriptor, and retires expired effects.
================
*/
int32_t CastLifecycle_TickPersistent(CGObjChar* pCaster, tagActiveSkillInstance* pInstance) {
	if (!pCaster || !pInstance || !pInstance->m_pExecution) {
		return CAST_OUTCOME_RELEASE;
	}

	// Check manual retirement / cancellation
	if (pInstance->m_dwRetirement == 0) {
		return SkillEffect_RetireContributionsAndLinks(pCaster, pInstance);
	}

	const tagRefSkill* pRefSkill = pInstance->m_pExecution->m_pRefSkill;
	uint32_t dwNow = ::GetTickCount();

	// 585172..5851CB: Efr3 bypasses the ordinary duration check. A command
	// with BOTH override bits uses +18; otherwise Dura is ref+280, not +298.
	// Native compares unsigned elapsed strictly greater, including duration 0.
	const auto* command = pInstance->m_pCommand;
	const uint32_t elapsed = dwNow - pInstance->m_dwStartTime;
	if (!pRefSkill->Param(0x294)) {
		bool expired = false;
		if (command && (command->m_byTargetFlags & 0x0C) == 0x0C) {
			expired = elapsed > command->m_dwDuration;
		} else if (!pRefSkill->Param(0x284) && pRefSkill->Param(0x280)) {
			const uint32_t duration = pRefSkill->Param(0x280)[0] + pInstance->m_pExecution->m_dwDurationBonus;
			expired = elapsed > duration;
		}
		if (expired) return SkillEffect_RetireContributionsAndLinks(pCaster, pInstance);
	}

	// Pulse periodic damage (Summ @ +0x308)
	if (pInstance->m_pExecution->m_pPeriodicDamage != nullptr) {
		Skill_ProcessPeriodicDamage(pCaster, pInstance);
	}

	return CAST_OUTCOME_KEEP;
}

/*
================
CastLifecycle_DistributeTargets

[RECONSTRUCTED - Native 0x005830B0 @ 0x00583610 - 0x00583790] (approx 450 bytes)
Handles distributing persistent effect / debuff / link to selected command targets:
1. Validates non-empty m_vecTargets in command.
2. If Efr/Efr2 shape parameter is present, validates with TargetSelection_DispatchByShape (mode 2).
   If shape dispatch fails, broadcasts stage end cancellation (0xB071) and releases instance.
3. Consumes calculated HP and MP resource costs on caster via ConsumeResources.
4. Broadcasts stage end completion (0xB071).
5. Loops across valid target entities, checks character status and buff acceptance,
   allocates active skill instances, registers active buffs, and broadcasts 0xB0BD.
6. Advances instance to Mode 1 (PERSISTENT_MODE_ACTIVATE).
================
*/
int32_t CastLifecycle_DistributeTargets(CGObjChar* pCaster, tagActiveSkillInstance* pInstance, bool bMainDistribution) {
	(void)bMainDistribution;
	if (!pCaster || !pInstance || !pInstance->m_pExecution) {
		return CAST_OUTCOME_RELEASE;
	}

	tagSkillExecutionContext* pExec = pInstance->m_pExecution;
	const tagRefSkill* pRefSkill = pExec->m_pRefSkill;
	Skill::sSkillPreEngageData* pCommand = pInstance->m_pCommand;
	CSkillManager* pSkillMgr = pCaster->GetSkillManager();

	if (!pRefSkill || !pCommand || pCommand->m_vecTargets.empty()) {
		pInstance->m_dwMode = PERSISTENT_MODE_ACTIVATE;
		return CastLifecycle_ActivatePersistent(pCaster, pInstance);
	}

	// Native 0x00583629: Validate targets via shape dispatcher if Efr is attached
	const uint32_t* pEfr = pRefSkill->pEfr;
	if (pEfr != nullptr) {
		uint16_t wShapeErr = TargetSelection_DispatchByShape(pCaster, pInstance, pEfr, 2, pRefSkill);
		if (wShapeErr != 0) {
			if (pSkillMgr != nullptr) {
				pSkillMgr->SendStageEndB071(2, 0, pExec->m_dwContextID);
			}
			return CAST_OUTCOME_RELEASE;
		}
	}

	// Native 0x00583689: Consume calculated HP/MP resources on caster
	pCaster->ConsumeResources(pExec->m_nCalculatedHPCost, pExec->m_nCalculatedMPCost, 4);
	pExec->m_nCalculatedHPCost = 0;
	pExec->m_nCalculatedMPCost = 0;

	// Native 0x005836C2: Broadcast stage end completion 0xB071
	if (pSkillMgr != nullptr) {
		bool bHasTargets = (pRefSkill->dwTargetCount > 0);
		pSkillMgr->SendStageEndB071(1, 0, pExec->m_dwContextID, bHasTargets);
	}

	// 58373B validates each recipient before allocating independent storage.
	// A rejected/vanished recipient does not manufacture a self effect.
	for (const auto& targetEntry : pCommand->m_vecTargets) {
		CGObjChar* target = ObjMgr_FindByID(targetEntry.dwGlobalID);
		if (!target || !target->IsCharacter()) continue;
		auto* manager = target->GetSkillManager();
		if (!manager || !manager->ValidateBuffReplacement(pRefSkill, pCaster)) continue;
		auto* effect = tagActiveSkillInstance::Allocate();
		effect->m_pCommand = Skill::sSkillPreEngageData::Allocate();
		effect->m_pCommand->m_dwSkillID = pRefSkill->dwSkillID;
		effect->m_pExecution = tagSkillExecutionContext::Allocate();
		effect->m_pExecution->m_pRefSkill = pRefSkill;
		effect->m_pExecution->m_byMode = target == pCaster ? 1 : 2;
		effect->m_pExecution->m_dwDurationBonus = pExec->m_dwDurationBonus;
		effect->m_dwMode = PERSISTENT_MODE_ACTIVATE;
		effect->m_dwStartTime = ::GetTickCount();
		effect->m_wStatus = 0x3000;
		// 5839AD..583B0C creates a source instance for each linked recipient.
		if (pRefSkill->Param(0x370)) {
			auto* source = tagActiveSkillInstance::Allocate();
			source->m_pCommand = Skill::sSkillPreEngageData::Allocate();
			source->m_pCommand->m_dwSkillID = pRefSkill->dwSkillID;
			source->m_pExecution = tagSkillExecutionContext::Allocate();
			source->m_pExecution->m_pRefSkill = pRefSkill;
			source->m_pExecution->m_dwDurationBonus = pExec->m_dwDurationBonus;
			source->m_dwMode = PERSISTENT_MODE_ACTIVATE;
			source->m_dwStartTime = ::GetTickCount();
			source->m_wStatus = 0x3000;
			auto* link = new tagCastLink();
			link->m_dwSourceActorID = pCaster->GetGlobalID();
			link->m_dwTargetActorID = target->GetGlobalID();
			link->m_dwSourceContextID = source->m_pExecution->m_dwContextID;
			link->m_dwTargetContextID = effect->m_pExecution->m_dwContextID;
			if (target->IsPlayer() && target->GetName()) link->m_strTargetName = target->GetName();
			source->m_pExecution->m_pCastLink = link;
			effect->m_pExecution->m_pCastLink = link;
			pSkillMgr->AddActiveSkill(source);
		}
		manager->AddActiveSkill(effect);
	}

	return CAST_OUTCOME_RELEASE;
}

/*
================
CastLifecycle_SpawnPersistent

[RECONSTRUCTED - Native 0x005830B0 @ 0x00584049 - 0x00584210] (approx 350 bytes)
Spawns persistent world object / totem (Efr3):
1. Consumes calculated resource costs on caster.
2. Broadcasts stage end completion (0xB071).
3. Engages skill execution with combat subsystem.
4. Advances instance to Mode 1 (PERSISTENT_MODE_ACTIVATE).
================
*/
int32_t CastLifecycle_SpawnPersistent(CGObjChar* pCaster, tagActiveSkillInstance* pInstance) {
	if (!pCaster || !pInstance || !pInstance->m_pExecution) {
		return CAST_OUTCOME_RELEASE;
	}

	tagSkillExecutionContext* pExec = pInstance->m_pExecution;
	CSkillManager* pSkillMgr = pCaster->GetSkillManager();

	// Native 0x0058405A: Consume calculated HP/MP resources on caster
	pCaster->ConsumeResources(pExec->m_nCalculatedHPCost, pExec->m_nCalculatedMPCost, 4);
	pExec->m_nCalculatedHPCost = 0;
	pExec->m_nCalculatedMPCost = 0;

	// Native 0x0058408A: Broadcast stage end completion 0xB071
	if (pSkillMgr != nullptr) {
		pSkillMgr->SendStageEndB071(1, 0, pExec->m_dwContextID);
	}

	// Native 0x00584210: Engage skill
	SkillCombat_EngageSkill(pCaster, pInstance);

	// Advance to Mode 1
	pInstance->m_dwMode = PERSISTENT_MODE_ACTIVATE;
	return CastLifecycle_ActivatePersistent(pCaster, pInstance);
}

/*
================
CastLifecycle_ProcessPersistent

[PARTIAL - Native 0x005830B0] (9247 bytes)
Action Category 3: Persistent & Buff Cast Handler
Dispatches multi-phase events across Begin, Tick, Migration, and Cancellation.
================
*/
int32_t CastLifecycle_ProcessPersistent(uint32_t dwEntryMode, CGObjChar* pCaster, tagActiveSkillInstance* pInstance, void* pExtra) {
	if (!pCaster || !pInstance || !pInstance->m_pExecution) {
		return CAST_OUTCOME_RELEASE;
	}

	const tagRefSkill* pRefSkill = pInstance->m_pExecution->m_pRefSkill;
	CSkillManager* pSkillMgr = pCaster->GetSkillManager();

	switch (dwEntryMode) {
		case CAST_EVENT_BEGIN: { // 0
			// Native 0x005830EE: Mask 0x1F, widened to 0x5F if matches execution selector
			uint32_t dwValidateMask = 0x1F;
			if (pRefSkill->MatchesExecutionSelector()) {
				dwValidateMask = 0x5F;
			}

			uint16_t wError = Skill_ValidateCast(pInstance, pCaster, dwValidateMask);
			if (wError != SKILL_SUCCESS) {
				if (pSkillMgr != nullptr) {
					pSkillMgr->SendSkillErrorResponseB070(wError);
				}
				return CAST_OUTCOME_RELEASE;
			}

			ComputePreparedSkillCosts(pCaster, pInstance->m_pExecution, true);

			// Initialize instance state
			pInstance->m_bActive = 1;
			pInstance->m_wStatus = 0x3000;
			pInstance->m_dwStartTime = ::GetTickCount();
			pInstance->m_dwMode = PERSISTENT_MODE_BEGIN;
			pInstance->m_dwRetirement = 1;

			// Native 0x00583567: SendCastBeginB070 to nearby sessions
			if (pSkillMgr != nullptr) {
				pSkillMgr->SendCastBeginB070(pInstance, pInstance->m_wStatus);
			}

			// If skill has castDelay, keep current instance
			if (pRefSkill->dwCastDelay > 0) {
				if (pSkillMgr != nullptr) {
					pSkillMgr->AddActiveSkill(pInstance); // 0x00583537
					pSkillMgr->SetCurrentInstance(pInstance);
				}
				return CAST_OUTCOME_KEEP;
			}

			// Fallthrough to Begin / Activate
			return CastLifecycle_BeginPersistent(pCaster, pInstance);
		}

		case CAST_EVENT_TICK: { // 2
			switch (pInstance->m_dwMode) {
				case PERSISTENT_MODE_BEGIN:
					return CastLifecycle_BeginPersistent(pCaster, pInstance);
				case PERSISTENT_MODE_ACTIVATE:
					return CastLifecycle_ActivatePersistent(pCaster, pInstance);
				case PERSISTENT_MODE_TICK:
					return CastLifecycle_TickPersistent(pCaster, pInstance);
				default:
					return CAST_OUTCOME_KEEP;
			}
		}

		case CAST_EVENT_WRITE_MIGRATION: { // 4
			BSLib::CPacket* pOutput = static_cast<BSLib::CPacket*>(pExtra);
			if (pOutput != nullptr) {
				if (pInstance->m_dwMode != PERSISTENT_MODE_TICK) {
					pOutput->WriteUint16(0);
				} else {
					// 58539D..5853B2 reads the command identity, not ref+4.
					pOutput->WriteUint16(static_cast<uint16_t>(pInstance->m_pCommand->m_dwSkillID));
					uint32_t dwNow = ::GetTickCount();
					// 5853B7..5853C6 subtracts modulo 2^32, then tests SF.
					// Comparing absolute ticks loses elapsed time across wrap.
					uint32_t dwElapsed = dwNow - pInstance->m_dwStartTime;
					if (dwElapsed & 0x80000000u) dwElapsed = 0;
					pOutput->WriteUint32(dwElapsed);
				}
			}
			return CAST_OUTCOME_SUCCESS;
		}

		case CAST_EVENT_READ_MIGRATION: { // 5
			BSLib::CPacket* pInput = static_cast<BSLib::CPacket*>(pExtra);
			if (pInput != nullptr) {
				uint32_t dwElapsed = 0;
				pInput->ReadUint32(&dwElapsed);

				pInstance->m_dwMode = PERSISTENT_MODE_TICK;
				pInstance->m_dwStartTime = ::GetTickCount() - dwElapsed;
				if (pSkillMgr != nullptr) {
					pSkillMgr->AddActiveBuff(pInstance);
				}
			}
			return CAST_OUTCOME_SUCCESS;
		}

		case CAST_EVENT_CANCEL: { // 6
			SkillEffect_RetireContributionsAndLinks(pCaster, pInstance);
			return CAST_OUTCOME_RELEASE_NOTIFY;
		}

		default:
			// Native 0x005854BE: Case 1, 3, or > 6 trigger ServerFramework_GenerateMiniDump() and return 1
			BSLib::GenerateMiniDump("SkillCast.cpp", __LINE__);
			return CAST_OUTCOME_SUCCESS;
	}
}

/*
================
CastLifecycle_ProcessInstant

[RECONSTRUCTED - Native 0x00586700] (2424 bytes)
Action Category 0: Direct / Instant Attack Handler
Handles:
  - Event 0 (Begin): Prerequisites check, HP/MP percentage & static cost calculation,
    delay queue vs zero-delay immediate resolution, cooldown registration, packet 0xB070.
  - Event 2 (Tick): Cast delay elapsed check, validation (status 8), packet 0xB071
    stage end broadcast, position effect dispatch, target engage, combat damage.
  - Event 6 (Cancel): Cancellation packet broadcast (0xB071 cancel), cleanup.
  - Unhandled events: returns 4 (proven @ 0x0058674F).
================
*/
int32_t CastLifecycle_ProcessInstant(uint32_t dwEntryMode, CGObjChar* pCaster, tagActiveSkillInstance* pInstance, void* pExtra) {
	(void)pExtra;
	if (!pCaster || !pInstance) {
		return CAST_OUTCOME_RELEASE;
	}

	tagSkillExecutionContext* pExec = pInstance->m_pExecution;
	if (!pExec || !pExec->m_pRefSkill) {
		return CAST_OUTCOME_RELEASE;
	}

	const tagRefSkill* pRefSkill = pExec->m_pRefSkill;
	CSkillManager* pSkillMgr = pCaster->GetSkillManager();
	if (!pSkillMgr) {
		return CAST_OUTCOME_RELEASE;
	}

	uint32_t dwContextID = pExec->m_dwContextID;

	// Event 6: Cancel
	if (dwEntryMode == CAST_EVENT_CANCEL) {
		pSkillMgr->SendStageEndB071(1, 0, dwContextID);
		if (pSkillMgr->GetCurrentInstance() == pInstance) {
			pSkillMgr->SetCurrentInstance(nullptr);
		}
		return CAST_OUTCOME_RELEASE;
	}

	if (dwEntryMode != CAST_EVENT_BEGIN && dwEntryMode != CAST_EVENT_TICK) {
		// Native 0x0058674F returns 4 for unhandled events
		return 4;
	}

	// Event 0: Begin
	if (dwEntryMode == CAST_EVENT_BEGIN) {
		uint16_t wError = Skill_ValidateCast(pInstance, pCaster, 0xFFFF);
		if (wError != 0) {
			pSkillMgr->SendSkillErrorResponseB070(wError);
			if (pSkillMgr->GetCurrentInstance() == pInstance) {
				pSkillMgr->SetCurrentInstance(nullptr);
			}
			return CAST_OUTCOME_RELEASE;
		}

		ComputePreparedSkillCosts(pCaster, pExec, false);

		// If castDelay == 0 (Instantaneous zero-cast delay skill)
		if (pRefSkill->dwCastDelay == 0) {
			if (pRefSkill->pEfr != nullptr) {
				if (TargetSelection_DispatchByShape(pCaster, pInstance, pRefSkill->pEfr, 0, pRefSkill) != 0) {
					if (pSkillMgr->GetCurrentInstance() == pInstance) {
						pSkillMgr->SetCurrentInstance(nullptr);
					}
					return CAST_OUTCOME_RELEASE;
				}
			}

			// 0x00586B9B: the hits are rolled before the cast is announced, so the 0xB070 packet already
			// carries the result batch the recipients step then applies
			SkillCombat_CalculateHitOutcome(pCaster, pInstance->m_pCommand, pExec);

			// Apply position effect (teleport, rush) if present
			if (pRefSkill->pTele || pRefSkill->pTel2 || pRefSkill->pTel3) {
				Skill_ApplyPositionEffect(pRefSkill->pTele, pCaster, pInstance, pRefSkill->pTel2, pRefSkill->pTel3);
			}

			pInstance->m_wStatus = 0x3002;
			pInstance->m_dwStartTime = ::GetTickCount();

			if (pRefSkill->dwCooldown > 0 && pCaster->IsPlayer()) {
				pCaster->RegisterSkillCooldownAndTimer(pRefSkill);
			}

			pSkillMgr->SendCastBeginB070(pInstance, pInstance->m_wStatus);
			SkillCombat_EngageSkill(pCaster, pInstance);

			if (pSkillMgr->GetCurrentInstance() == pInstance) {
				pSkillMgr->SetCurrentInstance(nullptr);
			}
			return CAST_OUTCOME_RELEASE;
		}

		pInstance->m_wStatus = 0x3002;
		pInstance->m_dwStartTime = ::GetTickCount();

		if (pRefSkill->dwCooldown > 0 && pCaster->IsPlayer()) {
			pCaster->RegisterSkillCooldownAndTimer(pRefSkill);
		}

		pSkillMgr->SendCastBeginB070(pInstance, pInstance->m_wStatus);
		pInstance->m_dwMode = 0;
		pSkillMgr->SetCurrentInstance(pInstance);
		pSkillMgr->AddActiveSkill(pInstance);
		return CAST_OUTCOME_KEEP;
	}

	// Event 2: Tick
	if (dwEntryMode == CAST_EVENT_TICK) {
		if (pRefSkill->dwCastDelay > 0) {
			if (pInstance->m_dwRetirement == 0) {
				pSkillMgr->SendStageEndB071(1, 0, dwContextID);
				if (pSkillMgr->GetCurrentInstance() == pInstance) {
					pSkillMgr->SetCurrentInstance(nullptr);
				}
				return CAST_OUTCOME_RELEASE;
			}
			if (Skill_ValidateCast(pInstance, pCaster, 0x1C) != 0) {
				pSkillMgr->SendStageEndB071(1, 0, dwContextID);
				if (pSkillMgr->GetCurrentInstance() == pInstance) {
					pSkillMgr->SetCurrentInstance(nullptr);
				}
				return CAST_OUTCOME_RELEASE;
			}
		}

		if (pRefSkill->dwCastDelay > 0 && (::GetTickCount() - pInstance->m_dwStartTime <= pRefSkill->dwCastDelay)) {
			return CAST_OUTCOME_KEEP; // Still in cast delay
		}

		if (pRefSkill->dwCastDelay == 0) {
			pSkillMgr->SendStageEndB071(1, 0, dwContextID);
		} else {
			// 0x00586DA4: the action stage rolls its own result, so the flags start clean
			pExec->m_dwResultFlags = 0;

			if (Skill_ValidateCast(pInstance, pCaster, 8) != 0) {
				pSkillMgr->SendStageEndB071(1, 0, dwContextID);
				if (pSkillMgr->GetCurrentInstance() == pInstance) {
					pSkillMgr->SetCurrentInstance(nullptr);
				}
				return CAST_OUTCOME_RELEASE;
			}

			if (pRefSkill->pEfr != nullptr) {
				if (TargetSelection_DispatchByShape(pCaster, pInstance, pRefSkill->pEfr, 0, pRefSkill) != 0) {
					if (pSkillMgr->GetCurrentInstance() == pInstance) {
						pSkillMgr->SetCurrentInstance(nullptr);
					}
					return CAST_OUTCOME_RELEASE;
				}
			}

			// 0x00586E98: same order as the instant path - roll the hits, then move, then announce
			SkillCombat_CalculateHitOutcome(pCaster, pInstance->m_pCommand, pExec);

			// Apply position effect (teleport, rush) if present
			if (pRefSkill->pTele || pRefSkill->pTel2 || pRefSkill->pTel3) {
				Skill_ApplyPositionEffect(pRefSkill->pTele, pCaster, pInstance, pRefSkill->pTel2, pRefSkill->pTel3);
			}

			// 0x00586FDD - 0x00587005: the action packet carries the hits the stage rolled
			pSkillMgr->SendActionStageB071(pInstance);
			SkillCombat_EngageSkill(pCaster, pInstance);
		}

		if (pSkillMgr->GetCurrentInstance() == pInstance) {
			pSkillMgr->SetCurrentInstance(nullptr);
		}
		return CAST_OUTCOME_RELEASE;
	}

	return CAST_OUTCOME_SUCCESS;
}

/*
================
CastLifecycle_ProcessProjectile

[RECONSTRUCTED - Native 0x005857B0] (2365 bytes)
Action Category 1: Projectile / Ranged Attack Handler
Handles:
  - Event 0 (Begin): Prerequisites check, projectile ammo consumption (0x3201),
    range/travel trajectory initialization, packet 0xB070.
  - Event 2 (Tick): Mode 0 (Aiming / cast delay) vs Mode 1 (In-flight travel timer
    using tagSkillTravelTimer @ 0x005AA2D0), packet 0xB071, destination impact.
  - Event 6 (Cancel): Cancellation packet broadcast (0xB071 cancel), cleanup.
================
*/
int32_t CastLifecycle_ProcessProjectile(uint32_t dwEntryMode, CGObjChar* pCaster, tagActiveSkillInstance* pInstance, void* pExtra) {
	(void)pExtra;
	if (!pCaster || !pInstance) {
		return CAST_OUTCOME_RELEASE;
	}

	tagSkillExecutionContext* pExec = pInstance->m_pExecution;
	if (!pExec || !pExec->m_pRefSkill) {
		return CAST_OUTCOME_RELEASE;
	}

	const tagRefSkill* pRefSkill = pExec->m_pRefSkill;
	Skill::sSkillPreEngageData* pCommand = pInstance->m_pCommand;
	CSkillManager* pSkillMgr = pCaster->GetSkillManager();
	if (!pSkillMgr) {
		return CAST_OUTCOME_RELEASE;
	}

	uint32_t dwContextID = pExec->m_dwContextID;

	// Event 6: Cancel
	if (dwEntryMode == CAST_EVENT_CANCEL) {
		pSkillMgr->SendStageEndB071(1, 0, dwContextID);
		if (pSkillMgr->GetCurrentInstance() == pInstance) {
			pSkillMgr->SetCurrentInstance(nullptr);
		}
		return CAST_OUTCOME_RELEASE;
	}

	if (dwEntryMode != CAST_EVENT_BEGIN && dwEntryMode != CAST_EVENT_TICK) {
		// Native 0x00585C54: Invalid events outside [0, 2, 6] trigger ServerFramework_GenerateMiniDump() and return 1
		BSLib::GenerateMiniDump("SkillCast.cpp", __LINE__);
		return CAST_OUTCOME_SUCCESS;
	}

	// Event 0: Begin
	if (dwEntryMode == CAST_EVENT_BEGIN) {
		uint16_t wError = Skill_ValidateCast(pInstance, pCaster, 0xFFFF);
		if (wError != 0) {
			pSkillMgr->SendSkillErrorResponseB070(wError);
			if (pSkillMgr->GetCurrentInstance() == pInstance) {
				pSkillMgr->SetCurrentInstance(nullptr);
			}
			return CAST_OUTCOME_RELEASE;
		}

		ComputePreparedSkillCosts(pCaster, pExec, false);

		// Consume projectile ammo if PC is using bow/crossbow
		if (pCaster->IsPlayer()) {
			pCaster->ConsumeAmmo(1);
		}

		pInstance->m_wStatus = 0x3002;
		pInstance->m_dwStartTime = ::GetTickCount();
		pSkillMgr->AddActiveSkill(pInstance);

		if (pRefSkill->dwCooldown > 0 && pCaster->IsPlayer()) {
			pCaster->RegisterSkillCooldownAndTimer(pRefSkill);
		}

		pSkillMgr->SendCastBeginB070(pInstance, pInstance->m_wStatus);

		if (pRefSkill->dwCastDelay == 0) {
			SkillCombat_EngageSkill(pCaster, pInstance);
			if (pSkillMgr->GetCurrentInstance() == pInstance) {
				pSkillMgr->SetCurrentInstance(nullptr);
			}
			return CAST_OUTCOME_RELEASE;
		}

		pInstance->m_dwMode = 0;
		pSkillMgr->SetCurrentInstance(pInstance);
		return CAST_OUTCOME_KEEP;
	}

	// Event 2: Tick
	if (dwEntryMode == CAST_EVENT_TICK) {
		// Mode 1: Projectile in-flight (Skill::sBowShotResult active)
		if (pInstance->m_dwMode == 1) {
			tagSkillTravelTimer* pTravel = pExec->m_pTravelTimer;
			if (!pTravel) {
				return CAST_OUTCOME_RELEASE;
			}
			if (::GetTickCount() - pTravel->dwStartedAt <= pTravel->dwDuration) {
				return CAST_OUTCOME_KEEP; // Still flying!
			}
			// Flight completed -> Impact!
			delete pTravel;
			pExec->m_pTravelTimer = nullptr;
			return CAST_OUTCOME_RELEASE;
		}

		if (pInstance->m_dwMode != 0) {
			return CAST_OUTCOME_KEEP;
		}

		// Mode 0: Cast delay
		if (Skill_ValidateCast(pInstance, pCaster, 0x1C) != 0) {
			pSkillMgr->SendStageEndB071(1, 0, dwContextID);
			if (pSkillMgr->GetCurrentInstance() == pInstance) {
				pSkillMgr->SetCurrentInstance(nullptr);
			}
			return CAST_OUTCOME_RELEASE;
		}

		if (::GetTickCount() - pInstance->m_dwStartTime <= pRefSkill->dwCastDelay) {
			return CAST_OUTCOME_KEEP; // Still drawing bow / aiming
		}

		if (Skill_ValidateCast(pInstance, pCaster, 8) != 0) {
			pSkillMgr->SendStageEndB071(1, 0, dwContextID);
			if (pSkillMgr->GetCurrentInstance() == pInstance) {
				pSkillMgr->SetCurrentInstance(nullptr);
			}
			return CAST_OUTCOME_RELEASE;
		}

		// Resolve target
		uint32_t dwTargetID = (pCommand && !pCommand->m_vecTargets.empty()) ? pCommand->m_vecTargets[0].dwGlobalID : (pCommand ? pCommand->m_dwTargetObjID : 0);
		CGObjChar* pTarget = g_pGame ? g_pGame->FindObjectByID(dwTargetID) : nullptr;
		if (!pTarget) {
			pSkillMgr->SendStageEndB071(1, 0, dwContextID);
			if (pSkillMgr->GetCurrentInstance() == pInstance) {
				pSkillMgr->SetCurrentInstance(nullptr);
			}
			return CAST_OUTCOME_RELEASE;
		}

		// Allocate Skill::sBowShotResult travel timer and compute flight duration
		tagSkillTravelTimer* pTravel = new tagSkillTravelTimer();
		float fDist = std::hypot(pTarget->m_fPosX - pCaster->m_fPosX, pTarget->m_fPosZ - pCaster->m_fPosZ);
		float fSpeed = static_cast<float>(pRefSkill->wProjectileSpeed);
		pTravel->dwStartedAt = ::GetTickCount();
		pTravel->dwDuration = (fSpeed <= 0.0f) ? 0 : static_cast<uint32_t>((static_cast<double>(fDist) * 1000.0) / fSpeed);
		pExec->m_pTravelTimer = pTravel;
		pInstance->m_dwMode = 1; // Transition to flight mode!

		if (pRefSkill->dwCastDelay > 0) {
			pSkillMgr->SendStageEndB071(1, 0, dwContextID, true);
		}

		SkillCombat_EngageSkill(pCaster, pInstance);

		// Caster is freed from active instance while projectile travels
		if (pSkillMgr->GetCurrentInstance() == pInstance) {
			pSkillMgr->SetCurrentInstance(nullptr);
		}
		return CAST_OUTCOME_KEEP; // Travel timer is active
	}

	return CAST_OUTCOME_SUCCESS;
}

/*
================
CastLifecycle_ProcessContinuous

[RECONSTRUCTED - Native 0x00587260] (80 bytes)
Action Category 4: Continuous Channeling Handler
Handles Event 3 (Install): EngageSkill + AddActiveSkill -> returns 0 (CAST_OUTCOME_KEEP).
Handles Event 6 (Cancel): SkillAction_Continuous_Cancel -> returns 0 (CAST_OUTCOME_KEEP).
Invalid events outside [3, 6] trigger ServerFramework_GenerateMiniDump.
Returns 1 (CAST_OUTCOME_SUCCESS) for all other paths / unhandled events.
================
*/
int32_t CastLifecycle_ProcessContinuous(uint32_t dwEntryMode, CGObjChar* pCaster, tagActiveSkillInstance* pInstance, void* pExtra) {
	(void)pExtra;
	if (!pCaster || !pInstance) {
		return CAST_OUTCOME_RELEASE;
	}

	// Native 0x00587263: if ((uint32_t)(dwEntryMode - 3) > 3) GenerateMiniDump()
	if (static_cast<uint32_t>(dwEntryMode - 3) > 3) {
		BSLib::GenerateMiniDump("SkillCast.cpp", __LINE__);
		return CAST_OUTCOME_SUCCESS;
	}

	switch (dwEntryMode) {
		case 3: { // Event 3: Install
			SkillCombat_EngageSkill(pCaster, pInstance);
			CSkillManager* pSkillMgr = pCaster->GetSkillManager();
			if (pSkillMgr != nullptr) {
				pSkillMgr->AddActiveSkill(pInstance);
			}
			return CAST_OUTCOME_KEEP;
		}

		case 6: { // Event 6: Cancel
			SkillAction_Continuous_Cancel(pInstance, pCaster);
			return CAST_OUTCOME_KEEP;
		}

		default:
			break;
	}

	return CAST_OUTCOME_SUCCESS;
}

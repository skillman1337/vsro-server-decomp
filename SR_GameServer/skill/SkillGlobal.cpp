/**
 * ============================================================================
 * Silkroad Online - Skill Validation & Combat Prerequisites
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\skill\SkillGlobal.cpp
 *
 * Implements:
 *   - CheckSkillPreEngageCondition [RECONSTRUCTED - Native 0x0058D8F0]
 * ============================================================================
 */

#include "SkillGlobal.h"
#include "../GObjChar.h"
#include "../GObjPC.h"
#include "../GItemEquip.h"
#include "../GItem.h"
#include "../GStorage.h"
#include "../Game.h"
#include "../GlobalPos.h"
#include "../SkillManager.h"
#include "../SkillResourceCost.h"
#include "../SpecialTargetManager.h"
#include "../Formulae.h"
#include "../../ServerCommon/InstanceSkill.h"
#include "../../JMX_Library/BSLib/BSLog.h"
#include "../../JMX_Library/NavMesh_new/RegionManagerBody.h"
#include "../../JMX_ServerFramework/ServerFramework/ServerConfig.h"
#include "../../JMX_ServerFramework/ServerFramework/ServerMain.h"
#include <windows.h>
#include <cmath>
#include <cstring>
#include <algorithm>

using BSLib::Log_Printf;
using ServerFramework::ServerFramework_GenerateMiniDump;

// Native 0x00C82624: Debug logging flag for SkillActionHandler
bool g_bDebugSkillActionHandler = false;

/*
================================================================================
NPC / Monster Classification TID Check Helpers
================================================================================
*/
inline bool TID_IsEventOrTriggerNPC(uint16_t wTypeID) {
	if ((wTypeID & 0x02) == 0) return false;
	if ((wTypeID & 0x1C) != 0x04) return false;
	if ((wTypeID & 0x60) != 0x40) return false;
	if ((wTypeID & 0x0780) != 0x0080) return false;
	return (wTypeID & 0xF800) == 0x1800;
}

inline bool TID_IsTownGuardNPC(uint16_t wTypeID) {
	if ((wTypeID & 0x02) == 0) return false;
	if ((wTypeID & 0x1C) != 0x04) return false;
	if ((wTypeID & 0x60) != 0x40) return false;
	if ((wTypeID & 0x0780) != 0x0080) return false;
	return (wTypeID & 0xF800) == 0x1000;
}

inline bool TID_IsGateOrFortressNPC(uint16_t wTypeID) {
	if ((wTypeID & 0x02) == 0) return false;
	if ((wTypeID & 0x1C) != 0x04) return false;
	if ((wTypeID & 0x60) != 0x40) return false;
	if ((wTypeID & 0x0780) != 0x0080) return false;
	return (wTypeID & 0xF800) == 0x2000;
}

inline bool CGObjChar_IsMountedOnHorseOrFellow(CGObjChar* pChar) {
	if (!pChar) return false;
	CGObjChar* pVehicle = pChar->GetTransportVehicle();
	return pVehicle && pVehicle->IsVehicleActive();
}

struct tagPCRelationState {
	uint32_t kind = 0;
	uint32_t team = 0;
};

inline tagPCRelationState CGObjPC_GetRelationState(CGObjChar* pChar) {
	tagPCRelationState state;
	if (!pChar) return state;
	CGObjPC* pPC = dynamic_cast<CGObjPC*>(pChar);
	if (pPC) {
		state.kind = pPC->GetJobType();
		state.team = pPC->GetJobType();
	}
	return state;
}

inline uint32_t CGObjPC_ComputeRelationCode(
	const tagPCRelationState& srcState,
	const tagPCRelationState& tgtState,
	CGObjChar* pTarget,
	CGObjChar* pSource
) {
	(void)pTarget; (void)pSource;
	if (srcState.kind != 0 && tgtState.kind != 0) {
		if (srcState.kind != tgtState.kind) {
			return 0x02; // Enemy / Hostile
		}
		return 0x01; // Friendly
	}
	return 0x10; // Neutral
}

inline bool CGObjPC_CheckRelationPredicate(
	CGObjChar* pTarget,
	CGObjChar* pSource,
	uint32_t dwRelationCode,
	uint32_t dwPredicateType
) {
	(void)pTarget; (void)pSource; (void)dwPredicateType;
	return (dwRelationCode == 0x02);
}

/*
================================================================================
tagRefSkill_MatchesExecutionSelector (Native 0x00589D20) (434 bytes)

Checks whether skill template has offensive/damage/debuff action parameters
(returns 1 for offensive/hostile targeting, 0 for beneficial/friendly targeting).
================================================================================
*/
inline bool tagRefSkill_MatchesExecutionSelector(const tagRefSkill* pRefSkill) {
	if (!pRefSkill) return false;
	const uint8_t* p = reinterpret_cast<const uint8_t*>(pRefSkill);
	if (*reinterpret_cast<const uint32_t* const*>(p + 0x41C) != nullptr) {
		return false;
	}
	static const uint16_t s_aOffensiveOffsets[] = {
		0x230, 0x30C, 0x310, 0x314, 0x318, 0x31C, 0x320,
		0x430, 0x434, 0x438, 0x43C, 0x440, 0x444, 0x44C,
		0x450, 0x454, 0x458, 0x45C, 0x460, 0x464, 0x468,
		0x46C, 0x470, 0x400, 0x474, 0x248, 0x234, 0x238,
		0x3C8, 0x3CC, 0x424, 0x2DC, 0x2C8, 0x4A0, 0x48C,
		0x2E0
	};
	for (size_t i = 0; i < sizeof(s_aOffensiveOffsets) / sizeof(s_aOffensiveOffsets[0]); ++i) {
		if (*reinterpret_cast<const uint32_t* const*>(p + s_aOffensiveOffsets[i]) != nullptr) {
			return true;
		}
	}
	return false;
}

/**
 * [RECONSTRUCTED - Native 0x0058CC70] (2055 bytes)
 * TargetValidation_ValidateAllTargets
 *
 * Source: SR_GameServer/skill/SkillGlobal.cpp (Proven @ 0x00AFDD00)
 *
 * Validates selected target candidates against caster state, range/region connectivity,
 * target life state, combat permissions, PvP relation state, abnormal immunities,
 * and monster capture parameters.
 */
uint16_t TargetValidation_ValidateAllTargets(
	CGObjChar* pCaster,
	const Skill::sSkillPreEngageData* pCommand,
	const tagRefSkill* pRefSkill
) {
	// Phase 1 (0x0058CC76): Null reference skill check
	if (!pRefSkill) {
		return SKILL_ERR_INVALID_TARGET; // 0x3003
	}

	// Phase 2 (0x0058CCA5): Request mode bit 0 (selected entity targets)
	if (pCommand && (pCommand->m_byTargetFlags & 0x01)) {
		// Phase 3 (0x0058CCA8): Target vector must not be empty
		if (pCommand->m_vecTargets.empty()) {
			return SKILL_ERR_INVALID_WEAPON; // 0x3006
		}

		// Phase 4 (0x0058CCBD): Provoke / Taunt abnormal state (+0xD34 bit 0x200)
		uint32_t dwSuppressedTargetID = 0;
		if (pCaster && (pCaster->m_dwAbnormalFlags & 0x200)) {
			if (pCaster->m_pRestrictedActionTarget) {
				dwSuppressedTargetID = pCaster->m_pRestrictedActionTarget->dwGlobalID;
			}
		}

		// Phase 5 (0x0058CCE0): Loop across target candidates
		const size_t nTargetCount = pCommand->m_vecTargets.size();
		for (size_t i = 0; i < nTargetCount; ++i) {
			const tagTargetCandidate& candidate = pCommand->m_vecTargets[i];
			const uint32_t dwTargetID = candidate.dwGlobalID;

			// Phase 6 (0x0058CD68): Resolve actor via ObjMgr_FindByID (0x00485D90)
			CGObjChar* pTarget = g_pGame ? g_pGame->FindObjectByID(dwTargetID) : nullptr;
			if (!pTarget || pTarget->IsPickPetCOS()) {
				return SKILL_ERR_INVALID_WEAPON; // 0x3006
			}

			// Phase 7 (0x0058CD95): Pos_RegionsCompatible (0x00430CE0)
			if (pCaster && !Pos_RegionsCompatible(pCaster->m_wRegionID, pTarget->m_wRegionID)) {
				return SKILL_ERR_INVALID_WEAPON; // 0x3006
			}

			// Phase 8 (0x0058CDBC): IsCharacter (VTable Slot +0x18)
			if (!pTarget->IsCharacter()) {
				if (pRefSkill->dwTargetDistribution && pRefSkill->byTargetGroupEnemyMonster) {
					return SKILL_ERR_INVALID_WEAPON; // 0x3006
				}
				continue;
			}

			// Phase 9 (0x0058CDCE): Suppressed target check
			if (dwSuppressedTargetID != 0 && dwSuppressedTargetID == dwTargetID) {
				return SKILL_ERR_INVALID_WEAPON; // 0x3006
			}

			// Phase 10 (0x0058CDDF): byTargetSelectDeadBody (+0x9F) bypass rules
			if (!pRefSkill->byTargetSelectDeadBody) {
				if (pRefSkill->pResu == nullptr) {
					uint8_t byLife = pTarget->GetLifeState();
					if (byLife == 2 || byLife == 3) {
						return SKILL_ERR_INVALID_WEAPON; // 0x3006
					}
				}

				if (pTarget->IsFortressStructure()) {
					if (pTarget->IsSiegeTargetRestricted()) {
						return SKILL_ERR_INVALID_WEAPON; // 0x3006
					}
					if (pCaster && pCaster->IsPlayer()) {
						if (pCaster->GetLastAttackSkillID() != pRefSkill->dwSkillID && !pRefSkill->byTargetGroupEnemyPlayer) {
							return SKILL_ERR_INVALID_WEAPON; // 0x3006
						}
						pCaster->m_dwLastAttackSkillID = pRefSkill->dwSkillID;
					}
					if (!pTarget->IsDropUsable2() && !pTarget->IsDropUsable1() && !pTarget->IsInteractiveWorldItem()) {
						return SKILL_ERR_INVALID_WEAPON; // 0x3006
					}
				} else if (pTarget->IsFortressHeart()) {
					if (pCaster && pCaster->IsPlayer()) {
						if (pCaster->GetLastAttackSkillID() != pRefSkill->dwSkillID) {
							return SKILL_ERR_INVALID_WEAPON; // 0x3006
						}
						pCaster->m_dwLastAttackSkillID = pRefSkill->dwSkillID;
					}
				}
			}

			// Phase 11 (0x0058CF09): tagRefSkill_MatchesExecutionSelector (0x00589D20) & CheckCombatPermission (+0x624)
			if (tagRefSkill_MatchesExecutionSelector(pRefSkill)) {
				uint32_t dwCombatError = 0;
				if (pCaster && !pCaster->CheckCombatPermission(pTarget, 0, &dwCombatError)) {
					return static_cast<uint16_t>(dwCombatError);
				}
			}

			// Phase 12 (0x0058CF3F): Skill_ValidateTargetPermissions (0x0058D7A0)
			if (!Skill_ValidateTargetPermissions(pRefSkill, pCaster, pTarget)) {
				return SKILL_ERR_INVALID_WEAPON; // 0x3006
			}

			// Phase 13 (0x0058CF4D): Restricted target and active instance context verification
			if (pCaster && (pCaster->m_dwAbnormalFlags & 0x200)) {
				if (pCaster->m_pRestrictedActionTarget && pCaster->m_pRestrictedActionTarget->dwGlobalID == pTarget->GetGlobalID()) {
					return SKILL_ERR_INVALID_WEAPON; // 0x3006
				}
			}

			if (pCaster && pCaster->m_pActiveCastInstance) {
				tagActiveSkillInstance* pActive = pCaster->m_pActiveCastInstance;
				if (!pActive->m_pExecution || pActive->m_pExecution->m_dwContextID != dwTargetID) {
					return SKILL_ERR_INVALID_WEAPON; // 0x3006
				}
			}

			// Phase 14 (0x0058CF88): COS Owner Resolution
			if (pTarget->IsCOS() && !pTarget->m_pOwnerChar) {
				return SKILL_ERR_INVALID_WEAPON; // 0x3006
			}

			// Phase 15 (0x0058CFA8): PvP and Player Relation Verification
			bool bTargetIsPlayer = pTarget->IsPlayer();
			bool bTargetPlayerOrCOS = bTargetIsPlayer || (pTarget->IsCOS() && pTarget->m_pOwnerChar);

			if (bTargetPlayerOrCOS) {
				if (pCaster && pCaster->IsPlayer() && pCaster != pTarget && !pCaster->IsGM()) {
					CGObjChar* pPrincipal = pTarget;
					if (pTarget->IsCOS() && pTarget->m_pOwnerChar) {
						pPrincipal = pTarget->m_pOwnerChar;
					}

					tagPCRelationState srcState = CGObjPC_GetRelationState(pCaster);
					tagPCRelationState tgtState = CGObjPC_GetRelationState(pPrincipal);
					uint32_t dwRelation = CGObjPC_ComputeRelationCode(srcState, tgtState, pPrincipal, pCaster);

					bool bSameParty = (pPrincipal->GetParty() != nullptr && pPrincipal->GetParty() == pCaster->GetParty());
					if (!bSameParty) {
						if (CGObjPC_CheckRelationPredicate(pPrincipal, pCaster, dwRelation, 1)) {
							if (!tagRefSkill_MatchesExecutionSelector(pRefSkill)) {
								return SKILL_ERR_INVALID_WEAPON; // 0x3006
							}
						} else if (dwRelation != 0x10) {
							if (tagRefSkill_MatchesExecutionSelector(pRefSkill)) {
								return SKILL_ERR_INVALID_WEAPON; // 0x3006
							}
						} else if (tgtState.kind == 8 && srcState.kind == 8) {
							if (!tagRefSkill_MatchesExecutionSelector(pRefSkill)) {
								return SKILL_ERR_INVALID_WEAPON; // 0x3006
							}
						}

						if (tgtState.kind != srcState.kind && !tagRefSkill_MatchesExecutionSelector(pRefSkill)) {
							return SKILL_ERR_INVALID_WEAPON; // 0x3006
						}
					}
				}

				// Phase 16 (0x0058D0EA): Resurrection level constraint (Resu @ +0x330)
				if (pRefSkill->pResu != nullptr && pCaster && pCaster->IsPlayer()) {
					if (pRefSkill->pResu[0] != 0 && pTarget->GetCombatLevel() > pRefSkill->pResu[0]) {
						return 0x3012; // SKILL_ERR_TARGET_LEVEL_TOO_HIGH
					}
				}
			}

			// Phase 17 (0x0058D15E): Abnormal status requirement (Reqa @ +0x3B8)
			if (pRefSkill->pReqa != nullptr) {
				if (pTarget->GetAbnormalImmunityFlags() != 8 && !(pRefSkill->pReqa[0] & pTarget->m_dwAbnormalFlags)) {
					return SKILL_ERR_INVALID_WEAPON; // 0x3006
				}
			}

			// Phase 18 (0x0058D18C): Combat condition requirement (Reqc @ +0x39C)
			if (pRefSkill->pReqc != nullptr) {
				if ((pRefSkill->pReqc[0] & 1) && pTarget->GetAbnormalImmunityFlags() != 8) {
					return SKILL_ERR_INVALID_WEAPON; // 0x3006
				}
				if ((pRefSkill->pReqc[0] & 8) && (!pCaster || pTarget->GetParty() != pCaster->GetParty())) {
					return SKILL_ERR_INVALID_WEAPON; // 0x3006
				}
				// Native 0x0058DFF4: Check owner condition bit (+0x1D0) if Reqc bit 0x20 is set
				if ((pRefSkill->pReqc[0] & 0x20) != 0) {
					if (pCaster && pCaster->GetSkillManager() && !pCaster->GetSkillManager()->CheckOwnerCondition()) {
						return 0x3032; // SKILL_ERR_MUTUAL_EXCLUSIVE_BUFF / condition unsatisfied
					}
				}
			}

			// Phase 19 (0x0058D1D3): Cast mode change / mount modifier (Msch @ +0x4A4)
			if (pRefSkill->pMsch != nullptr && pRefSkill->pMsch[0] == 2) {
				if (pTarget->IsPlayer() && (pTarget->IsFreePVPModeActive() || CGObjChar_IsMountedOnHorseOrFellow(pTarget))) {
					return SKILL_ERR_TARGET_MAX_STACK; // 0x3039
				}
				if (pRefSkill->pMsch[1] < pTarget->GetCombatLevel()) {
					return 0x3035; // SKILL_ERR_LEVEL_MISMATCH
				}
				if (pTarget->IsPlayer() && (pTarget->IsGM() || pTarget->GetLifeState() == 2)) {
					return SKILL_ERR_TARGET_MAX_STACK; // 0x3039
				}
			}

			// Phase 20 (0x0058D288): Linked companion admission (Lnks @ +0x370 / Lks2 @ +0x374)
			if (pRefSkill->pLnks != nullptr && pCaster) {
				CSkillManager* pSkillMgr = pCaster->GetSkillManager();
				if (pSkillMgr) {
					uint16_t wLinkError = pSkillMgr->ValidateBuffExclusionList(pRefSkill, pTarget);
					if (wLinkError != 0) {
						return wLinkError;
					}
				}
			}

			// Phase 21 (0x0058D2B0): Active Buff Replacement Validation (0x0059D870)
			bool bAlreadyCurrent = false;
			if (pCaster == pTarget && pCaster && pCaster->m_pActiveCastInstance) {
				tagActiveSkillInstance* pCur = pCaster->m_pActiveCastInstance;
				if (!pCur->m_pExecution) {
					return SKILL_ERR_TARGET_DEAD; // 0x3009
				}
				bAlreadyCurrent = (pCur->m_pExecution && pCur->m_pExecution->m_pRefSkill == pRefSkill);
			}

			if (!bAlreadyCurrent) {
				CSkillManager* pSkillMgr = pTarget->GetSkillManager();
				if (pSkillMgr && !pSkillMgr->ValidateBuffReplacement(pRefSkill, pCaster)) {
					return SKILL_ERR_SKILL_ON_COOLDOWN; // 0x300C
				}
			}

			// Phase 22 (0x0058D2E0): Monster Capture & NPC Exclusions (Ca @ +0x458, Mcap @ +0x4A8)
			if (pTarget->IsMonster()) {
				if (pRefSkill->pCa != nullptr && pTarget->GetMonsterClass() != 0) {
					return 0x3033; // SKILL_ERR_CANNOT_CAPTURE_CHAMPION
				}

				if (pRefSkill->pMcap != nullptr) {
					if (pTarget->GetMonsterClass() != 0) {
						return 0x3033; // SKILL_ERR_CANNOT_CAPTURE_CHAMPION
					}

					const tagRefObjCommon* pRef = pTarget->GetDataPermanent() ? pTarget->GetDataPermanent()->m_pRefObjCommon : nullptr;
					if (!pRef) {
						return SKILL_ERR_INVALID_WEAPON; // 0x3006
					}

					uint16_t wTypeID = pRef->m_wTypeID;
					if (TID_IsGateOrFortressNPC(wTypeID) ||
						TID_IsTownGuardNPC(wTypeID) ||
						TID_IsEventOrTriggerNPC(wTypeID)) {
						return SKILL_ERR_INVALID_WEAPON; // 0x3006
					}

					if (pRefSkill->pMcap[0] < pRef->m_byLevel) {
						return 0x3035; // SKILL_ERR_LEVEL_MISMATCH
					}
				}
			}
		}
	}

	// Phase 23 (0x0058D3D2): Request mode bit 1 (ground positions vector validation)
	if (pCommand && (pCommand->m_byTargetFlags & 0x02) && pCommand->m_vecTargetPos.empty()) {
		return SKILL_ERR_INVALID_WEAPON; // 0x3006
	}

	return SKILL_SUCCESS; // 0x0000
}

/**
 * [RECONSTRUCTED - Native 0x0058D480]
 * Skill_ValidateEquipmentRequirements
 *
 * Implements native equipment requirement validation for casting skills:
 * - Checks if actor is a player (Slot +0x1C @ 0x00482560); non-players bypass equipment checks.
 * - If no REQI parameter array is present at +0x3A0:
 *     - If both required weapon kinds at +0xC7 and +0xC8 are 0xFF: return 0 (SKILL_SUCCESS).
 *     - Queries primary weapon TID from slot 6.
 *     - If unarmed, weapon kind defaults to 1 (bare fists).
 *     - If kind is 16, validates fortress siege equipment flags.
 *     - Validates primary weapon durability via IsMainWeaponUsable() (Slot +0x5D0).
 * - If REQI parameter array is present at +0x3A0:
 *     - Queries primary weapon TID from slot 6 and secondary item TID from slot 7.
 *     - Iterates up to 5 parameter slots:
 *         - Opcodes 1, 2, 3, 9, 10, 11: Armor pieces (slots 1..6) and durability via IsEquipmentSlotUsable().
 *         - Opcode 4: Secondary weapon/shield kind and durability via IsSecondaryWeaponUsable().
 *         - Opcode 6: Primary weapon kind and durability via IsMainWeaponUsable().
 *         - Opcode 14: Avatar item in avatar slot 4 expiration check via GetAvatarStorageItem() & IsItemExpired().
 *     - Enforces REQN flag (+0x3B4).
 */
uint16_t Skill_ValidateEquipmentRequirements(
	CGObjChar* pCaster,
	const tagRefSkill* pSkill
) {
	// Native 0x0058D489 - 0x0058D497: If caster is not player (Slot +0x1C), return success
	if (pCaster == nullptr || !pCaster->IsPlayer()) {
		return SKILL_SUCCESS;
	}
	if (pSkill == nullptr) {
		return SKILL_SUCCESS;
	}

	// Native 0x0058D4A3: If no REQI parameters are attached at +0x3A0, jump to default branch (0x0058D6AA)
	if (pSkill->pReqI[0] == nullptr) {
		if (pSkill->byReqWeaponKind[0] == 0xFF && pSkill->byReqWeaponKind[1] == 0xFF) {
			return SKILL_SUCCESS;
		}

		uint16_t wMainTID = 0;
		CGItem* pMainItem = const_cast<CGStorage&>(pCaster->m_storage).GetItem(6);
		if (pMainItem != nullptr) {
			CGItemEquip* pEquip = dynamic_cast<CGItemEquip*>(pMainItem);
			if (pEquip == nullptr || pEquip->GetCurrentDurability() > 0) {
				wMainTID = pMainItem->GetTID().wType;
			}
		}

		uint8_t byKind = static_cast<uint8_t>((wMainTID >> 11) & 0x1F);
		if (byKind == 0) {
			byKind = 1; // Unarmed / fists default
		}

		uint16_t wError = SKILL_ERR_WEAPON_MISMATCH; // 0x300D
		for (size_t i = 0; i < 2; ++i) {
			uint8_t byReq = pSkill->byReqWeaponKind[i];
			if (byReq != 0xFF && byKind == byReq) {
				wError = (byKind == 16) ? SKILL_ERR_SPECIAL_ITEM_RESTRICTION : SKILL_SUCCESS;
				break;
			}
		}

		// Native 0x0058D719: Durability check on primary weapon (Slot +0x5D0)
		if (!pCaster->IsMainWeaponUsable()) {
			return SKILL_ERR_WEAPON_BROKEN; // 0x300F
		}

		return wError;
	}

	// Native 0x0058D4BD - 0x0058D4D4: REQI branch
	uint16_t wSecondaryTID = 0;
	CGItem* pSecItem = const_cast<CGStorage&>(pCaster->m_storage).GetItem(7);
	if (pSecItem != nullptr) {
		CGItemEquip* pEquip = dynamic_cast<CGItemEquip*>(pSecItem);
		if (pEquip == nullptr || pEquip->GetCurrentDurability() > 0) {
			wSecondaryTID = pSecItem->GetTID().wType;
		}
	}

	uint16_t wMainTID = 0;
	CGItem* pMainItem = const_cast<CGStorage&>(pCaster->m_storage).GetItem(6);
	if (pMainItem != nullptr) {
		CGItemEquip* pEquip = dynamic_cast<CGItemEquip*>(pMainItem);
		if (pEquip == nullptr || pEquip->GetCurrentDurability() > 0) {
			wMainTID = pMainItem->GetTID().wType;
		}
	}

	const uint16_t mainKind = wMainTID >> 11;
	const uint16_t offKind  = wSecondaryTID >> 11;

	uint16_t wError = 0;
	bool bAccepted = false;
	size_t nFulfilled = 0;

	for (size_t index = 0; index < 5; ++index) {
		const uint32_t* pRequirement = pSkill->pReqI[index];
		if (pRequirement == nullptr || bAccepted) {
			if ((nFulfilled != 0 && nFulfilled == index) || bAccepted) {
				return wError;
			}
			return wError ? wError : SKILL_ERR_WEAPON_MISMATCH; // 0x300D
		}

		switch (pRequirement[0]) {
		case 1: case 2: case 3: case 9: case 10: case 11:
			if (pRequirement[1] != 0) {
				static const uint32_t s_aSlotMap[7] = { 0, 0, 2, 1, 4, 3, 5 };
				uint32_t dwSlotIdx = (pRequirement[1] <= 6) ? s_aSlotMap[pRequirement[1]] : pRequirement[1];
				CGItem* pItem = const_cast<CGStorage&>(pCaster->m_storage).GetItem(dwSlotIdx);
				uint16_t itemTID = 0;
				if (pItem != nullptr) {
					CGItemEquip* pEquip = dynamic_cast<CGItemEquip*>(pItem);
					if (pEquip == nullptr || pEquip->GetCurrentDurability() > 0) {
						itemTID = pItem->GetTID().wType;
					}
				}
				if ((static_cast<uint16_t>(itemTID) >> 11) == pRequirement[1] &&
					pCaster->IsEquipmentSlotUsable(pRequirement[1])) {
					wError = 0;
					bAccepted = true;
				}
			} else {
				static const uint32_t s_aSlotMap[7] = { 0, 0, 2, 1, 4, 3, 5 };
				for (uint32_t equipmentSlot = 1; equipmentSlot <= 6; ++equipmentSlot) {
					CGItem* pItem = const_cast<CGStorage&>(pCaster->m_storage).GetItem(s_aSlotMap[equipmentSlot]);
					uint16_t itemTID = 0;
					if (pItem != nullptr) {
						CGItemEquip* pEquip = dynamic_cast<CGItemEquip*>(pItem);
						if (pEquip == nullptr || pEquip->GetCurrentDurability() > 0) {
							itemTID = pItem->GetTID().wType;
						}
					}
					if (((itemTID >> 7) & 0x0F) != pRequirement[0]) {
						wError = 0; // does not undo earlier matching slots
						break;
					}
					if (!pCaster->IsEquipmentSlotUsable(equipmentSlot)) {
						bAccepted = false;
						wError = SKILL_ERR_WEAPON_BROKEN; // 0x300F
						break;
					}
					bAccepted = true;
				}
			}
			break;

		case 4: // Secondary weapon / shield
			if (static_cast<uint32_t>(offKind) == pRequirement[1]) {
				if (pCaster->IsSecondaryWeaponUsable()) {
					wError = 0;
					bAccepted = true;
				} else {
					wError = SKILL_ERR_WEAPON_BROKEN; // 0x300F
				}
			}
			break;

		case 6: // Main weapon
			if (static_cast<uint32_t>(mainKind) == pRequirement[1]) {
				if (pCaster->IsMainWeaponUsable()) {
					wError = 0;
					bAccepted = true;
				} else {
					wError = SKILL_ERR_WEAPON_BROKEN; // 0x300F
				}
			}
			break;

		case 14: // Avatar / special item
			if (pRequirement[1] == 1) {
				CGItem* pAvatarItem = pCaster->GetAvatarStorageItem(4);
				if (pAvatarItem != nullptr) {
					CGItemEquip* pEquip = dynamic_cast<CGItemEquip*>(pAvatarItem);
					if (pEquip != nullptr && !pEquip->IsItemExpired()) {
						bAccepted = true;
					}
				}
			}
			break;

		default:
			break;
		}

		if (pSkill->pReqN != 0) {
			if (!bAccepted) {
				return wError ? wError : SKILL_ERR_WEAPON_MISMATCH; // 0x300D
			}
			bAccepted = false;
			++nFulfilled;
		}
	}

	return bAccepted ? wError : (wError ? wError : SKILL_ERR_WEAPON_MISMATCH);
}

void Skill_GetSecondaryWeaponTID(uint16_t* pTID, const CGObjChar* pChar) {
	if (pTID) *pTID = 0;
	if (pChar) {
		CGItem* pItem = const_cast<CGStorage&>(pChar->m_storage).GetItem(7);
		if (pItem) *pTID = pItem->GetTID().wType;
	}
}

void Skill_GetMainWeaponTID(uint16_t* pTID, const CGObjChar* pChar) {
	if (pTID) *pTID = 0;
	if (pChar) {
		CGItem* pItem = const_cast<CGStorage&>(pChar->m_storage).GetItem(6);
		if (pItem) *pTID = pItem->GetTID().wType;
	}
}

void Skill_GetArmorSlotTID(uint32_t dwSlot, const CGObjChar* pChar, uint16_t* pTID) {
	if (pTID) *pTID = 0;
	if (pChar) {
		static const uint32_t s_aMap[7] = { 0, 0, 2, 1, 4, 3, 5 };
		uint32_t s = (dwSlot <= 6) ? s_aMap[dwSlot] : dwSlot;
		CGItem* pItem = const_cast<CGStorage&>(pChar->m_storage).GetItem(s);
		if (pItem) *pTID = pItem->GetTID().wType;
	}
}

/*
================
CheckSkillPreEngageCondition
[PARTIAL - Native 0x0058D8F0] (3134 bytes)

CORRECTION (Claude): was Skill_ValidatePrerequisitesAndCost; the function names itself in its log
lines ("CheckSkillPreEngageCondition() -- Skill Cancel (Sleep)"). Rewritten from the machine code: a
null pRefSkill is resolved from the pre-engage data, the unconditional checks run first, and each
optional group is selected by a bit of dwCheckFlags:
  0x01 cooldown (player; 0x80 is passed through)   0x02 buff replacement / exclusion
  0x04 equipment requirements                      0x08 targets
  0x10 HP / MP cost                                0x20 required ammunition (slot 7)
  0x40 line of sight to every target (player)
Callers: 0x37 ProcessCommand, 0x17 cast BEGIN, 0x91 cast APPROACH / EXECUTE, 0x64 attack BEGIN,
0x24 attack APPROACH, 0x81 attack in range, 0x01 attack KEEP_UP.

Not ported yet, and failing closed: the placement check for skills with +0x49C (world object query
0x00531180 around the owner), the transport checks of Msid 1 and 2 (status map +0x1D8, +0x21EC), the
execution-selector cell test on owner +0x128, and the line-of-sight query (navmesh slot 30).
================
*/
uint32_t g_bLogSkillPreEngageCancel = 0; // Native 0x00C82624

static bool Skill_IsCastWhileDisabledSkill( const tagRefSkill* pSkill ) {
	static const char* const s_apszCodeNames[] = {
		"MSKILL_SD_HAROERIS_ATTACK04", "MSKILL_SD_SETH_ATTACK04", "MSKILL_SD_SETH_ATTACK10",
	};
	for ( const char* pszCodeName : s_apszCodeNames ) {
		// 0x0058D98F: CompareStringA(LOCALE_SYSTEM_DEFAULT (0x400), NORM_IGNORECASE | SORT_STRINGSORT) == CSTR_EQUAL
		if ( ::CompareStringA( 0x400, 0x10001, pSkill->m_Basic_Code.c_str(), -1, pszCodeName, -1 ) == CSTR_EQUAL ) {
			return true;
		}
	}
	return false;
}

uint16_t CheckSkillPreEngageCondition(
	CGObjChar* pCaster,
	Skill::sSkillPreEngageData* pPreEngage,
	uint32_t dwCheckFlags,
	const tagRefSkill* pSkill
) {
	CSkillManager* pSkillManager = pCaster->GetSkillManager();

	if ( pSkill == nullptr ) {
		ASSERT( g_pRefData != nullptr );
		pSkill = g_pRefData->FindSkill( pPreEngage->m_dwSkillID );
		if ( pSkill == nullptr ) {
			return 0x3003;
		}
	}

	const bool bCastWhileDisabled = Skill_IsCastWhileDisabledSkill( pSkill );

	// 0x0058DA2D: a request mode of exactly 0x20 (indirect skills, 0x0059B8D0) skips the disable checks.
	if ( pPreEngage->m_byTargetFlags != 0x20 ) {
		if ( ( pCaster->m_dwAbnormalFlags & 0x40 ) != 0 && !bCastWhileDisabled && pSkill->pParam594 == nullptr ) {
			if ( g_bLogSkillPreEngageCancel != 0 ) {
				BSLib::Log_Printf( 0x3000000, "CheckSkillPreEngageCondition() -- Skill Cancel (Sleep)  [target:%s]", pCaster->GetCodeName() );
			}
			if ( std::strstr( pCaster->GetCodeName(), "MOB_RM_SEALSTONE" ) == nullptr ) {
				return 0x3009;
			}
		}

		if ( pSkillManager->GetInstance214() != nullptr ) {
			if ( g_bLogSkillPreEngageCancel != 0 ) {
				BSLib::Log_Printf( 0x3000000, "CheckSkillPreEngageCondition() -- Skill Cancel (StoneSkill)  [target:%s]", pCaster->GetCodeName() );
			}
			return 0x3009;
		}

		if ( ( pCaster->m_dwAbnormalFlags & 0x4041 ) != 0 && pSkill->pParam594 == nullptr &&
			std::strstr( pCaster->GetCodeName(), "MOB_RM_SEALSTONE" ) == nullptr ) {
			return 0x3009;
		}
	}

	if ( pSkill->pParam2CC != nullptr && pSkillManager->GetInstance1F8() != nullptr ) {
		return 0x3009;
	}

	if ( pSkill->pParam49C != nullptr ) {
		// [PARTIAL] 0x0058DB45 - 0x0058DE13: nothing may stand within the placement radius (+0x294 -> +8,
		// 5x that for monsters). The world object query is not ported: fail closed.
		return 0x3037;
	}

	if ( pSkill->pMsch != nullptr ) {
		if ( pCaster->GetBodyMode() == 1 ) {
			return 0x3031;
		}
		if ( *pSkill->pMsch == 1 ) {
			ASSERT( g_pRefData != nullptr );
			const tagRefObjCommon* pTransport = g_pRefData->FindRefObjCommon( pPreEngage->m_dw20 );
			if ( pTransport == nullptr ) {
				return 0x3006;
			}
			if ( pCaster->GetLevel() < pTransport->m_byLevel ) {
				return 0x3008;
			}
			// [PARTIAL] 0x0058DEA7: also refused while status 1 is in the owner's status map (+0x1D8),
			// which is not ported: fail closed.
			return 0x3009;
		}
		if ( *pSkill->pMsch == 2 ) {
			CGObjChar* pVehicle = pCaster->GetTransportVehicle( 8 );
			if ( pVehicle != nullptr && pVehicle->IsVehicleActive() ) {
				return 0x3039;
			}
			// [PARTIAL] 0x0058DF0C: riding a horse or fellow (owner +0x21EC in 1..5) is not ported: fail closed.
			return 0x3039;
		}
	}

	if ( pSkill->pParam428 != nullptr ) {
		if ( pSkill->pParam4A0 == nullptr && pCaster->GetBodyMode() == 1 ) {
			return 0x3031;
		}
		if ( pCaster->IsPlayer() ) {
			const uint32_t dwKind = *pSkill->pParam428;
			if ( ( dwKind == 1 || dwKind == 2 ) && pCaster->GetCharData()->m_byBattleState != 0 ) {
				return 0x3028;
			}
		}
	}

	if ( pSkill->pReqc != nullptr ) {
		const uint32_t dwReqc = *pSkill->pReqc;
		if ( ( dwReqc & 0x04 ) != 0 ) {
			const double dCurrentHP = static_cast<double>( static_cast<int32_t>( pCaster->GetCurrentHP() ) );
			const double dHPLimit = static_cast<double>( static_cast<int32_t>( pCaster->GetMaxHP() ) ) * 0.30000001192092896;
			if ( dHPLimit < dCurrentHP ) {
				return 0x3036;
			}
		}
		if ( ( dwReqc & 0x10 ) != 0 && ( pPreEngage->m_byTargetFlags & 0x10 ) == 0 ) {
			return 0x3034;
		}
		if ( ( dwReqc & 0x20 ) != 0 && !pSkillManager->CheckOwnerCondition() ) {
			return 0x3032;
		}
	}

	if ( ( pCaster->m_dwAbnormalFlags & 0x80 ) != 0 && ( pSkill->pParam2EC != nullptr || pSkill->pParam2F4 != nullptr ) ) {
		return 0x3009;
	}
	if ( pSkillManager->GetInstance1DC() != nullptr &&
		( pSkill->pParam2EC != nullptr || pSkill->pParam2F0 != nullptr || pSkill->pParam2F4 != nullptr ) ) {
		return 0x3009;
	}

	if ( pCaster->IsPlayer() && ( dwCheckFlags & 0x01 ) != 0 ) {
		if ( !pCaster->GetCooltimeManager()->IsCooldownAvailable( pSkill, dwCheckFlags & 0x80, false ) ) {
			return 0x3005;
		}
	}

	if ( pCaster->IsPlayer() && ( pSkill->pParam274 != nullptr || pSkill->pParam2B4 != nullptr ) ) {
		const uint8_t byMotion = pCaster->GetMotionState();
		if ( byMotion == 0x04 || byMotion == 0x12 || byMotion == 0x08 || byMotion == 0x0F || byMotion == 0x11 ) {
			return 0x3009;
		}
		if ( pCaster->IsPlayer() && pCaster->GetCharData()->m_byTransportState == 1 ) {
			return 0x3009;
		}
	}

	if ( ( dwCheckFlags & 0x04 ) != 0 ) {
		uint16_t wError = Skill_ValidateEquipmentRequirements( pCaster, pSkill );
		if ( wError != 0 ) {
			return wError;
		}
	}

	if ( ( dwCheckFlags & 0x08 ) != 0 ) {
		if ( pSkill->byTargetRequired == 1 ) {
			uint16_t wError = TargetValidation_ValidateAllTargets( pCaster, pPreEngage, pSkill );
			if ( wError != 0 ) {
				return wError;
			}
		} else {
			if ( pSkill->MatchesExecutionSelector() && ( pSkill->pParam49C != nullptr || pSkill->byCastType != 1 ) ) {
				// [PARTIAL] 0x0058E170: refused (0x3009 with +0x49C, 0x3018 without) unless the owner's cell
				// (+0x128 -> +0x10, byte +2) allows it; the cell is not ported: fail closed.
				return ( pSkill->pParam49C != nullptr ) ? 0x3009 : 0x3018;
			}
			if ( pCaster->IsPlayer() && ( pPreEngage->m_byTargetFlags & Skill::SKILL_TARGET_FLAG_OBJECT ) != 0 ) {
				return 0x3006;
			}
		}
	}

	if ( ( dwCheckFlags & 0x10 ) != 0 ) {
		int32_t nHPCost = pSkill->wRequiredHP;
		int32_t nMPCost = pSkill->wRequiredMP;
		if ( pSkill->wConsumeHPRatio > 0 ) {
			nHPCost += static_cast<int32_t>( static_cast<double>( static_cast<int32_t>( pCaster->GetMaxHP() ) ) *
				( static_cast<double>( pSkill->wConsumeHPRatio ) / 100.0 ) );
		}
		if ( pSkill->wConsumeMPRatio > 0 ) {
			nMPCost += static_cast<int32_t>( static_cast<double>( static_cast<int32_t>( pCaster->GetMaxMP() ) ) *
				( static_cast<double>( pSkill->wConsumeMPRatio ) / 100.0 ) );
		}
		if ( pCaster->IsPlayer() ) {
			nMPCost = static_cast<int32_t>( static_cast<double>( pCaster->GetParamFloat( 0x8D ) ) / 100.0 * nMPCost );
		}
		if ( nHPCost > 0 && static_cast<int32_t>( pCaster->GetCurrentHP() ) < nHPCost ) {
			return 0x3013;
		}
		if ( nMPCost > 0 && static_cast<int32_t>( pCaster->GetCurrentMP() ) < nMPCost ) {
			return 0x3004;
		}
	}

	if ( ( dwCheckFlags & 0x02 ) != 0 ) {
		if ( pCaster->GetBodyMode() == 4 && pSkill->pParam428 != nullptr ) {
			return 0x300C;
		}
		if ( pSkill->pLnks == nullptr ) {
			if ( pSkill->byTargetRequired == 0 && !pSkillManager->ValidateBuffReplacement( pSkill, pCaster ) ) {
				return 0x300C;
			}
		} else if ( pSkill->Param( 0x294 ) != nullptr ) {
			uint16_t wError = Skill_ValidateBuffExclusionList( pSkillManager, pSkill, nullptr );
			if ( wError != 0 ) {
				return wError;
			}
		}
	}

	if ( ( dwCheckFlags & 0x20 ) != 0 && pCaster->IsPlayer() && pSkill->Param( 0x2A0 ) != nullptr ) {
		// Required ammunition in equipment slot 7: kind (+0) = TID bits 7..10, optional sub-kind (+4) =
		// TID >> 11, and a count above zero (0x004EC250).
		const uint32_t* pdwAmmo = pSkill->Param( 0x2A0 );
		CGItem* pAmmo = pCaster->GetEquippedAmmo();
		if ( pAmmo == nullptr ) {
			return 0x300E;
		}
		const uint16_t wTID = pAmmo->GetTID().wType;
		if ( ( ( wTID >> 7 ) & 0xF ) != pdwAmmo[0] ) {
			return 0x300E;
		}
		if ( pdwAmmo[1] != 0 && static_cast<uint32_t>( wTID >> 11 ) != pdwAmmo[1] ) {
			return 0x300E;
		}
		if ( pCaster->GetEquippedAmmoCount() <= 0 ) {
			return 0x300E;
		}
	}

	if ( ( dwCheckFlags & 0x40 ) != 0 && pCaster->IsPlayer() && !pPreEngage->m_vecTargets.empty() ) {
		// 0x0058E415: the owner's location is copied once; slot 30 walks that copy towards each target
		NavMesh::tagNavPos fromPos = pCaster->m_Location;
		for ( const tagTargetCandidate& target : pPreEngage->m_vecTargets ) {
			CGObjChar* pTarget = ObjMgr_FindByID( target.dwGlobalID );
			if ( pTarget == nullptr ) {
				return 0x3006;
			}
			// 0x0058E4AC - 0x0058E4FB: a target the owner cannot see refuses the cast
			NavMesh::tagNavPos toPos = pTarget->m_Location;
			if ( NavMesh::g_pRegionManager->IsLineOfSight( &fromPos, &toPos, 0 ) == 0 ) {
				return 0x3010;
			}
		}
	}

	return 0;
}

/*
================
Skill_ValidateCast

[RECONSTRUCTED - Native 0x0058D8D0] (30 bytes)
Thin wrapper delegating cast validation to CheckSkillPreEngageCondition.
================
*/
uint16_t Skill_ValidateCast(
	tagActiveSkillInstance* pInstance,
	CGObjChar* pCaster,
	uint32_t dwValidateFlags
) {
	if (!pInstance || !pCaster || !pInstance->m_pExecution || !pInstance->m_pExecution->m_pRefSkill) {
		return SKILL_ERR_INVALID_TARGET;
	}
	return CheckSkillPreEngageCondition(
		pCaster,
		pInstance->m_pCommand,
		dwValidateFlags,
		pInstance->m_pExecution->m_pRefSkill
	);
}

/*
================================================================================
Target Selection Subsystem [RECONSTRUCTED - Native 0x0058A020 - 0x0058CB70]
Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\skill\SkillGlobal.cpp
================================================================================
*/

/*
================
TargetSelection_Attenuate

Calculates attenuated power level: power = (100 - attenuation) * power / 100
================
*/
static inline uint8_t TargetSelection_Attenuate(uint8_t byPower, uint32_t dwAttenuation) {
	uint32_t dwProduct = (100U - dwAttenuation) * static_cast<uint32_t>(byPower);
	return static_cast<uint8_t>(dwProduct / 100U);
}

/*
================
TargetSelection_DistanceBetween

Calculates 3D Euclidean distance between two positions across world regions.
================
*/
static inline float TargetSelection_DistanceBetween(
	uint16_t wRegion1,
	const SRO_Vector3D& vPos1,
	uint16_t wRegion2,
	const SRO_Vector3D& vPos2
) {
	if (!Pos_RegionsCompatible(wRegion1, wRegion2)) {
		return 999999.0f;
	}
	SRO_Vector3D vDiff;
	Pos_Relative3D(&vDiff, wRegion1, &vPos1, wRegion2, &vPos2);
	if (vDiff.x == SRO_INCOMPATIBLE_POS) {
		return 999999.0f;
	}
	return std::sqrt(vDiff.x * vDiff.x + vDiff.y * vDiff.y + vDiff.z * vDiff.z);
}

/*
================
TargetSelection_InDirectionalShape

[RECONSTRUCTED - Native 0x0058AF60] (497 bytes)
Tests if candidate entity lies within the directional cylinder/cone volume defined
by direction vector and radius.
================
*/
bool TargetSelection_InDirectionalShape(
	CGObjChar* pCaster,
	CGObjChar* pCandidate,
	const SRO_Vector3D& vDirection,
	float fRadius
) {
	if (!pCaster || !pCandidate) {
		return false;
	}

	SRO_Vector3D vCasterPos(pCaster->m_fLocalPosX, pCaster->m_fLocalPosY, pCaster->m_fLocalPosZ);
	SRO_Vector3D vCandPos(pCandidate->m_fLocalPosX, pCandidate->m_fLocalPosY, pCandidate->m_fLocalPosZ);

	if (!Pos_RegionsCompatible(pCaster->m_wRegionID, pCandidate->m_wRegionID)) {
		return false;
	}

	SRO_Vector3D vDisp;
	Pos_Relative3D(&vDisp, pCaster->m_wRegionID, &vCasterPos, pCandidate->m_wRegionID, &vCandPos);
	if (vDisp.x == SRO_INCOMPATIBLE_POS) {
		return false;
	}

	float fDistXZ = std::sqrt(vDisp.x * vDisp.x + vDisp.z * vDisp.z);
	float fDirLength = std::sqrt(vDirection.x * vDirection.x + vDirection.y * vDirection.y + vDirection.z * vDirection.z);

	float fCasterRadius = static_cast<float>(pCaster->GetCollisionRadius());
	float fCandidateRadius = static_cast<float>(pCandidate->GetCollisionRadius());

	if (fDistXZ > (fDirLength + fCasterRadius + fCandidateRadius)) {
		return false;
	}

	if (fDistXZ <= 0.0001f || fDirLength <= 0.0001f) {
		return true;
	}

	SRO_Vector3D vNormDir = vDirection;
	Vec3_Normalize(&vNormDir);

	SRO_Vector3D vNormDisp = vDisp;
	vNormDisp.y = 0.0f;
	Vec3_Normalize(&vNormDisp);

	float fDot = vNormDir.x * vNormDisp.x + vNormDir.z * vNormDisp.z;
	if (fDot < -1.0f) fDot = -1.0f;
	if (fDot > 1.0f) fDot = 1.0f;

	float fAngle = std::acos(fDot);
	float fPerpDist = std::sin(fAngle) * fDistXZ;

	return fPerpDist <= (fRadius + fCandidateRadius);
}

/*
================
TargetSelection_SortByDescendingRatio

[RECONSTRUCTED - Native 0x0058C0E0] (133 bytes)
Sort predicate for chain/heal candidate priority: sorts candidates by health ratio
in descending order (highest damaged percentage / lowest health ratio first).
================
*/
bool TargetSelection_SortByDescendingRatio(CGObjChar* pFirst, CGObjChar* pSecond) {
	if (!pFirst || !pSecond) {
		return false;
	}
	uint32_t dwSecondMax = pSecond->GetMaxHP();
	if (dwSecondMax == 0) dwSecondMax = 1;
	double fSecondRatio = (static_cast<double>(pSecond->GetCurrentHP()) / static_cast<double>(dwSecondMax)) * 100.0;

	uint32_t dwFirstMax = pFirst->GetMaxHP();
	if (dwFirstMax == 0) dwFirstMax = 1;
	double fFirstRatio = (static_cast<double>(pFirst->GetCurrentHP()) / static_cast<double>(dwFirstMax)) * 100.0;

	return fFirstRatio > fSecondRatio;
}

/*
================
TargetSelection_EligiblePrimary

[RECONSTRUCTED - Native 0x0058A268] (shared across spatial selectors)
Evaluates primary entity candidate eligibility based on relation flags (+0x14):
  - bit 1 (0x02): generalAllowed
  - bit 2 (0x04): same party required
  - bit 3 (0x08): hostile target check
================
*/
bool TargetSelection_EligiblePrimary(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	uint32_t dwRelationFlags,
	uint32_t& dwGeneralAllowed,
	bool bChain
) {
	if (!pCaster || !pTarget || !pRefSkill) {
		return false;
	}

	if (dwRelationFlags & 2) {
		dwGeneralAllowed = 1;
	}

	uint32_t dwHostile = 0;
	if (dwGeneralAllowed) {
		if (tagRefSkill_MatchesExecutionSelector(pRefSkill)) {
			if (pCaster->GetSkillManager()) {
				dwHostile = pCaster->GetSkillManager()->IsHostileTargetEligible(pTarget);
			}
			if (dwHostile) {
				if (dwRelationFlags & 8) {
					return (dwGeneralAllowed != 0) || (dwHostile != 0);
				}
				dwGeneralAllowed = 0;
				dwHostile = 0;
			}
		} else if (pCaster->IsPlayer() && pTarget->IsPlayer() && pTarget->SameRelation(pCaster)) {
			return false;
		}

		bool bParty = false;
		if (pCaster->GetSkillManager()) {
			bParty = pCaster->GetSkillManager()->IsSameParty(pTarget);
		}
		if (bParty && !(dwRelationFlags & 4)) {
			dwGeneralAllowed = 0;
			return dwHostile != 0;
		}
		return (dwGeneralAllowed != 0) || bParty || (dwHostile != 0);
	}

	if ((dwRelationFlags & 8) && tagRefSkill_MatchesExecutionSelector(pRefSkill)) {
		if (pCaster->GetSkillManager()) {
			dwHostile = pCaster->GetSkillManager()->IsHostileTargetEligible(pTarget);
		}
		if ((!bChain || dwHostile == 1) && pCaster->IsPlayer() && pTarget->IsPlayer() && !pTarget->SameRelation(pCaster)) {
			return false;
		}
		if (dwHostile) {
			return true;
		}
	}

	if (!(dwRelationFlags & 4)) {
		return false;
	}
	bool bParty = false;
	if (pCaster->GetSkillManager()) {
		bParty = pCaster->GetSkillManager()->IsSameParty(pTarget);
	}
	return bParty || (dwHostile != 0);
}

/*
================
TargetSelection_EligibleSecondary

[RECONSTRUCTED - Native 0x0058A528]
Evaluates secondary entity candidate eligibility:
  - Requires slot +0x18 (IsCharacter == true)
  - Not self, not initial target
  - Life state must be ALIVE (GetLifeState == 1)
  - Not excluded runtime class / PickPet COS (slot +0x43C)
  - If execution selector matches -> hostile check
  - Else -> Detonate parameter (Dtnt @ +0x3D0) and target IsNPC (slot +0x28)
================
*/
bool TargetSelection_EligibleSecondary(
	CGObjChar* pCaster,
	CGObjChar* pInitial,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	uint32_t dwRelationFlags
) {
	if (!pCaster || !pTarget || !pRefSkill) {
		return false;
	}
	if (!(dwRelationFlags & 0x10)) {
		return false;
	}
	if (!pTarget->IsCharacter() || pTarget == pCaster || pTarget == pInitial) {
		return false;
	}
	if (pTarget->GetLifeState() != 1) {
		return false;
	}
	if (pTarget->IsPickPetCOS()) {
		return false;
	}

	if (tagRefSkill_MatchesExecutionSelector(pRefSkill)) {
		return (pCaster->GetSkillManager() != nullptr) &&
		       (pCaster->GetSkillManager()->IsHostileTargetEligible(pTarget) != 0);
	} else {
		return (pRefSkill->pDtnt != nullptr) && pTarget->IsNPC();
	}
}

/*
================
TargetSelection_ScanAroundCenter

Spatial scan loop over candidate entities around center point within radius.
================
*/
template<typename TFilter>
static void TargetSelection_ScanAroundCenter(
	CGObjChar* pCaster,
	CGObjChar* pInitial,
	const tagRefSkill* pRefSkill,
	uint32_t dwRelationFlags,
	uint16_t wCenterRegion,
	const SRO_Vector3D& vCenterPos,
	float fSearchRadius,
	bool bChain,
	TFilter&& acceptCallback
) {
	uint32_t dwGeneralAllowed = 0;

	for (tagCharListNode* pNode = g_pCharacterListHead; pNode != nullptr; pNode = pNode->pNext) {
		CGObjChar* pTarget = static_cast<CGObjChar*>(pNode->pOwner);
		if (!pTarget || pTarget == pInitial) {
			continue;
		}
		if (pTarget->GetLifeState() != 1) {
			continue;
		}
		if (pCaster->GetWorldID() != pTarget->GetWorldID()) {
			continue;
		}
		if (!Pos_RegionsCompatible(wCenterRegion, pTarget->m_wRegionID)) {
			continue;
		}

		SRO_Vector3D vTargetPos(pTarget->m_fLocalPosX, pTarget->m_fLocalPosY, pTarget->m_fLocalPosZ);
		float fDist = TargetSelection_DistanceBetween(wCenterRegion, vCenterPos, pTarget->m_wRegionID, vTargetPos);
		if (fDist > fSearchRadius) {
			continue;
		}

		if (pTarget == pCaster) {
			if (dwRelationFlags & 1) {
				if (!acceptCallback(pTarget, true)) {
					return;
				}
			}
		} else if (TargetSelection_EligiblePrimary(pCaster, pTarget, pRefSkill, dwRelationFlags, dwGeneralAllowed, bChain)) {
			if (!acceptCallback(pTarget, false)) {
				return;
			}
		}
	}

	if (dwRelationFlags & 0x10) {
		for (tagCharListNode* pNode = g_pCharacterListHead; pNode != nullptr; pNode = pNode->pNext) {
			CGObjChar* pTarget = static_cast<CGObjChar*>(pNode->pOwner);
			if (!pTarget) {
				continue;
			}
			if (pCaster->GetWorldID() != pTarget->GetWorldID()) {
				continue;
			}
			if (!Pos_RegionsCompatible(wCenterRegion, pTarget->m_wRegionID)) {
				continue;
			}

			SRO_Vector3D vTargetPos(pTarget->m_fLocalPosX, pTarget->m_fLocalPosY, pTarget->m_fLocalPosZ);
			float fDist = TargetSelection_DistanceBetween(wCenterRegion, vCenterPos, pTarget->m_wRegionID, vTargetPos);
			if (fDist > fSearchRadius) {
				continue;
			}

			if (TargetSelection_EligibleSecondary(pCaster, pInitial, pTarget, pRefSkill, dwRelationFlags)) {
				if (!acceptCallback(pTarget, false)) {
					return;
				}
			}
		}
	}
}

/*
================
TargetSelection_SelectShape

Selects entities within specified geometric shape and appends them to skill command.
================
*/
template<typename TShapePredicate>
static void TargetSelection_SelectShape(
	CGObjChar* pCaster,
	CGObjChar* pInitial,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode,
	uint16_t wCenterRegion,
	const SRO_Vector3D& vCenterPos,
	TShapePredicate&& shapePredicate
) {
	uint32_t dwMaximum = pParamEfr[3];
	uint32_t dwAttenuation = pParamEfr[4];
	uint8_t byPower = static_cast<uint8_t>(100U - dwAttenuation);

	TargetSelection_ScanAroundCenter(
		pCaster,
		pInitial,
		pRefSkill,
		pParamEfr[5],
		wCenterRegion,
		vCenterPos,
		300.0f,
		false,
		[&](CGObjChar* pCandidate, bool bSelf) -> bool {
			if (!bSelf && !shapePredicate(pCandidate)) {
				return true;
			}
			uint8_t byOldPower = byPower;
			byPower = TargetSelection_Attenuate(byPower, dwAttenuation);
			pCommand->m_vecTargets.push_back(tagTargetCandidate(pCandidate->m_dwGameID, static_cast<uint8_t>(dwMode), byOldPower));
			return dwMaximum == 0 || pCommand->m_vecTargets.size() < dwMaximum;
		}
	);
}

/*
================
TargetSelection_AroundSource

[RECONSTRUCTED - Native 0x0058A020] (1949 bytes)
Selects targets within spherical radius centered on caster.
================
*/
void TargetSelection_AroundSource(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
) {
	if (!pCaster || !pCommand || !pParamEfr || !pRefSkill) {
		return;
	}

	uint32_t dwRadius = pParamEfr[2];
	uint32_t dwBaseRadius = static_cast<uint32_t>(pCaster->GetCollisionRadius()) + dwRadius;

	if (pCaster->IsNPC()) {
		pCommand->m_vecTargets.clear();
		pTarget = nullptr;
	}

	SRO_Vector3D vCenter(pCaster->m_fLocalPosX, pCaster->m_fLocalPosY, pCaster->m_fLocalPosZ);

	TargetSelection_SelectShape(
		pCaster,
		pTarget,
		pRefSkill,
		pCommand,
		pParamEfr,
		dwMode,
		pCaster->m_wRegionID,
		vCenter,
		[&](CGObjChar* pCandidate) -> bool {
			SRO_Vector3D vCandPos(pCandidate->m_fLocalPosX, pCandidate->m_fLocalPosY, pCandidate->m_fLocalPosZ);
			float fDist = TargetSelection_DistanceBetween(pCaster->m_wRegionID, vCenter, pCandidate->m_wRegionID, vCandPos);
			uint32_t dwBound = dwBaseRadius + static_cast<uint32_t>(pCandidate->GetCollisionRadius());
			return fDist <= static_cast<float>(dwBound);
		}
	);
}

/*
================
TargetSelection_AroundTarget

[RECONSTRUCTED - Native 0x0058A7C0] (1944 bytes)
Selects targets within spherical radius centered on initial target.
================
*/
void TargetSelection_AroundTarget(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
) {
	if (!pCaster || !pCommand || !pParamEfr || !pRefSkill) {
		return;
	}
	if (!pTarget || pCommand->m_vecTargets.size() > 1) {
		return;
	}

	if (pRefSkill->pDtnt != nullptr && pTarget->IsPlayer()) {
		pCommand->m_vecTargets.clear();
	}

	uint32_t dwRadius = pParamEfr[2];
	uint32_t dwBaseRadius = static_cast<uint32_t>(pCaster->GetCollisionRadius()) + dwRadius;
	SRO_Vector3D vCenter(pTarget->m_fLocalPosX, pTarget->m_fLocalPosY, pTarget->m_fLocalPosZ);

	TargetSelection_SelectShape(
		pCaster,
		pTarget,
		pRefSkill,
		pCommand,
		pParamEfr,
		dwMode,
		pTarget->m_wRegionID,
		vCenter,
		[&](CGObjChar* pCandidate) -> bool {
			SRO_Vector3D vCandPos(pCandidate->m_fLocalPosX, pCandidate->m_fLocalPosY, pCandidate->m_fLocalPosZ);
			float fDist = TargetSelection_DistanceBetween(pTarget->m_wRegionID, vCenter, pCandidate->m_wRegionID, vCandPos);
			uint32_t dwBound = dwBaseRadius + static_cast<uint32_t>(pCandidate->GetCollisionRadius());
			return fDist <= static_cast<float>(dwBound);
		}
	);
}

/*
================
TargetSelection_DirectionalRange

[RECONSTRUCTED - Native 0x0058B160] (1802 bytes)
Selects targets within directional cone/cylinder extending from caster towards
initial target or ground point.
================
*/
void TargetSelection_DirectionalRange(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
) {
	if (!pCaster || !pCommand || !pParamEfr || !pRefSkill) {
		return;
	}
	if (!pTarget || pCommand->m_vecTargets.size() > 1) {
		return;
	}

	SRO_Vector3D vCasterPos(pCaster->m_fLocalPosX, pCaster->m_fLocalPosY, pCaster->m_fLocalPosZ);
	SRO_Vector3D vDir;

	if (pCommand->m_byTargetFlags & 0x40) {
		pCommand->m_vecTargets.clear();
		pTarget = nullptr;
		SRO_Vector3D vGround(pCommand->m_f30, pCommand->m_f34, pCommand->m_f38);
		Pos_Relative3D(&vDir, pCaster->m_wRegionID, &vCasterPos, pCommand->m_w2C, &vGround);
	} else {
		if (!pTarget) return;
		SRO_Vector3D vTargetPos(pTarget->m_fLocalPosX, pTarget->m_fLocalPosY, pTarget->m_fLocalPosZ);
		Pos_Relative3D(&vDir, pCaster->m_wRegionID, &vCasterPos, pTarget->m_wRegionID, &vTargetPos);
	}

	vDir.y = 0.0f;
	Vec3_Normalize(&vDir);

	float fRange = pRefSkill->wTargetRange ? static_cast<float>(pRefSkill->wTargetRange) : 150.0f;
	vDir.x *= fRange;
	vDir.y *= fRange;
	vDir.z *= fRange;

	float fShapeRadius = static_cast<float>(pParamEfr[2]);

	TargetSelection_SelectShape(
		pCaster,
		pTarget,
		pRefSkill,
		pCommand,
		pParamEfr,
		dwMode,
		pCaster->m_wRegionID,
		vCasterPos,
		[&](CGObjChar* pCandidate) -> bool {
			return TargetSelection_InDirectionalShape(pCaster, pCandidate, vDir, fShapeRadius);
		}
	);
}

/*
================
TargetSelection_DirectionalTarget

[RECONSTRUCTED - Native 0x0058B870] (1664 bytes)
Selects targets within directional line/penetration centered on initial target.
================
*/
void TargetSelection_DirectionalTarget(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
) {
	if (!pCaster || !pCommand || !pParamEfr || !pRefSkill) {
		return;
	}
	if (!pTarget || pCommand->m_vecTargets.size() > 1) {
		return;
	}

	SRO_Vector3D vCasterPos(pCaster->m_fLocalPosX, pCaster->m_fLocalPosY, pCaster->m_fLocalPosZ);
	SRO_Vector3D vTargetPos(pTarget->m_fLocalPosX, pTarget->m_fLocalPosY, pTarget->m_fLocalPosZ);
	SRO_Vector3D vDir;
	Pos_Relative3D(&vDir, pCaster->m_wRegionID, &vCasterPos, pTarget->m_wRegionID, &vTargetPos);

	float fShapeRadius = static_cast<float>(pParamEfr[2]);

	TargetSelection_SelectShape(
		pCaster,
		pTarget,
		pRefSkill,
		pCommand,
		pParamEfr,
		dwMode,
		pTarget->m_wRegionID,
		vTargetPos,
		[&](CGObjChar* pCandidate) -> bool {
			return TargetSelection_InDirectionalShape(pCaster, pCandidate, vDir, fShapeRadius);
		}
	);
}

/*
================
TargetSelection_Party

[RECONSTRUCTED - Native 0x0058BEF0] (496 bytes)
Selects party members within radius of caster.
================
*/
void TargetSelection_Party(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
) {
	(void)pTarget;
	if (!pCaster || !pCommand || !pParamEfr || !pRefSkill) {
		return;
	}
	if (!pCaster->IsPlayer()) {
		return;
	}

	uint32_t dwRadius = pParamEfr[2];
	uint32_t dwRelations = pParamEfr[5];

	if (dwRelations & 1) {
		pCommand->m_vecTargets.push_back(tagTargetCandidate(pCaster->m_dwGameID, static_cast<uint8_t>(dwMode), 100));
	}

	if (!pCaster->m_pParty) {
		return;
	}

	SRO_Vector3D vCasterPos(pCaster->m_fLocalPosX, pCaster->m_fLocalPosY, pCaster->m_fLocalPosZ);

	for (tagCharListNode* pNode = g_pCharacterListHead; pNode != nullptr; pNode = pNode->pNext) {
		CGObjChar* pMember = static_cast<CGObjChar*>(pNode->pOwner);
		if (!pMember || pMember == pCaster) {
			continue;
		}
		if (pMember->m_pParty != pCaster->m_pParty) {
			continue;
		}
		if (!pRefSkill->pResu && pMember->GetLifeState() != 1) {
			continue;
		}
		if (pCaster->GetWorldID() != pMember->GetWorldID()) {
			continue;
		}
		if (!Pos_RegionsCompatible(pCaster->m_wRegionID, pMember->m_wRegionID)) {
			continue;
		}

		SRO_Vector3D vMemberPos(pMember->m_fLocalPosX, pMember->m_fLocalPosY, pMember->m_fLocalPosZ);
		float fDist = TargetSelection_DistanceBetween(pCaster->m_wRegionID, vCasterPos, pMember->m_wRegionID, vMemberPos);
		if (fDist <= static_cast<float>(dwRadius)) {
			pCommand->m_vecTargets.push_back(tagTargetCandidate(pMember->m_dwGameID, static_cast<uint8_t>(dwMode), 100));
		}
	}
}

/*
================
TargetSelection_Chain

[RECONSTRUCTED - Native 0x0058C170] (2284 bytes)
Selects chain-jump targets based on distance, attenuation, and health ratio priority.
================
*/
void TargetSelection_Chain(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
) {
	if (!pCaster || !pCommand || !pParamEfr || !pRefSkill) {
		return;
	}

	std::vector<CGObjChar*> vecCandidates;
	uint32_t dwRadius = pParamEfr[2];
	uint32_t dwMaximum = pParamEfr[3];
	uint32_t dwAttenuation = pParamEfr[4];
	uint32_t dwRelations = pParamEfr[5];

	uint8_t byPower = static_cast<uint8_t>(100U - dwAttenuation);
	float fSearchRadius = static_cast<float>(dwMaximum + 1U) * static_cast<float>(dwRadius);
	if (fSearchRadius > 450.0f) {
		fSearchRadius = 450.0f;
	}

	if (dwRelations == 4 || dwRelations == 5) {
		if (dwRelations & 1) {
			vecCandidates.push_back(pCaster);
		}
		if (pCaster->m_pParty) {
			for (tagCharListNode* pNode = g_pCharacterListHead; pNode != nullptr; pNode = pNode->pNext) {
				CGObjChar* pCandidate = static_cast<CGObjChar*>(pNode->pOwner);
				if (pCandidate && pCandidate != pCaster && pCandidate != pTarget &&
				    pCandidate->m_pParty == pCaster->m_pParty && pCandidate->GetLifeState() == 1) {
					vecCandidates.push_back(pCandidate);
				}
			}
		}
	} else {
		if (!pTarget || pCommand->m_vecTargets.size() > 1) {
			return;
		}
		SRO_Vector3D vCenter(pTarget->m_fLocalPosX, pTarget->m_fLocalPosY, pTarget->m_fLocalPosZ);
		TargetSelection_ScanAroundCenter(
			pCaster,
			pTarget,
			pRefSkill,
			dwRelations,
			pTarget->m_wRegionID,
			vCenter,
			fSearchRadius,
			true,
			[&](CGObjChar* pCandidate, bool) -> bool {
				vecCandidates.push_back(pCandidate);
				return true;
			}
		);
	}

	// Health sorting prioritization (Eshp @ +0x3A0)
	if (pRefSkill->pEshp != nullptr && !pTarget) {
		if (vecCandidates.empty()) {
			return;
		}
		std::sort(vecCandidates.begin(), vecCandidates.end(), TargetSelection_SortByDescendingRatio);

		SRO_Vector3D vCasterPos(pCaster->m_fLocalPosX, pCaster->m_fLocalPosY, pCaster->m_fLocalPosZ);
		for (auto it = vecCandidates.begin(); it != vecCandidates.end(); ++it) {
			CGObjChar* pCand = *it;
			if (!pCand) continue;
			SRO_Vector3D vCandPos(pCand->m_fLocalPosX, pCand->m_fLocalPosY, pCand->m_fLocalPosZ);
			float fDist = TargetSelection_DistanceBetween(pCaster->m_wRegionID, vCasterPos, pCand->m_wRegionID, vCandPos);
			if (fDist <= static_cast<float>(dwRadius)) {
				pTarget = pCand;
				vecCandidates.erase(it);
				pCommand->m_vecTargets.push_back(tagTargetCandidate(pTarget->m_dwGameID, static_cast<uint8_t>(dwMode), 100));
				break;
			}
		}
	}

	if (!pTarget) {
		return;
	}

	int32_t nRounds = static_cast<int32_t>(dwMaximum - 1U);
	SRO_Vector3D vTargetPos(pTarget->m_fLocalPosX, pTarget->m_fLocalPosY, pTarget->m_fLocalPosZ);

	for (int32_t round = 0; round < nRounds; ++round) {
		size_t selectedIdx = vecCandidates.size();
		float fNearestDist = 10000.0f;

		for (size_t i = 0; i < vecCandidates.size(); ++i) {
			CGObjChar* pCand = vecCandidates[i];
			if (!pCand) continue;

			SRO_Vector3D vCandPos(pCand->m_fLocalPosX, pCand->m_fLocalPosY, pCand->m_fLocalPosZ);
			float fDist = TargetSelection_DistanceBetween(pTarget->m_wRegionID, vTargetPos, pCand->m_wRegionID, vCandPos);

			if (fDist > fSearchRadius) {
				vecCandidates[i] = nullptr;
			} else if (fDist <= static_cast<float>(dwRadius) && fDist < fNearestDist) {
				fNearestDist = fDist;
				selectedIdx = i;
			}
		}

		if (selectedIdx == vecCandidates.size()) {
			return;
		}

		uint8_t byOldPower = byPower;
		byPower = TargetSelection_Attenuate(byPower, dwAttenuation);
		pCommand->m_vecTargets.push_back(tagTargetCandidate(vecCandidates[selectedIdx]->m_dwGameID, static_cast<uint8_t>(dwMode), byOldPower));
		vecCandidates[selectedIdx] = nullptr;
	}
}

/*
================
TargetSelection_MonsterGroup

[RECONSTRUCTED - Native 0x0058CA60] (266 bytes)
Selects monster group / AI squad members sharing nest controller.
================
*/
void TargetSelection_MonsterGroup(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
) {
	(void)pRefSkill;
	if (!pCaster || !pCommand || !pParamEfr) {
		return;
	}
	if (!pCaster->IsNPC() || !pTarget) {
		return;
	}

	pCommand->m_vecTargets.clear();

	uint32_t dwRelations = pParamEfr[5];
	if (dwRelations & 1) {
		pCommand->m_vecTargets.push_back(tagTargetCandidate(pCaster->m_dwGameID, static_cast<uint8_t>(dwMode), 100));
	}

	for (tagCharListNode* pNode = g_pCharacterListHead; pNode != nullptr; pNode = pNode->pNext) {
		CGObjChar* pSquadMember = static_cast<CGObjChar*>(pNode->pOwner);
		if (!pSquadMember || pSquadMember == pCaster) {
			continue;
		}
		if (pSquadMember->IsNPC() && pSquadMember->m_wRegionID == pTarget->m_wRegionID) {
			pCommand->m_vecTargets.push_back(tagTargetCandidate(pSquadMember->m_dwGameID, static_cast<uint8_t>(dwMode), 100));
		}
	}
}

// Function pointer table matching native 0x00C63F64
typedef void (*FnTargetSelectionHandler)(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
);

static const FnTargetSelectionHandler g_aTargetSelectionHandlers[8] = {
	nullptr,                              // 0: Unused
	TargetSelection_AroundSource,         // 1: 0x0058A020
	TargetSelection_AroundTarget,         // 2: 0x0058A7C0
	TargetSelection_DirectionalRange,     // 3: 0x0058B160
	TargetSelection_DirectionalTarget,    // 4: 0x0058B870
	TargetSelection_Party,                // 5: 0x0058BEF0
	TargetSelection_Chain,                // 6: 0x0058C170
	TargetSelection_MonsterGroup          // 7: 0x0058CA60
};

/*
================
TargetSelection_DispatchByShape

[RECONSTRUCTED - Native 0x0058CB70] (243 bytes)
Dispatches target selection based on skill shape parameter ('Efr' / 'Efr3').
Decodes shape selector index (1..7), overrides party selection when relations
are 4 or 5, and invokes corresponding handler from g_aTargetSelectionHandlers.
================
*/
uint16_t TargetSelection_DispatchByShape(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
) {
	if (!pParamEfr) {
		ServerFramework_GenerateMiniDump();
		return 0x3006;
	}

	uint32_t dwShape = pParamEfr[1];
	if (dwShape > 7) {
		ServerFramework_GenerateMiniDump();
		return 0x3006;
	}

	if (!pRefSkill || !pCommand) {
		return 0x3006;
	}

	pCommand->m_dwTargetObjID14 = 0;
	CGObjChar* pInitialTarget = pTarget;

	if (!pCommand->m_vecTargets.empty()) {
		uint32_t dwFirstID = pCommand->m_vecTargets[0].dwGlobalID;
		CGObjChar* pCandidate = g_pGame ? g_pGame->FindObjectByID(dwFirstID) : nullptr;
		if (pCandidate && pCandidate->IsCharacter()) {
			pInitialTarget = pCandidate;
			pCommand->m_dwTargetObjID14 = pCandidate->m_dwGameID;
		}
	}

	uint32_t dwDispatch = dwShape;
	if (dwDispatch != 5 && dwDispatch != 6) {
		uint32_t dwRelations = pParamEfr[5];
		if (dwRelations == 4 || dwRelations == 5) {
			dwDispatch = 5;
		}
	}

	if (pCaster && pCaster->IsPlayer()) {
		if (pCaster->m_dwLastAttackSkillID != pRefSkill->dwSkillID) {
			pCaster->m_dwLastAttackSkillID = pRefSkill->dwSkillID;
		}
	}

	if (dwDispatch == 0 || dwDispatch > 7) {
		ServerFramework_GenerateMiniDump();
		return 0x3006;
	}

	FnTargetSelectionHandler pfnHandler = g_aTargetSelectionHandlers[dwDispatch];
	if (pfnHandler) {
		pfnHandler(pCaster, pInitialTarget, pRefSkill, pCommand, pParamEfr, dwMode);
	}

	return 0;
}

uint16_t TargetSelection_DispatchByShape(
	CGObjChar* pCaster,
	tagActiveSkillInstance* pInstance,
	const uint32_t* pParamEfr,
	uint32_t dwMode,
	const tagRefSkill* pRefSkill
) {
	if (!pInstance || !pInstance->m_pCommand) {
		return 0x3006;
	}
	return TargetSelection_DispatchByShape(
		pCaster,
		nullptr,
		pRefSkill,
		pInstance->m_pCommand,
		pParamEfr,
		dwMode
	);
}

/*
================
Skill_ValidateTargetPermissions

[RECONSTRUCTED - Native 0x0058D7A0] (291 bytes)
Validates target permissions against tagRefSkill permission flags:
  - +0x9E: Bypass permission checks (returns true if set)
  - If caster is not PC (IsPlayer == false), bypasses remaining checks
  - Target must not be monster if monster flag (+0x24) forbids
  - Evaluates player targets (+0x99, +0x9A) and self-target (+0x98)
  - Evaluates life state (+0x9F / +0xF8)
  - Evaluates NPC (+0x9B / +0x28) and player target restrictions (+0x9C)
  - Evaluates party requirement (+0x9A requires same party)
  - Evaluates downed target restriction (+0xFC == 8 requires Da parameter at +0x240)
================
*/
bool Skill_ValidateTargetPermissions(
	const tagRefSkill* pSkill,
	CGObjChar* pCaster,
	CGObjChar* pTarget
) {
	if (!pSkill || !pCaster || !pTarget) {
		return false;
	}

	// Native 0x0058D7A0: Check bypass flag at +0x9E
	if (pSkill->byTargetGroupDontCare != 0) { // +0x9E: Bypass permission checks flag
		return true;
	}

	// If caster is not a player, target permissions do not restrict cast
	if (!pCaster->IsPlayer()) {
		return true;
	}

	// Target cannot be a monster for certain skills (Slot +0x24: IsMonster)
	if (pTarget->IsMonster()) {
		return false;
	}

	uint8_t flag99 = pSkill->byTargetGroupAlly; // +0x99
	uint8_t flag9A = pSkill->byTargetGroupParty;  // +0x9A
	uint8_t flag98 = pSkill->byTargetGroupSelf;       // +0x98

	if (flag99 != 0 || flag9A != 0) {
		if (!pTarget->IsPlayer()) {
			return false;
		}
		if (flag98 == 0 && pCaster == pTarget) {
			return false;
		}
	} else if (flag98 == 0 && pCaster == pTarget) {
		return false;
	}

	// Life state check: flag +0x9F requires living entity (GetLifeState != 1)
	uint8_t flag9F = pSkill->byTargetSelectDeadBody; // +0x9F
	if (flag9F != 0) {
		if (pTarget->GetLifeState() == 1) { // 1 = Dead / Ghost
			return false;
		}
	}

	// NPC check: flag +0x9B
	uint8_t flag9B = pSkill->byTargetGroupEnemyMonster; // +0x9B
	uint8_t flag9C = pSkill->byTargetGroupEnemyPlayer;   // +0x9C
	if (flag9B == 0) {
		if (pTarget->IsNPC()) {
			return false;
		}
	} else if (flag9C == 0 && pTarget->IsPlayer()) {
		return false;
	}

	if (flag9C != 0 && flag9B == 0 && !pTarget->IsPlayer()) {
		return false;
	}

	// Party check: flag9A requires same party when flag99 == 0 and flag98 == 0
	if (flag99 == 0 && flag98 == 0 && flag9A != 0) {
		if (pCaster->m_pParty == nullptr || pTarget->m_pParty == nullptr) {
			return false;
		}
		if (pCaster->GetPartyID() != pTarget->GetPartyID()) {
			return false;
		}
	}

	// Down / Prone state check: MotionState 8 requires Da parameter at +0x240
	if (pTarget->GetMotionState() == 8) {
		const uint32_t* pDa = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pSkill) + 0x240);
		if (*pDa == 0) {
			return false;
		}
	}

	return true;
}

/*
================
Skill_ValidateBuffExclusionList

[RECONSTRUCTED - Native 0x0059DC80] (334 bytes)
Validates active buffs and links against incoming skill exclusion rules:
  - Iterates m_listActiveBuffs (+0x268) in CSkillManager
  - Compares Lnks (+0x370) and Lks2 (+0x374) records
  - Returns 0x300C (SKILL_ERR_SKILL_ON_COOLDOWN) on conflict, or 0x3037 / 0x3029 on max stack limit
================
*/
uint16_t Skill_ValidateBuffExclusionList(
	CSkillManager* pSkillMgr,
	const tagRefSkill* pSkill,
	CGObjChar* pTarget
) {
	if (!pSkillMgr || !pSkill) {
		return SKILL_SUCCESS;
	}

	const uint8_t* pSkillBytes = reinterpret_cast<const uint8_t*>(pSkill);
	const uint32_t* pIncomingLnks = *reinterpret_cast<const uint32_t* const*>(pSkillBytes + 0x370);
	if (!pIncomingLnks) {
		return SKILL_SUCCESS;
	}

	uint32_t dwLks2 = *reinterpret_cast<const uint32_t*>(pSkillBytes + 0x374);
	uint32_t dwMatchCount = 0;

	for (tagActiveSkillInstance* pActive : pSkillMgr->GetActiveBuffs()) {
		if (!pActive || !pActive->m_pExecution) {
			continue;
		}

		tagSkillExecutionContext* pExec = pActive->m_pExecution;
		const tagRefSkill* pActiveRef = pExec->m_pRefSkill;
		if (!pActiveRef) {
			continue;
		}

		const uint8_t* pActiveBytes = reinterpret_cast<const uint8_t*>(pActiveRef);
		// Check +0x65 flag and mode byte == 1
		if (pActiveBytes[0x65] == 0 || pExec->m_byMode != 1) {
			continue;
		}

		// Active cast link pointer must be present (+0x68)
		tagCastLink* pLink = pExec->m_pCastLink;
		if (!pLink) {
			continue;
		}

		const uint32_t* pActiveLnks = *reinterpret_cast<const uint32_t* const*>(pActiveBytes + 0x370);
		if (!pActiveLnks || *pActiveLnks != *pIncomingLnks) {
			continue;
		}

		uint32_t dwActiveLks2 = *reinterpret_cast<const uint32_t*>(pActiveBytes + 0x374);

		// Compare target entity
		bool bSameReceiver = false;
		if (pTarget != nullptr) {
			uint32_t dwTargetGID = pTarget->GetGlobalID();
			uint32_t dwLinkTargetGID = pLink->m_dwTargetActorID;
			bSameReceiver = (dwTargetGID == dwLinkTargetGID);
		}

		if (pSkill->dwSkillID == pActiveRef->dwSkillID) {
			if (dwLks2 != 0) {
				if (dwActiveLks2 != 0 && bSameReceiver) {
					return SKILL_ERR_SKILL_ON_COOLDOWN; // 0x300C
				}
			} else {
				if (dwActiveLks2 == 0) {
					if (bSameReceiver) {
						return SKILL_ERR_SKILL_ON_COOLDOWN; // 0x300C
					}
					dwMatchCount++;
				}
			}
		} else {
			if (dwLks2 == 0) {
				if (dwActiveLks2 == 0 && *pIncomingLnks != 0) {
					return SKILL_ERR_SKILL_ON_COOLDOWN; // 0x300C
				}
			} else if (dwActiveLks2 != 0) {
				if (bSameReceiver) {
					dwMatchCount++;
				}
			}
		}

		uint32_t dwMaxCount = pIncomingLnks[2];
		if (dwMaxCount != 0 && dwMatchCount == dwMaxCount) {
			const uint32_t* pQest = reinterpret_cast<const uint32_t*>(pSkillBytes + 0x49C);
			const uint32_t* pTrap = reinterpret_cast<const uint32_t*>(pSkillBytes + 0x4A0);
			if (*pQest != 0 && *pTrap != 0) {
				return SKILL_ERR_OBSTACLE_BLOCK; // 0x3037
			}
			return 0x3029;
		}
	}

	return SKILL_SUCCESS;
}

/*
================
TargetSelection_Cone

[RECONSTRUCTED - Native 0x0058AF60] (856 bytes)
Tests whether candidate character lies within a directional cone/wedge sector.
Lateral distance = sin(acos(dot(dir, offset))) * distance
Returns true if lateral distance < (candidate collision radius + dwWidth).
================
*/
bool TargetSelection_Cone(
	CGObjChar* pCaster,
	CGObjChar* pCandidate,
	float fDirX,
	float fDirY,
	float fDirZ,
	uint32_t dwWidth
) {
	if (!pCaster || !pCandidate) {
		return false;
	}

	// Compute relative 3D displacement vector between caster and candidate
	SRO_Vector3D vCasterPos(pCaster->m_fPosX, pCaster->m_fPosY, pCaster->m_fPosZ);
	SRO_Vector3D vCandidatePos(pCandidate->m_fPosX, pCandidate->m_fPosY, pCandidate->m_fPosZ);
	SRO_Vector3D vOffset;

	Pos_Relative3D(&vOffset, pCaster->m_wRegionID, &vCasterPos, pCandidate->m_wRegionID, &vCandidatePos);
	vOffset.y = 0.0f;

	float fDistSq = vOffset.x * vOffset.x + vOffset.z * vOffset.z;
	float fDist = std::sqrt(fDistSq);

	float fDirLenSq = fDirX * fDirX + fDirY * fDirY + fDirZ * fDirZ;
	float fDirLen = std::sqrt(fDirLenSq);

	int32_t nCasterRadius = pCaster->GetCollisionRadius();
	int32_t nCandidateRadius = pCandidate->GetCollisionRadius();

	double dBaseRange = static_cast<double>(nCasterRadius) + static_cast<double>(fDirLen);
	if (dBaseRange + static_cast<double>(nCandidateRadius) < static_cast<double>(fDist)) {
		return false;
	}

	// Normalize vectors for angular comparison
	SRO_Vector3D vDir(fDirX, fDirY, fDirZ);
	Vec3_Normalize(&vDir);
	Vec3_Normalize(&vOffset);

	float fDot = vDir.x * vOffset.x + vDir.y * vOffset.y + vDir.z * vOffset.z;
	if (fDot < -1.0f) fDot = -1.0f;
	if (fDot > 1.0f) fDot = 1.0f;

	float fAngle = std::acos(fDot);
	float fSinAngle = std::sin(fAngle);
	float fLateralDist = fSinAngle * fDist;

	uint32_t dwBound = static_cast<uint32_t>(nCandidateRadius) + dwWidth;
	return fLateralDist < static_cast<float>(dwBound);
}

// CORRECTION (Claude): the skill action dispatch table (0x00C63C7C), SkillActionHandler (0x00589B50) and the
// per-category lifecycle handlers used to be reconstructed twice - here and in SkillCast.cpp. Only the
// SkillCast.cpp set is reachable (CSkillManager drives SkillCast::SkillActionHandler), so the copy that lived
// here is gone; SkillCast.cpp is the single reconstruction of 0x00586700 and its siblings.

/*
================
Skill_ApplyPositionEffect

[RECONSTRUCTED - Native 0x005862E0] (1049 bytes)
Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\skill\SkillGlobal.cpp
Caller: SkillAction_Instant (0x00586700) @ 0x00586BDB / 0x00586F8D

Evaluates teleportation and rush/dash position parameters:
  - tel3: Target-relative displacement / rush attack
  - tele: Ground-targeted teleportation / blink
  - tel2: Range-limited blink
Computes 3D displacement vector, enforces maximum range clamp,
adjusts for target bounding box padding, tests collision via
IRegionManager::QueryMovement (0x00CC387C), and allocates/stores
tagSkillPositionResult in the execution context.
================
*/
int32_t Skill_ApplyPositionEffect(
	const uint32_t* pTele,
	CGObjChar* pActor,
	tagActiveSkillInstance* pInstance,
	const uint32_t* pTel2,
	const uint32_t* pTel3
) {
	if ( !pActor || !pInstance ) {
		return 0;
	}

	Skill::sSkillPreEngageData*		pCommand = pInstance->m_pCommand;
	tagSkillExecutionContext*	pExec = pInstance->m_pExecution;
	if ( !pCommand || !pExec ) {
		return 0;
	}

	SRO_Vector3D	vSource;
	vSource.x = pActor->m_fLocalPosX;
	vSource.y = pActor->m_fLocalPosY;
	vSource.z = pActor->m_fLocalPosZ;
	uint16_t	wSourceRegion = pActor->m_wRegionID;

	SRO_Vector3D	vDest = { 0.0f, 0.0f, 0.0f };
	uint16_t	wDestRegion = 0;
	float		fMaxLimit = 0.0f;
	float		fSizePadding = 0.0f;

	if ( pTel3 ) {
		// Target-relative position effect (e.g. rush attack)
		if ( pCommand->m_vecTargets.empty() ) {
			return 0;
		}

		fMaxLimit = static_cast<float>( pTel3[1] );
		pExec->m_dwResultFlags |= 8;

		uint32_t	dwTargetID = pCommand->m_vecTargets.front().dwGlobalID;
		CGObjChar*	pTarget = ObjMgr_FindByID( dwTargetID );
		if ( !pTarget ) {
			return 0;
		}

		wDestRegion = pTarget->m_wRegionID;
		vDest.x = pTarget->m_fLocalPosX;
		vDest.y = pTarget->m_fLocalPosY;
		vDest.z = pTarget->m_fLocalPosZ;

		float	fSourceSize = static_cast<float>( pActor->GetCollisionRadius() );
		float	fTargetSize = static_cast<float>( pTarget->GetCollisionRadius() );
		fSizePadding = fSourceSize + fTargetSize;
	} else {
		// Ground-targeted position effect (e.g. teleport / blink)
		if ( pCommand->m_vecTargetPos.empty() ) {
			return 0;
		}

		if ( pTele ) {
			fMaxLimit = static_cast<float>( pTele[1] );
			pExec->m_dwResultFlags |= 8;
		} else if ( pTel2 ) {
			fMaxLimit = static_cast<float>( pTel2[1] );
			pExec->m_dwResultFlags |= 2;
		}

		const Skill::sSkillTargetPos&	posRec = pCommand->m_vecTargetPos.front();
		wDestRegion = posRec.m_wRegionID;
		vDest.x = static_cast<float>( posRec.m_fX );
		vDest.y = static_cast<float>( posRec.m_fY );
		vDest.z = static_cast<float>( posRec.m_fZ );
	}

	// Compute relative 3D displacement vector between source and destination
	SRO_Vector3D	vDir;
	Pos_Relative3D( &vDir, wSourceRegion, &vSource, wDestRegion, &vDest );

	float	fDistSq = vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z;
	float	fDistance = std::sqrt( fDistSq );

	if ( fDistance > fMaxLimit && fMaxLimit > 0.0f ) {
		// Destination exceeds max range: clamp to fMaxLimit along unit vector
		Vec3_Normalize( &vDir );
		vDir.x *= fMaxLimit;
		vDir.y *= fMaxLimit;
		vDir.z *= fMaxLimit;

		wDestRegion = wSourceRegion;
		vDest.x = vSource.x + vDir.x;
		vDest.y = vSource.y + vDir.y;
		vDest.z = vSource.z + vDir.z;

		Pos_NormalizeOutdoorRegion( &wDestRegion, &vDest );
	} else if ( pTel3 ) {
		// Target-relative: back off along direction vector by size padding
		Vec3_Normalize( &vDir );
		vDest.x -= vDir.x * fSizePadding;
		vDest.y -= 0.0f * fSizePadding;
		vDest.z -= vDir.z * fSizePadding;
	}

	// Set path check flag at actor +0xC04
	pActor->m_dwPathCheckFlag = 1;

	int32_t	nActorMode = ( pActor->IsPlayer() || pActor->IsCOS() ) ? 1 : 0;

	// Query world movement collision via IRegionManager (0x00CC387C)
	if ( NavMesh::g_pRegionManager ) {
		// CORRECTION (Claude): this was an anonymous struct with two DWORDs before the region, which does not match
		// tagNavPos on x64. The source is the actor's own location (+0x7C) so the query starts from its resolved
		// navmesh cell; the destination is only a position and is resolved by the query.
		NavMesh::tagNavPos srcPos = pActor->m_Location;
		NavMesh::tagNavPos dstPos;

		srcPos.wRegionID = wSourceRegion;
		srcPos.fPosX = vSource.x;
		srcPos.fPosY = vSource.y;
		srcPos.fPosZ = vSource.z;

		dstPos.wRegionID = wDestRegion;
		dstPos.fPosX = vDest.x;
		dstPos.fPosY = vDest.y;
		dstPos.fPosZ = vDest.z;

		int32_t	nMoveResult = NavMesh::g_pRegionManager->QueryMovement( nActorMode, 1, &srcPos, &dstPos, nullptr, pActor );
		if ( nMoveResult & 0x10000000 ) {
			pActor->m_dwPathCheckFlag = 0;
			return 0;
		}
	}

	// Allocate and link tagSkillPositionResult
	tagSkillPositionResult*	pPosResult = new tagSkillPositionResult();
	pPosResult->m_dwUnk0 = 0;
	pPosResult->m_bActive = 1;
	pPosResult->m_pad05 = 0;
	pPosResult->m_wRegionID = wDestRegion;
	pPosResult->m_nX = static_cast<int32_t>( vDest.x );
	pPosResult->m_nY = static_cast<int32_t>( vDest.y );
	pPosResult->m_nZ = static_cast<int32_t>( vDest.z );

	pExec->m_pPositionResult = pPosResult;
	return 1;
}

/*
================
SkillAction_Projectile

[RECONSTRUCTED - Native 0x005857B0] (2365 bytes)
Action Handler 1: Projectile / travelling skill execution lifecycle.
Validates cast on Begin (event 0), advances projectile flight during Tick (event 2),
and detonates on destination impact.
================
*/
int32_t SkillAction_Projectile(int32_t nEvent, CGObjChar* pCaster, void* pSkillActor, int32_t nReserved, void* pContext) {
	(void)nReserved;
	(void)pContext;

	if (!pCaster || !pSkillActor) {
		return 1;
	}

	tagActiveSkillInstance* pInstance = reinterpret_cast<tagActiveSkillInstance*>(pSkillActor);
	CSkillManager* pSkillMgr = pCaster->GetSkillManager();
	if (!pSkillMgr || !pInstance->m_pExecution || !pInstance->m_pExecution->m_pRefSkill) {
		return 1;
	}

	const tagRefSkill* pRefSkill = pInstance->m_pExecution->m_pRefSkill;
	Skill::sSkillPreEngageData* pCommand = pInstance->m_pCommand;
	tagSkillExecutionContext* pExec = pInstance->m_pExecution;
	uint32_t dwContextID = pExec->m_dwContextID;

	// Event 6: Cancel
	if (nEvent == 6) {
		pSkillMgr->SendStageEndB071(1, 0, dwContextID);
		if (pCaster->m_pActiveCastInstance == pInstance) {
			pCaster->m_pActiveCastInstance = nullptr;
		}
		return 2; // CastOutcome::Release
	}

	if (nEvent != 0 && nEvent != 2) {
		return 1; // CastOutcome::Ignored
	}

	// Event 0: Begin
	if (nEvent == 0) {
		uint16_t wError = Skill_ValidateCast(pInstance, pCaster, 0xFFFF);
		if (wError != 0) {
			pSkillMgr->SendSkillErrorResponseB070(wError);
			if (pCaster->m_pActiveCastInstance == pInstance) {
				pCaster->m_pActiveCastInstance = nullptr;
			}
			return 2; // CastOutcome::Release
		}

		ComputePreparedSkillCosts(pCaster, pExec, false);

		// Consume projectile ammo if PC is using bow/crossbow
		if (pCaster->IsPlayer()) {
			pCaster->ConsumeAmmo(1);
		}

		pInstance->m_wStatus = 0x3002;
		pInstance->m_dwStartTime = GetTickCount();
		pSkillMgr->AddActiveSkill(pInstance);

		if (pRefSkill->dwCooldown > 0 && pCaster->IsPlayer()) {
			pCaster->RegisterSkillCooldownAndTimer(pRefSkill);
		}

		pSkillMgr->SendCastBeginB070(pInstance, pInstance->m_wStatus);

		if (pRefSkill->dwCastDelay == 0) {
			SkillCombat_EngageSkill(pCaster, pInstance);
			if (pCaster->m_pActiveCastInstance == pInstance) {
				pCaster->m_pActiveCastInstance = nullptr;
			}
			return 2; // CastOutcome::Release
		}

		pInstance->m_byMode = 0;
		pCaster->m_pActiveCastInstance = pInstance;
		return 0; // CastOutcome::Keep
	}

	// Event 2: Tick
	if (nEvent == 2) {
		// Mode 1: Projectile in-flight (Skill::sBowShotResult active)
		if (pInstance->m_byMode == 1) {
			tagSkillTravelTimer* pTravel = pExec->m_pTravelTimer;
			if (!pTravel) {
				return 2;
			}
			if (GetTickCount() - pTravel->dwStartedAt <= pTravel->dwDuration) {
				return 0; // Still flying!
			}
			// Flight completed -> Impact!
			delete pTravel;
			pExec->m_pTravelTimer = nullptr;
			return 2; // Release instance
		}

		if (pInstance->m_byMode != 0) {
			return 0;
		}

		// Mode 0: Cast delay
		if (Skill_ValidateCast(pInstance, pCaster, 0x1C) != 0) {
			pSkillMgr->SendStageEndB071(1, 0, dwContextID);
			if (pCaster->m_pActiveCastInstance == pInstance) {
				pCaster->m_pActiveCastInstance = nullptr;
			}
			return 2;
		}

		if (GetTickCount() - pInstance->m_dwStartTime <= pRefSkill->dwCastDelay) {
			return 0; // Still drawing bow / aiming
		}

		if (Skill_ValidateCast(pInstance, pCaster, 8) != 0) {
			pSkillMgr->SendStageEndB071(1, 0, dwContextID);
			if (pCaster->m_pActiveCastInstance == pInstance) {
				pCaster->m_pActiveCastInstance = nullptr;
			}
			return 2;
		}

		// Resolve target
		uint32_t dwTargetID = (pCommand && !pCommand->m_vecTargets.empty()) ? pCommand->m_vecTargets[0].dwGlobalID : (pCommand ? pCommand->m_dwTargetObjID : 0);
		CGObjChar* pTarget = g_pGame ? g_pGame->FindObjectByID(dwTargetID) : nullptr;
		if (!pTarget) {
			pSkillMgr->SendStageEndB071(1, 0, dwContextID);
			if (pCaster->m_pActiveCastInstance == pInstance) {
				pCaster->m_pActiveCastInstance = nullptr;
			}
			return 2;
		}

		// Allocate Skill::sBowShotResult travel timer and compute flight duration
		tagSkillTravelTimer* pTravel = new tagSkillTravelTimer();
		float fDist = std::hypot(pTarget->m_fPosX - pCaster->m_fPosX, pTarget->m_fPosZ - pCaster->m_fPosZ);
		float fSpeed = static_cast<float>(pRefSkill->wProjectileSpeed);
		pTravel->dwStartedAt = GetTickCount();
		pTravel->dwDuration = (fSpeed <= 0.0f) ? 0 : static_cast<uint32_t>((static_cast<double>(fDist) * 1000.0) / fSpeed);
		pExec->m_pTravelTimer = pTravel;
		pInstance->m_byMode = 1; // Transition to flight mode!

		if (pRefSkill->dwCastDelay > 0) {
			pSkillMgr->SendStageEndB071(1, 0, dwContextID, true);
		}

		SkillCombat_EngageSkill(pCaster, pInstance);

		// Caster is freed from active instance while projectile travels
		if (pCaster->m_pActiveCastInstance == pInstance) {
			pCaster->m_pActiveCastInstance = nullptr;
		}
		return 0; // CastOutcome::Keep (travel timer is active)
	}

	return 1;
}

/*
================
SkillAction_AreaAttack

[RECONSTRUCTED - Native 0x005830B0] (9247 bytes)
Action Handler 3: Area of Effect / Ground-targeted skill execution lifecycle.
Processes ground target validation, secondary entity splash search, and
radial attenuation calculation.
================
*/
int32_t SkillAction_AreaAttack(int32_t nEvent, CGObjChar* pCaster, void* pSkillActor, int32_t nReserved, void* pContext) {
	(void)nReserved;
	(void)pContext;

	if (!pCaster || !pSkillActor) {
		return 1;
	}

	if (nEvent == 6) {
		return 2;
	}

	if (nEvent == 0) {
		return 0;
	}

	if (nEvent == 2) {
		tagActiveSkillInstance* pInstance = reinterpret_cast<tagActiveSkillInstance*>(pSkillActor);
		if (!pInstance || !pInstance->m_pExecution) {
			return 0;
		}

		tagSkillExecutionContext* pExec = pInstance->m_pExecution;

		// Native 0x00584668: Ramp parameter updates (Mom parameter @ +0x57C)
		Skill_UpdateRampContributions(pCaster, pInstance);

		// Native 0x0058482E: Single-target links
		bool bEnd = (pInstance->m_dwRetirement == 0);
		if (pExec->m_pCastLink) {
			CastLifecycle_UpdateLinks(pCaster, pSkillActor, bEnd);
		} else if (pExec->m_pAreaLink) {
			CastLifecycle_UpdateAreaLink(pCaster, pSkillActor, bEnd);
		}

		// Native 0x0058537E: Periodic damage pulses (Summ parameter @ +0x308)
		if (pExec->m_pPeriodicDamage) {
			Skill_ProcessPeriodicDamage(pCaster, pInstance);
		}

		return 0;
	}

	return 1;
}

/*
================
g_adwAbnormalStatusBit

[RECONSTRUCTED - 0x00C63EC8]
Status index to status bit. Every entry is 1 << index except the two the table swaps: index 2 carries 0x08 and
index 3 carries 0x04. CGObjChar_ClearAbnormalStateSlot (0x004A5660) and CGObjChar_CureAbnormalStates
(0x004A56C0) walk the same 32 entries.
================
*/
const uint32_t g_adwAbnormalStatusBit[32] = {
	0x00000001, 0x00000002, 0x00000008, 0x00000004, 0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000, 0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000, 0x20000000, 0x40000000, 0x80000000
};


/*
================
g_awBurnStatusRateTable

[RECONSTRUCTED - 0x00C63C94] (141 entries)
The growth curve status 2 reads with its parameter level (0x00590BFA: word [level * 4 + 0x00C63C94]).
================
*/
const uint32_t g_adwBurnStatusRate[141] = {
	0, 8, 9, 11, 12, 14, 15, 17, 19, 20, 22, 24,
	26, 29, 31, 33, 36, 39, 41, 44, 47, 50, 54, 57,
	61, 64, 68, 72, 76, 81, 85, 90, 95, 100, 106, 111,
	117, 123, 129, 136, 142, 149, 157, 164, 172, 180, 189, 197,
	206, 216, 226, 236, 246, 257, 268, 280, 292, 305, 318, 331,
	345, 359, 374, 389, 405, 422, 439, 457, 475, 494, 513, 533,
	554, 576, 598, 621, 645, 670, 695, 721, 749, 777, 806, 835,
	866, 898, 931, 965, 1000, 1036, 1074, 1112, 1152, 1193, 1236, 1280,
	1325, 1371, 1419, 1469, 1520, 1573, 1627, 1683, 1741, 1800, 1862, 1925,
	1991, 2058, 2127, 2199, 2272, 2348, 2427, 2507, 2590, 2676, 2764, 2855,
	2949, 3045, 3145, 3247, 3352, 3461, 3573, 3688, 3806, 3928, 4054, 4183,
	4316, 4454, 4595, 4740, 4890, 5044, 5202, 5365, 5533
};

/*
================
SkillCombat_GetBurnEntry
[RECONSTRUCTED - 0x00590BFA]
================
*/
static uint16_t SkillCombat_GetBurnEntry(uint32_t dwLevel) {
	if (dwLevel >= 141) {
		return 0; // the native reads past the table; the port keeps inside it
	}
	return static_cast<uint16_t>(g_adwBurnStatusRate[dwLevel]);
}

/*
================
tagSkillStatusEffect_Init

[RECONSTRUCTED - 0x005AA450] (45 bytes)
================
*/
static void SkillCombat_InitStatusEffect(tagSkillStatusEffect* pEffect, uint8_t byStatusIndex) {
	pEffect->m_byStatusIndex = byStatusIndex;
	pEffect->m_dwStatusBit = g_adwAbnormalStatusBit[byStatusIndex];
	pEffect->m_byCategory = 0;
	if ((pEffect->m_dwStatusBit & 0x3F) != 0) {
		pEffect->m_byCategory = 1;
	} else if ((pEffect->m_dwStatusBit & 0x017FEFC0) != 0) {
		pEffect->m_byCategory = 2;
	}
}

/*
================
SkillCombat_AppendStatusEffect

[RECONSTRUCTED - 0x00597950 / 0x005AA190] the record is taken from its pool and appended to the hit group's list
The fields every status writes are filled here (0x0059083A - 0x0059087D): who cast it, on whom, and the caster's
account when it is a player.
================
*/
static tagSkillStatusEffect* SkillCombat_AppendStatusEffect(
	tagSkillTargetHitGroup* pRec,
	uint8_t byStatusIndex,
	CGObjChar* pCaster,
	CGObjChar* pTarget
) {
	pRec->m_listStatus.push_back(tagSkillStatusEffect());
	tagSkillStatusEffect* pEffect = &pRec->m_listStatus.back();
	std::memset(pEffect, 0, sizeof(*pEffect));

	SkillCombat_InitStatusEffect(pEffect, byStatusIndex);

	pEffect->m_dwCasterID = pCaster->GetGlobalID();
	pEffect->m_wCasterTID = pCaster->GetTID().wType;
	pEffect->m_dwTargetID = pTarget->GetGlobalID();
	pEffect->m_dwCasterJID = 0;
	pEffect->m_dw5C = 0;
	if (pCaster->IsPlayer()) {
		pEffect->m_dwCasterJID = pCaster->GetJID(); // 0x0059087A: slot 2
	}
	return pEffect;
}

/*
================
SkillCombat_RollDamageOverTimeStatus

[RECONSTRUCTED - 0x005907A6 - 0x00590887 and its five repeats]
The shape the first six statuses share. The roll takes the skill's probability plus the target's parameter
0xA9; the strength is the skill's level scaled down by the target's resist (parameter 0x1B + index) and then
lowered by the target's reduction (parameter 0x91 + index). Nothing is recorded when nothing is left.
================
*/
static tagSkillStatusEffect* SkillCombat_RollDamageOverTimeStatus(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	tagSkillTargetHitGroup* pRec,
	const uint32_t* pParam,
	uint8_t byStatusIndex,
	uint8_t byRankIndex,
	uint32_t dwRollKey,
	uint32_t dwResistParamID,
	uint32_t dwReduceParamID,
	int32_t nStatusResistBase
) {
	CSkillManager* pSkillMgr = pCaster->GetSkillManager();
	if (pSkillMgr == nullptr) {
		return nullptr;
	}

	if (!pSkillMgr->RollProbability(static_cast<int32_t>(pParam[1]) + nStatusResistBase, dwRollKey)) {
		return nullptr; // 0x00590784
	}

	const int32_t nResist = static_cast<int32_t>(pTarget->GetParamFloat(dwResistParamID));
	if (nResist >= 100) {
		return nullptr; // 0x005907A6: a fully resistant target takes nothing
	}

	int32_t nLevel = static_cast<int32_t>(
		static_cast<double>(100 - nResist) / 100.0 * static_cast<double>(static_cast<int32_t>(pParam[0])));
	nLevel -= static_cast<int32_t>(pTarget->GetParamFloat(dwReduceParamID));
	if (nLevel <= 0) {
		return nullptr; // 0x005907F8
	}

	tagSkillStatusEffect* pEffect = SkillCombat_AppendStatusEffect(pRec, byStatusIndex, pCaster, pTarget);
	pEffect->m_byGrade = 0;
	pEffect->m_wLevel = static_cast<uint16_t>(nLevel);
	pEffect->m_dwChance = pParam[1];
	// 0x00590832: the duration of a damage over time status comes from the same growth table the monster
	// ranks use, indexed by the status and its strength
	pEffect->m_dwDuration = static_cast<uint32_t>(
		Formulae::GetMonsterRankMultiplier(byRankIndex, pEffect->m_wLevel));
	return pEffect;
}

/*
================
SkillCombat_RollControlStatus

[RECONSTRUCTED - 0x00590F06 - 0x005910F4 and its sixteen repeats]
The shape the statuses from index 6 up share. The level gap between the target and the skill's own level costs
2.5 % of the duration and 5 % of the probability per level, each with its own floor (half the duration, a tenth
of the probability), and the skill modifiers registered for this status shift the result before the roll.
================
*/
static tagSkillStatusEffect* SkillCombat_RollControlStatus(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	tagSkillTargetHitGroup* pRec,
	const uint32_t* pParam,
	uint8_t byStatusIndex,
	uint32_t dwRollKey,
	int32_t nStatusResistBase,
	int32_t nResistFlag
) {
	CSkillManager* pSkillMgr = pCaster->GetSkillManager();
	if (pSkillMgr == nullptr) {
		return nullptr;
	}

	// 0x00590F1F: the target's level against the skill's, ten levels per parameter step
	const int32_t nSkillLevel = (static_cast<int32_t>(pParam[2]) + nResistFlag) * 10;
	int32_t nLevelGap = static_cast<int32_t>(pTarget->GetLevel()) - nSkillLevel;
	if (nLevelGap < 0) {
		nLevelGap = 0; // 0x00590F44
	}

	// 0x00590F4A: the duration falls 2.5 % per level and never below half
	int32_t nDuration = static_cast<int32_t>(
		static_cast<double>(static_cast<int32_t>(pParam[0])) *
		(1.0 - static_cast<double>(nLevelGap) * 2.5 / 100.0));
	const int32_t nDurationFloor = static_cast<int32_t>(static_cast<double>(static_cast<int32_t>(pParam[0])) * 0.5);
	if (nDuration < nDurationFloor) {
		nDuration = nDurationFloor; // 0x00590FE9
	}

	// 0x00590F89: the probability falls 5 % per level and never below a tenth
	int32_t nChance = static_cast<int32_t>(
		static_cast<double>(static_cast<int32_t>(pParam[1])) *
		(1.0 - static_cast<double>(nLevelGap * 5) / 100.0));
	const int32_t nChanceFloor = static_cast<int32_t>(static_cast<double>(static_cast<int32_t>(pParam[1])) * 0.1);
	if (nChance < nChanceFloor) {
		nChance = nChanceFloor; // 0x00590FF3
	}

	// [PARTIAL] 0x00591013: CSkillManager_RegisterModifierEntry (0x0059DE50) looks the status up in the
	// caster's modifier table (manager + index * 0x18 + 0x14) and shifts both values. The table is filled by
	// the buff registration the port does not model, so no modifier is applied here.

	nChance += nStatusResistBase; // 0x00591043

	if (!pSkillMgr->RollProbability(nChance, dwRollKey)) {
		return nullptr; // 0x0059107F
	}

	tagSkillStatusEffect* pEffect = SkillCombat_AppendStatusEffect(pRec, byStatusIndex, pCaster, pTarget);
	pEffect->m_byGrade = static_cast<uint8_t>(pParam[2]);
	pEffect->m_wLevel = 0;
	pEffect->m_dwDuration = static_cast<uint32_t>(nDuration);
	return pEffect;
}

/*
================
SkillCombat_RollAbnormalStatus

[PARTIAL - Native 0x00590680] (11956 bytes)
Rolls every abnormal status the skill carries against one target and appends a record for each that lands to
the hit group's status list; the returned mask is what the caller ORs into the record (0x0058F432).
The native code has all twenty three rolls written out one after another - the two shapes above are what those
blocks share, and each status below adds the payload words its own parameter record carries.

Not ported: the skill modifier table the control statuses consult (0x0059DE50), and the two statuses that have
no parameter of their own (index 0x0C and 0x17), which the native does not roll either.
================
*/
uint32_t SkillCombat_RollAbnormalStatus(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	tagSkillTargetHitGroup* pRec,
	const tagRefSkill* pRefSkill,
	const tagSkillHitResult* pHit
) {
	if (pCaster == nullptr || pTarget == nullptr || pRec == nullptr || pRefSkill == nullptr) {
		return 0;
	}

	// 0x005906A6: an attack companion, a special target record and a fortress structure take no status
	if (pTarget->IsAttackCOS() || pRec->m_byDamageKind == 2 || pTarget->IsFortressStructure()) {
		return 0;
	}

	const int32_t nStatusResistBase = static_cast<int32_t>(pTarget->GetParamFloat(0xA9)); // 0x005906E6
	const int32_t nResistFlag = (nStatusResistBase != 0) ? 1 : 0;                         // 0x005906FC

	// 0x0059071D: a hit that was answered (flag 8) blocks every status unless the skill overrides it (+0x450)
	bool bBlocked = false;
	if (pHit != nullptr) {
		if ((pHit->m_byFlags & 8) != 0 && pRefSkill->Param(0x450) == nullptr) {
			bBlocked = true;
		}
	}

	uint32_t dwStatusMask = 0;
	const uint32_t* pParam = nullptr;
	tagSkillStatusEffect* pEffect = nullptr;

	// --- the six damage over time statuses (0x0059075C - 0x00590FA0) -------------------------------------
	if ((pParam = pRefSkill->Param(0x30C)) != nullptr && !bBlocked) {
		if (SkillCombat_RollDamageOverTimeStatus(pCaster, pTarget, pRec, pParam, 0, 0, 0x01000000, 0x1B, 0x91,
			nStatusResistBase) != nullptr) {
			dwStatusMask |= 0x00000001; // 0x00590881
		}
	}
	if ((pParam = pRefSkill->Param(0x310)) != nullptr && !bBlocked) {
		if (SkillCombat_RollDamageOverTimeStatus(pCaster, pTarget, pRec, pParam, 1, 1, 0x02000000, 0x1C, 0x92,
			nStatusResistBase) != nullptr) {
			dwStatusMask |= 0x00000002;
		}
	}
	if ((pParam = pRefSkill->Param(0x314)) != nullptr && !bBlocked) {
		pEffect = SkillCombat_RollDamageOverTimeStatus(pCaster, pTarget, pRec, pParam, 3, 3, 0x03000000, 0x1D,
			0x93, nStatusResistBase);
		if (pEffect != nullptr) {
			pEffect->m_dwShockParam = pParam[2]; // 0x00590AAA
			dwStatusMask |= 0x00000004;
		}
	}
	if ((pParam = pRefSkill->Param(0x318)) != nullptr && !bBlocked) {
		pEffect = SkillCombat_RollDamageOverTimeStatus(pCaster, pTarget, pRec, pParam, 2, 2, 0x04000000, 0x1E,
			0x94, nStatusResistBase);
		if (pEffect != nullptr) {
			// 0x00590BD3: this status carries an overlap limit and a rate word of its own
			const uint32_t* pOverlap = pRefSkill->Param(0x384);
			pEffect->m_dwOverlapLimit = (pOverlap != nullptr) ? pOverlap[0] : 2000;
			pEffect->m_fBurnRate = static_cast<float>(static_cast<int32_t>(pParam[1]));
			pEffect->m_dwChance = pParam[1];
			pEffect->m_wBurnEntry = SkillCombat_GetBurnEntry(pParam[2]); // 0x00590C02
			dwStatusMask |= 0x00000008;
		}
	}
	if ((pParam = pRefSkill->Param(0x31C)) != nullptr && !bBlocked) {
		pEffect = SkillCombat_RollDamageOverTimeStatus(pCaster, pTarget, pRec, pParam, 4, 4, 0x05000000, 0x1F,
			0x95, nStatusResistBase);
		if (pEffect != nullptr) {
			const uint32_t* pOverlap = pRefSkill->Param(0x384);
			pEffect->m_dwOverlapLimit = (pOverlap != nullptr) ? pOverlap[0] : 2000;
			pEffect->m_dwPoisonParam = pParam[2]; // 0x00590D77
			// [PARTIAL] 0x00590D8C: parameter 0x500 adds the caster's matching skill modifier, which needs
			// the modifier table the port does not keep.
			dwStatusMask |= 0x00000010;
		}
	}
	if ((pParam = pRefSkill->Param(0x320)) != nullptr && !bBlocked) {
		if (SkillCombat_RollDamageOverTimeStatus(pCaster, pTarget, pRec, pParam, 5, 5, 0x06000000, 0x20, 0x96,
			nStatusResistBase) != nullptr) {
			dwStatusMask |= 0x00000020;
		}
	}

	// --- the control statuses (0x00590F06 - 0x005934F0) -------------------------------------------------
	struct SControlStatus {
		uint32_t dwParamSlot;
		uint8_t  byStatusIndex;
		uint32_t dwRollKey;
	};
	static const SControlStatus kControl[] = {
		{ 0x430, 0x06, 0x07000000 }, { 0x434, 0x07, 0x08000000 }, { 0x438, 0x08, 0x09000000 },
		{ 0x43C, 0x09, 0x0A000000 }, { 0x440, 0x0A, 0x0B000000 }, { 0x444, 0x0B, 0x0C000000 },
		{ 0x44C, 0x0D, 0x0E000000 }, { 0x450, 0x0E, 0x0F000000 }, { 0x454, 0x0F, 0x11000000 },
		{ 0x458, 0x10, 0x12000000 }, { 0x45C, 0x13, 0x13000000 }, { 0x460, 0x14, 0x14000000 },
		{ 0x464, 0x11, 0x15000000 }, { 0x468, 0x12, 0x16000000 }, { 0x46C, 0x15, 0x17000000 },
		{ 0x470, 0x16, 0x18000000 }, { 0x474, 0x18, 0x19000000 }
	};

	for (size_t i = 0; i < sizeof(kControl) / sizeof(kControl[0]); ++i) {
		const SControlStatus& status = kControl[i];
		pParam = pRefSkill->Param(status.dwParamSlot);
		if (pParam == nullptr || bBlocked) {
			continue;
		}

		pEffect = SkillCombat_RollControlStatus(pCaster, pTarget, pRec, pParam, status.byStatusIndex,
			status.dwRollKey, nStatusResistBase, nResistFlag);
		if (g_bDebugSkillActionHandler) {
			ServerFramework::Log_Printf(0x3000000, "RollAbnormalStatus() status %u on %08X: %s",
				static_cast<uint32_t>(status.byStatusIndex), pRec->m_dwTargetID,
				(pEffect != nullptr) ? "applied" : "resisted");
		}
		if (pEffect == nullptr) {
			continue;
		}

		// The payload each status copies out of its own parameter record
		switch (status.byStatusIndex) {
		case 0x0A:
			pEffect->m_dwShortSightParam = pParam[3]; // 0x005919C3
			break;
		case 0x0B:
			pEffect->m_dwPoisonParam = pParam[3];     // 0x00591C00
			pEffect->m_dwBleedParam = pParam[4];      // 0x00591C0C
			break;
		case 0x0D:
			pEffect->m_dwDarknessParam = pParam[3];   // 0x00591E15
			break;
		case 0x0F:
			pEffect->m_dwDiseaseParam = pParam[3];    // 0x005922F3
			break;
		case 0x13:
		case 0x14:
			pEffect->m_dwPoisonParam = pParam[3];     // 0x0059272D / 0x00592936
			break;
		case 0x11:
		case 0x12:
			pEffect->m_dwDecayParam = pParam[3];      // 0x00592B3F / 0x00592D48
			break;
		case 0x15:
		case 0x16:
			pEffect->m_dwPanicParam = pParam[3];      // 0x00592F53 / 0x00593174
			pEffect->m_dwDecayParam = pParam[4];      // 0x00592F5F / 0x00593180
			pEffect->m_dwPoisonParam = pParam[5];     // 0x00592F6B / 0x0059318C
			break;
		case 0x18:
			pEffect->m_dwHiddenParam = pParam[3];     // 0x005934BA
			break;
		default:
			break;
		}

		// 0x00590BBE and its repeats: the statuses that overlap carry the 0x384 limit
		const uint32_t* pOverlap = pRefSkill->Param(0x384);
		if (pOverlap != nullptr) {
			pEffect->m_dwOverlapLimit = pOverlap[0];
		} else if (status.byStatusIndex == 0x0A || status.byStatusIndex == 0x0B ||
			status.byStatusIndex == 0x0F || status.byStatusIndex == 0x10) {
			pEffect->m_dwOverlapLimit = 2000;
		}

		dwStatusMask |= g_adwAbnormalStatusBit[status.byStatusIndex];
	}

	return dwStatusMask;
}

/*
================
SkillCombat_GetSkillMasteryRank

[RECONSTRUCTED - Native 0x0059E770] (80 bytes)
The hit ratio bonus the damage formulae take: the higher of the caster's two required mastery ranks, and 0 for
a skill without the 'getv' record (+0x574).
================
*/
static uint8_t SkillCombat_GetSkillMasteryRank(const tagRefSkill* pRefSkill, const CSkillManager* pSkillMgr) {
	if (pRefSkill == nullptr || pSkillMgr == nullptr || pRefSkill->Param(0x574) == nullptr) {
		return 0;
	}

	uint8_t byRankA = 1;
	uint8_t byRankB = 1;
	if (pRefSkill->dwReqMasteryID[0] != 0) {
		const tagSkillMasteryData* pMastery = pSkillMgr->FindMastery(pRefSkill->dwReqMasteryID[0]);
		byRankA = (pMastery != nullptr && pMastery->m_pRefRecord != nullptr) ? pMastery->m_pRefRecord->GetLevel() : 0;
	}
	if (pRefSkill->dwReqMasteryID[1] != 0) {
		const tagSkillMasteryData* pMastery = pSkillMgr->FindMastery(pRefSkill->dwReqMasteryID[1]);
		byRankB = (pMastery != nullptr && pMastery->m_pRefRecord != nullptr) ? pMastery->m_pRefRecord->GetLevel() : 0;
	}
	return (byRankA > byRankB) ? byRankA : byRankB;
}

/*
================
SkillCombat_GetMonsterDamageScale

[RECONSTRUCTED - Native 0x005874D0] (241 bytes)
Damage multiplier applied when the target is a monster: 1.0 for monster type 0 and 1.3 for type 1, clamped into
[0.1, 10], then raised by the monster class - 1.5 for classes 4, 5 and 6 and 1.8 for class 7.
================
*/
static float SkillCombat_GetMonsterDamageScale(const CGObjChar* pTarget) {
	if (pTarget == nullptr) {
		return 1.0f;
	}

	float fScale = 1.0f;
	const uint8_t byMonsterType = pTarget->GetMonsterType();
	if (byMonsterType == 1) {
		fScale = 1.29999995f;
	} else if (byMonsterType != 0) {
		BSLib::Log_Printf(0x2000001, "Unknown Monster Type Detected!!!!!!!!!!");
	}

	if (fScale < 0.100000001f) {
		fScale = 0.100000001f; // 0x005874FE
	} else if (fScale > 10.0f) {
		fScale = 10.0f;        // 0x00587521
	}

	switch (pTarget->GetMonsterClass()) {
	case 4:
	case 5:
	case 6:
		return static_cast<float>(static_cast<double>(fScale) * 1.5);
	case 7:
		return static_cast<float>(static_cast<double>(fScale) * 1.7999999523162842);
	default:
		return fScale;
	}
}

/*
================
SkillCombat_IsTargetDamageable

[PARTIAL - Native 0x0058E540] (168 bytes)
The native code refuses a caster whose current skill (+0xC0C) forbids it, whose body mode is 8 (dead) or whose
vehicle is in a state that blocks the hit. The vehicle and body mode registries the tail reads (+0xC0C, +0xD18)
are not ported, so only the states the port models are tested.
================
*/
bool SkillCombat_IsTargetDamageable(CGObjChar* pCaster) {
	if (pCaster == nullptr) {
		return false;
	}
	if (pCaster->GetBodyMode() == 8) {
		return false; // 0x0058E55B
	}
	return true;
}

/*
================
SkillCombat_ApplyKnockdown

[PARTIAL - Native 0x0058FEF0 - 0x00590162]
The knockdown record (+0x23C) rolls against the target's level and, when it lands, pushes it twenty units
straight away from the caster: the hit becomes a position hit (kind 4) carrying the destination and the record
carries the same destination for the movement side.
Not ported: the recovery time the reference object holds at +0x25C (0x00590128) and slot 348 (+0x570), which
starts the knocked down motion on the target.
================
*/
static bool SkillCombat_ApplyKnockdown(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	const uint32_t* pKnockdown,
	tagSkillTargetHitGroup* pRec,
	tagSkillHitResult* pHit,
	uint8_t byHitKind
) {
	CSkillManager* pSkillMgr = pCaster->GetSkillManager();
	if (pSkillMgr == nullptr) {
		return false;
	}

	// 0x0058FF3E: the chance the record carries, weighed against the two levels
	const uint8_t byChance = Formulae::CalculateStatusEffectProbability(pTarget, pKnockdown[0], pKnockdown[1]);
	const uint32_t dwRollKey = (byHitKind == 4) ? 0x44000000u : 0x45000000u;
	if (!pSkillMgr->RollProbability(byChance, dwRollKey | (pRefSkill->dwSkillID & 0x00FFFFFF))) {
		return false; // 0x0058FF6D
	}

	pHit->m_byHeader = byHitKind; // 0x0058FF7A

	// 0x0058FFEF: the direction is the caster to target line, flattened, twenty units long
	SRO_Vector3D targetPos(pTarget->m_fLocalPosX, pTarget->m_fLocalPosY, pTarget->m_fLocalPosZ);
	SRO_Vector3D casterPos(pCaster->m_fLocalPosX, pCaster->m_fLocalPosY, pCaster->m_fLocalPosZ);
	SRO_Vector3D dir;
	Pos_Relative3D(&dir, pCaster->m_wRegionID, &casterPos, pTarget->m_wRegionID, &targetPos);
	if (dir.x == SRO_INCOMPATIBLE_POS) {
		return false;
	}
	dir.y = 0.0f;            // 0x00590009
	Vec3_Normalize(&dir);    // 0x00590017

	SRO_Position dest;
	dest.wRegionID = pTarget->m_wRegionID;
	dest.pos.x = static_cast<float>(static_cast<double>(dir.x) * 20.0 + targetPos.x); // 0x00590023, 0x00590059
	dest.pos.y = static_cast<float>(static_cast<double>(dir.y) * 20.0 + targetPos.y);
	dest.pos.z = static_cast<float>(static_cast<double>(dir.z) * 20.0 + targetPos.z);
	Pos_NormalizeOutdoorRegion(&dest.wRegionID, &dest.pos); // 0x005900B1

	pHit->m_wRegion = dest.wRegionID;                        // 0x005900BF
	pHit->m_nPosX = static_cast<int32_t>(dest.pos.x);        // 0x005900C3
	pHit->m_nPosY = static_cast<int32_t>(dest.pos.y);
	pHit->m_nPosZ = static_cast<int32_t>(dest.pos.z);

	pRec->m_dwHasDisplacement = 1;                           // 0x005900FE
	pRec->m_wDisplaceRegion = dest.wRegionID;
	pRec->m_nDisplaceX = static_cast<int32_t>(dest.pos.x);
	pRec->m_nDisplaceY = static_cast<int32_t>(dest.pos.y);
	pRec->m_nDisplaceZ = static_cast<int32_t>(dest.pos.z);
	pRec->m_dw38 = 1;
	pRec->m_dw3C = 1;
	pRec->m_byMotion = 8;                                    // 0x0059011A
	pRec->m_by45 = 0;

	// 0x00590122: the reference object's recovery time plus the skill's cast time, in seconds
	pRec->m_fDisplaceTime = static_cast<float>(
		static_cast<double>(static_cast<int32_t>(pRefSkill->dwCastTime)) / 1000.0 + 0.5);
	return true;
}

/*
================
SkillCombat_CalculateHitOutcome

[PARTIAL - Native 0x0058E5F0] (8298 bytes)
Builds the result batch the 0xB070 cast packet carries: one record per target of the command, each holding the
hit stages the skill rolls. Per stage it rolls the attack rating against the target's parry, rolls the hit
chance, picks the damage by the record kind (weapon damage, level scaled, special target damage or weapon
attack power), adds the fixed / HP ratio / heal parameters and accumulates the total the recipients step applies.

Not ported, each guarded where the native branches into it:
  - the absorb record a target with a damage shield (+0x0C0C) gets (0x0058EC6C, 0x0058EE49): the shield
    instance is not modelled, so no second record is produced
  - the abnormal status roll and its area links (sub_590680, 11956 bytes; 0x0058F429, 0x0058F713)
  - the damage share / reflect pass (CSkillManager_ProcessDamageEffects 0x005A0B80; 0x0058F72F)
  - the knockback and position hits (0x0058FFDA - 0x005903B6), which need the movement controllers
================
*/
int32_t SkillCombat_CalculateHitOutcome(
	CGObjChar* pCaster,
	Skill::sSkillPreEngageData* pCommand,
	tagSkillExecutionContext* pExec
) {
	if (pCaster == nullptr || pCommand == nullptr || pExec == nullptr || pExec->m_pRefSkill == nullptr) {
		return 0;
	}

	const tagRefSkill* pRefSkill = pExec->m_pRefSkill;
	CSkillManager* pSkillMgr = pCaster->GetSkillManager();
	if (pSkillMgr == nullptr) {
		return 0;
	}

	// 0x0058E60B: a skill that carries neither an attack record nor any of the effect parameters produces no
	// result at all, unless its execution selector (+0x28C) is kind 6.
	bool bEffectOnly = false;
	if (pRefSkill->Param(0x230) == nullptr && pRefSkill->Param(0x248) == nullptr) {
		static const uint32_t kEffectParams[] = {
			0x30C, 0x310, 0x314, 0x318, 0x31C, 0x320, 0x430, 0x434, 0x438, 0x43C, 0x440, 0x444, 0x44C,
			0x450, 0x454, 0x458, 0x45C, 0x460, 0x464, 0x234, 0x238, 0x46C, 0x470, 0x400, 0x474, 0x468,
			0x3D0, 0x3C8, 0x424, 0x3CC
		};
		bool bAnyEffect = false;
		for (size_t i = 0; i < sizeof(kEffectParams) / sizeof(kEffectParams[0]); ++i) {
			if (pRefSkill->Param(kEffectParams[i]) != nullptr) {
				bAnyEffect = true;
				break;
			}
		}
		if (!bAnyEffect) {
			const uint32_t* pSelector = pRefSkill->Param(0x28C);
			if (pSelector == nullptr || pSelector[1] != 6) {
				return 0; // 0x0059064F
			}
		}
		bEffectOnly = true;
	}

	// 0x0058E7A7: the context keeps one batch and it is reused across stages of the same cast
	tagSkillResultBatch* pBatch = pExec->m_pResultBatch;
	if (pBatch == nullptr) {
		pBatch = new tagSkillResultBatch();
		pExec->m_pResultBatch = pBatch;
	} else {
		pBatch->m_listTargets.clear();
	}

	pBatch->m_byKind = 1;
	const uint32_t* pStages = pRefSkill->Param(0x288);
	if (pStages != nullptr && pStages[0] == 2) {
		pBatch->m_byKind = static_cast<uint8_t>(pStages[1]); // 0x0058E7DC
	}
	pBatch->m_byDamageStages = 0;
	ASSERT(pBatch->m_byKind > 0); // 0x0058E7EF
	if (pBatch->m_byKind == 0) {
		pBatch->m_byKind = 1;
	}
	pExec->m_dwResultFlags |= 1; // 0x0058E7F4

	// 0x0058E7F8: the attack rating the stages roll against the target's parry
	const uint32_t* pAttackParam = pRefSkill->Param(0x230);
	uint8_t byAttackRating = 0;
	if (pAttackParam != nullptr && (pAttackParam[0] & 4) != 0) {
		byAttackRating = static_cast<uint8_t>(static_cast<int32_t>(pCaster->GetParamFloat(0x0C)));
		const uint32_t* pRatingBonus = pRefSkill->Param(0x244);
		if (pRatingBonus != nullptr) {
			const int32_t nScaled = static_cast<int32_t>(
				static_cast<double>(static_cast<int32_t>(pRatingBonus[1])) / 100.0 * static_cast<double>(byAttackRating));
			byAttackRating = static_cast<uint8_t>(byAttackRating +
				static_cast<uint8_t>(nScaled + static_cast<int32_t>(pRatingBonus[0])));
		}
	}

	const uint32_t dwSkillID = pRefSkill->dwSkillID & 0x00FFFFFF;

	if (g_bDebugSkillActionHandler) {
		ServerFramework::Log_Printf(0x3000000,
			"CalculateHitOutcome() skill %u: %zu targets, %u stages, effect only %d",
			pRefSkill->dwSkillID, pCommand->m_vecTargets.size(),
			static_cast<uint32_t>(pBatch->m_byKind), bEffectOnly ? 1 : 0);
	}

	// 0x0058E8A9: one record per target of the command
	for (size_t nTarget = 0; nTarget < pCommand->m_vecTargets.size(); ++nTarget) {
		const tagTargetCandidate& candidate = pCommand->m_vecTargets[nTarget];

		CGObjChar* pTarget = ObjMgr_FindByID(candidate.dwGlobalID);
		if (pTarget == nullptr || !pTarget->IsChar()) {
			continue; // 0x0058E90C / 0x0058E91D
		}
		if (pRefSkill->Param(0x3D0) != nullptr && !pTarget->IsMonster()) {
			continue; // 0x0058E923: a monster-only skill skips everything else
		}

		pBatch->m_listTargets.push_back(tagSkillTargetHitGroup()); // 0x0058E94A
		tagSkillTargetHitGroup& rec = pBatch->m_listTargets.back();
		std::memset(&rec.m_pad0[0], 0, sizeof(rec.m_pad0));

		// 0x0058E97E: how the damage of this record is rolled
		rec.m_byDamageKind = 0;
		if (pCaster->IsPlayer()) {
			if (pTarget->IsFortressStructure() || pTarget->IsFortressHeart()) {
				rec.m_byDamageKind = 3; // 0x0058EA34
			} else if ((pCaster->GetEquippedPrimaryWeaponTID() & 0xF800) == 0x8000) {
				rec.m_byDamageKind = 4; // 0x0058E9FD: a weapon that only carries its own attack power
			} else if (pTarget->IsMonster() && g_pSpecialTargetManager != nullptr &&
				g_pSpecialTargetManager->ContainsTargetID(pTarget->GetRefObjID())) {
				rec.m_byDamageKind = 2; // 0x0058EA2E
			}
		}

		rec.m_dwTargetID = candidate.dwGlobalID;
		rec.m_byEvasionRate = 0;
		rec.m_dwStatusMask = 0;
		const uint32_t* pAbnormal = pRefSkill->Param(0x23C);
		rec.m_byAbnormalKind = (pAbnormal != nullptr) ? static_cast<uint8_t>(pAbnormal[1]) : 0;
		rec.m_byAttackRating = byAttackRating;
		rec.m_byEvadeCount = 0;
		rec.m_byStageCount = 0;
		rec.m_dwTotalDamage = 0;
		rec.m_dwRecovery = 0;
		rec.m_dw38 = 0;
		rec.m_dw3C = 0;
		rec.m_dwKilled = 0;
		rec.m_dwIsMPDamage = 0;

		// 0x0058EA81: the target's evasion, unless the skill brings its own answer (+0x248). It is the
		// target's own rate (param 0x0A) plus its buffs, lowered by the caster's evasion reduction (0x38).
		if (pRefSkill->Param(0x248) == nullptr) {
			int32_t nEvasion = static_cast<int32_t>(pTarget->GetParamFloat(0x0A));
			if (pAttackParam != nullptr) {
				ASSERT(g_pRefData != nullptr); // 0x0058EAD2
				nEvasion += static_cast<int32_t>(
					Formulae::GetAttackPowerBuff(static_cast<uint8_t>(pAttackParam[0]), pTarget));
			}
			rec.m_byEvasionRate = static_cast<uint8_t>(static_cast<int32_t>(
				static_cast<double>(static_cast<uint8_t>(nEvasion)) /
				(static_cast<double>(pCaster->GetParamFloat(0x38)) / 100.0 + 1.0)));
		}

		// 0x0058EB64: the target's evasion lowers the attack rating this record rolls with
		const uint8_t byTargetAttackRating = static_cast<uint8_t>(static_cast<int32_t>(
			static_cast<double>(rec.m_byAttackRating) /
			(static_cast<double>(pTarget->GetParamFloat(0x39)) / 100.0 + 1.0)));

		const uint32_t* pParryParam = pRefSkill->Param(0x248);
		rec.m_byParryRate = (pParryParam != nullptr) ? static_cast<uint8_t>(pParryParam[0]) : 0;

		// [PARTIAL] 0x0058EC6C: a target carrying a damage shield (+0x0C0C) gets a second record that takes
		// part of the hit. The shield instance is not ported, so every hit lands on the target itself.

		bool bTerminated = false;
		for (int32_t nStage = 0; nStage < static_cast<int32_t>(pBatch->m_byKind) && !bTerminated; ++nStage) {
			rec.m_listHits.push_back(tagSkillHitResult()); // 0x0058ECF0
			tagSkillHitResult& hit = rec.m_listHits.back();
			std::memset(&hit, 0, sizeof(hit));

			// 0x0058ED7F: 2 when the attack rating beats the target, 1 otherwise
			hit.m_byFlags = 1;
			if (!bEffectOnly &&
				pSkillMgr->RollProbability(byTargetAttackRating, 0x43000000u | dwSkillID)) {
				hit.m_byFlags = 2;
			}
			if (pCaster->GetBodyMode() == 1) {
				hit.m_byFlags |= 4; // 0x0058EDC6
			}
			if ((pCommand->m_byTargetFlags & 0x10) != 0) {
				hit.m_byFlags |= 8; // 0x0058EDD7
			}
			if (!pTarget->IsPlayer() && pCaster->IsPlayer()) {
				const int32_t nCritical = static_cast<int32_t>(pCaster->GetParamFloat(0xBC));
				if (pSkillMgr->RollProbability(nCritical, 0x4D000000u | dwSkillID)) {
					hit.m_byFlags |= 0x20; // 0x0058EE35: critical
				}
			}

			// 0x0058F0C1: the target's evasion answers first - when it rolls, the stage is recorded as
			// evaded and no damage is computed for it
			if (!bEffectOnly && pSkillMgr->RollProbability(rec.m_byEvasionRate, 0x42000000u | dwSkillID)) {
				hit.m_byHeader = 2; // 0x0058F0EF
				++rec.m_byEvadeCount;
				if (g_bDebugSkillActionHandler) {
					ServerFramework::Log_Printf(0x3000000,
						"CalculateHitOutcome() stage %d on %08X evaded [evasion %u]",
						nStage, rec.m_dwTargetID, static_cast<uint32_t>(rec.m_byEvasionRate));
				}
				// [PARTIAL] 0x0058F0F7: a target carrying the damage block at +0x0D2C answers an evaded
				// stage with its own damage pass. The block is not ported, so the stage ends here.
				continue;
			}

			// 0x0058F739: the stage connects
			++pBatch->m_byDamageStages;
			if (!bEffectOnly && pSkillMgr->RollProbability(rec.m_byParryRate, 0x48000000u | dwSkillID)) {
				hit.m_byHeader = 0x86; // 0x0058F774: blocked, and the remaining stages are dropped
				rec.m_dwTerminated = 1;
				bTerminated = true;
				continue;
			}

			hit.m_byHeader = 0; // 0x0058F797: a damage hit
			++rec.m_byStageCount;

			hit.m_dwAmount = 0;
			hit.m_dwExtra08 = 0;
			int32_t nMagical = 0;
			int32_t nPhysical = 0;
			const uint8_t byMasteryRank = SkillCombat_GetSkillMasteryRank(pRefSkill, pSkillMgr);

			switch (rec.m_byDamageKind) {
			case 0: {
				// 0x0058F12E: weapon damage, magical and physical halves
				nMagical = Formulae::CalculateMagicalDamage(pRefSkill, hit.m_byFlags, pTarget, pCaster, byMasteryRank);
				nPhysical = Formulae::CalculatePhysicalDamage(pRefSkill, hit.m_byFlags, pTarget, pCaster, byMasteryRank);

				// 0x0058F18E: a skill with the +0x240 record scales both halves while the target is dead-bodied
				const uint32_t* pScale = pRefSkill->Param(0x240);
				if (pScale != nullptr && pTarget->GetBodyMode() == 8) {
					nMagical = static_cast<int32_t>(
						static_cast<double>(static_cast<int32_t>(pScale[0]) * nMagical) / 100.0);
					nPhysical = static_cast<int32_t>(
						static_cast<double>(static_cast<int32_t>(pScale[0]) * nPhysical) / 100.0);
				}
				if (candidate.byMode == 1) {
					nMagical = 0; // 0x0058F24B
				}
				hit.m_dwAmount = static_cast<uint32_t>(nMagical + nPhysical);
				break;
			}
			case 1: {
				// 0x0058F63B: level scaled damage, replaced by the special target damage when there is one
				hit.m_byFlags = 1;
				hit.m_dwAmount = static_cast<uint32_t>(Formulae::RollLevelScaledDamage(pCaster));
				if (pTarget->IsMonster() && g_pSpecialTargetManager != nullptr &&
					g_pSpecialTargetManager->ContainsTargetID(pTarget->GetRefObjID())) {
					hit.m_dwAmount = g_pSpecialTargetManager->GetSpecialDamage(pTarget->GetRefObjID());
				}
				break;
			}
			case 2: {
				// 0x0058F6A4
				hit.m_dwAmount = (g_pSpecialTargetManager != nullptr)
					? g_pSpecialTargetManager->GetSpecialDamage(pTarget->GetRefObjID()) : 0;
				break;
			}
			default: {
				// 0x0058F6C5: the weapon's own attack power
				const uint16_t wWeaponTID = pCaster->GetEquippedPrimaryWeaponTID();
				hit.m_dwAmount = static_cast<uint32_t>(
					Formulae::CalculateWeaponAttackPower(pCaster, static_cast<uint16_t>(wWeaponTID >> 11)));
				break;
			}
			}

			// 0x0058F4B5: the parameters that replace or extend the rolled damage
			const uint32_t* pHeal = pRefSkill->Param(0x400);
			if (pHeal != nullptr) {
				hit.m_dwAmount = static_cast<uint32_t>(
					Formulae::CalculateSkillHeal(pTarget, pRefSkill, pCaster, rec.m_byDamageKind));
			} else {
				const uint32_t* pFixed = pRefSkill->Param(0x234);
				const uint32_t* pRatio = pRefSkill->Param(0x238);
				if (pFixed != nullptr) {
					hit.m_dwAmount += static_cast<uint32_t>(
						Formulae::CalculateFixedSkillDamage(pFixed, pTarget, pRefSkill->Param(0x538), pCaster));
				} else if (pRatio != nullptr) {
					hit.m_dwAmount += static_cast<uint32_t>(Formulae::CalculateHPRatioDamage(pTarget, pRatio));
				}

				// 0x0058F525: extra damage while the target carries one of the listed abnormal states
				const uint32_t* pStateBonus = pRefSkill->Param(0x3BC);
				if (pStateBonus != nullptr && (pStateBonus[0] & pTarget->m_dwAbnormalFlags) != 0) {
					hit.m_dwAmount = static_cast<uint32_t>(
						static_cast<double>(static_cast<int32_t>(hit.m_dwAmount)) *
						(static_cast<double>(static_cast<int32_t>(pStateBonus[1])) / 100.0 + 1.0));
				}

				// 0x0058F58C: the target's share of a multi target skill
				hit.m_dwAmount = static_cast<uint32_t>(
					static_cast<double>(candidate.byAttenuation * static_cast<int32_t>(hit.m_dwAmount)) / 100.0);
			}

			// 0x0058F5D5: monsters take a scaled amount, and never zero
			if (pTarget->IsMonster()) {
				hit.m_dwAmount = static_cast<uint32_t>(
					static_cast<double>(SkillCombat_GetMonsterDamageScale(pTarget)) *
					static_cast<double>(static_cast<int32_t>(hit.m_dwAmount)));
				if (hit.m_dwAmount == 0) {
					hit.m_dwAmount = 1; // 0x0058F62F
				}
			}

			// 0x0058F713: the statuses the skill carries are rolled against this target and their bits are
			// collected on the record
			rec.m_dwStatusMask |= SkillCombat_RollAbnormalStatus(pCaster, pTarget, &rec, pRefSkill, &hit);

			// 0x0058FEF0: a knockdown record pushes the target away and ends the stage loop
			const uint32_t* pKnockdown = pRefSkill->Param(0x23C);
			if (pKnockdown != nullptr && SkillCombat_IsTargetDamageable(pTarget) && candidate.byMode == 0) {
				if (SkillCombat_ApplyKnockdown(pCaster, pTarget, pRefSkill, pKnockdown, &rec, &hit, 4)) {
					bTerminated = true; // 0x0058FFCE
				}
			}

			// 0x00590164: the pull record does the same with its own roll, unless the stage already pushed
			const uint32_t* pPull = pRefSkill->Param(0x254);
			if (pPull != nullptr && hit.m_byHeader != 4 && candidate.byMode == 0) {
				// [PARTIAL] 0x0059017C: the native also requires bit 2 of the target's reference object at
				// +0x258, which the port does not model, so every target is eligible.
				if (SkillCombat_ApplyKnockdown(pCaster, pTarget, pRefSkill, pPull, &rec, &hit, 5)) {
					bTerminated = true; // 0x0059022B
				}
			}

			// 0x0058F72F: the hit is offered to the group the caster fights in
			pSkillMgr->ProcessDamageEffects(pCaster, pTarget, &rec, &hit);

			// 0x0059058D: the record carries what the recipients step hands to CGObjChar::ApplyHit
			rec.m_dwTotalDamage += hit.m_dwAmount;
			if (g_bDebugSkillActionHandler) {
				ServerFramework::Log_Printf(0x3000000,
					"CalculateHitOutcome() stage %d on %08X: kind %u flags %02X amount %u total %u",
					nStage, rec.m_dwTargetID, static_cast<uint32_t>(rec.m_byDamageKind),
					static_cast<uint32_t>(hit.m_byFlags), hit.m_dwAmount, rec.m_dwTotalDamage);
			}
			if (static_cast<int32_t>(pTarget->GetCurrentHP()) <= static_cast<int32_t>(rec.m_dwTotalDamage)) {
				hit.m_byHeader |= 0x80;  // 0x005905A4: the blow that takes the target down
				rec.m_dwTerminated = 1;
				bTerminated = true;
				if (pCaster->GetBodyMode() != 1 && pTarget->GetLifeState() == 1) {
					rec.m_dwKilled = 1; // 0x005905E0
				}
				pExec->m_dwResultFlags |= 0x40;
			}
		}
	}

	return 1;
}

/*
================
SkillCombat_ApplyResultRecipients

[PARTIAL - Native 0x00593800] (1731 bytes)
Hands every record of the batch to its target: the accumulated damage goes through CGObjChar::ApplyHit and a
record that drains instead of damaging is subtracted from the caster's own shield instance.

Not ported: the guild and party area effects the record's +0x70 - +0x78 links carry (0x005938xx), the berserk
point gain roll (0x00593A60) and the damage reduction / recovery pass (0x005939xx), each of which needs a
registry the port does not model.
================
*/
void SkillCombat_ApplyResultRecipients(
	Skill::sSkillPreEngageData* pCommand,
	tagSkillExecutionContext* pExec,
	CGObjChar* pCaster
) {
	if (pExec == nullptr || pCaster == nullptr || pExec->m_pResultBatch == nullptr) {
		return;
	}
	(void)pCommand;

	tagSkillResultBatch* pBatch = pExec->m_pResultBatch;
	for (std::list<tagSkillTargetHitGroup>::iterator it = pBatch->m_listTargets.begin();
		it != pBatch->m_listTargets.end(); ++it) {
		tagSkillTargetHitGroup& rec = *it;

		CGObjChar* pTarget = ObjMgr_FindByID(rec.m_dwTargetID);
		if (pTarget == nullptr || !pTarget->IsChar()) {
			continue; // 0x00593856
		}

		if (rec.m_dwIsMPDamage != 0) {
			// 0x00593C60: an MP draining hit never touches hit points
			const uint32_t dwCurrentMP = pTarget->GetCurrentMP();
			pTarget->SetCurrentMP((dwCurrentMP < rec.m_dwTotalDamage) ? 0 : dwCurrentMP - rec.m_dwTotalDamage);
			continue;
		}

		// 0x00593BA0: a record whose stages ended early takes the target's remaining hit points instead of
		// the accumulated total, so the killing blow is exact
		uint32_t dwDamage = rec.m_dwTotalDamage;
		if (rec.m_dwTerminated != 0) {
			dwDamage = pTarget->GetCurrentHP();
		}

		pTarget->ApplyHit(pCaster, static_cast<int32_t>(dwDamage),
			static_cast<int32_t>(rec.m_dwRecovery), 1, 0); // 0x00593BCE

		// 0x00593BEF: a hidden target keeps whatever it is under; otherwise the damage wakes it
		if ((pTarget->m_dwAbnormalFlags & 0x01000000) == 0) {
			if ((rec.m_dwRecovery & 0x40) != 0) {
				pTarget->ClearAbnormalStateSlot(6); // 0x00593C0C: the sleep ends
				// [PARTIAL] 0x00593C28: slot 343 (+0x55C) tells the movement side the target woke up
			}
			if ((pTarget->m_dwAbnormalFlags & 0x4000) != 0) {
				CSkillManager* pTargetMgr = pTarget->GetSkillManager();
				if (pTargetMgr != nullptr && pTargetMgr->RollProbability(25, 0x10000000)) {
					pTarget->ClearAbnormalStateSlot(0x0E); // 0x00593C57: one hit in four shakes off a stun
				}
			}
		}

		// 0x00593C8F: every status the hit rolled is written into the target's slots
		SkillCombat_ApplyStatusEffects(&rec, pTarget);

		if (g_bDebugSkillActionHandler) {
			ServerFramework::Log_Printf(0x3000000, "ApplyResultRecipients() %08X took %u, HP now %u",
				rec.m_dwTargetID, dwDamage, pTarget->GetCurrentHP());
		}
	}
}

/*
================
SkillCombat_ApplyStatusEffects

[RECONSTRUCTED - Native 0x00593ED0] (341 bytes)
Hands every status the hit group carries to the target and publishes the new state block once at the end, if
at least one of them took hold (0x00593F37). A status that is only written into its slot never reaches the
client otherwise.
================
*/
void SkillCombat_ApplyStatusEffects(tagSkillTargetHitGroup* pRec, CGObjChar* pTarget) {
	if (pRec == nullptr || pTarget == nullptr) {
		return;
	}

	int32_t bAnyApplied = 0; // 0x00593EE7
	for (std::list<tagSkillStatusEffect>::const_iterator it = pRec->m_listStatus.begin();
		it != pRec->m_listStatus.end(); ++it) {
		if (pTarget->ApplyAbnormalStateRecord(*it) != 0) { // 0x00593F0C
			bAnyApplied = 1;                               // 0x00593F15
		}
	}

	if (bAnyApplied != 0) {
		pTarget->SendAbnormalStateUpdate(); // 0x00593F37
	}
}

/*
================
SkillCombat_ApplySkillEffectsToTargets

[PARTIAL - Native 0x00593F50] (2898 bytes)
The non damage half of a cast: summon and transform (+0x4A4 / +0x4A8), resurrect (+0x330), the teleport of the
targets (+0x40C) and the status cures. None of those registries are ported - summoning needs the monster
spawner, resurrect the death queue and the cures the abnormal status table - so the function only reports what
it would do. The buff half of a cast does not run through here: it is applied by
CSkillManager::ApplyBuffModifiersToActor, which SkillCombat_EngageSkill calls.
================
*/
void SkillCombat_ApplySkillEffectsToTargets(tagActiveSkillInstance* pInstance) {
	if (pInstance == nullptr || pInstance->m_pExecution == nullptr) {
		return;
	}

	const tagRefSkill* pRefSkill = pInstance->m_pExecution->m_pRefSkill;
	if (pRefSkill == nullptr) {
		return;
	}

	if (pRefSkill->Param(0x4A4) != nullptr || pRefSkill->Param(0x4A8) != nullptr ||
		pRefSkill->Param(0x330) != nullptr || pRefSkill->Param(0x40C) != nullptr) {
		BSLib::Log_Printf(0x2000001,
			"SkillCombat_ApplySkillEffectsToTargets: skill %u carries a summon / resurrect / teleport effect that is not ported",
			pRefSkill->dwSkillID);
	}
}

/*
================
SkillCombat_EngageSkill

[RECONSTRUCTED - Native 0x00593540] (692 bytes)
Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\skill\SkillCombat.cpp
Executes skill engagement:
1. Debug logging if g_bDebugSkillActionHandler is true
2. Charges HP/MP resources via SetCurrentHP / SetCurrentMP
3. Deducts SP via OffsetSkillPoint if player
4. Evaluates teleportation / position effect if position result is present
5. Checks parameter tags (Rcnt, Dura, Dmgp, Mssn)
6. Queues buff modifiers into CSkillManager
================
*/
void SkillCombat_EngageSkill(CGObjChar* pCaster, tagActiveSkillInstance* pInstance) {
	if (!pCaster || !pInstance) {
		return;
	}

	tagSkillExecutionContext* pExec = pInstance->m_pExecution;
	if (!pExec || !pExec->m_pRefSkill) {
		return;
	}

	const tagRefSkill* pRefSkill = pExec->m_pRefSkill;

	if (g_bDebugSkillActionHandler) {
		ServerFramework::Log_Printf(0x3000000, "EngageSkill() [target:%s]",
			pCaster->GetName() ? pCaster->GetName() : "");
	}

	// 1. Resource deduction
	int32_t nCostHP = pExec->m_nCalculatedHPCost;
	int32_t nCostMP = pExec->m_nCalculatedMPCost;
	pCaster->ConsumeResources(nCostHP, nCostMP, 4); // 593599, virtual +308
	if (pExec->m_byCalculatedBerserkCost && pCaster->IsPlayer()) {
		static_cast<CGObjPC*>(pCaster)->ModifyBerserkPoints(-int32_t(pExec->m_byCalculatedBerserkCost), 0);
	}
	// 593540 does not clear costs; ownership of one-time execution belongs
	// to the action controller, not an accidental zeroing side effect here.

	// 1b. The result batch the hit outcome built is what actually reaches the targets (0x005935D5)
	if (pExec->m_pResultBatch != nullptr) {
		SkillCombat_ApplyResultRecipients(pInstance->m_pCommand, pExec, pCaster);
	}

	// 2. Teleportation / displacement execution
	if (pExec->m_pPositionResult != nullptr) {
		const tagSkillPositionResult* pPos = pExec->m_pPositionResult;
		pCaster->TeleportToCoordinates(pPos->m_wRegionID, static_cast<float>(pPos->m_nX), static_cast<float>(pPos->m_nY), static_cast<float>(pPos->m_nZ));
	}

	// 3. Repeating / duration timers
	if (pRefSkill->Param(0x284) != nullptr) { // 5936EE: repeating-cost instruction
		pExec->m_pRepeatingCost = new tagRepeatingCost();
		if (pExec->m_pRepeatingCost != nullptr) {
			pExec->m_pRepeatingCost->m_dwChargedAt = GetTickCount();
		}
	}

	// 4. Periodic damage timers
	if (pRefSkill->m_pParamDmgp != nullptr) {
		pExec->m_pPeriodicDamage = new tagPeriodicDamagePulse();
		if (pExec->m_pPeriodicDamage != nullptr) {
			pExec->m_pPeriodicDamage->m_dwStartedAt = GetTickCount();
		}
	}

	// 5. The summon / resurrect / teleport half of the cast (0x005937DD)
	SkillCombat_ApplySkillEffectsToTargets(pInstance);
}

/*
================
SkillAction_Continuous_Cancel

[RECONSTRUCTED - Native 0x00587210] (80 bytes)
Cancels a continuous / channeling skill instance.
Checks Dmgr (+0x418) to clear pCaster->m_dwFieldC34 (+0xC34).
Checks Real (+0x300) to cancel active action session in CSkillManager.
================
*/
void* SkillAction_Continuous_Cancel(tagActiveSkillInstance* pActiveSkill, CGObjChar* pCaster) {
	if (!pActiveSkill || !pCaster) {
		return nullptr;
	}

	const tagSkillExecutionContext* pExec = pActiveSkill->m_pExecution;
	if (!pExec || !pExec->m_pRefSkill) {
		return nullptr;
	}

	const tagRefSkill* pRefSkill = pExec->m_pRefSkill;

	// Dmgr (+0x418): Clear field 0xC34 on actor
	const uint32_t* pDmgr = pRefSkill->m_pParamDmgr;
	if (pDmgr != nullptr) {
		pCaster->m_dwFieldC34 = 0;
	}

	// Real (+0x300): Cancel action session in CSkillManager
	const uint32_t* pReal = pRefSkill->m_pParamReal;
	if (pReal != nullptr) {
		CSkillManager* pSkillMgr = pCaster->GetSkillManager();
		if (pSkillMgr != nullptr) {
			pSkillMgr->UpdateRealModifiers(pReal, pExec->m_dwContextID, true);
		}
	}

	return const_cast<tagRefSkill*>(pRefSkill);
}

/*
================
SkillAction_Continuous

[RECONSTRUCTED - Native 0x00587260] (80 bytes)
Action Handler 4: Continuous / Channeling skill execution lifecycle.
Handles Event 3 (Install): EngageSkill + AddActiveSkill.
Handles Event 6 (Cancel): SkillAction_Continuous_Cancel.
Invalid events outside [3, 6] trigger ServerFramework_GenerateMiniDump.
================
*/
int32_t SkillAction_Continuous(int32_t nEvent, CGObjChar* pCaster, void* pSkillActor, int32_t nReserved, void* pContext) {
	(void)nReserved;
	(void)pContext;

	if (!pCaster || !pSkillActor) {
		return 1;
	}

	// Native 0x00587263: if ((uint32_t)(nEvent - 3) > 3) GenerateMiniDump()
	if (static_cast<uint32_t>(nEvent - 3) > 3) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 1;
	}

	tagActiveSkillInstance* pInstance = reinterpret_cast<tagActiveSkillInstance*>(pSkillActor);

	switch (nEvent) {
		case 3: { // Event 3: Install
			SkillCombat_EngageSkill(pCaster, pInstance);
			CSkillManager* pSkillMgr = pCaster->GetSkillManager();
			if (pSkillMgr != nullptr) {
				pSkillMgr->AddActiveSkill(pInstance);
			}
			return 0; // Handled / Keep
		}
		case 6: { // Event 6: Cancel
			SkillAction_Continuous_Cancel(pInstance, pCaster);
			return 0; // Handled / Keep
		}
		default:
			// Events 4 & 5 (WriteMigration, ReadMigration) fall through
			break;
	}

	return 1; // Ignored
}

/*
===============================================================================
Skill Parameter Indexing System [RECONSTRUCTED - Native 0x00587630] (7509 bytes)
Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\skill\SkillGlobal.cpp
Called by: SkillGlobal_LoadReferenceData (0x005893E0) @ 0x005894E9
===============================================================================
*/

struct tagSkillParamFixedRule {
	uint32_t dwTag;
	uint16_t wSlotOffset;
	uint8_t  byWordCount;
	bool     bNormalizeMask;
};

struct tagSkillParamGetValueRule {
	uint32_t dwSelector;
	uint16_t wSlotOffset;
};

static const tagSkillParamFixedRule s_aFixedParamRules[] = {
	{ 0x0000616F, 0x274, 1, false }, // 'ao'
	{ 0x00006172, 0x268, 3, false }, // 'ar'
	{ 0x0000626C, 0x444, 6, false }, // 'bl'
	{ 0x00006272, 0x278, 3, true  }, // 'br'
	{ 0x00006275, 0x318, 4, false }, // 'bu'
	{ 0x00006361, 0x458, 4, false }, // 'ca'
	{ 0x0000636B, 0x248, 2, false }, // 'ck'
	{ 0x00006372, 0x244, 3, false }, // 'cr'
	{ 0x00006461, 0x240, 2, false }, // 'da'
	{ 0x0000646E, 0x44C, 5, false }, // 'dn'
	{ 0x00006473, 0x454, 5, false }, // 'ds'
	{ 0x00006572, 0x27C, 3, false }, // 'er'
	{ 0x00006573, 0x314, 4, false }, // 'es'
	{ 0x00006662, 0x310, 3, false }, // 'fb'
	{ 0x00006665, 0x43C, 4, false }, // 'fe'
	{ 0x0000667A, 0x30C, 3, false }, // 'fz'
	{ 0x00006872, 0x24C, 3, false }, // 'hr'
	{ 0x00006B62, 0x254, 3, false }, // 'kb'
	{ 0x00006B6F, 0x23C, 3, false }, // 'ko'
	{ 0x00006D63, 0x288, 3, false }, // 'mc'
	{ 0x00006D79, 0x440, 5, false }, // 'my'
	{ 0x00007073, 0x31C, 4, false }, // 'ps'
	{ 0x00007077, 0x2B4, 5, true  }, // 'pw'
	{ 0x00007274, 0x434, 4, false }, // 'rt'
	{ 0x00007275, 0x250, 2, false }, // 'ru'
	{ 0x00007365, 0x430, 4, false }, // 'se'
	{ 0x0000736C, 0x438, 4, false }, // 'sl'
	{ 0x00007374, 0x450, 4, false }, // 'st'
	{ 0x00007462, 0x474, 5, false }, // 'tb'
	{ 0x00007A62, 0x320, 3, false }, // 'zb'
	{ 0x00617474, 0x230, 6, true  }, // 'att'
	{ 0x00646172, 0x26C, 3, true  }, // 'dar'
	{ 0x00647275, 0x3E4, 3, false }, // 'dru'
	{ 0x00647474, 0x420, 3, false }, // 'dtt'
	{ 0x00676472, 0x4C8, 2, false }, // 'gdr'
	{ 0x00687069, 0x2AC, 3, false }, // 'hpi'
	{ 0x006D6F6D, 0x57C, 5, false }, // 'mom'
	{ 0x006D7069, 0x2B0, 3, false }, // 'mpi'
	{ 0x006D7372, 0x590, 3, false }, // 'msr'
	{ 0x006E6D66, 0x594, 1, false }, // 'nmf'
	{ 0x006E6D68, 0x598, 1, false }, // 'nmh'
	{ 0x0070616F, 0x350, 2, false }, // 'pao'
	{ 0x00736B63, 0x490, 4, true  }, // 'skc'
	{ 0x00737069, 0x584, 2, false }, // 'spi'
	{ 0x61626972, 0x3F8, 3, false }, // 'abir'
	{ 0x61626E62, 0x41C, 2, false }, // 'abnb'
	{ 0x616C6375, 0x4B4, 2, false }, // 'alcu'
	{ 0x61706175, 0x2D8, 3, false }, // 'apau'
	{ 0x61707275, 0x2D4, 3, false }, // 'apru'
	{ 0x61746361, 0x3BC, 3, false }, // 'atca'
	{ 0x62627566, 0x360, 1, false }, // 'bbuf'
	{ 0x62677261, 0x2F8, 3, false }, // 'bgra'
	{ 0x626C6472, 0x58C, 4, false }, // 'bldr'
	{ 0x63627566, 0x358, 1, false }, // 'cbuf'
	{ 0x63686372, 0x368, 2, false }, // 'chcr'
	{ 0x636D6372, 0x36C, 2, false }, // 'cmcr'
	{ 0x636E736D, 0x2A0, 4, false }, // 'cnsm'
	{ 0x63736870, 0x46C, 7, false }, // 'cshp'
	{ 0x63736974, 0x460, 5, false }, // 'csit'
	{ 0x63736D64, 0x468, 5, false }, // 'csmd'
	{ 0x63736D70, 0x470, 7, false }, // 'csmp'
	{ 0x63737064, 0x464, 5, false }, // 'cspd'
	{ 0x63737372, 0x45C, 5, false }, // 'cssr'
	{ 0x63757261, 0x334, 2, false }, // 'cura'
	{ 0x6375726C, 0x410, 4, false }, // 'curl'
	{ 0x63757274, 0x40C, 3, false }, // 'curt'
	{ 0x64636D70, 0x2A8, 2, false }, // 'dcmp'
	{ 0x64637269, 0x42C, 2, false }, // 'dcri'
	{ 0x64656670, 0x25C, 4, false }, // 'defp'
	{ 0x64656672, 0x260, 3, false }, // 'defr'
	{ 0x64676D70, 0x4CC, 2, false }, // 'dgmp'
	{ 0x646D6772, 0x418, 5, false }, // 'dmgr'
	{ 0x646D6774, 0x3DC, 3, false }, // 'dmgt'
	{ 0x64727532, 0x3E8, 3, true  }, // 'dru2'
	{ 0x64746E74, 0x3D0, 3, false }, // 'dtnt'
	{ 0x64747470, 0x424, 3, false }, // 'dttp'
	{ 0x64757261, 0x280, 2, false }, // 'dura'
	{ 0x65667461, 0x4B0, 1, false }, // 'efta'
	{ 0x65736870, 0x298, 1, false }, // 'eshp'
	{ 0x65736874, 0x29C, 3, false }, // 'esht'
	{ 0x65787069, 0x3EC, 3, false }, // 'expi'
	{ 0x65787075, 0x580, 3, false }, // 'expu'
	{ 0x66697470, 0x578, 1, false }, // 'fitp'
	{ 0x6865616C, 0x324, 5, false }, // 'heal'
	{ 0x68696465, 0x428, 4, false }, // 'hide'
	{ 0x6869746D, 0x2C8, 1, false }, // 'hitm'
	{ 0x686E7470, 0x48C, 1, false }, // 'hntp'
	{ 0x68737432, 0x2BC, 2, false }, // 'hst2'
	{ 0x68737433, 0x2C0, 2, false }, // 'hst3'
	{ 0x68737465, 0x2B8, 2, false }, // 'hste'
	{ 0x68776972, 0x4D0, 2, false }, // 'hwir'
	{ 0x68776974, 0x588, 2, false }, // 'hwit'
	{ 0x696E7469, 0x3F4, 3, false }, // 'inti'
	{ 0x69726763, 0x2D0, 3, false }, // 'irgc'
	{ 0x6C667374, 0x400, 2, false }, // 'lfst'
	{ 0x6C6B6167, 0x47C, 3, false }, // 'lkag'
	{ 0x6C6B6370, 0x378, 2, false }, // 'lkcp'
	{ 0x6C6B6464, 0x480, 2, false }, // 'lkdd'
	{ 0x6C6B6468, 0x3E0, 4, false }, // 'lkdh'
	{ 0x6C6B6472, 0x478, 4, true  }, // 'lkdr'
	{ 0x6C6B7332, 0x374, 1, false }, // 'lks2'
	{ 0x6C6E6B73, 0x370, 5, false }, // 'lnks'
	{ 0x6C75636B, 0x4B8, 2, false }, // 'luck'
	{ 0x6D636170, 0x4A8, 3, false }, // 'mcap'
	{ 0x6D736363, 0x4C0, 2, false }, // 'mscc'
	{ 0x6D736368, 0x4A4, 3, false }, // 'msch'
	{ 0x6D736964, 0x4C4, 2, false }, // 'msid'
	{ 0x6D737463, 0x4BC, 2, false }, // 'mstc'
	{ 0x6D776474, 0x3D8, 2, false }, // 'mwdt'
	{ 0x6D776868, 0x328, 2, false }, // 'mwhh'
	{ 0x6D776873, 0x404, 2, false }, // 'mwhs'
	{ 0x6D776D68, 0x32C, 2, false }, // 'mwmh'
	{ 0x6D777474, 0x3C4, 2, false }, // 'mwtt'
	{ 0x6E627566, 0x35C, 1, false }, // 'nbuf'
	{ 0x6F646172, 0x270, 3, true  }, // 'odar'
	{ 0x6F6E6666, 0x284, 3, false }, // 'onff'
	{ 0x6F766C32, 0x37C, 2, false }, // 'ovl2'
	{ 0x70636475, 0x364, 3, false }, // 'pcdu'
	{ 0x70636872, 0x498, 2, false }, // 'pchr'
	{ 0x70646D32, 0x238, 2, false }, // 'pdm2'
	{ 0x70646D67, 0x234, 2, false }, // 'pdmg'
	{ 0x70687031, 0x348, 1, false }, // 'php1'
	{ 0x706D6467, 0x340, 5, false }, // 'pmdg'
	{ 0x706D6470, 0x344, 5, false }, // 'pmdp'
	{ 0x706D6870, 0x338, 5, false }, // 'pmhp'
	{ 0x706D6D70, 0x33C, 5, false }, // 'pmmp'
	{ 0x706D7031, 0x34C, 1, false }, // 'pmp1'
	{ 0x706F6C61, 0x304, 3, false }, // 'pola'
	{ 0x70756C73, 0x384, 2, false }, // 'puls'
	{ 0x70776474, 0x3D4, 2, false }, // 'pwdt'
	{ 0x70777474, 0x3C0, 2, false }, // 'pwtt'
	{ 0x71657374, 0x49C, 5, false }, // 'qest'
	{ 0x72637572, 0x414, 2, false }, // 'rcur'
	{ 0x7265616C, 0x300, 4, false }, // 'real'
	{ 0x72656174, 0x2FC, 3, false }, // 'reat'
	{ 0x72657161, 0x3B8, 2, false }, // 'reqa'
	{ 0x72657163, 0x39C, 2, false }, // 'reqc'
	{ 0x7265716E, 0x3B4, 1, false }, // 'reqn'
	{ 0x72657375, 0x330, 3, false }, // 'resu'
	{ 0x72687275, 0x2A4, 3, false }, // 'rhru'
	{ 0x726D7574, 0x4AC, 2, false }, // 'rmut'
	{ 0x72706B74, 0x2CC, 1, false }, // 'rpkt'
	{ 0x73617073, 0x258, 3, false }, // 'saps'
	{ 0x73636C73, 0x380, 2, false }, // 'scls'
	{ 0x736C6363, 0x494, 1, false }, // 'slcc'
	{ 0x73706461, 0x264, 3, false }, // 'spda'
	{ 0x73746E73, 0x448, 4, false }, // 'stns'
	{ 0x73747269, 0x3F0, 3, false }, // 'stri'
	{ 0x73756D6D, 0x308, 6, false }, // 'summ'
	{ 0x74616E74, 0x3C8, 3, false }, // 'tant'
	{ 0x74636D64, 0x488, 3, false }, // 'tcmd'
	{ 0x74637463, 0x2C4, 1, false }, // 'tctc'
	{ 0x74656C32, 0x2F0, 3, false }, // 'tel2'
	{ 0x74656C33, 0x2F4, 3, false }, // 'tel3'
	{ 0x74656C65, 0x2EC, 3, false }, // 'tele'
	{ 0x74657264, 0x2DC, 2, false }, // 'terd'
	{ 0x74686C64, 0x484, 2, false }, // 'thld'
	{ 0x74687264, 0x2E0, 2, false }, // 'thrd'
	{ 0x746B7373, 0x3FC, 2, false }, // 'tkss'
	{ 0x746E7432, 0x3CC, 3, false }, // 'tnt2'
	{ 0x74706164, 0x2E8, 2, false }, // 'tpad'
	{ 0x74706464, 0x2E4, 2, false }, // 'tpdd'
	{ 0x7472616E, 0x408, 3, false }, // 'tran'
	{ 0x74726170, 0x4A0, 1, false }, // 'trap'
};

static const tagSkillParamGetValueRule s_aGetValueRules[] = {
	{ 0x42444D44, 0x554 }, // BDMD
	{ 0x424C4154, 0x530 }, // BLAT
	{ 0x42534850, 0x534 }, // BSHP
	{ 0x43424154, 0x510 }, // CBAT
	{ 0x43425241, 0x50C }, // CBRA
	{ 0x434F4154, 0x4F0 }, // COAT
	{ 0x44474141, 0x524 }, // DGAA
	{ 0x44474154, 0x51C }, // DGAT
	{ 0x44474852, 0x520 }, // DGHR
	{ 0x44534352, 0x550 }, // DSCR
	{ 0x44534552, 0x54C }, // DSER
	{ 0x44544154, 0x528 }, // DTAT
	{ 0x44544452, 0x52C }, // DTDR
	{ 0x45315341, 0x4D4 }, // E1SA
	{ 0x45324141, 0x4DC }, // E2AA
	{ 0x45324148, 0x4E0 }, // E2AH
	{ 0x45325341, 0x4D8 }, // E2SA
	{ 0x45414154, 0x4EC }, // EAAT
	{ 0x46494154, 0x4F4 }, // FIAT
	{ 0x484C4154, 0x560 }, // HLAT
	{ 0x484C4250, 0x56C }, // HLBP
	{ 0x484C4653, 0x564 }, // HLFS
	{ 0x484C4D44, 0x55C }, // HLMD
	{ 0x484C4D49, 0x568 }, // HLMI
	{ 0x484C5255, 0x558 }, // HLRU
	{ 0x484C534D, 0x570 }, // HLSM
	{ 0x4C494154, 0x4F8 }, // LIAT
	{ 0x4D414154, 0x574 }, // MAAT
	{ 0x4D554154, 0x540 }, // MUAT
	{ 0x4D554352, 0x548 }, // MUCR
	{ 0x4D554552, 0x544 }, // MUER
	{ 0x52504255, 0x508 }, // RPBU
	{ 0x52504455, 0x500 }, // RPDU
	{ 0x52505455, 0x504 }, // RPTU
	{ 0x53414141, 0x538 }, // SAAA
	{ 0x53544455, 0x518 }, // STDU
	{ 0x53545350, 0x514 }, // STSP
	{ 0x54524141, 0x53C }, // TRAA
	{ 0x57494D44, 0x4E4 }, // WIMD
	{ 0x57495255, 0x4E8 }, // WIRU
};

static inline void SkillParam_NormalizeMask(uint32_t& dwMask) {
	switch (dwMask) {
	case 1:
	case 2:
	case 3:
		dwMask |= 0x0C;
		break;
	case 4:
	case 8:
	case 12:
		dwMask |= 0x03;
		break;
	default:
		break;
	}
}

/*
================
SkillGlobal_BuildParameterIndex

[RECONSTRUCTED - Native 0x00587630] (7509 bytes)
Resets the derived parameter pointer table (+0x230 to +0x5AC) of tagRefSkill,
then parses the 49 raw reference parameter words (+0x16C).
Maps FourCC tags to parameter pointer slots, normalizes target bitmasks,
and handles getv/efr/setv/reqi/ssou records.
================
*/
void SkillGlobal_BuildParameterIndex(tagRefSkill* pRefSkill) {
	if (!pRefSkill) {
		return;
	}

	uint8_t* pRaw = reinterpret_cast<uint8_t*>(pRefSkill);

	// 0x00587641: Clear 0x37C bytes (223 pointer slots) starting at +0x230
	for (const uint32_t*& pSlot : pRefSkill->m_apParam) {
		pSlot = nullptr;
	}

	size_t cursor = 0;

	// Native 0x00587662: Loop over up to 49 parameter words
	while (cursor < 49) {
		const uint32_t dwTag = pRefSkill->m_dwParamWords[cursor];
		if (dwTag == 0) {
			++cursor;
			continue;
		}

		// Native 0x005882DC: 'getv' (0x67657476) - Get-value parameter record
		if (dwTag == 0x67657476) {
			if (cursor + 1 < 49) {
				const uint32_t dwSelector = pRefSkill->m_dwParamWords[cursor + 1];
				bool bFound = false;
				for (size_t i = 0; i < sizeof(s_aGetValueRules) / sizeof(s_aGetValueRules[0]); ++i) {
					if (s_aGetValueRules[i].dwSelector == dwSelector) {
						pRefSkill->SetParam(s_aGetValueRules[i].wSlotOffset, &pRefSkill->m_dwParamWords[cursor + 1]);
						bFound = true;
						break;
					}
				}
				if (!bFound) {
					ServerFramework::Log_Printf(0x2000000, "unknown skill param getv selector!!! [RefSkill ID: %d, Selector: 0x%08X]",
						pRefSkill->dwSkillID, dwSelector);
					ServerFramework::ServerFramework_GenerateMiniDump();
				}
			}
			cursor += 2;
			continue;
		}

		// Native 0x005882B5: 'efr ' (0x00656672) - Effect record (rank 1, 2, or 3)
		if (dwTag == 0x00656672) {
			if (cursor + 1 < 49) {
				const uint32_t dwRank = pRefSkill->m_dwParamWords[cursor + 1];
				switch (dwRank) {
				case 1:
					pRefSkill->SetParam(0x28C, &pRefSkill->m_dwParamWords[cursor + 1]);
					break;
				case 2:
					pRefSkill->SetParam(0x290, &pRefSkill->m_dwParamWords[cursor + 1]);
					break;
				case 3:
					pRefSkill->SetParam(0x294, &pRefSkill->m_dwParamWords[cursor + 1]);
					break;
				default:
					break;
				}
			}
			cursor += 7;
			continue;
		}

		// Native 0x0058902A: 'setv' (0x73657476) - Repeated value assignment (up to 5 entries)
		if (dwTag == 0x73657476) {
			size_t slotIndex = 0;
			for (; slotIndex < 5; ++slotIndex) {
				const uint16_t wSlot = static_cast<uint16_t>(0x388 + slotIndex * 4);
				if (pRefSkill->Param(wSlot) == nullptr) {
					pRefSkill->SetParam(wSlot, &pRefSkill->m_dwParamWords[cursor + 1]);
					break;
				}
			}
			if (slotIndex == 5) {
				ServerFramework::Log_Printf(0x2000000, "Too many setv skill params!!! [RefSkill ID: %d]", pRefSkill->dwSkillID);
				ServerFramework::ServerFramework_GenerateMiniDump();
			}
			cursor += 4;
			continue;
		}

		// Native 0x00588F27: 'reqi' (0x72657169) - Repeated item requirement (up to 5 entries)
		if (dwTag == 0x72657169) {
			size_t slotIndex = 0;
			for (; slotIndex < 5; ++slotIndex) {
				const uint16_t wSlot = static_cast<uint16_t>(0x3A0 + slotIndex * 4);
				if (pRefSkill->Param(wSlot) == nullptr) {
					pRefSkill->SetParam(wSlot, &pRefSkill->m_dwParamWords[cursor + 1]);
					break;
				}
			}
			if (slotIndex == 5) {
				ServerFramework::Log_Printf(0x2000000, "Too many reqi skill params!!! [RefSkill ID: %d]", pRefSkill->dwSkillID);
				ServerFramework::ServerFramework_GenerateMiniDump();
			}
			cursor += 3;
			continue;
		}

		// Native 0x005890CB: 'ssou' (0x73736F75) - Summon tail descriptor; parsing terminates here
		if (dwTag == 0x73736F75) {
			pRefSkill->SetParam(0x354, &pRefSkill->m_dwParamWords[cursor + 1]);
			return;
		}

		// Check fixed rules table (164 rules)
		bool bFound = false;
		for (size_t i = 0; i < sizeof(s_aFixedParamRules) / sizeof(s_aFixedParamRules[0]); ++i) {
			if (s_aFixedParamRules[i].dwTag == dwTag) {
				pRefSkill->SetParam(s_aFixedParamRules[i].wSlotOffset, &pRefSkill->m_dwParamWords[cursor + 1]);
				if (s_aFixedParamRules[i].bNormalizeMask && (cursor + 1 < 49)) {
					SkillParam_NormalizeMask(pRefSkill->m_dwParamWords[cursor + 1]);
				}
				cursor += s_aFixedParamRules[i].byWordCount;
				bFound = true;
				break;
			}
		}

		if (bFound) {
			continue;
		}

		// Native 0x005892EC: Unknown parameter tag diagnostic
		ServerFramework::Log_Printf(0x2000000, "unknown skill param!!! [RefSkill ID: %d, ParamValue: %d]",
			pRefSkill->dwSkillID, dwTag);
		ServerFramework::ServerFramework_GenerateMiniDump();
		break;
	}
}

/*
================
tagRefSkill::BuildParameterIndex
================
*/
void tagRefSkill::BuildParameterIndex() {
	SkillGlobal_BuildParameterIndex(this);
}


/*
================
CastLifecycle_PulseDue

[RECONSTRUCTED - Native 0x0058489D / 0x00584A62]
Evaluates whether period ticks from 'Puls' parameter record have elapsed.
================
*/
bool CastLifecycle_PulseDue(
	uint32_t dwNowTick,
	uint32_t& dwLastTick,
	const tagRefSkill* pRefSkill
) {
	if (!pRefSkill) {
		return true;
	}

	// ParameterSlot::Puls is at offset +0x384 in tagRefSkill
	const uint8_t* pRawSkill = reinterpret_cast<const uint8_t*>(pRefSkill);
	const uint32_t* pParamPuls = pRefSkill->Param(0x384);
	if (!pParamPuls) {
		return true;
	}

	uint32_t dwPeriod = pParamPuls[0];
	if (dwNowTick - dwLastTick < dwPeriod) {
		return false;
	}

	dwLastTick = dwNowTick;
	return true;
}

/*
================
CastLifecycle_UpdateLinks

[RECONSTRUCTED - Native 0x0058482E - 0x00584A50]
Updates single-target persistent links across ticks. Validates distance,
line-of-sight, alive status, and executes pulse attacks.
================
*/
void CastLifecycle_UpdateLinks(
	CGObjChar* pCaster,
	void* pSkillActor,
	bool& bEnd
) {
	if (!pCaster || !pSkillActor) {
		return;
	}

	// Execution context pointer at pSkillActor + 0x18
	uint8_t* pRawActor = reinterpret_cast<uint8_t*>(pSkillActor);
	uint8_t* pExecution = *reinterpret_cast<uint8_t**>(pRawActor + 0x18);
	if (!pExecution) {
		return;
	}

	tagPersistentLink* pLink = *reinterpret_cast<tagPersistentLink**>(pExecution + 0x68);
	if (!pLink) {
		return;
	}

	uint8_t byMode = pExecution[0x20];
	if (byMode != 1) {
		if (g_pGame && !g_pGame->FindObjectByID(pLink->m_dwSourceActorID)) {
			pLink->m_dwSourceActorID = 0;
			bEnd = true;
		}
		return;
	}

	CGObjChar* pTarget = g_pGame ? g_pGame->FindObjectByID(pLink->m_dwTargetActorID) : nullptr;
	if (!pTarget) {
		pLink->m_dwTargetActorID = 0;
		bEnd = true;
		return;
	}

	// Reference skill pointer at pExecution + 0x08
	const tagRefSkill* pRefSkill = *reinterpret_cast<const tagRefSkill**>(pExecution + 0x08);
	if (!pRefSkill) {
		return;
	}

	const uint8_t* pRawSkill = reinterpret_cast<const uint8_t*>(pRefSkill);

	// Query target alive status via slot 6 (+0x18)
	if (pTarget->GetLifeState() == 1 && pRefSkill->MatchesExecutionSelector()) {
		// ParameterSlot::Hntp check at offset +0x48C
		const uint32_t* pHntp = pRefSkill->Param(0x48C);
		if (!pHntp) {
			bool bDue = CastLifecycle_PulseDue(::GetTickCount(), pLink->m_dwLastTick, pRefSkill);
			const uint32_t* pHitm = pRefSkill->Param(0x2C8);
			if (!pHitm && bDue) {
				// Native 0x00584936: Reset primary command target ID to 0 before dispatch
				uint8_t* pPrimaryCmd = *reinterpret_cast<uint8_t**>(pRawActor + 0x14);
				if (pPrimaryCmd) {
					*reinterpret_cast<uint32_t*>(pPrimaryCmd + 0x10) = 0;
				}

				// Reset auxiliary costs
				uint8_t* pAuxExec = *reinterpret_cast<uint8_t**>(pRawActor + 0x20);
				if (pAuxExec) {
					*reinterpret_cast<uint32_t*>(pAuxExec + 0x10) = 0; // hpCost
					*reinterpret_cast<uint32_t*>(pAuxExec + 0x14) = 0; // mpCost
					pAuxExec[0x18] = 0;                                // spCost
				}
			}
			return;
		}
	}

	// ParameterSlot::Lnks check at offset +0x370
	const uint32_t* pLnks = pRefSkill->Param(0x370);
	if (pLnks && pLnks[1] != 0) {
		if (pTarget->m_dwWorldID != pCaster->m_dwWorldID) {
			bEnd = true;
			return;
		}

		SRO_Vector3D casterPos(pCaster->m_fLocalPosX, pCaster->m_fLocalPosY, pCaster->m_fLocalPosZ);
		SRO_Vector3D targetPos(pTarget->m_fLocalPosX, pTarget->m_fLocalPosY, pTarget->m_fLocalPosZ);
		SRO_Vector3D diff;

		Pos_Relative3D(&diff, pCaster->m_wRegionID, &casterPos, pTarget->m_wRegionID, &targetPos);
		if (diff.x == SRO_INCOMPATIBLE_POS) {
			bEnd = true;
			return;
		}

		float fDist = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
		if (fDist > static_cast<float>(pLnks[1])) {
			bEnd = true;
		}
	}
}

/*
================
CastLifecycle_UpdateAreaLink

[RECONSTRUCTED - Native 0x00584A50 - 0x00585172]
Updates multi-target party aura links. Re-evaluates distance, cleans departed members,
discovers nearby party members within radius, and applies periodic healing or buffs.
================
*/
void CastLifecycle_UpdateAreaLink(
	CGObjChar* pCaster,
	void* pSkillActor,
	bool& bEnd
) {
	if (!pCaster || !pSkillActor) {
		return;
	}

	uint8_t* pRawActor = reinterpret_cast<uint8_t*>(pSkillActor);
	uint8_t* pExecution = *reinterpret_cast<uint8_t**>(pRawActor + 0x18);
	if (!pExecution) {
		return;
	}

	tagPersistentAreaLink* pAreaLink = *reinterpret_cast<tagPersistentAreaLink**>(pExecution + 0x6C);
	if (!pAreaLink) {
		return;
	}

	uint8_t byMode = pExecution[0x20];
	if (byMode != 1) {
		if (g_pGame && !g_pGame->FindObjectByID(pAreaLink->m_dwSourceActorID)) {
			bEnd = true;
		}
		return;
	}

	const tagRefSkill* pRefSkill = *reinterpret_cast<const tagRefSkill**>(pExecution + 0x08);
	if (!pRefSkill) {
		return;
	}

	bool bDue = CastLifecycle_PulseDue(::GetTickCount(), pAreaLink->m_dwLastTick, pRefSkill);

	// Case 1: Area selector attack
	if (pRefSkill->MatchesExecutionSelector()) {
		if (!bDue) {
			return;
		}

		uint8_t* pAuxCmd = *reinterpret_cast<uint8_t**>(pRawActor + 0x1C);
		if (pAuxCmd) {
			*reinterpret_cast<uint32_t*>(pAuxCmd + 0x10) = 0; // targetId = 0
		}
		return;
	}

	// Case 2: Non-selector party aura / periodic recovery
	const uint8_t* pRawSkill = reinterpret_cast<const uint8_t*>(pRefSkill);
	const uint32_t* pEshp = pRefSkill->Param(0x398);

	float fLowestHPPercent = 0.0f;
	CGObjChar* pRecoveryTarget = nullptr;

	SRO_Vector3D casterPos(pCaster->m_fLocalPosX, pCaster->m_fLocalPosY, pCaster->m_fLocalPosZ);

	for (auto it = pAreaLink->m_setMembers.begin(); it != pAreaLink->m_setMembers.end();) {
		CGObjChar* pMember = g_pGame ? g_pGame->FindObjectByID(*it) : nullptr;
		if (!pMember) {
			it = pAreaLink->m_setMembers.erase(it);
			continue;
		}

		bool bKeep = false;
		if (pCaster->m_dwWorldID == pMember->m_dwWorldID) {
			if (Pos_RegionsCompatible(pCaster->m_wRegionID, pMember->m_wRegionID)) {
				SRO_Vector3D memberPos(pMember->m_fLocalPosX, pMember->m_fLocalPosY, pMember->m_fLocalPosZ);
				SRO_Vector3D diff;
				Pos_Relative3D(&diff, pCaster->m_wRegionID, &casterPos, pMember->m_wRegionID, &memberPos);

				if (diff.x != SRO_INCOMPATIBLE_POS) {
					float fDist = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
					if (fDist <= pAreaLink->m_fRadius) {
						bKeep = true;
					}
				}
			} else {
				bKeep = true;
			}
		}

		if (pMember == pCaster) {
			bKeep = true;
		}

		if (bKeep && pEshp) {
			uint32_t dwCurHP = pMember->GetCurrentHP();
			uint32_t dwMaxHP = pMember->GetMaxHP();
			if (dwMaxHP > 0) {
				float fPercent = (static_cast<float>(dwCurHP) / static_cast<float>(dwMaxHP)) * 100.0f;
				if (fLowestHPPercent == 0.0f || fPercent < fLowestHPPercent) {
					fLowestHPPercent = fPercent;
					pRecoveryTarget = pMember;
				}
			}
		}

		++it;
	}

	// Apply periodic recovery if due (Native 0x00584E5B calling CSkillManager_ApplyHealRecovery)
	if (bDue && pRecoveryTarget && pCaster->GetSkillManager()) {
		// Native 0x005A09F0: CSkillManager_ApplyHealRecovery
		pCaster->GetSkillManager()->ApplyHealRecovery(pRecoveryTarget, pRefSkill);
	}
}

/*
================
SkillEffect_RetireContributionsAndLinks

[PARTIAL - Native 0x005829D0] (1749 bytes)
Cleans up and retires all active buffs and links associated with an area attack.
Audit 2026-09-20: this legacy overload omits the native selector, parameter,
owner-slot and notification branches and does not implement the native return
value 3. It is not an independent exact implementation of this address.
================
*/
void SkillEffect_RetireContributionsAndLinks(
	CGObjChar* pCaster,
	void* pSkillActor
) {
	if (!pCaster || !pSkillActor) {
		return;
	}

	uint8_t* pRawActor = reinterpret_cast<uint8_t*>(pSkillActor);
	uint8_t* pExecution = *reinterpret_cast<uint8_t**>(pRawActor + 0x18);
	if (!pExecution) {
		return;
	}

	tagPersistentAreaLink* pAreaLink = *reinterpret_cast<tagPersistentAreaLink**>(pExecution + 0x6C);
	if (pAreaLink) {
		for (uint32_t dwMemberID : pAreaLink->m_setMembers) {
			CGObjChar* pMember = g_pGame ? g_pGame->FindObjectByID(dwMemberID) : nullptr;
			if (pMember && pMember != pCaster) {
				// Clear link pointer and retire buff
				CSkillManager* pSkillMgr = pMember->GetSkillManager();
				if (pSkillMgr) {
					// Retire persistent active buff
				}
			}
		}
		pAreaLink->m_setMembers.clear();
		*reinterpret_cast<tagPersistentAreaLink**>(pExecution + 0x6C) = nullptr;
	}

	tagPersistentLink* pLink = *reinterpret_cast<tagPersistentLink**>(pExecution + 0x68);
	if (pLink) {
		*reinterpret_cast<tagPersistentLink**>(pExecution + 0x68) = nullptr;
	}
}

/*
================
SkillPacket_WriteHitResult

[RECONSTRUCTED - Native 0x005855F0] (203 bytes)
Writes a single skill hit result to a CMsg packet buffer:
  - Header byte (+0x05)
  - Damage/amount and flags combined: (dwAmount << 8) | byFlags (4 bytes)
  - If kind == 7: writes extra word 1A and extra word 1C
  - If kind == 0, 4, 5: writes extra dword (+0x08)
  - If kind == 4, 5: writes destination region (+0x0C) and 3D float coordinates (+0x0E)
================
*/
void SkillPacket_WriteHitResult(const tagSkillHitResult* pHit, BSLib::CPacket* pPacket) {
	if (!pHit || !pPacket) {
		return;
	}

	pPacket->WriteUint8(pHit->m_byHeader);
	uint8_t byKind = pHit->m_byHeader & 0x7F;

	if (byKind == 7) {
		uint32_t dwValue = (pHit->m_dwAmount << 8) | static_cast<uint32_t>(pHit->m_byFlags);
		pPacket->WriteUint32(dwValue);
		pPacket->WriteUint16(pHit->m_wExtra1A);
		pPacket->WriteUint16(pHit->m_wExtra1C);
		return;
	}

	if (byKind == 0 || byKind == 4 || byKind == 5) {
		uint32_t dwValue = (pHit->m_dwAmount << 8) | static_cast<uint32_t>(pHit->m_byFlags);
		pPacket->WriteUint32(dwValue);
		pPacket->WriteUint32(pHit->m_dwExtra08);
	}

	if (byKind == 4 || byKind == 5) {
		pPacket->WriteUint16(pHit->m_wRegion);
		// 0x005856B8: the twelve bytes go out as they are, and the hit outcome put integers there
		pPacket->WriteUint32(static_cast<uint32_t>(pHit->m_nPosX));
		pPacket->WriteUint32(static_cast<uint32_t>(pHit->m_nPosY));
		pPacket->WriteUint32(static_cast<uint32_t>(pHit->m_nPosZ));
	}
}

/*
================
SkillPacket_WriteTargetHits

[RECONSTRUCTED - Native 0x005856D0] (96 bytes)
Writes target GID (+0x08) and iterates the hit list at +0x5C.
================
*/
void SkillPacket_WriteTargetHits(const tagSkillTargetHitGroup* pTargetHit, BSLib::CPacket* pPacket) {
	if (!pTargetHit || !pPacket) {
		return;
	}

	pPacket->WriteUint32(pTargetHit->m_dwTargetID);
	for (const auto& hit : pTargetHit->m_listHits) {
		SkillPacket_WriteHitResult(&hit, pPacket);
	}
}

/*
================
SkillPacket_WriteResultBatch

[RECONSTRUCTED - Native 0x00585730] (124 bytes)
Writes batch kind byte (+0x05), target count (+0x10), and iterates list of targets.
================
*/
void SkillPacket_WriteResultBatch(const tagSkillResultBatch* pBatch, BSLib::CPacket* pPacket) {
	if (!pBatch || !pPacket) {
		return;
	}

	pPacket->WriteUint8(pBatch->m_byKind);
	pPacket->WriteUint8(static_cast<uint8_t>(pBatch->m_listTargets.size()));
	for (const auto& target : pBatch->m_listTargets) {
		SkillPacket_WriteTargetHits(&target, pPacket);
	}
}

/*
================
SkillPacket_WritePositionResult

[RECONSTRUCTED - Native 0x005862B0] (39 bytes)
Writes position result: region (2 bytes at +0x06) and 3 float coordinates (12 bytes at +0x08).
================
*/
void SkillPacket_WritePositionResult(const tagSkillPositionResult* pPos, BSLib::CPacket* pPacket) {
	if (!pPos || !pPacket) {
		return;
	}

	pPacket->WriteUint16(pPos->m_wRegionID);
	// Coordinates in native binary are stored as 3 consecutive floats (x, y, z)
	const float* pCoords = reinterpret_cast<const float*>(&pPos->m_nX);
	pPacket->WriteFloat(pCoords[0]);
	pPacket->WriteFloat(pCoords[1]);
	pPacket->WriteFloat(pCoords[2]);
}

/*
================
Skill_UpdateRampContributions

[RECONSTRUCTED - Native 0x00584668 - 0x005847A0]
Evaluates 'Mom' parameter record (+0x57C), increments ramp counter,
computes float value = (rampCount * ramp[3] * ramp[1]), and writes
10 pairs of parameters to CGParamKeeper: (0x80,0), (0x81,0), (0x82,0),
(0x83,0), (5,1), (6,1), (9,0), (9,1), (0x0B,1), (0x0B,0).
================
*/
void Skill_UpdateRampContributions(
	CGObjChar* pActor,
	tagActiveSkillInstance* pInstance
) {
	if (!pActor || !pInstance || !pInstance->m_pExecution) {
		return;
	}

	tagSkillExecutionContext* pExec = pInstance->m_pExecution;
	const tagRefSkill* pRef = pExec->m_pRefSkill;
	if (!pRef) {
		return;
	}

	const uint32_t* pParamMom = pRef->Param(0x57C);
	if (!pParamMom) {
		return;
	}

	uint32_t dwNow = GetTickCount();
	if (pExec->m_dwRampCount != 0 && (dwNow - pExec->m_dwRampStartedAt) < pParamMom[0]) {
		return;
	}

	pExec->m_dwRampStartedAt = dwNow;
	++pExec->m_dwRampCount;

	// Native 0x005846B8: imul eax, [ecx+0xC]; imul eax, [ecx+0x4]; fild; fstp
	int32_t nProduct = static_cast<int32_t>(pExec->m_dwRampCount) * static_cast<int32_t>(pParamMom[3]) * static_cast<int32_t>(pParamMom[1]);
	float fValue = static_cast<float>(nProduct);

	CGParamKeeper& keeper = pActor->m_paramKeeper;
	keeper.SetParamFloat(0x80, 0, 0, fValue);
	keeper.SetParamFloat(0x81, 0, 0, fValue);
	keeper.SetParamFloat(0x82, 0, 0, fValue);
	keeper.SetParamFloat(0x83, 0, 0, fValue);
	keeper.SetParamFloat(5,    1, 0, fValue);
	keeper.SetParamFloat(6,    1, 0, fValue);
	keeper.SetParamFloat(9,    0, 0, fValue);
	keeper.SetParamFloat(9,    1, 0, fValue);
	keeper.SetParamFloat(0x0B, 1, 0, fValue);
	keeper.SetParamFloat(0x0B, 0, 0, fValue);
}

/*
================
Skill_ProcessPeriodicDamage

[RECONSTRUCTED - Native 0x00582750] (632 bytes)
Validates periodic pulse target and tick interval against 'Summ' parameter record (+0x308),
queries attacker's mastery rank for the active weapon skill via CSkillManager_GetSkillMasteryRank (0x0059E770),
calculates combined magical & physical skill damage, applies special target multipliers (0x00615A20),
executes CGObjChar::ApplyHit (slot 319 @ +0x4FC), and broadcasts 0x30D1 packet to nearby sessions.
================
*/
int32_t Skill_ProcessPeriodicDamage(
	CGObjChar* pAttacker,
	tagActiveSkillInstance* pInstance
) {
	if (!pAttacker || !pInstance || !pInstance->m_pExecution) {
		return 0;
	}

	tagSkillExecutionContext* pExec = pInstance->m_pExecution;
	tagPeriodicDamagePulse* pPulse = pExec->m_pPeriodicDamage;
	if (!pPulse) {
		return 0;
	}

	if (pPulse->m_dwTargetID == 0) {
		return 0;
	}

	CGObjChar* pTarget = g_pGame ? g_pGame->FindObjectByID(pPulse->m_dwTargetID) : nullptr;
	if (!pTarget) {
		pPulse->m_dwTargetID = 0;
		return 0;
	}

	// Native 0x00582794: slot 6 (+0x18) - target must be alive
	if (pTarget->GetLifeState() != 1) {
		pPulse->m_dwTargetID = 0;
		return 0;
	}

	// 5827A6: virtual +3E0 is IsFortressStructure (AEC0A4 -> 482AB0).
	// It is not a second HP or invulnerability test.
	if (pTarget->IsFortressStructure()) {
		pPulse->m_dwTargetID = 0;
		return 0;
	}

	// Native 0x005827BB: slot 62 (+0xF8) - target life state must be alive (1)
	if (pTarget->GetLifeState() != 1) {
		pPulse->m_dwTargetID = 0;
		return 0;
	}

	const tagRefSkill* pRefSkill = pExec->m_pRefSkill;
	if (!pRefSkill) {
		return 0;
	}

	// Native 0x005827E2: 'Summ' parameter at +0x308
	const uint8_t* pRawSkill = reinterpret_cast<const uint8_t*>(pRefSkill);
	const uint32_t* pParamSumm = pRefSkill->Param(0x308);
	if (!pParamSumm) {
		return 0;
	}

	uint32_t dwNow = GetTickCount();
	// Interval check against pParamSumm[2] (eax - esi[0x0C] < edx[8])
	if ((dwNow - pPulse->m_dwLastTick) < pParamSumm[2]) {
		return 0;
	}

	// Total duration check against pParamSumm[2] (eax - esi[0x08] > edx[8])
	if ((dwNow - pPulse->m_dwStartedAt) > pParamSumm[2]) {
		return 0;
	}

	pPulse->m_dwLastTick = dwNow;

	// Native 0x00582816: CSkillManager_GetSkillMasteryRank via slot 347 (+0x56C)
	uint32_t dwAttackSkillID = pAttacker->GetMainWeaponAttackSkillID();
	CSkillManager* pSkillMgr = pAttacker->GetSkillManager();
	const tagSkillData* pSkillData = pSkillMgr ? pSkillMgr->FindSkillByID(dwAttackSkillID) : nullptr;
	const tagRefSkill* pLearnedRef = pSkillData ? pSkillData->m_pRefSkill : nullptr;

	uint8_t byHitRatioBonus = 0;
	if (pLearnedRef && pSkillMgr) {
		const uint8_t* pRawLearned = reinterpret_cast<const uint8_t*>(pLearnedRef);
		const uint32_t* pGetv = pLearnedRef->Param(0x574);
		if (pGetv) {
			uint8_t byRank1 = 1;
			uint8_t byRank2 = 1;
			if (pLearnedRef->dwReqMasteryID[0] != 0) {
				const tagSkillMasteryData* pMastery = pSkillMgr->FindMastery(pLearnedRef->dwReqMasteryID[0]);
				if (pMastery && pMastery->m_pRefRecord) {
					byRank1 = pMastery->m_pRefRecord->GetLevel();
				}
			}
			if (pLearnedRef->dwReqMasteryID[1] != 0) {
				const tagSkillMasteryData* pMastery = pSkillMgr->FindMastery(pLearnedRef->dwReqMasteryID[1]);
				if (pMastery && pMastery->m_pRefRecord) {
					byRank2 = pMastery->m_pRefRecord->GetLevel();
				}
			}
			byHitRatioBonus = (byRank1 > byRank2) ? byRank1 : byRank2;
		}
	}

	// Native 0x00582865: Formulae::CalculateMagicalSkillDamage_Direct (0x0040F1B0)
	int32_t nMagicalDmg = Formulae::CalculateMagicalSkillDamage_Direct(pAttacker, nullptr, pTarget, byHitRatioBonus);
	// Native 0x0058289B: Formulae::CalculatePhysicalSkillDamage_Direct (0x0040F3D0)
	int32_t nPhysicalDmg = Formulae::CalculatePhysicalSkillDamage_Direct(pAttacker, nullptr, pTarget, byHitRatioBonus);

	int32_t nTotalDmg = nMagicalDmg + nPhysicalDmg;
	if (nTotalDmg <= 0) {
		nTotalDmg = 1;
	}

	// Native 0x0058291F: Update target HP
	uint32_t dwCurHP = pTarget->GetCurrentHP();
	uint32_t dwDmg = static_cast<uint32_t>(nTotalDmg);
	pTarget->SetCurrentHP((dwCurHP > dwDmg) ? (dwCurHP - dwDmg) : 0);

	uint16_t wDamageWord = static_cast<uint16_t>(nTotalDmg);
	if (pTarget->GetCurrentHP() == 0) {
		wDamageWord |= 0x8000; // Native death marker bit
	}

	// Native 0x00582963: Allocate and broadcast packet 0x30D1
	BSLib::CPacket* pPkt = BSLib::CPacket::Allocate(1);
	if (pPkt) {
		pPkt->SetOpcode(0x30D1);
		pPkt->WriteUint32(pExec->m_dwContextID);
		pPkt->WriteUint32(pPulse->m_dwTargetID);
		pPkt->WriteUint16(wDamageWord);
		pAttacker->SendPacketToNearbySessions(pPkt);
		pPkt->Release();
	}

	return 1;
}




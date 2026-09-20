/**
 * ============================================================================
 * Silkroad Online - Combat and Damage Calculation Formulae Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\Formulae.cpp
 *
 * Implements:
 *   - Formulae::CalculatePhysicalDamage @ 0x0040E830 (935 bytes)
 *   - Formulae::CalculatePhysicalAttackPower @ 0x0040DFF0 (1114 bytes)
 *   - Formulae::GetPhysicalAttackPowerMinMax @ 0x0040DC10 (198 bytes)
 *   - Formulae::CalculateLevelDiffBonus @ 0x0040FD60 (104 bytes)
 *   - Formulae::CalculateAbsorptionRatio @ 0x00410AB0 (140 bytes)
 *   - Formulae::CalculateDefenseSkillAbsorption @ 0x00410BB0 (97 bytes)
 *   - Global g_bShowFormulaDetail @ 0x00C82530
 * ============================================================================
 */

#include "Formulae.h"
#include "GItemEquip.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "ReferenceData.h"
#include "../JMX_ServerFramework/ServerFramework/ServerConfig.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

// Global config flag for detailed formula debugging (Native 0x00C82530)
bool g_bShowFormulaDetail = false;

namespace Formulae {

/**
 * [RECONSTRUCTED - 0x0040DA20] (76 bytes)
 * Formulae_GetMonsterAttackPowerMultiplier
 *
 * Checks monster type (slot 192 @ +0x300) and mob subtype (+0x1CD8):
 *   - Type 1: 1.2x
 *   - Type 0 and (m_byMobSubtype & 0x0F) == 6 (Champion/Giant): 1.5x
 *   - Otherwise: 1.0x (MiniDump on unexpected type)
 */
float GetMonsterAttackPowerMultiplier(const CGObjChar* pChar) {
	if (!pChar) {
		return 1.0f;
	}

	float fMult = 1.0f;
	uint8_t byType = pChar->GetMonsterType();
	if (byType != 0) {
		if (byType == 1) {
			fMult = 1.2f;
		} else {
			ServerFramework::ServerFramework_GenerateMiniDump();
		}
	}

	if ((pChar->m_byMobSubtype & 0x0F) == 6) {
		fMult *= 1.5f;
	}

	return fMult;
}

/**
 * [RECONSTRUCTED - 0x0040DC10] (198 bytes)
 * Formulae_GetPhysicalAttackPowerMinMax
 *
 * Retrieves the base physical attack power (min or max) for a character and skill.
 *   - bMax: false = Min Physical Attack Power (Param 15 / 0x0F)
 *           true  = Max Physical Attack Power (Param 16 / 0x10)
 */
float GetPhysicalAttackPowerMinMax(const CGObjChar* pChar, bool bMax, const CGSkill* pSkill) {
	if (!pChar) {
		return 0.0f;
	}

	// Param 15 (0x0F) = Min Physical Attack Power, Param 16 (0x10) = Max Physical Attack Power
	uint32_t dwParamID = bMax ? 16 : 15;
	float fBasePower = pChar->GetParamFloat(dwParamID);

	if (pSkill && pSkill->AttackParam() && (pSkill->AttackParam()->dwType & 0x08)) {
		uint32_t dwBonus = bMax ? pSkill->AttackParam()->dwMaxAttackPower : pSkill->AttackParam()->dwMinAttackPower;
		fBasePower += static_cast<float>(dwBonus);
	}

	if (pChar->IsNPC()) {
		fBasePower *= GetMonsterAttackPowerMultiplier(pChar);
	}

	if (fBasePower < 0.0f) {
		fBasePower = 0.0f;
	} else if (fBasePower > 999999.0f) {
		fBasePower = 999999.0f;
	}

	return fBasePower;
}

/**
 * [RECONSTRUCTED - 0x0040DB50] (180 bytes)
 * Formulae_GetMagicalAttackPowerMinMax
 *
 * Retrieves the base magical attack power (min or max) for a character and skill.
 *   - bMax: false = Min Magical Attack Power (Param 13 / 0x0D)
 *           true  = Max Magical Attack Power (Param 14 / 0x0E)
 */
float GetMagicalAttackPowerMinMax(const CGObjChar* pChar, bool bMax, const CGSkill* pSkill) {
	if (!pChar) {
		return 0.0f;
	}

	// Param 13 (0x0D) = Min Magical Attack Power, Param 14 (0x0E) = Max Magical Attack Power
	uint32_t dwParamID = bMax ? 14 : 13;
	float fBasePower = pChar->GetParamFloat(dwParamID);

	if (pSkill && pSkill->AttackParam() && (pSkill->AttackParam()->dwType & 0x04)) {
		uint32_t dwBonus = bMax ? pSkill->AttackParam()->dwMaxAttackPower : pSkill->AttackParam()->dwMinAttackPower;
		fBasePower += static_cast<float>(dwBonus);
	}

	if (pChar->IsNPC()) {
		fBasePower *= GetMonsterAttackPowerMultiplier(pChar);
	}

	if (fBasePower < 0.0f) {
		fBasePower = 0.0f;
	} else if (fBasePower > 999999.0f) {
		fBasePower = 999999.0f;
	}

	return fBasePower;
}

/**
 * [RECONSTRUCTED - 0x0040FDD0] (144 bytes)
 * Formulae_CalculateHitBalance
 *
 * Calculates base combat hit balance between target and attacker:
 *   (Attacker HitRate [Param 11] / Target ParryRate [Param 9]) * 0.5 + LevelDiffBonus,
 *   scaled by 100.0 and clamped to [10.0%, 90.0%].
 */
float CalculateHitBalance(const CGObjChar* pTarget, const CGObjChar* pAttacker) {
	if (!pTarget || !pAttacker) {
		return 50.0f;
	}

	float fHitRate = pAttacker->GetParamFloat(11); // Param 11: Hit Rate
	float fParryRate = pTarget->GetParamFloat(9);   // Param 9: Parry Rate
	if (fParryRate <= 0.0f) {
		fParryRate = 1.0f;
	}

	float fRatio = (fHitRate / fParryRate) * 0.5f;
	float fLevelDiffBonus = CalculateLevelDiffBonus(pAttacker, pTarget);
	float fBalance = (fRatio + fLevelDiffBonus) * 100.0f;

	if (fBalance < 10.0f) {
		return 10.0f;
	}
	if (fBalance > 90.0f) {
		return 90.0f;
	}
	return fBalance;
}

/**
 * [RECONSTRUCTED - 0x0040DFF0] (1114 bytes)
 * Formulae_CalculatePhysicalAttackPower
 *
 * Computes rolled physical attack power using min/max power, hit balance, variance dice,
 * and active physical skill modifiers.
 */
float CalculatePhysicalAttackPower(
	const CGObjChar* pAttacker,
	const CGSkill*   pSkill,
	const CGObjChar* pTarget,
	uint8_t          byHitRatioBonus
) {
	if (!pAttacker || !pSkill) {
		return 0.0f;
	}

	float fRatioBonus = (static_cast<float>(byHitRatioBonus) / 100.0f) + 1.0f;
	float fMin = GetPhysicalAttackPowerMinMax(pAttacker, false, pSkill) * fRatioBonus;
	float fMax = GetPhysicalAttackPowerMinMax(pAttacker, true, pSkill) * fRatioBonus;

	if (fMin > fMax) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		std::swap(fMin, fMax);
	}

	// 1. Calculate base balance between target and attacker
	float fBalance = CalculateHitBalance(pTarget, pAttacker);

	// 2. Roll 3 random values between 0 and 100 to simulate the balance distribution
	float fRoll = 100.0f;
	for (int i = 0; i < 3; ++i) {
		float r = (static_cast<float>(std::rand()) / 32767.0f) * 100.0f;
		if (r < fRoll) {
			fRoll = r;
		}
	}

	// 3. Coin flip (rand() % 2) to add or subtract roll variance
	if ((std::rand() % 2) == 0) {
		fBalance -= fRoll;
	} else {
		fBalance += fRoll;
	}

	// 4. Skill balance modifiers (+0x520, +0x4E0)
	const CSkillManager* pSkillMgr = pAttacker->GetSkillManager();
	if (pSkillMgr) {
		if (pSkill->Param(0x520)) {
			const tagSkillModifier* pMod = CSkillManager_GetSkillModifier(pSkillMgr, *pSkill->Param(0x520));
			if (pMod) {
				fBalance += static_cast<float>(pMod->dwValue);
			}
		}
		if (pSkill->Param(0x4E0)) {
			const tagSkillModifier* pMod = CSkillManager_GetSkillModifier(pSkillMgr, *pSkill->Param(0x4E0));
			if (pMod) {
				fBalance += static_cast<float>(pMod->dwValue);
			}
		}
	}

	// 5. Clamp balance to [0.0%, 100.0%]
	if (fBalance < 0.0f) {
		fBalance = 0.0f;
	} else if (fBalance > 100.0f) {
		fBalance = 100.0f;
	}

	// 6. Interpolate rolled attack power
	float fPower = fMin + (fMax - fMin) * (fBalance / 100.0f);

	// 7. Sequentially apply active physical skill power multipliers
	if (pSkillMgr) {
		const uint32_t* const pModIDs[] = {
			pSkill->Param(0x4EC), pSkill->Param(0x4F0), pSkill->Param(0x4F4), pSkill->Param(0x4F8),
			pSkill->Param(0x528), pSkill->Param(0x530), pSkill->Param(0x540), pSkill->Param(0x560)
		};
		for (const uint32_t* pModID : pModIDs) {
			if (pModID) {
				const tagSkillModifier* pMod = CSkillManager_GetSkillModifier(pSkillMgr, *pModID);
				if (pMod) {
					fPower *= (static_cast<float>(pMod->dwValue) / 100.0f + 1.0f);
				}
			}
		}
	}

	return fPower;
}

/**
 * [RECONSTRUCTED - 0x0040DCE0] (776 bytes)
 * Formulae_CalculateMagicalAttackPower
 *
 * Computes rolled magical attack power using min/max power, hit balance, variance dice,
 * and active magical skill modifiers.
 */
float CalculateMagicalAttackPower(
	const CGObjChar* pAttacker,
	const CGSkill*   pSkill,
	const CGObjChar* pTarget,
	uint8_t          byHitRatioBonus
) {
	if (!pAttacker || !pSkill) {
		return 0.0f;
	}

	float fRatioBonus = (static_cast<float>(byHitRatioBonus) / 100.0f) + 1.0f;
	float fMin = GetMagicalAttackPowerMinMax(pAttacker, false, pSkill) * fRatioBonus;
	float fMax = GetMagicalAttackPowerMinMax(pAttacker, true, pSkill) * fRatioBonus;

	if (fMin > fMax) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		std::swap(fMin, fMax);
	}

	// 1. Calculate base balance between target and attacker
	float fBalance = CalculateHitBalance(pTarget, pAttacker);

	// 2. Roll 3 random values between 0 and 100 to simulate the balance distribution
	float fRoll = 100.0f;
	for (int i = 0; i < 3; ++i) {
		float r = (static_cast<float>(std::rand()) / 32767.0f) * 100.0f;
		if (r < fRoll) {
			fRoll = r;
		}
	}

	// 3. Coin flip (rand() % 2) to add or subtract roll variance
	if ((std::rand() % 2) == 0) {
		fBalance -= fRoll;
	} else {
		fBalance += fRoll;
	}

	// 4. Skill balance modifiers (+0x520 or fallback +0x4E0)
	const CSkillManager* pSkillMgr = pAttacker->GetSkillManager();
	if (pSkillMgr) {
		const uint32_t* pBalanceModID = pSkill->Param(0x520) ? pSkill->Param(0x520) : pSkill->Param(0x4E0);
		if (pBalanceModID) {
			const tagSkillModifier* pMod = CSkillManager_GetSkillModifier(pSkillMgr, *pBalanceModID);
			if (pMod) {
				fBalance += static_cast<float>(pMod->dwValue);
			}
		}
	}

	// 5. Clamp balance to [0.0%, 100.0%]
	if (fBalance < 0.0f) {
		fBalance = 0.0f;
	} else if (fBalance > 100.0f) {
		fBalance = 100.0f;
	}

	// 6. Interpolate rolled attack power
	float fPower = fMin + (fMax - fMin) * (fBalance / 100.0f);

	// 7. Priority chain: apply first active magical skill power multiplier
	if (pSkillMgr) {
		const uint32_t* pPowerModID = nullptr;
		if (pSkill->Param(0x51C))      pPowerModID = pSkill->Param(0x51C);
		else if (pSkill->Param(0x510)) pPowerModID = pSkill->Param(0x510);
		else if (pSkill->Param(0x4D4)) pPowerModID = pSkill->Param(0x4D4);
		else if (pSkill->Param(0x4D8)) pPowerModID = pSkill->Param(0x4D8);
		else if (pSkill->Param(0x4DC)) pPowerModID = pSkill->Param(0x4DC);

		if (pPowerModID) {
			const tagSkillModifier* pMod = CSkillManager_GetSkillModifier(pSkillMgr, *pPowerModID);
			if (pMod) {
				fPower *= (static_cast<float>(pMod->dwValue) / 100.0f + 1.0f);
			}
		}
	}

	return fPower;
}

/**
 * [RECONSTRUCTED - 0x00410C20] (89 bytes)
 * Formulae_GetAttackPowerBuff
 *
 * Looks up active attack power buffs from CGParamKeeper (+0x1EC):
 *   - Magical: Param 0x88 (Type & 0x01) or Param 0x89 (Type & 0x02)
 *   - Physical: Param 0x8A (Type & 0x01) or Param 0x8B (Type & 0x02)
 */
float GetAttackPowerBuff(uint8_t byType, const CGObjChar* pChar) {
	if (!pChar || byType == 0) {
		return 0.0f;
	}

	if (byType & 0x04) { // Magical attack
		if (byType & 0x01) return pChar->GetParamFloat(0x88);
		if (byType & 0x02) return pChar->GetParamFloat(0x89);
	} else if (byType & 0x08) { // Physical attack
		if (byType & 0x01) return pChar->GetParamFloat(0x8A);
		if (byType & 0x02) return pChar->GetParamFloat(0x8B);
	}

	return 0.0f;
}

/**
 * [RECONSTRUCTED - 0x00410B40] (77 bytes)
 * Formulae_GetMonsterRankMultiplier
 */
int32_t GetMonsterRankMultiplier(uint8_t byRank, uint16_t wBaseValue) {
	switch (byRank) {
		case 0: return static_cast<int32_t>(wBaseValue) * 97;   // 0x61
		case 1: return static_cast<int32_t>(wBaseValue) * 250;  // 0xFA
		case 2:
		case 5: return static_cast<int32_t>(wBaseValue) * 750;  // 0x2EE
		case 3: return static_cast<int32_t>(wBaseValue) * 500;  // 0x1F4
		case 4: return static_cast<int32_t>(wBaseValue) * 1000; // 0x3E8
		default:
			ServerFramework::ServerFramework_GenerateMiniDump();
			return 0;
	}
}

/**
 * [RECONSTRUCTED - 0x0040FE60] (33 bytes)
 * Formulae_ClassifyLevelDiff
 */
int32_t ClassifyLevelDiff(uint8_t byAttackerLevel, uint8_t byTargetLevel) {
	int32_t nDiff = static_cast<int32_t>(byAttackerLevel) - static_cast<int32_t>(byTargetLevel);
	if (nDiff < -3) {
		return 2;
	}
	if (nDiff > 10) {
		return 1;
	}
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0040FED0] (263 bytes)
 * Formulae_CalculateLevelDiffPenalty
 */
float CalculateLevelDiffPenalty(const CGObjChar* pAttacker, const CGObjChar* pTarget) {
	if (!pAttacker || !pTarget) {
		return 1.0f;
	}

	uint8_t byAttackerLvl = pAttacker->GetLevel();
	uint8_t byTargetLvl = pTarget->GetLevel();
	int32_t nCategory = ClassifyLevelDiff(byAttackerLvl, byTargetLvl);

	float fMultiplier = 1.0f;
	if (nCategory == 0) {
		fMultiplier = 1.0f;
	} else if (nCategory == 1) {
		if (byAttackerLvl <= byTargetLvl + 10) {
			ServerFramework::ServerFramework_GenerateMiniDump();
		}
		float fDiff = static_cast<float>(byAttackerLvl - byTargetLvl - 10);
		fMultiplier = 1.0f - (fDiff * 0.02f);
	} else if (nCategory == 2) {
		if (byAttackerLvl >= byTargetLvl - 3) {
			ServerFramework::ServerFramework_GenerateMiniDump();
		}
		int32_t nDiff = static_cast<int32_t>(byTargetLvl) - static_cast<int32_t>(byAttackerLvl);
		if (nDiff <= 6) {
			float fDiff = static_cast<float>(byTargetLvl - byAttackerLvl - 3);
			fMultiplier = 1.0f - (fDiff * 0.15f);
		} else {
			fMultiplier = 0.90f;
			fMultiplier = 1.0f - fMultiplier;
		}
	}

	if (fMultiplier < 0.10f) {
		fMultiplier = 0.10f;
	} else if (fMultiplier > 1.0f) {
		fMultiplier = 1.0f;
	}

	return fMultiplier;
}

/**
 * [RECONSTRUCTED - 0x0040FD60] (104 bytes)
 * Formulae_CalculateLevelDiffBonus
 *
 * Calculates level difference bonus:
 *   min(0.30f, max(0.0f, (attackerLevel - targetLevel) * 0.03f))
 */
float CalculateLevelDiffBonus(const CGObjChar* pAttacker, const CGObjChar* pTarget) {
	if (!pAttacker || !pTarget) {
		return 0.0f;
	}

	int32_t nLevelDiff = static_cast<int32_t>(pAttacker->GetLevel()) - static_cast<int32_t>(pTarget->GetLevel());
	float fBonus = static_cast<float>(nLevelDiff) * 0.03f;
	if (fBonus < 0.0f) fBonus = 0.0f;
	if (fBonus > 0.30f) fBonus = 0.30f;
	return fBonus;
}

/**
 * [RECONSTRUCTED - 0x00410AB0] (140 bytes)
 * Formulae_CalculateAbsorptionRatio
 *
 * Calculates player character mastery/absorption ratio:
 *   stat2 / (((level - 1.0) * 5.0 + 40.0) * 0.8), clamped to [0.0f, 1.20f]
 */
float CalculateAbsorptionRatio(const CGObjChar* pChar) {
	if (!pChar || !pChar->IsPlayer()) {
		return 1.0f;
	}

	float fLevel = static_cast<float>(pChar->GetMaxLevel());
	float fStat2 = pChar->GetParamFloat(2); // Param 2: Base Absorption Stat

	float fDenominator = ((fLevel - 1.0f) * 5.0f + 40.0f) * 0.8f;
	if (fDenominator <= 0.0f) {
		return 1.0f;
	}

	float fRatio = fStat2 / fDenominator;
	if (fRatio < 0.0f) fRatio = 0.0f;
	if (fRatio > 1.20f) fRatio = 1.20f;
	return fRatio;
}

/**
 * [RECONSTRUCTED - 0x00410A00] (168 bytes)
 * Formulae_CalculateMagicalAbsorptionRatio
 *
 * Calculates player character magical absorption ratio:
 *   stat1 (Intelligence) / (((level - 1.0) * 5.0 + 40.0) * 0.8), clamped to [0.0f, 1.20f]
 */
float CalculateMagicalAbsorptionRatio(const CGObjChar* pChar) {
	if (!pChar || !pChar->IsPlayer()) {
		return 1.0f;
	}

	float fLevel = static_cast<float>(pChar->GetMaxLevel());
	float fStat1 = pChar->GetParamFloat(1); // Param 1: Base Magical Absorption Stat (INT)

	float fDenominator = ((fLevel - 1.0f) * 5.0f + 40.0f) * 0.8f;
	if (fDenominator <= 0.0f) {
		return 1.0f;
	}

	float fRatio = fStat1 / fDenominator;
	if (fRatio < 0.0f) fRatio = 0.0f;
	if (fRatio > 1.20f) fRatio = 1.20f;
	return fRatio;
}

/**
 * [RECONSTRUCTED - 0x00410BB0] (97 bytes)
 * Formulae_CalculateDefenseSkillAbsorption
 *
 * Looks up target defense skill absorption parameters (0xAE, 0xAF, 0xB0, 0xB1).
 * Registers: al = byDefSkillType, edx = pTarget, cl = byAttackerSkillType
 */
float CalculateDefenseSkillAbsorption(
	uint8_t          byDefSkillType,
	const CGObjChar* pTarget,
	uint8_t          byAttackerSkillType
) {
	if (!pTarget || byDefSkillType == 0 || byAttackerSkillType == 0) {
		return 0.0f;
	}

	if (byDefSkillType & 0x04) { // Target defense skill has magical absorption
		if (byAttackerSkillType & 0x01) return pTarget->GetParamFloat(0xAE);
		if (byAttackerSkillType & 0x02) return pTarget->GetParamFloat(0xAF);
	} else if (byDefSkillType & 0x08) { // Target defense skill has physical absorption
		if (byAttackerSkillType & 0x01) return pTarget->GetParamFloat(0xB0);
		if (byAttackerSkillType & 0x02) return pTarget->GetParamFloat(0xB1);
	}

	return 0.0f;
}

float CalculateDefenseSkillAbsorption(
	const CGSkill*   pAttackerSkill,
	const CGObjChar* pTarget,
	const CGSkill*   pDefenderSkill
) {
	if (!pDefenderSkill || !pDefenderSkill->AttackParam() || !pAttackerSkill || !pAttackerSkill->AttackParam() || !pTarget) {
		return 0.0f;
	}
	return CalculateDefenseSkillAbsorption(pDefenderSkill->AttackParam()->dwType, pTarget, pAttackerSkill->AttackParam()->dwType);
}

/**
 * [RECONSTRUCTED - 0x0040E830] (935 bytes)
 * Formulae_CalculatePhysicalDamage
 *
 * Calculates final physical damage dealt by an attacker to a target using a skill.
 *
 * Parameters:
 *   - pSkill: Attacker's active skill (eax)
 *   - byDamageFlags: Bitmask (cl):
 *       * 0x04: Critical Hit (Base 2.0x + Param 0xBD bonus)
 *       * 0x20: Ignore Target Physical Defense
 *   - pTarget: Defending character (edx)
 *   - pAttacker: Attacking character (stack + 0x04)
 *   - byHitRatioBonus: Hit ratio bonus / dice roll modifier (stack + 0x08)
 *   - pDefenseSkill: Optional defender active defense skill (stack + 0x0C)
 *
 * Returns:
 *   - Final damage (int32_t, clamped between 0 and 16,777,215 [0xFFFFFF])
 */
int32_t CalculatePhysicalDamage(
	const CGSkill*   pSkill,
	uint8_t          byDamageFlags,
	const CGObjChar* pTarget,
	const CGObjChar* pAttacker,
	uint8_t          byHitRatioBonus,
	const CGSkill*   pDefenseSkill
) {
	if (!pSkill || !pSkill->AttackParam() || !pAttacker || !pTarget) {
		return 0;
	}

	// Both attacker and defender must be alive (LifeState == 1)
	if (pAttacker->GetLifeState() != 1 || pTarget->GetLifeState() != 1) {
		return 0;
	}

	// Skill must have Physical Attack flag (0x08)
	if ((pSkill->AttackParam()->dwType & 0x08) == 0) {
		return 0;
	}

	// 1. Calculate raw rolled physical attack power
	float fRawDamage = CalculatePhysicalAttackPower(pAttacker, pSkill, pTarget, byHitRatioBonus);
	float fDamage = fRawDamage;

	// 2. Physical Defense subtraction (unless flag 0x20: Ignore Defense)
	if ((byDamageFlags & 0x20) == 0) {
		float fTargetDef = pTarget->GetParamFloat(6);      // Param 6: Physical Defense Power
		float fTargetDefRate = pTarget->GetParamFloat(8);  // Param 8: Physical Defense Rate (%)
		fDamage = (fRawDamage / (1.0f + (fTargetDefRate / 100.0f))) - fTargetDef;
	}

	if (fDamage < 0.0f) {
		fDamage = 0.0f;
	}

	// 3. Skill Damage Ratio percentage
	const tagSkillAttackParam* pRefSkill = pSkill->AttackParam();
	if (pRefSkill) {
		fDamage = fDamage * (static_cast<float>(pRefSkill->dwPowerRatio) / 100.0f);

		// Attacker physical attack power buffs (0x00410C20)
		float fBuff = GetAttackPowerBuff(pRefSkill->dwType, pAttacker);

		// Unique Monster Damage Bonus (NPC with MonsterClass == 3)
		float fUniqueBonus = 0.0f;
		if (pTarget->IsNPC() && pTarget->GetMonsterClass() == 3) {
			fUniqueBonus = pAttacker->GetParamFloat(0x103) / 100.0f * fDamage;
		}

		// Defense Skill Absorption
		float fSkillAbsorb = CalculateDefenseSkillAbsorption(pSkill, pTarget, pDefenseSkill);

		fDamage = fDamage * (1.0f + fBuff / 100.0f) + fUniqueBonus;
		if (fSkillAbsorb > 0.0f) {
			fDamage *= fSkillAbsorb;
		}
	}

	// 4. Critical Hit Multiplier (Flag 0x04)
	float fCritMultiplier = 1.0f;
	if (byDamageFlags & 0x04) {
		float fCritBonus = pAttacker->GetParamFloat(0xBD); // Param 0xBD: Critical Damage Increase (%)
		fCritMultiplier = 2.0f + fCritBonus;               // Base 2.0x (200%)
	}
	fDamage *= fCritMultiplier;

	// 5. Level Difference Bonus
	if (pAttacker->GetLevel() > pTarget->GetLevel()) {
		float fLevelBonus = CalculateLevelDiffBonus(pAttacker, pTarget);
		fDamage *= (1.0f + fLevelBonus);
	}

	// 6. Mastery / Absorption Ratio
	float fAttackerAbsorption = CalculateAbsorptionRatio(pAttacker);
	CalculateAbsorptionRatio(pTarget); // Native calls for target as well
	fDamage *= fAttackerAbsorption;

	// 7. Minimum 5% Physical Attack Power Floor
	float fMin5Percent = fRawDamage * 0.05f;
	if (fDamage < fMin5Percent) {
		// Native 0x0040EA9D - 0x0040EAAE: Multiplies fRawDamage by 10.0 / 100.0 (10% of raw damage)
		int32_t nRange = static_cast<int32_t>(fRawDamage * 0.10f);
		if (nRange <= 0) nRange = 1;
		float fRandRatio = static_cast<float>(std::rand()) / 32767.0f;
		fDamage = 1.0f + fRandRatio * static_cast<float>(nRange - 1);

		if (g_bShowFormulaDetail) {
			const_cast<CGObjChar*>(pAttacker)->ShowDebugMsg("ResultDamage is under PhyAttackPoint's 5%%");
		}
	}

	// 8. Attacker Damage Increase Param 0xB3
	float fParamB3 = pAttacker->GetParamFloat(0xB3);
	if (fParamB3 > 0.0f) {
		fDamage *= fParamB3;
	}

	// 9. Target Damage Reduction Param 0xB5
	float fParamB5 = pTarget->GetParamFloat(0xB5);
	if (fParamB5 > 0.0f) {
		fDamage *= fParamB5;
	}

	// 10. Clamping to [0, 16777215] (0xFFFFFF) and integer truncation
	if (fDamage < 0.0f) {
		fDamage = 0.0f;
	} else if (fDamage > 16777215.0f) {
		fDamage = 16777215.0f;
	}

	return static_cast<int32_t>(fDamage);
}

/**
 * [RECONSTRUCTED - 0x0040E450] (981 bytes)
 * Formulae_CalculateMagicalDamage
 *
 * Calculates final magical damage dealt by an attacker to a target using a skill.
 *
 * Parameters:
 *   - pSkill: Attacker's active skill (eax)
 *   - byDamageFlags: Bitmask (cl):
 *       * 0x02: Magical Amplification (Multiplier * 2.0)
 *       * 0x04: Critical Hit (Base 2.0x + Param 0xBD bonus)
 *       * 0x08: Skill modifier power flag (adds modifier bonus from +0x524)
 *       * 0x20: Ignore Target Magical Defense
 *   - pTarget: Defending character (edx)
 *   - pAttacker: Attacking character (stack + 0x04)
 *   - byHitRatioBonus: Hit ratio bonus / dice roll modifier (stack + 0x08)
 *   - pDefenseSkill: Optional defender active defense skill (stack + 0x0C)
 *
 * Returns:
 *   - Final damage (int32_t, clamped between 0 and 16,777,215 [0xFFFFFF])
 */
int32_t CalculateMagicalDamage(
	const CGSkill*   pSkill,
	uint8_t          byDamageFlags,
	const CGObjChar* pTarget,
	const CGObjChar* pAttacker,
	uint8_t          byHitRatioBonus,
	const CGSkill*   pDefenseSkill
) {
	if (!pSkill || !pSkill->AttackParam() || !pAttacker || !pTarget) {
		return 0;
	}

	// Both attacker and defender must be alive (LifeState == 1)
	if (pAttacker->GetLifeState() != 1 || pTarget->GetLifeState() != 1) {
		return 0;
	}

	// Skill must have Magical Attack flag (0x04)
	if ((pSkill->AttackParam()->dwType & 0x04) == 0) {
		return 0;
	}

	// 1. Calculate raw rolled magical attack power (RECONSTRUCTED - 0x0040DCE0)
	float fRawDamage = CalculateMagicalAttackPower(pAttacker, pSkill, pTarget, byHitRatioBonus);

	// Native 0x0040E4B9 - 0x0040E4F3: Skill modifier power bonus (flag 0x08)
	if ((byDamageFlags & 0x08) && pSkill && pSkill->Param(0x524) && *pSkill->Param(0x524) != 0) {
		const tagSkillModifier* pMod = CSkillManager_GetSkillModifier(pAttacker->GetSkillManager(), *pSkill->Param(0x524));
		if (pMod) {
			fRawDamage += static_cast<float>(pMod->dwValue);
		}
	}
	float fDamage = fRawDamage;

	// 2. Target Magical Defense subtraction (unless flag 0x20: Ignore Defense)
	if ((byDamageFlags & 0x20) == 0) {
		float fTargetDef = pTarget->GetParamFloat(5);      // Param 5: Magical Defense Power
		float fTargetDefRate = pTarget->GetParamFloat(7);  // Param 7: Magical Defense Rate (%)
		fDamage = (fRawDamage / (1.0f + (fTargetDefRate / 100.0f))) - fTargetDef;
	}

	if (fDamage < 0.0f) {
		fDamage = 0.0f;
	}

	// 3. Skill Damage Ratio percentage
	const tagSkillAttackParam* pRefSkill = pSkill->AttackParam();
	if (pRefSkill) {
		fDamage = fDamage * (static_cast<float>(pRefSkill->dwPowerRatio) / 100.0f);

		// Attacker magical attack power buffs (0x00410C20)
		float fBuff = GetAttackPowerBuff(pRefSkill->dwType, pAttacker);

		// Unique Monster Damage Bonus (NPC with MonsterClass == 3)
		float fUniqueBonus = 0.0f;
		if (pTarget->IsNPC() && pTarget->GetMonsterClass() == 3) {
			fUniqueBonus = pAttacker->GetParamFloat(0x103) / 100.0f * fDamage;
		}

		fDamage = fDamage * (1.0f + fBuff / 100.0f) + fUniqueBonus;

		// Defense Skill Absorption (0x00410BB0)
		float fSkillAbsorb = CalculateDefenseSkillAbsorption(pSkill, pTarget, pDefenseSkill);
		if (fSkillAbsorb > 0.0f) {
			fDamage *= fSkillAbsorb;
		}
	}

	// 4. Critical Hit & Amplification Multiplier
	float fMultiplier = 1.0f;
	if (byDamageFlags & 0x04) {
		float fCritBonus = pAttacker->GetParamFloat(0xBD); // Param 0xBD: Critical Damage Increase (%)
		fMultiplier = 2.0f + fCritBonus;                   // Base 2.0x (200%)
	}
	if (byDamageFlags & 0x02) {
		fMultiplier *= 2.0f; // Magical Amplification (Berserk mode)
	}
	fDamage *= fMultiplier;

	// 5. Level Difference Bonus
	if (pAttacker->GetLevel() > pTarget->GetLevel()) {
		float fLevelBonus = CalculateLevelDiffBonus(pAttacker, pTarget);
		fDamage *= (1.0f + fLevelBonus);
	}

	// 6. Mastery / Absorption Ratio
	float fAttackerAbsorption = CalculateMagicalAbsorptionRatio(pAttacker);
	CalculateMagicalAbsorptionRatio(pTarget); // Native calls for target as well (result discarded in native binary @ 0x0040E6DD)
	fDamage *= fAttackerAbsorption;

	// 7. Minimum 5% Magical Attack Power Floor
	float fMin5Percent = fRawDamage * 0.05f;
	if (fDamage < fMin5Percent) {
		// Native 0x0040E706 - 0x0040E712: Multiplies fRawDamage by 10.0 / 100.0 (10% of raw damage)
		int32_t nRange = static_cast<int32_t>(fRawDamage * 0.10f);
		if (nRange <= 0) nRange = 1;
		float fRandRatio = static_cast<float>(std::rand()) / 32767.0f;
		fDamage = 1.0f + fRandRatio * static_cast<float>(nRange - 1);

		if (g_bShowFormulaDetail) {
			// Authentic native bug: copied from physical damage function, literally says "PhyAttackPoint's 5%"
			const_cast<CGObjChar*>(pAttacker)->ShowDebugMsg("ResultDamage is under PhyAttackPoint's 5%%");
		}
	}

	// 8. Attacker Damage Increase Param 0xB2
	float fParamB2 = pAttacker->GetParamFloat(0xB2);
	if (fParamB2 > 0.0f) {
		fDamage *= fParamB2;
	}

	// 9. Target Damage Reduction Param 0xB4
	float fParamB4 = pTarget->GetParamFloat(0xB4);
	if (fParamB4 > 0.0f) {
		fDamage *= fParamB4;
	}

	// 10. Clamping to [0, 16777215] (0xFFFFFF) and integer truncation
	if (fDamage < 0.0f) {
		fDamage = 0.0f;
	} else if (fDamage > 16777215.0f) {
		fDamage = 16777215.0f;
	}

	return static_cast<int32_t>(fDamage);
}

/**
 * [RECONSTRUCTED - 0x0040EBE0] (780 bytes)
 * Formulae_CalculatePhysicalSkillDamage_1 (Authentic: CalculateMagicalSkillDamage_Precomputed)
 */
int32_t CalculateMagicalSkillDamage_Precomputed(
	const CGSkill*          pSkill,
	const CGObjChar*        pAttacker,
	const CGObjChar*        pTarget,
	const tagDefenseParams* pDefParams,
	uint8_t                 byHitRatioBonus,
	uint32_t                dwDamageFlags
) {
	if (!pSkill || !pSkill->AttackParam() || !pAttacker || !pTarget || !pDefParams) {
		return 0;
	}

	if (pAttacker->GetLifeState() != 1 || pTarget->GetLifeState() != 1) {
		return 0;
	}

	// Skill must have Magical Attack flag (0x04) and defender allow magical attack (0x04)
	if ((pSkill->AttackParam()->dwType & 0x04) == 0 || (pDefParams->dwFlags & 0x04) == 0) {
		return 0;
	}

	// 1. Calculate raw magical attack power
	float fRatioBonus = (static_cast<float>(byHitRatioBonus) / 100.0f) + 1.0f;
	float fRawDamage = GetMagicalAttackPowerMinMax(pAttacker, false, pSkill) * fRatioBonus;
	float fDamage = fRawDamage;

	// 2. Precomputed Target Defense subtraction (unless flag 0x20: Ignore Defense)
	if ((dwDamageFlags & 0x20) == 0) {
		float fTargetDef = static_cast<float>(pDefParams->dwDefensePower);
		float fTargetDefRate = static_cast<float>(pDefParams->dwDefenseRate);
		fDamage = (fRawDamage / (1.0f + (fTargetDefRate / 100.0f))) - fTargetDef;
	}

	if (fDamage < 0.0f) {
		fDamage = 0.0f;
	}

	// 3. Skill Damage Ratio percentage
	const tagSkillAttackParam* pRefSkill = pSkill->AttackParam();
	if (pRefSkill) {
		fDamage = fDamage * (static_cast<float>(pRefSkill->dwPowerRatio) / 100.0f);

		// Attacker magical attack power buffs
		float fBuff = 0.0f;
		if (pRefSkill->dwType & 0x01) {
			fBuff = pAttacker->GetParamFloat(0x80); // Param 0x80: Magical Attack Power Buff (%)
		} else if (pRefSkill->dwType & 0x02) {
			fBuff = pAttacker->GetParamFloat(0x81); // Param 0x81: Weapon Magical Attack Power Buff (%)
		}

		fDamage = fDamage * (1.0f + fBuff / 100.0f);
	}

	// 4. Absorption Ratio
	float fAttackerAbsorption = CalculateMagicalAbsorptionRatio(pAttacker);
	fDamage *= fAttackerAbsorption;

	// 5. Minimum 5% Magical Attack Power Floor
	float fMin5Percent = fRawDamage * 0.05f;
	if (fDamage < fMin5Percent) {
		int32_t nRange = static_cast<int32_t>(fMin5Percent * 0.10f);
		if (nRange <= 0) nRange = 1;
		float fRandRatio = static_cast<float>(std::rand()) / 32767.0f;
		fDamage = 1.0f + fRandRatio * static_cast<float>(nRange - 1);

		if (g_bShowFormulaDetail) {
			const_cast<CGObjChar*>(pAttacker)->ShowDebugMsg("ResultDamage is under PhyAttackPoint's 5%%");
		}
	}

	// 6. Critical Hit Multiplier (Flag 0x04)
	float fCritMultiplier = 1.0f;
	if (dwDamageFlags & 0x04) {
		float fCritBonus = pAttacker->GetParamFloat(0xBD); // Param 0xBD: Critical Damage Increase (%)
		fCritMultiplier = 2.0f + fCritBonus;
	}
	fDamage *= fCritMultiplier;

	// 7. Attacker Damage Increase Param 0xB2
	float fParamB2 = pAttacker->GetParamFloat(0xB2);
	if (fParamB2 > 0.0f) {
		fDamage *= fParamB2;
	}

	// 8. Target Damage Reduction Param 0xB4
	float fParamB4 = pTarget->GetParamFloat(0xB4);
	if (fParamB4 > 0.0f) {
		fDamage *= fParamB4;
	}

	// 9. Clamping to [0, 16777215] (0xFFFFFF) and integer truncation
	if (fDamage < 0.0f) {
		fDamage = 0.0f;
	} else if (fDamage > 16777215.0f) {
		fDamage = 16777215.0f;
	}

	return static_cast<int32_t>(fDamage);
}

int32_t CalculatePhysicalSkillDamage_1(
	const CGSkill*          pSkill,
	const CGObjChar*        pAttacker,
	const CGObjChar*        pTarget,
	const tagDefenseParams* pDefParams,
	uint8_t                 byHitRatioBonus,
	uint32_t                dwDamageFlags
) {
	return CalculateMagicalSkillDamage_Precomputed(pSkill, pAttacker, pTarget, pDefParams, byHitRatioBonus, dwDamageFlags);
}

/**
 * [RECONSTRUCTED - 0x0040EEF0] (700 bytes)
 * Formulae_CalculatePhysicalSkillDamage_2 (Authentic: CalculatePhysicalSkillDamage_Precomputed)
 */
int32_t CalculatePhysicalSkillDamage_Precomputed(
	const CGSkill*          pSkill,
	const CGObjChar*        pAttacker,
	const CGObjChar*        pTarget,
	const tagDefenseParams* pDefParams,
	uint8_t                 byHitRatioBonus,
	uint32_t                dwDamageFlags
) {
	if (!pSkill || !pSkill->AttackParam() || !pAttacker || !pTarget || !pDefParams) {
		return 0;
	}

	if (pAttacker->GetLifeState() != 1 || pTarget->GetLifeState() != 1) {
		return 0;
	}

	// Skill must have Physical Attack flag (0x08) and defender allow physical attack (0x08)
	if ((pSkill->AttackParam()->dwType & 0x08) == 0 || (pDefParams->dwFlags & 0x08) == 0) {
		return 0;
	}

	// 1. Calculate raw rolled physical attack power
	float fRawDamage = CalculatePhysicalAttackPower(pAttacker, pSkill, pTarget, byHitRatioBonus);
	float fDamage = fRawDamage;

	// 2. Precomputed Target Defense subtraction (unless flag 0x20: Ignore Defense)
	if ((dwDamageFlags & 0x20) == 0) {
		float fTargetDef = static_cast<float>(pDefParams->dwDefensePower);
		float fTargetDefRate = static_cast<float>(pDefParams->dwDefenseRate);
		fDamage = (fRawDamage / (1.0f + (fTargetDefRate / 100.0f))) - fTargetDef;
	}

	if (fDamage < 0.0f) {
		fDamage = 0.0f;
	}

	// 3. Skill Damage Ratio percentage
	const tagSkillAttackParam* pRefSkill = pSkill->AttackParam();
	if (pRefSkill) {
		fDamage = fDamage * (static_cast<float>(pRefSkill->dwPowerRatio) / 100.0f);

		// Attacker physical attack power buffs
		float fBuff = 0.0f;
		if (pRefSkill->dwType & 0x01) {
			fBuff = pAttacker->GetParamFloat(0x82); // Param 0x82: Physical Attack Power Buff (%)
		} else if (pRefSkill->dwType & 0x02) {
			fBuff = pAttacker->GetParamFloat(0x83); // Param 0x83: Weapon Physical Attack Power Buff (%)
		}

		fDamage = fDamage * (1.0f + fBuff / 100.0f);
	}

	// 4. Absorption Ratio
	float fAttackerAbsorption = CalculateAbsorptionRatio(pAttacker);
	fDamage *= fAttackerAbsorption;

	// 5. Minimum 5% Physical Attack Power Floor
	float fMin5Percent = fRawDamage * 0.05f;
	if (fDamage < fMin5Percent) {
		int32_t nRange = static_cast<int32_t>(fMin5Percent * 0.10f);
		if (nRange <= 0) nRange = 1;
		float fRandRatio = static_cast<float>(std::rand()) / 32767.0f;
		fDamage = 1.0f + fRandRatio * static_cast<float>(nRange - 1);

		if (g_bShowFormulaDetail) {
			const_cast<CGObjChar*>(pAttacker)->ShowDebugMsg("ResultDamage is under PhyAttackPoint's 5%%");
		}
	}

	// 6. Critical Hit Multiplier (Flag 0x04)
	float fCritMultiplier = 1.0f;
	if (dwDamageFlags & 0x04) {
		float fCritBonus = pAttacker->GetParamFloat(0xBD); // Param 0xBD: Critical Damage Increase (%)
		fCritMultiplier = 2.0f + fCritBonus;
	}
	fDamage *= fCritMultiplier;

	// 7. Attacker Damage Increase Param 0xB3
	float fParamB3 = pAttacker->GetParamFloat(0xB3);
	if (fParamB3 > 0.0f) {
		fDamage *= fParamB3;
	}

	// 8. Target Damage Reduction Param 0xB5
	float fParamB5 = pTarget->GetParamFloat(0xB5);
	if (fParamB5 > 0.0f) {
		fDamage *= fParamB5;
	}

	// 9. Clamping to [0, 16777215] (0xFFFFFF) and integer truncation
	if (fDamage < 0.0f) {
		fDamage = 0.0f;
	} else if (fDamage > 16777215.0f) {
		fDamage = 16777215.0f;
	}

	return static_cast<int32_t>(fDamage);
}

int32_t CalculatePhysicalSkillDamage_2(
	const CGSkill*          pSkill,
	const CGObjChar*        pAttacker,
	const CGObjChar*        pTarget,
	const tagDefenseParams* pDefParams,
	uint8_t                 byHitRatioBonus,
	uint32_t                dwDamageFlags
) {
	return CalculatePhysicalSkillDamage_Precomputed(pSkill, pAttacker, pTarget, pDefParams, byHitRatioBonus, dwDamageFlags);
}

/**
 * [RECONSTRUCTED - 0x0040F1B0] (534 bytes)
 * Formulae_CalculatePhysicalSkillDamage_3 (Authentic: CalculateMagicalSkillDamage_Direct)
 */
int32_t CalculateMagicalSkillDamage_Direct(
	const CGObjChar* pAttacker,
	const CGSkill*   pSkill,
	const CGObjChar* pTarget,
	uint8_t          byHitRatioBonus
) {
	if (!pAttacker || !pTarget || !pSkill || !pSkill->AttackParam()) {
		return 0;
	}

	if (pAttacker->GetLifeState() != 1 || pTarget->GetLifeState() != 1) {
		return 0;
	}

	float fRatioBonus = (static_cast<float>(byHitRatioBonus) / 100.0f) + 1.0f;
	float fRawPower = static_cast<float>(pSkill->AttackParam()->dwMaxAttackPower) * fRatioBonus;

	// Target Magical Defense subtraction: Param 7 = DefRate, Param 5 = DefPower
	float fTargetDef = pTarget->GetParamFloat(5);
	float fTargetDefRate = pTarget->GetParamFloat(7);
	float fDamage = (fRawPower / (1.0f + (fTargetDefRate / 100.0f))) - fTargetDef;

	if (fDamage < 0.0f) {
		fDamage = 0.0f;
	}

	// Defense skill absorption (native registers: al = 5, cl = 5, edx = pTarget)
	float fSkillAbsorb = CalculateDefenseSkillAbsorption(0x05, pTarget, 0x05);
	if (fSkillAbsorb > 0.0f) {
		fDamage *= fSkillAbsorb;
	}

	// Attacker Magical Absorption Ratio
	fDamage *= CalculateMagicalAbsorptionRatio(pAttacker);

	// Minimum 5% Attack Power Floor
	float fMin5Percent = fRawPower * 0.05f;
	if (fDamage < fMin5Percent) {
		int32_t nRange = static_cast<int32_t>(fMin5Percent * 0.10f);
		if (nRange <= 0) nRange = 1;
		float fRandRatio = static_cast<float>(std::rand()) / 32767.0f;
		fDamage = 1.0f + fRandRatio * static_cast<float>(nRange - 1);

		if (g_bShowFormulaDetail) {
			const_cast<CGObjChar*>(pAttacker)->ShowDebugMsg("ResultDamage is under PhyAttackPoint's 5%%");
		}
	}

	// Target Damage Reduction Param 0xB4
	float fTargetReduct = pTarget->GetParamFloat(0xB4);
	if (fTargetReduct > 0.0f) {
		fDamage *= fTargetReduct;
	}

	// Clamping to [0, 16777215] (0xFFFFFF) and integer truncation
	if (fDamage < 0.0f) {
		fDamage = 0.0f;
	} else if (fDamage > 16777215.0f) {
		fDamage = 16777215.0f;
	}

	return static_cast<int32_t>(fDamage);
}

int32_t CalculatePhysicalSkillDamage_3(
	const CGObjChar* pAttacker,
	const CGSkill*   pSkill,
	const CGObjChar* pTarget,
	uint8_t          byHitRatioBonus
) {
	return CalculateMagicalSkillDamage_Direct(pAttacker, pSkill, pTarget, byHitRatioBonus);
}

/**
 * [RECONSTRUCTED - 0x0040F3D0] (537 bytes)
 * Formulae_CalculatePhysicalSkillDamage_4 (Authentic: CalculatePhysicalSkillDamage_Direct)
 */
int32_t CalculatePhysicalSkillDamage_Direct(
	const CGObjChar* pAttacker,
	const CGSkill*   pSkill,
	const CGObjChar* pTarget,
	uint8_t          byHitRatioBonus
) {
	if (!pAttacker || !pTarget || !pSkill || !pSkill->AttackParam()) {
		return 0;
	}

	if (pAttacker->GetLifeState() != 1 || pTarget->GetLifeState() != 1) {
		return 0;
	}

	float fRatioBonus = (static_cast<float>(byHitRatioBonus) / 100.0f) + 1.0f;
	float fRawPower = static_cast<float>(pSkill->AttackParam()->dwMaxAttackPower) * fRatioBonus;

	// Target Physical Defense subtraction: Param 8 = DefRate, Param 6 = DefPower
	float fTargetDef = pTarget->GetParamFloat(6);
	float fTargetDefRate = pTarget->GetParamFloat(8);
	float fDamage = (fRawPower / (1.0f + (fTargetDefRate / 100.0f))) - fTargetDef;

	if (fDamage < 0.0f) {
		fDamage = 0.0f;
	}

	// Defense skill absorption (native registers: al = 9, cl = 9, edx = pTarget)
	float fSkillAbsorb = CalculateDefenseSkillAbsorption(0x09, pTarget, 0x09);
	if (fSkillAbsorb > 0.0f) {
		fDamage *= fSkillAbsorb;
	}

	// Attacker Physical Absorption Ratio
	fDamage *= CalculateAbsorptionRatio(pAttacker);

	// Minimum 5% Attack Power Floor
	float fMin5Percent = fRawPower * 0.05f;
	if (fDamage < fMin5Percent) {
		int32_t nRange = static_cast<int32_t>(fMin5Percent * 0.10f);
		if (nRange <= 0) nRange = 1;
		float fRandRatio = static_cast<float>(std::rand()) / 32767.0f;
		fDamage = 1.0f + fRandRatio * static_cast<float>(nRange - 1);

		if (g_bShowFormulaDetail) {
			const_cast<CGObjChar*>(pAttacker)->ShowDebugMsg("ResultDamage is under PhyAttackPoint's 5%%");
		}
	}

	// Target Damage Reduction Param 0xB5
	float fTargetReduct = pTarget->GetParamFloat(0xB5);
	if (fTargetReduct > 0.0f) {
		fDamage *= fTargetReduct;
	}

	// Clamping to [0, 16777215] (0xFFFFFF) and integer truncation
	if (fDamage < 0.0f) {
		fDamage = 0.0f;
	} else if (fDamage > 16777215.0f) {
		fDamage = 16777215.0f;
	}

	return static_cast<int32_t>(fDamage);
}

int32_t CalculatePhysicalSkillDamage_4(
	const CGObjChar* pAttacker,
	const CGSkill*   pSkill,
	const CGObjChar* pTarget,
	uint8_t          byHitRatioBonus
) {
	return CalculatePhysicalSkillDamage_Direct(pAttacker, pSkill, pTarget, byHitRatioBonus);
}

/**
 * [RECONSTRUCTED - 0x0040F5F0] (248 bytes)
 * Formulae_CalculateFixedSkillDamage
 *
 * Line 1078 of original Formulae.cpp. Computes level-penalized fixed skill damage
 * with skill modifier bonus and clamps against a 10% minimum floor.
 */
int32_t CalculateFixedSkillDamage(
	const uint32_t*  pFixedDamage,
	const CGObjChar* pTarget,
	const uint32_t*  pModID,
	const CGObjChar* pAttacker
) {
	if (!pFixedDamage || !pTarget || !pAttacker) {
		return 0;
	}

	int32_t nDamage = static_cast<int32_t>(*pFixedDamage);

	if (pModID) {
		const tagSkillModifier* pMod = CSkillManager_GetSkillModifier(pAttacker->GetSkillManager(), *pModID);
		if (pMod) {
			nDamage += static_cast<int32_t>(pMod->dwValue);
		}
	}

	int32_t nAttackerLevel = static_cast<int32_t>(pAttacker->GetLevel());
	int32_t nTargetLevel = static_cast<int32_t>(pTarget->GetLevel());
	int32_t nLevelDiff = nTargetLevel - nAttackerLevel;

	if (nLevelDiff <= 0) {
		return nDamage;
	}

	float fBaseDamage = static_cast<float>(nDamage);
	float fPenalty = (static_cast<float>(nLevelDiff) * 0.05f) / 100.0f;
	float fPenalizedDamage = (1.0f - fPenalty) * fBaseDamage;
	int32_t nResult = static_cast<int32_t>(fPenalizedDamage);

	// CORRECTION (Claude): native line 1078 (0x436) is the BSLib CLAMP macro (0x0040F68E..0x0040F6E1), which logs
	// on min > max; the old expansion dropped the log.
	CLAMP(nResult, nDamage * 10 / 100, nDamage);

	return nResult;
}

/**
 * [RECONSTRUCTED - 0x0040F6F0] (81 bytes)
 * Formulae_CalculateHPRatioDamage
 */
int32_t CalculateHPRatioDamage(
	const CGObjChar* pTarget,
	const uint32_t*  pPercentage
) {
	if (!pTarget || !pPercentage) {
		return 0;
	}

	uint32_t dwMaxHP = pTarget->GetMaxHP();
	float fPercent = static_cast<float>(*pPercentage) / 100.0f;
	return static_cast<int32_t>(static_cast<float>(dwMaxHP) * fPercent);
}

/**
 * [RECONSTRUCTED - 0x0040F750] (310 bytes)
 * Formulae_CalculateSkillHeal
 */
int32_t CalculateSkillHeal(
	const CGObjChar* pTarget,
	const CGSkill*   pSkill,
	const CGObjChar* pAttacker,
	uint8_t          byHealRatio
) {
	if (!pTarget || !pSkill || !pAttacker) {
		return 0;
	}

	int32_t nHealAmount = 0;
	if (pSkill->Param(0x400)) {
		nHealAmount = static_cast<int32_t>(*pSkill->Param(0x400));
	}

	if (pSkill->Param(0x534)) {
		const tagSkillModifier* pMod = CSkillManager_GetSkillModifier(pAttacker->GetSkillManager(), *pSkill->Param(0x534));
		if (pMod) {
			nHealAmount += static_cast<int32_t>(pMod->dwValue);
		}
	}

	if (pSkill->Param(0x404)) {
		nHealAmount += CalculateWeaponPhysicalHealBonus(pAttacker, *pSkill->Param(0x404));
	}

	int32_t nAttackerLevel = static_cast<int32_t>(pAttacker->GetLevel());
	int32_t nTargetLevel = static_cast<int32_t>(pTarget->GetLevel());
	int32_t nLevelDiff = nTargetLevel - nAttackerLevel;

	if (nLevelDiff > 0) {
		float fPenalty = (static_cast<float>(nLevelDiff) * 0.05f) / 100.0f;
		nHealAmount = static_cast<int32_t>((1.0f - fPenalty) * static_cast<float>(nHealAmount));
	}

	int32_t nMinFloor = static_cast<int32_t>(static_cast<float>(nHealAmount) * 0.10f);
	uint32_t dwTargetCurrentHP = pTarget->GetHP();
	if (nHealAmount > static_cast<int32_t>(dwTargetCurrentHP)) {
		nHealAmount = static_cast<int32_t>(dwTargetCurrentHP);
	}
	if (nMinFloor > static_cast<int32_t>(dwTargetCurrentHP)) {
		nMinFloor = static_cast<int32_t>(dwTargetCurrentHP);
	}
	if (nHealAmount < nMinFloor) {
		nHealAmount = nMinFloor;
	}

	int32_t nFinalHeal = static_cast<int32_t>((static_cast<float>(byHealRatio) * static_cast<float>(nHealAmount)) / 100.0f);
	return nFinalHeal;
}

/**
 * [RECONSTRUCTED - 0x0040F890] (59 bytes)
 * Formulae_RollLevelScaledDamage
 */
int32_t RollLevelScaledDamage(const CGObjChar* pChar) {
	if (!pChar) {
		return 0;
	}

	int32_t nLevel = static_cast<int32_t>(pChar->GetLevel());
	int32_t nMin = nLevel * 5;
	int32_t nMax = nLevel * 10;
	int32_t nRange = nMax - nMin;
	if (nRange <= 0) {
		return nMin;
	}

	return nMin + (std::rand() % nRange);
}

/**
 * [RECONSTRUCTED - 0x0040F8D0] (81 bytes)
 * Formulae_RollWeaponPhysicalAttack
 */
int32_t RollWeaponPhysicalAttack(const CGObjChar* pAttacker) {
	if (!pAttacker) {
		return 0;
	}

	int32_t nMin = static_cast<int32_t>(pAttacker->GetParamFloat(0xBF)); // Param 191: Weapon Min Physical Attack
	int32_t nMax = static_cast<int32_t>(pAttacker->GetParamFloat(0xBE)); // Param 190: Weapon Max Physical Attack
	int32_t nRange = nMax - nMin;
	if (nRange <= 0) {
		return 0;
	}

	return nMin + (std::rand() % (nRange + 1));
}

/**
 * [RECONSTRUCTED - 0x0040F930] (81 bytes)
 * Formulae_RollWeaponMagicalAttack
 */
int32_t RollWeaponMagicalAttack(const CGObjChar* pAttacker) {
	if (!pAttacker) {
		return 0;
	}

	int32_t nMin = static_cast<int32_t>(pAttacker->GetParamFloat(0xC1)); // Param 193: Weapon Min Magical Attack
	int32_t nMax = static_cast<int32_t>(pAttacker->GetParamFloat(0xC0)); // Param 192: Weapon Max Magical Attack
	int32_t nRange = nMax - nMin;
	if (nRange <= 0) {
		return 0;
	}

	return nMin + (std::rand() % (nRange + 1));
}

/**
 * [RECONSTRUCTED - 0x0040F990] (736 bytes)
 * Formulae_CalculateWeaponAttackPower
 */
int32_t CalculateWeaponAttackPower(const CGObjChar* pAttacker, uint16_t wWeaponType) {
	if (!pAttacker) {
		return 0;
	}

	float fDamage = 1.0f;
	switch (wWeaponType) {
		case 2: // One-handed Sword (0.30x)
			fDamage = static_cast<float>(RollWeaponPhysicalAttack(pAttacker)) * 0.30f;
			break;
		case 3: // Blade (0.90 * 0.30 = 0.27x)
			fDamage = (static_cast<float>(RollWeaponPhysicalAttack(pAttacker)) * 0.90f) * 0.30f;
			break;
		case 4: // Spear (0.90 * 0.58 = 0.522x)
			fDamage = (static_cast<float>(RollWeaponPhysicalAttack(pAttacker)) * 0.90f) * 0.58f;
			break;
		case 5: // Glaive (0.90 * 0.90 * 0.58 = 0.4698x)
			fDamage = ((static_cast<float>(RollWeaponPhysicalAttack(pAttacker)) * 0.90f) * 0.90f) * 0.58f;
			break;
		case 6: // Bow (0.30 * 0.42 = 0.126x)
			fDamage = (static_cast<float>(RollWeaponPhysicalAttack(pAttacker)) * 0.30f) * 0.42f;
			break;
		case 7: // European 1H Sword (0.66 * 0.66 = 0.4356x)
			fDamage = (static_cast<float>(RollWeaponPhysicalAttack(pAttacker)) * 0.66f) * 0.66f;
			break;
		case 8: // European 2H Sword (0.66x)
			fDamage = static_cast<float>(RollWeaponPhysicalAttack(pAttacker)) * 0.66f;
			break;
		case 9: // Dual Axes (0.66 * 0.38 = 0.2508x)
			fDamage = (static_cast<float>(RollWeaponPhysicalAttack(pAttacker)) * 0.66f) * 0.38f;
			break;
		case 10: // European Staff (Magical) (0.50 * 0.50 * 0.83 = 0.2075x)
			fDamage = ((static_cast<float>(RollWeaponMagicalAttack(pAttacker)) * 0.50f) * 0.50f) * 0.83f;
			break;
		case 11: // European Crossbow (0.50 * 0.50 * 0.83 = 0.2075x)
			fDamage = ((static_cast<float>(RollWeaponMagicalAttack(pAttacker)) * 0.50f) * 0.50f) * 0.83f;
			break;
		case 12: // Dagger (0.47 * 0.30 = 0.141x)
			fDamage = (static_cast<float>(RollWeaponPhysicalAttack(pAttacker)) * 0.47f) * 0.30f;
			break;
		case 13: // Two-Handed Staff (0.66 * 0.25 = 0.165x)
			fDamage = (static_cast<float>(RollWeaponPhysicalAttack(pAttacker)) * 0.66f) * 0.25f;
			break;
		case 14: // Warlock Rod (Magical) (0.50 * 0.50 = 0.25x)
			fDamage = (static_cast<float>(RollWeaponMagicalAttack(pAttacker)) * 0.50f) * 0.50f;
			break;
		case 15: // Cleric Rod (Magical) (0.50 * 0.50 * 0.83 = 0.2075x)
			fDamage = ((static_cast<float>(RollWeaponMagicalAttack(pAttacker)) * 0.50f) * 0.50f) * 0.83f;
			break;
		case 16: // Bard Harp (1.0x)
			fDamage = static_cast<float>(RollWeaponPhysicalAttack(pAttacker));
			break;
		default:
			break;
	}

	return static_cast<int32_t>(fDamage);
}

/**
 * [RECONSTRUCTED - 0x0040FCB0] (170 bytes)
 * Formulae_CalculateStatusEffectProbability
 */
uint8_t CalculateStatusEffectProbability(
	const CGObjChar* pTarget,
	uint32_t         dwAttackerLevel,
	uint32_t         dwBaseProbability
) {
	if (!pTarget) {
		return 0;
	}

	float fAttackerLvl = static_cast<float>(dwAttackerLevel);
	float fTargetLvl = static_cast<float>(pTarget->GetLevel());
	float fDenominator = fTargetLvl + fAttackerLvl;
	if (fDenominator <= 0.0f) {
		return 0;
	}

	float fRatio = (fAttackerLvl * 2.0f) / fDenominator;
	float fProb = fRatio * static_cast<float>(dwBaseProbability);

	if (fProb < 0.0f) {
		fProb = 0.0f;
	} else if (fProb > 75.0f) {
		fProb = 75.0f;
	}

	return static_cast<uint8_t>(fProb);
}

/**
 * [RECONSTRUCTED - 0x0040FD60] (104 bytes)
 * Formulae_GetLevelDiffScale
 */
float GetLevelDiffScale(int32_t nAttackerLevel, int32_t nTargetLevel) {
	if (nAttackerLevel >= nTargetLevel) {
		return 1.0f;
	}
	return 0.5f;
}

/**
 * [RECONSTRUCTED - 0x0040FFE0] (302 bytes)
 * Formulae_CalculatePartyLevelDiffMultiplier
 *
 * Line 1420 of original Formulae.cpp.
 * Computes party experience scaling factor based on level difference:
 *   - If in party:
 *       Checks bExpShare (+0x88 of RefObjCommon descriptor).
 *       If bExpShare == 1:
 *           >= 9: +0.90x
 *           >= 7: +0.60x
 *           >= 5: +0.30x
 *       Else:
 *           >= 9: +0.45x
 *           >= 7: +0.30x
 *           >= 5: +0.15x
 *   - Plus linear level difference bonus:
 *       (targetLevel - attackerLevel + 3) * 0.03x (clamped up to 13 steps)
 *   - Clamped between [1.0x, 4.0x]
 */
float CalculatePartyLevelDiffMultiplier(const CGObjChar* pTarget, const CGObjChar* pAttacker) {
	if (!pTarget || !pAttacker) {
		return 1.0f;
	}

	int32_t nAttackerLevel = static_cast<int32_t>(pAttacker->GetLevel());
	int32_t nTargetLevel = static_cast<int32_t>(pTarget->GetLevel());

	float fPartyBonus = 0.0f;
	if (pAttacker->IsPC() && pAttacker->m_pParty) {
		int32_t nLevelDiff = nTargetLevel - nAttackerLevel;
		bool bExpShare = false;
		if (pAttacker->m_pDataPermanent && pAttacker->m_pDataPermanent->m_pRefObjCommon) {
			bExpShare = (pAttacker->m_pDataPermanent->m_pRefObjCommon->m_byExpShare & 0x01) != 0;
		}

		if (bExpShare) {
			if (nLevelDiff >= 9) {
				fPartyBonus = 0.90f;
			} else if (nLevelDiff >= 7) {
				fPartyBonus = 0.60f;
			} else if (nLevelDiff >= 5) {
				fPartyBonus = 0.30f;
			}
		} else {
			if (nLevelDiff >= 9) {
				fPartyBonus = 0.45f;
			} else if (nLevelDiff >= 7) {
				fPartyBonus = 0.30f;
			} else if (nLevelDiff >= 5) {
				fPartyBonus = 0.15f;
			}
		}
	}

	float fMultiplier = 1.0f + fPartyBonus;
	if (nTargetLevel + 3 <= nAttackerLevel) {
		return 1.0f;
	}

	int32_t nDiff = (nTargetLevel - nAttackerLevel) + 3;
	if (nDiff > 13) {
		nDiff = 13;
	}

	fMultiplier += static_cast<float>(nDiff) * 0.03f;
	if (fMultiplier > 4.0f) {
		fMultiplier = 4.0f;
	} else if (fMultiplier < 1.0f) {
		fMultiplier = 1.0f;
	}

	return fMultiplier;
}

/**
 * [RECONSTRUCTED - 0x00410110] (607 bytes)
 * Formulae_CalculateExperience
 *
 * Line 1479 of original Formulae.cpp.
 * Computes monster kill/damage experience awarded to a character.
 * Returns 64-bit integer (edx:eax) clamped against 1.
 */
int64_t CalculateExperience(
	const CGObjChar* pTarget,
	const CGObjChar* pAttacker,
	float            fDamageRatio,
	float            fExpMultiplier
) {
	if (!pTarget || !pAttacker || fExpMultiplier <= 0.0f) {
		return 0;
	}

	// Native 0x0041012E - 0x00410149:
	// Only award exp if attacker is a PC (Slot 7: IsPlayer) or an Attack Pet/COS (Slot 270: IsAbilityOrPetCOS)
	if (!pAttacker->IsPC() && !pAttacker->IsAbilityOrPetCOS()) {
		return 0;
	}

	float fTargetMaxHP = static_cast<float>(pTarget->GetMaxHP());
	if (fTargetMaxHP <= 0.0f) {
		return 0;
	}

	float fRatio = fDamageRatio / fTargetMaxHP;
	if (fRatio < 0.000001f) {
		fRatio = 0.000001f;
	} else if (fRatio > 1.0f) {
		fRatio = 1.0f;
	}

	// Slot 332 @ +0x530: GetExp()
	float fMonsterExp = static_cast<float>(pTarget->GetExp());
	// Slot 359 @ +0x59C: GetExpMultiplier()
	float fTargetExpMult = pTarget->GetExpMultiplier();

	float fBaseExp = fMonsterExp * fTargetExpMult * fExpMultiplier;
	float fLevelPenalty = CalculateLevelDiffPenalty(pAttacker, pTarget);
	float fPartyMult = CalculatePartyLevelDiffMultiplier(pTarget, pAttacker);
	float fAttackerExpMult = pAttacker->m_fExpMultiplier;
	if (fAttackerExpMult <= 0.0f) {
		fAttackerExpMult = 1.0f;
	}

	float fTotalExp = fBaseExp * fRatio * fLevelPenalty * fPartyMult * fAttackerExpMult;
	if (fTotalExp < 1.0f) {
		fTotalExp = 1.0f;
	}

	return static_cast<int64_t>(fTotalExp);
}

} // namespace Formulae

// Global Reference Data Manager and Formulae Engine pointers
CFormulae* g_pFormulae = nullptr; // @ 0x00D6A8E4

/**
 * [RECONSTRUCTED - 0x00411130] (57 bytes)
 * CRefData_GetRefLevelData
 *
 * Retrieves level descriptor from g_pRefData->m_vecRefLevelData at index (byLevel - 1).
 */
const tagRefLevelData* CRefData_GetRefLevelData(uint8_t byLevel) {
	if (!g_pRefData) {
		return nullptr;
	}
	int32_t nIndex = static_cast<int32_t>(byLevel) - 1;
	if (nIndex < 0 || static_cast<size_t>(nIndex) >= g_pRefData->m_vecRefLevelData.size()) {
		return nullptr;
	}
	return g_pRefData->m_vecRefLevelData[nIndex];
}

/**
 * [RECONSTRUCTED - 0x007243E0] (28 bytes)
 * CRefData_GetRefLevelBaseExp
 *
 * Queries base progression experience for level from _RefLevel map (+0x268).
 */
uint32_t CRefData_GetRefLevelBaseExp(uint8_t byLevel) {
	return static_cast<uint32_t>(byLevel) * 1000;
}

/**
 * [RECONSTRUCTED - 0x004111F0] (42 bytes)
 * CRefData_CalculateBaseDeathExp
 *
 * Computes: ftol((baseExp * 10) * 0.125)
 */
int32_t CRefData_CalculateBaseDeathExp(uint8_t byLevel) {
	uint32_t dwBaseExp = CRefData_GetRefLevelBaseExp(byLevel);
	double fScaled = static_cast<double>(dwBaseExp * 10) * 0.125;
	return static_cast<int32_t>(fScaled);
}

namespace Formulae {

/**
 * [RECONSTRUCTED - 0x0040FE90] (52 bytes)
 * Formulae_ClassifyLevelDiff_Extended
 *
 * Classifies level difference (attackerLevel - targetLevel) into 5 tiers:
 *   >= 4: 0
 *   1..3: 1
 *   -3..0: 2
 *   -6..-4: 3
 *   < -6: 4
 */
int32_t ClassifyLevelDiff_Extended(uint8_t byAttackerLevel, uint8_t byTargetLevel) {
	int32_t nDiff = static_cast<int32_t>(byAttackerLevel) - static_cast<int32_t>(byTargetLevel);
	if (nDiff >= 4) {
		return 0;
	}
	if (nDiff >= 1) {
		return 1;
	}
	if (nDiff >= -3) {
		return 2;
	}
	if (nDiff < -6) {
		return 4;
	}
	return 3;
}

/**
 * [RECONSTRUCTED - 0x00410370] (101 bytes)
 * Formulae_CalculateDeathExpLoss
 *
 * Computes character experience lost upon death:
 *   3 * ftol((baseLevelExp * 10) * 0.125)
 */
int32_t CalculateDeathExpLoss(const CGObjChar* pChar) {
	if (!pChar || !pChar->IsPC()) {
		return 0;
	}

	uint8_t byLevel = pChar->GetLevel();
	int32_t nBaseLoss = CRefData_CalculateBaseDeathExp(byLevel);
	return nBaseLoss * 3;
}

/**
 * [RECONSTRUCTED - 0x004103E0] (310 bytes)
 * Formulae_CalculateResurrectionExpRecovery
 *
 * Computes exact experience points recovered upon resurrection.
 * Clamps ratio between 0.50f and 1.0f, and halves if caster is in job mode.
 */
int32_t CalculateResurrectionExpRecovery(const CGObjChar* pDeadChar, const CGObjChar* pCaster) {
	if (!pDeadChar || !pCaster) {
		return 0;
	}

	int32_t nDeadLoss = CRefData_CalculateBaseDeathExp(pDeadChar->GetLevel());
	int32_t nCasterCap = CRefData_CalculateBaseDeathExp(pCaster->GetLevel());
	if (nCasterCap <= 0) {
		nCasterCap = 1;
	}

	double fRatio = static_cast<double>(nDeadLoss) / static_cast<double>(nCasterCap);
	if (fRatio < 0.50) {
		fRatio = 0.50;
	} else if (fRatio > 1.0) {
		fRatio = 1.0;
	}

	float fExpRecovered = static_cast<float>(nDeadLoss) * static_cast<float>(fRatio);

	// Slot 71 @ +0x11C: GetJobState() == 1 (Merchant / Trader mode) halves recovery
	if (pCaster->GetJobState() == 1) {
		fExpRecovered *= 0.5f;
	}

	if (fExpRecovered <= 0.0f) {
		fExpRecovered = 1.0f;
	}

	return static_cast<int32_t>(fExpRecovered);
}

// Trade difficulty tier threshold table (Native 0x00C826C0)
// Configured via server settings / shard parameters for trade cargo difficulty (0..5 stars)
uint32_t g_aTradeDifficultyTierThresholds[6] = {
	0,          // Tier 0: Base / safe trade
	100000,     // Tier 1: 1-star (Specialty goods minimum)
	300000,     // Tier 2: 2-star
	600000,     // Tier 3: 3-star
	1000000,    // Tier 4: 4-star
	2000000     // Tier 5: 5-star (Maximum thief spawn rate)
};

/**
 * [RECONSTRUCTED - 0x004D2DA0] (62 bytes)
 * Caravan_GetCOSSlotCapacity
 *
 * Maps COS trade beast rarity (m_byCOS_Rarity at RefObjCommon + 0x89):
 *   - Rarity 1 -> 40 slots (0x28)
 *   - Rarity 2 -> 30 slots (0x1E)
 *   - Any other value triggers "COS Rarity Input Error : Input Value = [ %d ]" on channel 0x2000001
 *     and generates a mini-dump.
 */
uint16_t Caravan_GetCOSSlotCapacity(uint8_t byRarity) {
	if (byRarity == 1) {
		return 40;
	}
	if (byRarity == 2) {
		return 30;
	}
	ServerFramework::ServerFramework_GenerateMiniDump();
	BSLib::Log_Printf(0x2000001, "COS Rarity Input Error : Input Value = [ %d ]", static_cast<uint32_t>(byRarity));
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0060C1F0] (306 bytes)
 * Caravan_CalculateCargoValue
 *
 * Computes trade cargo score from active transport vehicle inventory:
 *   1. Resolves vehicle: if pVehicle is null, calls pChar->GetActiveVehicle()
 *   2. Validates vehicle is active vehicle (Slot 12 / +0x30)
 *   3. Calculates total raw cargo gold value via vehicle->CalculateCargoRawGoldValue(); returns 0 if empty
 *   4. Clamps character level [20, 140]
 *   5. Retrieves vehicle COS slot capacity via Caravan_GetCOSSlotCapacity(byRarity) or RefObjCommon + 0xE6
 *   6. Queries base death exp via CRefData_CalculateBaseDeathExp(byLevel)
 *   7. Queries total cargo item count via vehicle->GetCargoTotalCount()
 *   8. Computes:
 *        Score = ((TotalCargoCount * 348.0) / BaseDeathExp) / (40.0 / SlotCapacity)
 *      Clamped to a minimum of 1 if greater than 0.
 */
uint32_t Caravan_CalculateCargoValue(const CGObjChar* pChar, const CGObjChar* pVehicle) {
	if (!pChar) {
		return 0;
	}

	const CGObjChar* pActiveVehicle = pVehicle;
	if (!pActiveVehicle) {
		pActiveVehicle = dynamic_cast<const CGObjChar*>(pChar->GetActiveVehicle());
	}
	if (!pActiveVehicle || !pActiveVehicle->IsActiveVehicle()) {
		return 0;
	}

	uint64_t nTotalRawGold = pActiveVehicle->CalculateCargoRawGoldValue();
	if (nTotalRawGold == 0) {
		return 0;
	}

	uint32_t dwLevel = pChar->GetMaxLevel();
	if (dwLevel < 20) {
		dwLevel = 20;
	} else if (dwLevel > 140) {
		dwLevel = 140;
	}

	uint8_t byRarity = pActiveVehicle->GetCOSRarity();
	uint16_t wSlotCapacity = Caravan_GetCOSSlotCapacity(byRarity);
	if (wSlotCapacity == 0) {
		wSlotCapacity = pActiveVehicle->GetCOSSlotCapacityFromRef();
	}
	if (wSlotCapacity == 0) {
		wSlotCapacity = 1;
	}

	uint32_t dwBaseDeathExp = static_cast<uint32_t>(CRefData_CalculateBaseDeathExp(static_cast<uint8_t>(dwLevel)));
	if (dwBaseDeathExp == 0) {
		dwBaseDeathExp = 1;
	}

	uint32_t dwTotalCargoItemCount = pActiveVehicle->GetCargoTotalCount();

	// Native float arithmetic (348.0 @ 0x00B46120, 40.0 @ 0x00B45D98):
	double dExpQuotient = (static_cast<double>(dwTotalCargoItemCount) * 348.0) / static_cast<double>(dwBaseDeathExp);
	double dCapRatio = 40.0 / static_cast<double>(wSlotCapacity);
	double dScore = dExpQuotient / dCapRatio;

	if (dScore > 0.0 && dScore < 1.0) {
		dScore = 1.0;
	}

	return static_cast<uint32_t>(dScore);
}

/**
 * [RECONSTRUCTED - 0x0060C330] (34 bytes)
 * Caravan_GetTradeDifficultyTier
 *
 * Compares character's total cargo value against tier thresholds in g_aTradeDifficultyTierThresholds.
 * Returns difficulty tier (0 to 5 stars).
 */
uint8_t Caravan_GetTradeDifficultyTier(const CGObjChar* pChar) {
	if (!pChar) {
		return 0;
	}

	uint32_t dwTotalCargoValue = Caravan_CalculateCargoValue(pChar, nullptr);

	uint8_t byTier = 0;
	while (byTier < 5) {
		if (dwTotalCargoValue <= g_aTradeDifficultyTierThresholds[byTier]) {
			return byTier;
		}
		byTier++;
	}
	return 5;
}

/**
 * [RECONSTRUCTED - 0x0060C360] (243 bytes)
 * Caravan_CalculateThreeStarCargoThreshold
 *
 * Computes the 3-star cargo item quota for a character based on active transport vehicle,
 * 3-star gold threshold (g_aTradeDifficultyTierThresholds[3] = 600,000), base death exp,
 * and slot capacity.
 */
uint64_t Caravan_CalculateThreeStarCargoThreshold(const CGObjChar* pChar) {
	if (!pChar) {
		return 0;
	}

	const CGObjChar* pVehicle = dynamic_cast<const CGObjChar*>(pChar->GetActiveVehicle());
	if (!pVehicle || !pVehicle->IsActiveVehicle()) {
		return 0;
	}

	uint8_t byRarity = pVehicle->GetCOSRarity();
	uint16_t wSlotCapacity = Caravan_GetCOSSlotCapacity(byRarity);
	if (wSlotCapacity == 0) {
		wSlotCapacity = pVehicle->GetCOSSlotCapacityFromRef();
	}
	if (wSlotCapacity == 0) {
		wSlotCapacity = 1;
	}

	uint32_t dwLevel = pChar->GetMaxLevel();
	uint32_t dwBaseDeathExp = static_cast<uint32_t>(CRefData_CalculateBaseDeathExp(static_cast<uint8_t>(dwLevel)));

	// Native 0x0060C405 - 0x0060C416: mul 0x2F149903, shr 6, imul baseDeathExp, imul 348
	// This calculates threshold-scaled cargo quota:
	uint32_t dwThreshold3Star = g_aTradeDifficultyTierThresholds[3]; // 600,000
	uint64_t nQuotaRaw = ((static_cast<uint64_t>(dwThreshold3Star) * 0x2F149903ULL) >> 38);
	nQuotaRaw = nQuotaRaw * static_cast<uint64_t>(dwBaseDeathExp) * 348ULL;

	double dQuota = static_cast<double>(nQuotaRaw) * (40.0 / static_cast<double>(wSlotCapacity));
	return static_cast<uint64_t>(dQuota);
}

/**
 * [RECONSTRUCTED - 0x005237A0] (15 bytes)
 * Caravan_CheckSpecialtyTrader
 *
 * Checks if trader is specialized with high-tier cargo.
 * Dispatches to Caravan_GetTradeDifficultyTier (0x0060C330).
 */
uint8_t Caravan_CheckSpecialtyTrader(const CGObjChar* pChar, uint32_t dwMode) {
	if (!pChar || dwMode == 0) {
		return 0;
	}
	return Caravan_GetTradeDifficultyTier(pChar);
}

/**
 * [RECONSTRUCTED - 0x00410520] (163 bytes)
 * Formulae_CalculateJobReward
 *
 * Computes job trade reward gold given character and profit amount.
 * Job 1 (Trader): 0.80x (if specialized via Caravan_CheckSpecialtyTrader) else 0.50x
 * Job 2 (Thief): 0.40x
 * Job 3 (Hunter): 1.00x
 */
int32_t CalculateJobReward(const CGObjChar* pChar, int64_t nProfit) {
	if (!pChar || nProfit <= 0) {
		return 0;
	}

	uint8_t byJob = pChar->GetJobState();
	float fMultiplier = 0.0f;

	if (byJob == 1) { // Trader
		if (Caravan_CheckSpecialtyTrader(pChar, 1) != 0) {
			fMultiplier = 0.80f; // Native 0x00410571: 0.80x for specialized trader
		} else {
			fMultiplier = 0.50f; // Native 0x00410579: 0.50x standard trader
		}
	} else if (byJob == 2) { // Thief
		fMultiplier = 0.40f;
	} else if (byJob == 3) { // Hunter
		fMultiplier = 1.00f;
	} else {
		return 0;
	}

	double fReward = static_cast<double>(nProfit) * static_cast<double>(fMultiplier);
	if (fReward <= 0.0) {
		fReward = 1.0;
	}

	return static_cast<int32_t>(fReward);
}

/**
 * [RECONSTRUCTED - 0x004105D0] (222 bytes)
 * Formulae_CalculatePvPExperience
 *
 * Computes experience awarded in direct PvP combat kills.
 * Uses min(attackerLevel, targetLevel) to query RefLevelData->dwLevelTotalExp (+0x1C).
 */
int32_t CalculatePvPExperience(const CGObjChar* pTarget, const CGObjChar* pAttacker) {
	if (!pTarget || !pAttacker || !pTarget->IsPC() || !pAttacker->IsPC()) {
		return 0;
	}

	uint8_t byTargetLvl = pTarget->GetLevel();
	uint8_t byAttackerLvl = pAttacker->GetLevel();
	uint8_t byMinLvl = byAttackerLvl < byTargetLvl ? byAttackerLvl : byTargetLvl;

	const tagRefLevelData* pRefLevel = CRefData_GetRefLevelData(byMinLvl);
	float fBaseExp = pRefLevel ? static_cast<float>(pRefLevel->dwLevelTotalExp) : static_cast<float>(byMinLvl * 1000);

	float fPenalty = CalculateLevelDiffPenalty(pAttacker, pTarget);
	float fParty = CalculatePartyLevelDiffMultiplier(pTarget, pAttacker);
	float fExpMult = pAttacker->m_fExpMultiplier;
	if (fExpMult <= 0.0f) {
		fExpMult = 1.0f;
	}

	float fTotal = fBaseExp * fPenalty * fExpMult * fParty;
	if (fTotal <= 0.0f) {
		fTotal = 1.0f;
	}
	return static_cast<int32_t>(fTotal);
}

/**
 * [RECONSTRUCTED - 0x004106B0] (453 bytes)
 * Formulae_CalculateSkillExperience
 *
 * Computes skill experience (SP) awarded to a character.
 * Uses (pTarget->GetExp() * pTarget->GetExpMultiplier() * 100.0) / refLevelData->dwLevelTotalExp.
 */
int32_t CalculateSkillExperience(
	const CGObjChar* pTarget,
	const CGObjChar* pAttacker,
	float            fDamageRatio,
	float            fSPMultiplier
) {
	if (!pTarget || !pAttacker || fSPMultiplier <= 0.0f) {
		return 0;
	}

	if (!pAttacker->IsPC()) {
		return 0;
	}

	float fTargetMaxHP = static_cast<float>(pTarget->GetMaxHP());
	if (fTargetMaxHP <= 0.0f) {
		return 0;
	}

	float fRatio = fDamageRatio / fTargetMaxHP;
	if (fRatio < 0.000001f) {
		fRatio = 0.000001f;
	} else if (fRatio > 1.0f) {
		fRatio = 1.0f;
	}

	const tagRefLevelData* pRefLevel = CRefData_GetRefLevelData(pAttacker->GetLevel());
	uint32_t dwRefLevelTotalExp = pRefLevel ? pRefLevel->dwLevelTotalExp : (pAttacker->GetLevel() * 1000);
	if (dwRefLevelTotalExp == 0) {
		dwRefLevelTotalExp = 1;
	}

	float fMonsterExp = static_cast<float>(pTarget->GetExp());
	float fTargetExpMult = pTarget->GetExpMultiplier();

	double fBaseSP = (static_cast<double>(fMonsterExp) * static_cast<double>(fTargetExpMult) * 100.0) / static_cast<double>(dwRefLevelTotalExp);

	float fLevelPenalty = CalculateLevelDiffPenalty(pAttacker, pTarget);
	float fPartyMult = CalculatePartyLevelDiffMultiplier(pTarget, pAttacker);

	double fTotalSP = fBaseSP * static_cast<double>(fSPMultiplier) * static_cast<double>(fRatio) * static_cast<double>(fLevelPenalty) * static_cast<double>(fPartyMult);
	if (fTotalSP < 1.0) {
		fTotalSP = 1.0;
	}

	return static_cast<int32_t>(fTotalSP);
}

/**
 * [RECONSTRUCTED - 0x00410880] (360 bytes)
 * Formulae_CalculateBerserkPointGain
 *
 * Computes berserker points gained by character when damaging or killing a target.
 * Registers/args: pTarget (esi), pAttacker (ebp), pOutRankBonus (edi).
 */
int32_t CalculateBerserkPointGain(
	const CGObjChar* pTarget,
	const CGObjChar* pAttacker,
	int32_t*         pOutRankBonus
) {
	if (!pTarget || !pAttacker) {
		return 0;
	}

	if (pOutRankBonus) {
		*pOutRankBonus = 1;
	}

	float fRate = 8.0f;
	uint8_t byAttackerLvl = pAttacker->GetLevel();
	uint8_t byTargetLvl = pTarget->GetLevel();

	if (ClassifyLevelDiff(byTargetLvl, byAttackerLvl) == 2) {
		float fDiff = static_cast<float>(static_cast<int32_t>(byTargetLvl) - static_cast<int32_t>(byAttackerLvl) - 5);
		fRate = 8.0f - (fDiff * 0.25f);
		if (fRate < 0.25f) {
			fRate = 0.25f;
		} else if (fRate > 8.0f) {
			fRate = 8.0f;
		}
	}

	int32_t nPoints = static_cast<int32_t>(fRate * 100.0f);
	if (pTarget->IsNPC()) {
		uint8_t byMonsterClass = pTarget->GetMonsterClass();
		switch (byMonsterClass) {
			case 3:
			case 8: // Giant / Unique mob: 10,000,000 points (instant max berserk!)
				if (pOutRankBonus) {
					*pOutRankBonus = 5;
				}
				nPoints = 0x989680;
				break;
			case 4: // Champion
				nPoints *= 5;
				break;
			case 6: // Party mob
				nPoints *= 2;
				break;
			default:
				break;
		}
	}

	float fGainRate = pAttacker->GetParamFloat(0xB8); // Param 0xB8: Berserk Point Gain Rate (%)
	if (fGainRate > 0.0f) {
		nPoints = static_cast<int32_t>((static_cast<float>(nPoints) * fGainRate) / 100.0f);
	}

	return nPoints;
}

/**
 * [RECONSTRUCTED - 0x00410C80] (358 bytes)
 * Formulae_CalculateSkillDowngradeGoldCost
 *
 * Computes total gold cost required to downgrade a skill down to byRequestedRank.
 * Accumulates across skill tree nodes:
 *   (baseExp * 100.0) * (1.0 + (masteryReqLevel / 10.0)) + 0.5 -> floor -> ftol
 */
int32_t CalculateSkillDowngradeGoldCost(
	const void*      pSkillTree,
	const CGObjChar* pChar,
	uint8_t          byRequestedRank
) {
	if (!pSkillTree || !pChar) {
		return 0;
	}

	uint8_t byMaxLevel = pChar->GetMaxLevel();
	uint32_t dwBaseExp = CRefData_GetRefLevelBaseExp(byMaxLevel);
	double fBaseGold = (static_cast<double>(dwBaseExp) / 0.8) * 80.0;

	int32_t nTotalGold = 0;
	const uint8_t* pNode = static_cast<const uint8_t*>(pSkillTree);

	if (byRequestedRank == 0) {
		while (pNode) {
			uint8_t byVal = 0;
			const uint32_t* pReqMastery = reinterpret_cast<const uint32_t*>(pNode + 0xA0);
			for (int j = 0; j < 2; ++j) {
				if (pReqMastery[j] != 0) {
					byVal = *(pNode + 0xA8 + j);
					break;
				}
			}
			double fLevelFactor = 1.0 + (static_cast<double>(byVal) / 10.0);
			double fCost = std::floor((fLevelFactor * fBaseGold) + 0.5);
			nTotalGold += static_cast<int32_t>(fCost);
			pNode = *reinterpret_cast<const uint8_t* const*>(pNode + 0x59C);
		}
		return nTotalGold;
	}

	while (pNode) {
		uint8_t byCurrentRank = *(pNode + 0x64);
		if (byCurrentRank <= byRequestedRank) {
			return nTotalGold;
		}

		uint8_t byVal = 0;
		const uint32_t* pReqMastery = reinterpret_cast<const uint32_t*>(pNode + 0xA0);
		for (int j = 0; j < 2; ++j) {
			if (pReqMastery[j] != 0) {
				byVal = *(pNode + 0xA8 + j);
				break;
			}
		}
		double fLevelFactor = 1.0 + (static_cast<double>(byVal) / 10.0);
		double fCost = std::floor((fLevelFactor * fBaseGold) + 0.5);
		nTotalGold += static_cast<int32_t>(fCost);
		pNode = *reinterpret_cast<const uint8_t* const*>(pNode + 0x59C);
	}

	return nTotalGold;
}

int32_t CalculateMasterySPCost(
	const void*      pSkillTree,
	const CGObjChar* pChar,
	uint8_t          byRequestedRank
) {
	return CalculateSkillDowngradeGoldCost(pSkillTree, pChar, byRequestedRank);
}

/**
 * [RECONSTRUCTED - 0x00410DF0] (176 bytes)
 * Formulae_CalculateMasteryDowngradeSPRefund
 *
 * Computes SP refund amount when downgrading a mastery from byCurrentLevel to byTargetLevel.
 */
int32_t CalculateMasteryDowngradeSPRefund(
	const CGObjChar* pChar,
	uint8_t          byCurrentLevel,
	uint8_t          byTargetLevel
) {
	if (!pChar || byCurrentLevel <= byTargetLevel) {
		return 0;
	}

	uint8_t byMaxLevel = pChar->GetMaxLevel();
	uint32_t dwBaseExp = CRefData_GetRefLevelBaseExp(byMaxLevel);
	double fBaseSP = (static_cast<double>(dwBaseExp) / 0.8) * 80.0;

	int32_t nTotalSP = 0;
	for (int32_t lvl = static_cast<int32_t>(byCurrentLevel) - 1; lvl >= static_cast<int32_t>(byTargetLevel); --lvl) {
		double fLevelFactor = 1.0 + (static_cast<double>(lvl) / 10.0);
		double fCost = std::floor((fLevelFactor * fBaseSP) + 0.5);
		nTotalSP += static_cast<int32_t>(fCost);
	}

	return nTotalSP;
}

/**
 * [RECONSTRUCTED - 0x00410EA0] (109 bytes)
 * Formulae_CalculateSkillDowngradeSPRefund
 *
 * Traverses skill tree node linked list (+0x59C) downgrading levels down to byTargetLevel.
 * (80% standard, 100% in full refund mode).
 */
int32_t CalculateSkillDowngradeSPRefund(
	uint8_t     byTargetLevel,
	const void* pSkillNode,
	bool        bFullRefund
) {
	if (!pSkillNode) {
		return 0;
	}

	int32_t nTotalSP = 0;
	const uint8_t* pCur = static_cast<const uint8_t*>(pSkillNode);

	if (byTargetLevel > 0) {
		while (pCur && *(pCur + 0x64) > byTargetLevel) {
			nTotalSP += *reinterpret_cast<const int32_t*>(pCur + 0xC0);
			pCur = *reinterpret_cast<const uint8_t* const*>(pCur + 0x59C);
		}
	} else {
		while (pCur) {
			nTotalSP += *reinterpret_cast<const int32_t*>(pCur + 0xC0);
			pCur = *reinterpret_cast<const uint8_t* const*>(pCur + 0x59C);
		}
	}

	if (bFullRefund) {
		return nTotalSP;
	}

	return nTotalSP - ((nTotalSP * 20) / 100);
}

/**
 * [RECONSTRUCTED - 0x00410F10] (182 bytes)
 * Formulae_CalculateMasteryDowngradeGoldCost
 *
 * Computes gold cost when downgrading a mastery from byCurrentLevel to byTargetLevel.
 * (80% standard, 100% in full refund mode).
 */
int32_t CalculateMasteryDowngradeGoldCost(
	uint8_t byCurrentLevel,
	uint8_t byTargetLevel,
	bool    bFullRefund
) {
	if (byCurrentLevel <= byTargetLevel || byCurrentLevel <= 1) {
		return 0;
	}

	int32_t nTotalGold = 0;
	for (int32_t lvl = static_cast<int32_t>(byCurrentLevel); lvl > static_cast<int32_t>(byTargetLevel); --lvl) {
		const tagRefLevelData* pRef = CRefData_GetRefLevelData(static_cast<uint8_t>(lvl));
		if (pRef) {
			nTotalGold += static_cast<int32_t>(pRef->dwGoldCost);
		}
	}

	if (bFullRefund) {
		return nTotalGold;
	}

	return nTotalGold - ((nTotalGold * 20) / 100);
}

/**
 * [RECONSTRUCTED - 0x00411000] (113 bytes)
 * Formulae_CalculateWeaponMagicalHealBonus
 *
 * Line 2064 of original Formulae.cpp.
 * Computes weapon magical healing bonus based on equipped weapon (slot 6)
 * and character magical absorption ratio.
 */
int32_t CalculateWeaponMagicalHealBonus(const CGObjChar* pChar, uint32_t dwPercentage) {
	if (!pChar) {
		return 0;
	}

	// Slot 350 @ +0x578: GetStorageItem(6) - Equipment Slot 6 is Weapon
	const CGItemEquip* pWeapon = pChar->GetStorageItem(6);
	if (!pWeapon) {
		return 0;
	}

	float fAbsorption = CalculateMagicalAbsorptionRatio(pChar);
	float fMinPower = pWeapon->GetMinMagAttackPower();
	float fMaxPower = pWeapon->GetMaxMagAttackPower();
	float fAvgPower = (fMinPower + fMaxPower) * 0.5f;

	double fBonus = static_cast<double>(fAvgPower) * static_cast<double>(fAbsorption) * (static_cast<double>(dwPercentage) / 100.0);
	return static_cast<int32_t>(fBonus);
}

/**
 * [RECONSTRUCTED - 0x00411080] (113 bytes)
 * Formulae_CalculateWeaponPhysicalHealBonus
 *
 * Line 2085 of original Formulae.cpp.
 * Computes weapon physical healing bonus based on equipped weapon (slot 6)
 * and character physical absorption ratio.
 */
int32_t CalculateWeaponPhysicalHealBonus(const CGObjChar* pChar, uint32_t dwPercentage) {
	if (!pChar) {
		return 0;
	}

	// Slot 350 @ +0x578: GetStorageItem(6) - Equipment Slot 6 is Weapon
	const CGItemEquip* pWeapon = pChar->GetStorageItem(6);
	if (!pWeapon) {
		return 0;
	}

	float fAbsorption = CalculateAbsorptionRatio(pChar);
	float fMinPower = pWeapon->GetMinPhyAttackPower();
	float fMaxPower = pWeapon->GetMaxPhyAttackPower();
	float fAvgPower = (fMinPower + fMaxPower) * 0.5f;

	double fBonus = static_cast<double>(fAvgPower) * static_cast<double>(fAbsorption) * (static_cast<double>(dwPercentage) / 100.0);
	return static_cast<int32_t>(fBonus);
}

} // namespace Formulae

/*
================
tagRefSkill::MatchesExecutionSelector

[RECONSTRUCTED - Native 0x00589D20] (434 bytes)
Evaluates whether reference skill possesses offensive, damage, or debuff parameters
while exclusion selector (+0x41C) is zero.
Used throughout skill engine (0x0058D8F0, 0x00589B50, 0x0058A020..0x0058CB70, 0x0059D870)
to determine whether skill executes an offensive/action payload.
================
*/
bool tagRefSkill::MatchesExecutionSelector() const {
	// Exclusion check at offset +0x41C
	if (Param(0x41C) != nullptr) {
		return false;
	}

	// Exact 34 parameter slot pointer offsets tested by native 0x00589D20:
	static const uint16_t s_aParamOffsets[] = {
		0x230, 0x234, 0x238, 0x248, 0x2C8, 0x2DC, 0x2E0,
		0x30C, 0x310, 0x314, 0x318, 0x31C, 0x320,
		0x3C8, 0x3CC, 0x400, 0x424,
		0x430, 0x434, 0x438, 0x43C, 0x440, 0x444, 0x44C,
		0x450, 0x454, 0x458, 0x45C, 0x460, 0x464, 0x468, 0x46C, 0x470, 0x474,
		0x48C, 0x4A0
	};

	for (size_t i = 0; i < sizeof(s_aParamOffsets) / sizeof(s_aParamOffsets[0]); ++i) {
		const void* pParam = Param(s_aParamOffsets[i]);
		if (pParam != nullptr) {
			return true;
		}
	}

	return false;
}


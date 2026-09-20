/**
 * ============================================================================
 * Silkroad Online - Combat and Damage Calculation Formulae
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\Formulae.h
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

#ifndef _SR_GAMESERVER_FORMULAE_H_
#define _SR_GAMESERVER_FORMULAE_H_

#include <cstdint>
#include <vector>
#include "GObjChar.h"
#include "SkillManager.h"
#include "ReferenceData.h"

// [RECONSTRUCTED - Native 0x00ADF4A4 / 0x0040DA70]
class CFormulae {
public:
	virtual ~CFormulae() = default; // Slot 0 @ +0x00 (0x0040DB00 / 0x0040DAE0)
};

extern CFormulae* g_pFormulae; // @ 0x00D6A8E4

// [RECONSTRUCTED - Native 0x00411130] (57 bytes)
// CRefData_GetRefLevelData: indexes m_vecRefLevelData[byLevel - 1]
const tagRefLevelData* CRefData_GetRefLevelData(uint8_t byLevel);

// [RECONSTRUCTED - Native 0x007243E0] (28 bytes)
// CRefData_GetRefLevelBaseExp: looks up base progression exp for level in g_pRefData + 0x268
uint32_t CRefData_GetRefLevelBaseExp(uint8_t byLevel);

// [RECONSTRUCTED - Native 0x004111F0] (42 bytes)
// CRefData_CalculateBaseDeathExp: ftol((baseExp * 10) * 0.125)
int32_t CRefData_CalculateBaseDeathExp(uint8_t byLevel);

// Skill template descriptor (_RefSkill)
// Pointed to by CGSkill::m_pRefSkillData at offset +0x230
struct tagRefSkill {
	uint8_t   byType;                  // +0x00: Skill Type flags (0x01: PhysAtkBuff, 0x02: WpnPhysBuff, 0x04: MagAtk, 0x08: PhysAtk)
	uint8_t   pad01[3];                // +0x01 - +0x03
	union {
		uint32_t  dwSkillID;               // +0x04: Unique Skill ID (e.g. SKILL_CH_SWORD_..., SKILL_EU_DAGGER_...)
		uint32_t  dwSkillPowerRatio;       // +0x04: Skill power percentage (e.g. 100, 150, 250)
	};
	union {
		uint32_t  dwGroupID;               // +0x08: Skill Group / Series ID (proven @ 0x0059D812)
		uint32_t  dwMinAttackPower;        // +0x08: Min physical attack power bonus
	};
	uint32_t    dwMaxAttackPower;        // +0x0C: Max physical attack power bonus
	std::string m_Basic_Code;            // +0x10: Basic Skill CodeName (e.g. "SKILL_CH_SWORD_01", proven @ 0x006F6A36)
	uint8_t     pad2C[0x38];             // +0x2C - +0x63
	uint8_t     byRank;                  // +0x64: Skill Rank / Level (proven @ 0x0059D817)
	uint8_t     byCastType;              // +0x65: Cast Type (2 = targeted skill)
	uint8_t     pad66[2];                // +0x66 - +0x67
	uint32_t    dwCastingDuration;       // +0x68: Casting duration / NextSkillID
	uint32_t    pad6C;                   // +0x6C
	union {
		uint32_t dwTargetCount;          // +0x70: Target count / max targets (proven @ 0x005836AB)
		uint32_t pad70;                  // +0x70
	};
	union {
		uint32_t dwActionDuration;       // +0x74: Action animation duration
		uint32_t dwCastDelay;            // +0x74: Cast delay in ms (proven @ 0x0058359E)
	};
	uint32_t    dwCastTime;              // +0x78: Skill casting time in ms
	uint32_t    dwCooldown;              // +0x7C: Cooldown duration in ms (proven @ 0x0064C770, 0x0064C1D0)
	uint32_t    dwCooldownGlobal;        // +0x80: Global cooldown duration in ms (proven @ 0x0064C82D)
	uint8_t     pad84[8];                // +0x84 - +0x8B
	union {
		struct {
			uint8_t   byPackedState[3];      // +0x8C - +0x8E: 3-byte state mask
			uint8_t   byCooldownGroup;       // +0x8F: Cooldown Group ID (proven @ 0x0064C877, 0x0064C1E5)
		};
		uint32_t      dwPackedStates;        // +0x8C: Packed 3-byte state mask + group ID
	};
	uint8_t     byAutoAttackChain;       // +0x90: Auto attack chain flag (1 = chain)
	uint8_t     pad91;                   // +0x91
	uint16_t    wTargetRange;            // +0x92: Target range in meters
	// CORRECTION (Claude): +0x94 - +0x9F are the _RefSkill target columns in table order. +0x94 is a flag,
	// not a type enum: the command actor clears the targets when it is 0 and skips the approach for
	// required ground targets (+0x94 == 1 && +0x96 == 1, 0x004AE749); CheckSkillPreEngageCondition only
	// validates targets when it is 1 (0x0058E12F).
	union {
		uint8_t byTargetRequired;        // +0x94: Target_Required
		uint8_t byTargetDistribution;    // +0x94: alias (0x00583609)
		uint32_t dwTargetDistribution;   // +0x94 - +0x97: any of the four target flags
	};
	uint8_t     byTargetTypeAnimal;      // +0x95: TargetType_Animal
	uint8_t     byTargetTypeLand;        // +0x96: TargetType_Land
	uint8_t     byTargetTypeBuilding;    // +0x97: TargetType_Building
	uint8_t     byTargetGroupSelf;       // +0x98: TargetGroup_Self
	uint8_t     byTargetGroupAlly;       // +0x99: TargetGroup_Ally
	uint8_t     byTargetGroupParty;      // +0x9A: TargetGroup_Party
	uint8_t     byTargetGroupEnemyMonster; // +0x9B: TargetGroup_Enemy_M
	uint8_t     byTargetGroupEnemyPlayer;  // +0x9C: TargetGroup_Enemy_P
	uint8_t     byTargetGroupNeutral;    // +0x9D: TargetGroup_Neutral
	uint8_t     byTargetGroupDontCare;   // +0x9E: TargetGroup_DontCare
	uint8_t     byTargetSelectDeadBody;  // +0x9F: TargetEtc_SelectDeadBody
	uint32_t    dwReqMasteryID[2];       // +0xA0 - +0xA7: Required mastery IDs (proven @ 0x0059E488)
	uint8_t     byReqMasteryLevel[2];    // +0xA8 - +0xA9: Required mastery levels (proven @ 0x0059E4AC)
	uint8_t     byReqSTR;                // +0xAA: Required STR stat (proven @ 0x0059E4C5)
	uint8_t     padAB;                   // +0xAB
	uint8_t     byReqINT;                // +0xAC: Required INT stat (proven @ 0x0059E4E9)
	uint8_t     padAD[3];                // +0xAD - +0xAF
	uint32_t    dwReqSkillGroupID[3];    // +0xB0 - +0xBB: Required prerequisite skill group IDs (proven @ 0x0059E554)
	uint8_t     byReqSkillRank[3];       // +0xBC - +0xBE: Required prerequisite skill ranks (proven @ 0x0059E57A)
	uint8_t     padBF;                   // +0xBF
	uint32_t    dwSkillPointCost;        // +0xC0 - +0xC3: Required Skill Points (SP) (proven @ 0x0059E592)
	uint8_t     byCountry;               // +0xC4: Country requirement (0: China, 1: Europe, 3: Universal) (proven @ 0x0059E506)
	uint8_t     padC5[2];                // +0xC5 - +0xC6
	uint8_t     byReqWeaponKind[2];      // +0xC7 - +0xC8: Required weapon kind 1 & 2 (0xFF = none, verified @ 0x0058D480)
	union {
		uint8_t padC9[0x0C];             // +0xC9 - +0xD4
		struct {
			uint8_t  padC9_0;            // +0xC9
			uint16_t wRequiredHP;        // +0xCA: Static required HP (proven @ 0x00586720)
			uint16_t wRequiredMP;        // +0xCC: Static required MP (proven @ 0x00586727)
			uint16_t wConsumeHPRatio;    // +0xCE: Percentage HP consumption ratio (proven @ 0x0058673B)
			uint16_t wConsumeMPRatio;    // +0xD0: Percentage MP consumption ratio (proven @ 0x00586756)
			uint8_t  byBerserkPointCost; // +0xD2: 5867FC -> context byte +18 -> 5935BE
			uint8_t  padD3;              // +0xD3
			uint8_t  padD4;              // +0xD4
		};
	};
	uint8_t     byVisibilityD5;          // +0xD5: Visibility / UI flag (0xFF = hidden, proven @ 0x0059D4F7)
	uint8_t     padD6[0x92];             // +0xD6 - +0x167
	union {
		uint32_t  dwActionCategory;        // +0x168: Action Category (0: Instant, 1: Projectile, 3: Persistent, 4: Continuous) (proven @ 0x00589B84)
		uint32_t  m_dwActionCategory;
	};
	union {
		uint8_t   pad16C[0xC4];            // +0xC4: 196 bytes (+0x16C - +0x22F)
		uint32_t  m_dwParamWords[49];      // +0x16C - +0x22F: 49 raw skill parameter words (verified @ 0x00587630)
	};
	// CORRECTION (Claude): native +0x230 - +0x5AB is one table of 223 parameter pointers. SkillGlobal_
	// BuildParameterIndex (0x00587641) clears it and points the slots into m_dwParamWords; +0x59C / +0x5A0
	// hold the rank links. The members that were declared here as 32-bit values (dwBuffType, dwNbuf,
	// dwReqMastery, ...) are pointers the machine code only tests for null or dereferences, and on x64
	// the native offsets are not struct offsets. The table is therefore an array indexed by
	// (native offset - 0x230) / 4; Param / SetParam take the native offset, and the names alias the slots.
	union {
		const uint32_t* m_apParam[223];
		struct {
			const uint32_t* pCastFlag;                           // +0x230: was dwCastFlag
			const uint32_t* m_apParam234[7];                     // +0x234 - +0x24F
			const uint32_t* pRangeModifier;                      // +0x250: Range bonus
			const uint32_t* m_apParam254[8];                     // +0x254 - +0x273
			const uint32_t* pParam274;                           // +0x274: while set on the running instance's skill, 0x7070 is ignored (0x0059B810)
			const uint32_t* m_apParam278[3];                     // +0x278 - +0x283
			const uint32_t* pBuffType;                           // +0x284: was dwBuffType; set for toggles / buffs
			const uint32_t* m_apParam288[4];                     // +0x288 - +0x297
			const uint32_t* pCastDuration;                       // +0x298: was dwCastDuration
			const uint32_t* m_apParam29C[6];                     // +0x29C - +0x2B3
			const uint32_t* pParam2B4;                           // +0x2B4: fills CSkillManager +0x1DC
			const uint32_t* m_apParam2B8[5];                     // +0x2B8 - +0x2CB
			const uint32_t* pParam2CC;                           // +0x2CC: fills CSkillManager +0x1F8
			const uint32_t* m_apParam2D0[7];                     // +0x2D0 - +0x2EB
			const uint32_t* pParam2EC;                           // +0x2EC: was dwReqMP
			const uint32_t* pParam2F0;                           // +0x2F0: was dwReqHP
			const uint32_t* pParam2F4;                           // +0x2F4
			const uint32_t* m_apParam2F8[14];                    // +0x2F8 - +0x32F
			const uint32_t* pResu;                               // +0x330: Resurrect skill param {max_level, recovery_rate}
			const uint32_t* m_apParam334[9];                     // +0x334 - +0x357
			const uint32_t* pCbuf;                               // +0x358: was dwCbuf
			const uint32_t* pNbuf;                               // +0x35C: was dwNbuf
			const uint32_t* m_apParam360[4];                     // +0x360 - +0x36F
			const uint32_t* pLnks;                               // +0x370: Linked skill descriptor {opcode, count}
			const uint32_t* pLks2;                               // +0x374: Linked skill parameter 2
			const uint32_t* m_apParam378[9];                     // +0x378 - +0x39B
			const uint32_t* pReqc;                               // +0x39C: Condition requirement tuple (bit 0x04 HP <= 30 %, 0x10, 0x20)
			const uint32_t* pReqI[5];                            // +0x3A0: equipment requirement tuples {opcode, value}
			const uint32_t* pReqN;                               // +0x3B4: was dwReqN: all-required conjunction
			const uint32_t* pReqa;                               // +0x3B8: Abnormal status requirement bitmask
			const uint32_t* m_apParam3BC[27];                    // +0x3BC - +0x427
			const uint32_t* pParam428;                           // +0x428: was dwReqWeaponType1
			const uint32_t* m_apParam42C[7];                     // +0x42C - +0x447
			const uint32_t* pParam448;                           // +0x448: fills CSkillManager +0x214
			const uint32_t* m_apParam44C[3];                     // +0x44C - +0x457
			const uint32_t* pCa;                                 // +0x458: Capture parameter
			const uint32_t* m_apParam45C[16];                    // +0x45C - +0x49B
			const uint32_t* pParam49C;                           // +0x49C: was dwReqMastery: placement radius check in 0x0058DB45
			const uint32_t* pParam4A0;                           // +0x4A0: was dwReqWeaponType2
			const uint32_t* pMsch;                               // +0x4A4: Msid: 1 summon transport, 2 ride check
			const uint32_t* pMcap;                               // +0x4A8: Monster capture level parameter
			const uint32_t* m_apParam4AC[15];                    // +0x4AC - +0x4E7
			const uint32_t* pParam4E8;                           // +0x4E8: Skill modifier param key 1
			const uint32_t* m_apParam4EC[8];                     // +0x4EC - +0x50B
			const uint32_t* pParam50C;                           // +0x50C: Skill modifier param key 2
			const uint32_t* m_apParam510[33];                    // +0x510 - +0x593
			const uint32_t* pParam594;                           // +0x594: was dwCastTypeFlag: cast while asleep / sitting allowed
			const uint32_t* pParam598;                           // +0x598: was dwStanceOrBuffType
			const tagRefSkill* pPreviousRankSkill;               // +0x59C: previous rank (0x0059BFE0)
			const tagRefSkill* pNextRankSkill;                   // +0x5A0: next rank (0x0059D504)
			const uint32_t* m_apParam5A4[2];                     // +0x5A4 - +0x5AB
		};
	};

	// The attack parameter record the skill points at (+0x230), read as {type, power ratio, min, max} by the
	// damage formulae (0x0040E49D tests its type byte, 0x0040DCE0 reads the powers).
	const struct tagSkillAttackParam* AttackParam() const;

	// Parameter slot at native offset dwNativeOffset (0x230 <= offset < 0x5AC, multiple of 4).
	const uint32_t* Param(uint32_t dwNativeOffset) const {
		return m_apParam[(dwNativeOffset - 0x230) / 4];
	}
	void SetParam(uint32_t dwNativeOffset, const uint32_t* pValue) {
		m_apParam[(dwNativeOffset - 0x230) / 4] = pValue;
	}

	// Convenience compatibility aliases
	const uint32_t* m_pParamCbuf = nullptr;
	const uint32_t* m_pParamDura = nullptr;
	const uint32_t* m_pParamSumm = nullptr;
	const uint32_t* m_pParamMsch = nullptr;
	const uint32_t* m_pParamEfr3 = nullptr;
	const uint32_t* m_pParamReqn = nullptr;
	const uint32_t* m_pParamReal = nullptr;
	const uint32_t* pReal = nullptr;
	const uint32_t* m_pParamDmgp = nullptr;
	const uint32_t* m_pParamDmgr = nullptr;
	const uint32_t* pDttp = nullptr;
	const uint32_t* pOvl2 = nullptr;
	const uint32_t* pEfr2 = nullptr;
	const uint32_t* pEfr = nullptr;
	const uint32_t* pTele = nullptr;
	const uint32_t* pTel2 = nullptr;
	const uint32_t* pTel3 = nullptr;
	const uint32_t* pAtt = nullptr;
	const uint32_t* pDtnt = nullptr;
	const uint32_t* pEshp = nullptr;
	uint32_t        wProjectileSpeed = 0;
	uint16_t        bySkillCostHP = 0;
	uint16_t        bySkillCostMP = 0;
	const uint32_t* pSkc = nullptr;
	uint8_t         m_bIsBuff = 0;
	const char*     strSkillName = nullptr;

	// [RECONSTRUCTED - Native 0x00589D20]
	// Checks whether this skill reference possesses offensive/damage/debuff action execution descriptors
	bool MatchesExecutionSelector() const;

	// [RECONSTRUCTED - Native 0x00587630]
	// Builds the derived parameter pointer table from raw parameter words
	void BuildParameterIndex();

};

/**
 * The 'atk' attack parameter record a skill points at through its +0x230 slot.
 * CORRECTION (Claude): this file used to declare a separate CGSkill whose +0x230 was a tagRefSkill pointer and
 * whose +0x4D4.. were named modifier members. The machine code has one object: the skill the execution context
 * carries (tagRefSkill) whose +0x230 - +0x5AB is the parameter pointer table (0x0040E45C tests +0x230 and
 * 0x0040E4B9 reads +0x524 on the same pointer). CGSkill is that object, the parameter slots are reached with
 * Param(native offset), and this record is what +0x230 points at.
 */
struct tagSkillAttackParam {
	uint32_t dwType;            // +0x00: 0x04 magical attack, 0x08 physical attack (0x0040E49D)
	uint32_t dwPowerRatio;      // +0x04: skill power percentage
	uint32_t dwMinAttackPower;  // +0x08
	uint32_t dwMaxAttackPower;  // +0x0C
};

typedef tagRefSkill CGSkill;

inline const tagSkillAttackParam* tagRefSkill::AttackParam() const {
	return reinterpret_cast<const tagSkillAttackParam*>(Param(0x230));
}

// Global config flag for detailed formula debugging (Native 0x00C82530)
extern bool g_bShowFormulaDetail;

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
float GetMonsterAttackPowerMultiplier(const CGObjChar* pChar);

/**
 * [RECONSTRUCTED - 0x0040DC10] (198 bytes)
 * Formulae_GetPhysicalAttackPowerMinMax
 *
 * Retrieves the base physical attack power (min or max) for a character and skill.
 *   - bMax: false = Min Physical Attack Power (Param 15 / 0x0F)
 *           true  = Max Physical Attack Power (Param 16 / 0x10)
 */
float GetPhysicalAttackPowerMinMax(const CGObjChar* pChar, bool bMax, const CGSkill* pSkill);

/**
 * [RECONSTRUCTED - 0x0040DB50] (180 bytes)
 * Formulae_GetMagicalAttackPowerMinMax
 *
 * Retrieves the base magical attack power (min or max) for a character and skill.
 *   - bMax: false = Min Magical Attack Power (Param 13 / 0x0D)
 *           true  = Max Magical Attack Power (Param 14 / 0x0E)
 */
float GetMagicalAttackPowerMinMax(const CGObjChar* pChar, bool bMax, const CGSkill* pSkill);

/**
 * [RECONSTRUCTED - 0x0040FDD0] (144 bytes)
 * Formulae_CalculateHitBalance
 *
 * Calculates base combat hit balance between target and attacker:
 *   (Attacker HitRate [Param 11] / Target ParryRate [Param 9]) * 0.5 + LevelDiffBonus,
 *   scaled by 100.0 and clamped to [10.0%, 90.0%].
 */
float CalculateHitBalance(const CGObjChar* pTarget, const CGObjChar* pAttacker);

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
);

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
);

/**
 * [RECONSTRUCTED - 0x00410C20] (89 bytes)
 * Formulae_GetAttackPowerBuff
 *
 * Looks up active attack power buffs from CGParamKeeper (+0x1EC):
 *   - Magical: Param 0x88 (Type & 0x01) or Param 0x89 (Type & 0x02)
 *   - Physical: Param 0x8A (Type & 0x01) or Param 0x8B (Type & 0x02)
 */
float GetAttackPowerBuff(uint8_t byType, const CGObjChar* pChar);

/**
 * [RECONSTRUCTED - 0x00410B40] (77 bytes)
 * Formulae_GetMonsterRankMultiplier
 *
 * Returns monster experience/HP scaling multiplier based on rank (0..5):
 *   Case 0 (Normal): 97x (0x61)
 *   Case 1 (Champion): 250x (0xFA)
 *   Case 2/5 (Giant/Elite): 750x (0x2EE)
 *   Case 3 (Party): 500x (0x1F4)
 *   Case 4 (Party Champion): 1000x (0x3E8)
 */
int32_t GetMonsterRankMultiplier(uint8_t byRank, uint16_t wBaseValue);

/**
 * [RECONSTRUCTED - 0x0040FE60] (33 bytes)
 * Formulae_ClassifyLevelDiff
 *
 * Classifies level difference (attackerLevel - targetLevel):
 *   - diff < -3: 2 (Target much higher level)
 *   - diff > 10: 1 (Attacker much higher level)
 *   - otherwise: 0 (Normal)
 */
int32_t ClassifyLevelDiff(uint8_t byAttackerLevel, uint8_t byTargetLevel);

/**
 * [RECONSTRUCTED - 0x0040FED0] (263 bytes)
 * Formulae_CalculateLevelDiffPenalty
 *
 * Calculates experience/drop multiplier based on level difference:
 *   - Case 0: 1.0f
 *   - Case 1: 1.0f - (diff - 10) * 0.02f, clamped to [0.10f, 1.0f]
 *   - Case 2: 1.0f - (diff - 3) * 0.15f (if diff <= 6) or 0.10f, clamped to [0.10f, 1.0f]
 */
float CalculateLevelDiffPenalty(const CGObjChar* pAttacker, const CGObjChar* pTarget);

/**
 * [RECONSTRUCTED - 0x0040FD60] (104 bytes)
 * Formulae_CalculateLevelDiffBonus
 *
 * Calculates level difference bonus:
 *   min(0.30f, max(0.0f, (attackerLevel - targetLevel) * 0.03f))
 */
float CalculateLevelDiffBonus(const CGObjChar* pAttacker, const CGObjChar* pTarget);

/**
 * [RECONSTRUCTED - 0x00410AB0] (140 bytes)
 * Formulae_CalculateAbsorptionRatio (Physical)
 *
 * Calculates player character mastery/absorption ratio:
 *   stat2 (Strength) / (((level - 1.0) * 5.0 + 40.0) * 0.8), clamped to [0.0f, 1.20f]
 */
float CalculateAbsorptionRatio(const CGObjChar* pChar);

/**
 * [RECONSTRUCTED - 0x00410A00] (168 bytes)
 * Formulae_CalculateMagicalAbsorptionRatio
 *
 * Calculates player character magical absorption ratio:
 *   stat1 (Intelligence) / (((level - 1.0) * 5.0 + 40.0) * 0.8), clamped to [0.0f, 1.20f]
 */
float CalculateMagicalAbsorptionRatio(const CGObjChar* pChar);

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
);

float CalculateDefenseSkillAbsorption(
	const CGSkill*   pAttackerSkill,
	const CGObjChar* pTarget,
	const CGSkill*   pDefenderSkill
);

/**
 * [RECONSTRUCTED - 0x0040E830] (935 bytes)
 * Formulae_CalculatePhysicalDamage
 *
 * Calculates final physical damage dealt by an attacker to a target using a skill.
 */
int32_t CalculatePhysicalDamage(
	const CGSkill*   pSkill,
	uint8_t          byDamageFlags,
	const CGObjChar* pTarget,
	const CGObjChar* pAttacker,
	uint8_t          byHitRatioBonus,
	const CGSkill*   pDefenseSkill = nullptr
);

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
 *       * 0x08: Skill modifier power flag
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
	const CGSkill*   pDefenseSkill = nullptr
);

// Precomputed defender defense parameters struct
struct tagDefenseParams {
	uint32_t dwFlags;         // +0x00: Bit 0x04 = Magical attack allowed, Bit 0x08 = Physical attack allowed
	uint32_t dwReserved;      // +0x04
	uint32_t dwDefensePower;  // +0x08: Magical or Physical Defense Power
	uint32_t dwDefenseRate;   // +0x0C: Magical or Physical Defense Rate (%)
};

/**
 * [RECONSTRUCTED - 0x0040EBE0] (780 bytes)
 * Formulae_CalculatePhysicalSkillDamage_1 (Authentic: CalculateMagicalSkillDamage_Precomputed)
 *
 * Computes magical skill damage using precomputed/cached defender defense parameters.
 */
int32_t CalculateMagicalSkillDamage_Precomputed(
	const CGSkill*          pSkill,
	const CGObjChar*        pAttacker,
	const CGObjChar*        pTarget,
	const tagDefenseParams* pDefParams,
	uint8_t                 byHitRatioBonus,
	uint32_t                dwDamageFlags
);

int32_t CalculatePhysicalSkillDamage_1(
	const CGSkill*          pSkill,
	const CGObjChar*        pAttacker,
	const CGObjChar*        pTarget,
	const tagDefenseParams* pDefParams,
	uint8_t                 byHitRatioBonus,
	uint32_t                dwDamageFlags
);

/**
 * [RECONSTRUCTED - 0x0040EEF0] (700 bytes)
 * Formulae_CalculatePhysicalSkillDamage_2 (Authentic: CalculatePhysicalSkillDamage_Precomputed)
 *
 * Computes physical skill damage using precomputed/cached defender defense parameters.
 */
int32_t CalculatePhysicalSkillDamage_Precomputed(
	const CGSkill*          pSkill,
	const CGObjChar*        pAttacker,
	const CGObjChar*        pTarget,
	const tagDefenseParams* pDefParams,
	uint8_t                 byHitRatioBonus,
	uint32_t                dwDamageFlags
);

int32_t CalculatePhysicalSkillDamage_2(
	const CGSkill*          pSkill,
	const CGObjChar*        pAttacker,
	const CGObjChar*        pTarget,
	const tagDefenseParams* pDefParams,
	uint8_t                 byHitRatioBonus,
	uint32_t                dwDamageFlags
);

/**
 * [RECONSTRUCTED - 0x0040F1B0] (534 bytes)
 * Formulae_CalculatePhysicalSkillDamage_3 (Authentic: CalculateMagicalSkillDamage_Direct)
 *
 * Computes direct magical skill damage without precomputed parameters.
 */
int32_t CalculateMagicalSkillDamage_Direct(
	const CGObjChar* pAttacker,
	const CGSkill*   pSkill,
	const CGObjChar* pTarget,
	uint8_t          byHitRatioBonus
);

int32_t CalculatePhysicalSkillDamage_3(
	const CGObjChar* pAttacker,
	const CGSkill*   pSkill,
	const CGObjChar* pTarget,
	uint8_t          byHitRatioBonus
);

/**
 * [RECONSTRUCTED - 0x0040F3D0] (537 bytes)
 * Formulae_CalculatePhysicalSkillDamage_4 (Authentic: CalculatePhysicalSkillDamage_Direct)
 *
 * Computes direct physical skill damage without precomputed parameters.
 */
int32_t CalculatePhysicalSkillDamage_Direct(
	const CGObjChar* pAttacker,
	const CGSkill*   pSkill,
	const CGObjChar* pTarget,
	uint8_t          byHitRatioBonus
);

int32_t CalculatePhysicalSkillDamage_4(
	const CGObjChar* pAttacker,
	const CGSkill*   pSkill,
	const CGObjChar* pTarget,
	uint8_t          byHitRatioBonus
);

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
);

/**
 * [RECONSTRUCTED - 0x0040F6F0] (81 bytes)
 * Formulae_CalculateHPRatioDamage
 *
 * Computes percentage-of-max-HP damage:
 *   TargetMaxHP * (*pPercentage / 100.0)
 */
int32_t CalculateHPRatioDamage(
	const CGObjChar* pTarget,
	const uint32_t*  pPercentage
);

/**
 * [RECONSTRUCTED - 0x0040F750] (310 bytes)
 * Formulae_CalculateSkillHeal
 *
 * Computes skill heal recovery amount, adds weapon heal bonus, adjusts
 * for level difference, and applies recovery via CGObjChar_ApplyDamageAbsorptionReduction.
 */
int32_t CalculateSkillHeal(
	const CGObjChar* pTarget,
	const CGSkill*   pSkill,
	const CGObjChar* pAttacker,
	uint8_t          byHealRatio
);

/**
 * [RECONSTRUCTED - 0x0040F890] (59 bytes)
 * Formulae_RollLevelScaledDamage
 *
 * Rolls random value uniformly between (Level * 5) and (Level * 10).
 */
int32_t RollLevelScaledDamage(const CGObjChar* pChar);

/**
 * [RECONSTRUCTED - 0x0040F8D0] (81 bytes)
 * Formulae_RollWeaponPhysicalAttack
 *
 * Rolls random value uniformly between Weapon Min Physical (Param 191) and Max (Param 190).
 */
int32_t RollWeaponPhysicalAttack(const CGObjChar* pAttacker);

/**
 * [RECONSTRUCTED - 0x0040F930] (81 bytes)
 * Formulae_RollWeaponMagicalAttack
 *
 * Rolls random value uniformly between Weapon Min Magical (Param 193) and Max (Param 192).
 */
int32_t RollWeaponMagicalAttack(const CGObjChar* pAttacker);

/**
 * [RECONSTRUCTED - 0x0040F990] (736 bytes)
 * Formulae_CalculateWeaponAttackPower
 *
 * Dispatches roll based on weapon type (TID bits 11..15) and applies authentic scaling factor.
 */
int32_t CalculateWeaponAttackPower(const CGObjChar* pAttacker, uint16_t wWeaponType);

/**
 * [RECONSTRUCTED - 0x0040FCB0] (170 bytes)
 * Formulae_CalculateStatusEffectProbability
 *
 * Computes debuff / abnormal state landing probability:
 *   min(75, (AttackerLevel * 2 / (AttackerLevel + TargetLevel)) * BaseRate)
 */
uint8_t CalculateStatusEffectProbability(
	const CGObjChar* pTarget,
	uint32_t         dwAttackerLevel,
	uint32_t         dwBaseProbability
);

/**
 * [RECONSTRUCTED - 0x0040FD60] (104 bytes)
 * Formulae_GetLevelDiffScale
 */
float GetLevelDiffScale(int32_t nAttackerLevel, int32_t nTargetLevel);

/**
 * [RECONSTRUCTED - 0x0040FFE0] (302 bytes)
 * Formulae_CalculatePartyLevelDiffMultiplier
 *
 * Computes party experience sharing multiplier based on level difference and party mode.
 */
float CalculatePartyLevelDiffMultiplier(const CGObjChar* pTarget, const CGObjChar* pAttacker);

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
);

/**
 * [RECONSTRUCTED - 0x004D2DA0] (62 bytes)
 * Caravan_GetCOSSlotCapacity
 *
 * Maps COS trade beast rarity (m_byCOS_Rarity at RefObjCommon + 0x89):
 *   - Rarity 1 -> 40 slots
 *   - Rarity 2 -> 30 slots
 *   - Any other value triggers "COS Rarity Input Error : Input Value = [ %d ]" and mini-dump
 */
uint16_t Caravan_GetCOSSlotCapacity(uint8_t byRarity);

/**
 * [RECONSTRUCTED - 0x0060C1F0] (306 bytes)
 * Caravan_CalculateCargoValue
 *
 * Computes exact cargo score from active transport vehicle, player level,
 * base death exp, and slot capacity.
 */
uint32_t Caravan_CalculateCargoValue(const CGObjChar* pChar, const CGObjChar* pVehicle = nullptr);

/**
 * [RECONSTRUCTED - 0x0060C330] (34 bytes)
 * Caravan_GetTradeDifficultyTier
 *
 * Compares total cargo value against tier thresholds (0 to 5 stars).
 */
uint8_t Caravan_GetTradeDifficultyTier(const CGObjChar* pChar);

/**
 * [RECONSTRUCTED - 0x0060C360] (243 bytes)
 * Caravan_CalculateThreeStarCargoThreshold
 *
 * Computes the 3-star cargo item quota for a character based on active transport vehicle,
 * 3-star gold threshold (g_aTradeDifficultyTierThresholds[3] = 600,000), base death exp,
 * and slot capacity.
 */
uint64_t Caravan_CalculateThreeStarCargoThreshold(const CGObjChar* pChar);

/**
 * [RECONSTRUCTED - 0x005237A0] (15 bytes)
 * Caravan_CheckSpecialtyTrader
 *
 * Verifies if character is specialized trader with high cargo tier.
 * Returns difficulty tier (1..5) for specialized trader, 0 otherwise.
 */
uint8_t Caravan_CheckSpecialtyTrader(const CGObjChar* pChar, uint32_t dwMode);

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
int32_t ClassifyLevelDiff_Extended(uint8_t byAttackerLevel, uint8_t byTargetLevel);

/**
 * [RECONSTRUCTED - 0x00410370] (101 bytes)
 * Formulae_CalculateDeathExpLoss
 *
 * Computes character experience lost upon death:
 *   3 * ftol((baseLevelExp * 10) * 0.125)
 */
int32_t CalculateDeathExpLoss(const CGObjChar* pChar);

/**
 * [RECONSTRUCTED - 0x004103E0] (310 bytes)
 * Formulae_CalculateResurrectionExpRecovery
 *
 * Computes exact experience points recovered upon resurrection.
 * Clamps ratio between 0.50f and 1.0f, and halves if caster is in job mode.
 */
int32_t CalculateResurrectionExpRecovery(const CGObjChar* pDeadChar, const CGObjChar* pCaster);

/**
 * [RECONSTRUCTED - 0x00410520] (163 bytes)
 * Formulae_CalculateJobReward
 *
 * Computes job trade reward gold given character and profit amount.
 * Job 1 (Trader): 0.80x (if specialized) else 0.50x
 * Job 2 (Thief): 0.40x
 * Job 3 (Hunter): 1.00x
 */
int32_t CalculateJobReward(const CGObjChar* pChar, int64_t nProfit);

/**
 * [RECONSTRUCTED - 0x004105D0] (222 bytes)
 * Formulae_CalculatePvPExperience
 *
 * Computes experience awarded in direct PvP combat kills.
 * Uses min(attackerLevel, targetLevel) to query RefLevelData->dwLevelTotalExp (+0x1C).
 */
int32_t CalculatePvPExperience(const CGObjChar* pTarget, const CGObjChar* pAttacker);

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
);

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
	int32_t*         pOutRankBonus = nullptr
);

/**
 * [RECONSTRUCTED - 0x00410C80] (358 bytes)
 * Formulae_CalculateMasterySPCost
 *
 * Computes total SP cost required to upgrade a mastery to the target level.
 * Accumulates across skill nodes:
 *   (baseExp * 100.0) * (1.0 + (nodeValue / 10.0)) + 0.5 -> floor -> ftol
 */
int32_t CalculateMasterySPCost(
	const void*      pSkillTree,
	const CGObjChar* pChar,
	uint8_t          byRequestedRank = 0
);

int32_t CalculateSkillDowngradeGoldCost(
	const void*      pSkillTree,
	const CGObjChar* pChar,
	uint8_t          byRequestedRank = 0
);

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
);

/**
 * [RECONSTRUCTED - 0x00410EA0] (109 bytes)
 * Formulae_CalculateSkillDowngradeSPRefund
 *
 * Traverses skill tree node linked list (+0x59C) downgrading levels down to byTargetLevel.
 * (80% standard, 100% full refund mode).
 */
int32_t CalculateSkillDowngradeSPRefund(
	uint8_t          byTargetLevel,
	const void*      pSkillNode,
	bool             bFullRefund
);

/**
 * [RECONSTRUCTED - 0x00410F10] (182 bytes)
 * Formulae_CalculateMasteryDowngradeGoldCost
 *
 * Computes gold cost when downgrading a mastery from byCurrentLevel to byTargetLevel.
 * (80% standard, 100% full refund mode).
 */
int32_t CalculateMasteryDowngradeGoldCost(
	uint8_t          byCurrentLevel,
	uint8_t          byTargetLevel,
	bool             bFullRefund
);

/**
 * [RECONSTRUCTED - 0x00411000] (113 bytes)
 * Formulae_CalculateWeaponMagicalHealBonus
 *
 * Computes magical healing bonus derived from equipped weapon (slot 6).
 */
int32_t CalculateWeaponMagicalHealBonus(const CGObjChar* pChar, uint32_t dwPercentage);

/**
 * [RECONSTRUCTED - 0x00411080] (113 bytes)
 * Formulae_CalculateWeaponPhysicalHealBonus
 *
 * Computes physical healing bonus derived from equipped weapon (slot 6).
 */
int32_t CalculateWeaponPhysicalHealBonus(const CGObjChar* pChar, uint32_t dwPercentage);

} // namespace Formulae

#endif // _SR_GAMESERVER_FORMULAE_H_

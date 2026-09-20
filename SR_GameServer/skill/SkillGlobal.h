/**
 * ============================================================================
 * Silkroad Online - Skill Validation & Combat Prerequisites
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\skill\SkillGlobal.h
 *
 * Implements:
 *   - CheckSkillPreEngageCondition [PARTIAL - Native 0x0058D8F0]
 *   - Skill validation error codes (Joymax 0x30XX wire protocol)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_SKILL_SKILLGLOBAL_H_
#define _SR_GAMESERVER_SKILL_SKILLGLOBAL_H_

#include <cstdint>
#include <set>
#include "../GObjChar.h"
#include "../GCharAutoCommandActor.h"
#include "../Formulae.h"
#include "../GlobalPos.h"

struct tagActiveSkillInstance;
struct tagSkillPositionResult;

// Skill Validation Error Codes matching Joymax 0x30XX wire protocol
enum SkillErrorCode : uint16_t {
	SKILL_SUCCESS                         = 0x0000,
	SKILL_ERR_INVALID_TARGET              = 0x3003,
	SKILL_ERR_INSUFFICIENT_MP             = 0x3004,
	SKILL_ERR_COOLDOWN_ACTIVE             = 0x3005, // Native 0x0058E08C: CCooltimeManager cooldown active
	SKILL_ERR_INVALID_WEAPON              = 0x3006,
	SKILL_ERR_CANNOT_CAST_MOUNTED         = 0x3008,
	SKILL_ERR_TARGET_DEAD                 = 0x3009,
	SKILL_ERR_SKILL_ON_COOLDOWN           = 0x300C,
	SKILL_ERR_WEAPON_MISMATCH             = 0x300D,
	SKILL_ERR_PEACE_ZONE_RESTRICTION      = 0x300E,
	SKILL_ERR_WEAPON_BROKEN               = 0x300F,
	SKILL_ERR_INSUFFICIENT_HP             = 0x3010,
	SKILL_ERR_STATUS_RESTRICTED           = 0x3013,
	SKILL_ERR_LOW_MASTERY_LEVEL           = 0x3028,
	SKILL_ERR_TARGET_INVULNERABLE         = 0x3031,
	SKILL_ERR_MUTUAL_EXCLUSIVE_BUFF       = 0x3032,
	SKILL_ERR_CANNOT_ATTACK_FRIENDLY      = 0x3034,
	SKILL_ERR_PVP_RESTRICTION             = 0x3036,
	SKILL_ERR_OBSTACLE_BLOCK              = 0x3037,
	SKILL_ERR_CANNOT_CAST_WHILE_SITTING   = 0x3038,
	SKILL_ERR_TARGET_MAX_STACK            = 0x3039,
	SKILL_ERR_SPECIAL_ITEM_RESTRICTION    = 0x3047,
};

/**
 * [RECONSTRUCTED - Native 0x0058CC70]
 * TargetValidation_ValidateAllTargets
 *
 * Validates selected target candidates against caster state, range/region connectivity,
 * target life state, combat permissions, PvP relation state, abnormal immunities,
 * and monster capture parameters.
 */
uint16_t TargetValidation_ValidateAllTargets(
	CGObjChar* pCaster,
	const Skill::sSkillPreEngageData* pCommand,
	const tagRefSkill* pRefSkill
);

/**
 * [RECONSTRUCTED - Native 0x0058D480]
 * Skill_ValidateEquipmentRequirements
 *
 * Validates player character equipment constraints:
 * - Checks player status (non-players bypass equipment checks)
 * - Evaluates default weapon kinds (slot 6 main-hand, unarmed fists = 1)
 * - Evaluates 5-slot REQI parameter arrays for weapon, shield, armor, and avatar item expiration
 * - Enforces REQN conjunction rule
 */
uint16_t Skill_ValidateEquipmentRequirements(
	CGObjChar* pCaster,
	const tagRefSkill* pSkill
);

void Skill_GetSecondaryWeaponTID(uint16_t* pTID, const CGObjChar* pChar);
void Skill_GetMainWeaponTID(uint16_t* pTID, const CGObjChar* pChar);
void Skill_GetArmorSlotTID(uint32_t dwSlot, const CGObjChar* pChar, uint16_t* pTID);

/**
 * [PARTIAL - Native 0x0058D8F0] (3134 bytes)
 * CheckSkillPreEngageCondition (was Skill_ValidatePrerequisitesAndCost; see SkillGlobal.cpp)
 *
 * Returns 0 when the owner may use pSkill (resolved from pPreEngage when null) with pPreEngage,
 * otherwise the 0x30xx refusal. dwCheckFlags selects the optional check groups.
 */
uint16_t CheckSkillPreEngageCondition(
	CGObjChar* pCaster,
	Skill::sSkillPreEngageData* pPreEngage,
	uint32_t dwCheckFlags,
	const tagRefSkill* pSkill
);

// The action dispatch table (0x00C63C7C), SkillActionHandler (0x00589B50) and the per-category lifecycle
// handlers live in SkillCast.h / SkillCast.cpp; they used to be declared here as well.

// [RECONSTRUCTED - Native 0x0058D8D0] (30 bytes)
// Cast validation wrapper delegating to CheckSkillPreEngageCondition
uint16_t Skill_ValidateCast(
	tagActiveSkillInstance* pInstance,
	CGObjChar* pCaster,
	uint32_t dwValidateFlags
);

// [RECONSTRUCTED - Native 0x0058CB70] (243 bytes)
// Dispatches target selection based on skill shape parameter ('Efr' / 'Efr3')
uint16_t TargetSelection_DispatchByShape(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
);

uint16_t TargetSelection_DispatchByShape(
	CGObjChar* pCaster,
	tagActiveSkillInstance* pInstance,
	const uint32_t* pParamEfr,
	uint32_t dwMode,
	const tagRefSkill* pRefSkill
);

// Shape Handler 1: Around Source (Radius @ Caster) @ 0x0058A020 (1949 bytes)
void TargetSelection_AroundSource(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
);

// Shape Handler 2: Around Target (Radius @ Target / Detonate) @ 0x0058A7C0 (1944 bytes)
void TargetSelection_AroundTarget(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
);

// Shape Handler 3: Directional Range (Ground Direction / Cone) @ 0x0058B160 (1802 bytes)
void TargetSelection_DirectionalRange(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
);

// Shape Handler 4: Directional Target (Penetration / Line) @ 0x0058B870 (1664 bytes)
void TargetSelection_DirectionalTarget(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
);

// Shape Handler 5: Party (Radius Party Members) @ 0x0058BEF0 (496 bytes)
void TargetSelection_Party(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
);

// Shape Handler 6: Chain (Nearest Neighbor / Lowest HP Sorting) @ 0x0058C170 (2284 bytes)
void TargetSelection_Chain(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
);

// Shape Handler 7: Monster Group (AI Squad Members) @ 0x0058CA60 (266 bytes)
void TargetSelection_MonsterGroup(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	const tagRefSkill* pRefSkill,
	Skill::sSkillPreEngageData* pCommand,
	const uint32_t* pParamEfr,
	uint32_t dwMode
);

/**
 * [RECONSTRUCTED - Native 0x005862E0] (1049 bytes)
 * Skill_ApplyPositionEffect
 *
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\skill\SkillGlobal.cpp
 * Called by: CastLifecycle_ProcessInstant (0x00586700) @ 0x00586BDB / 0x00586F8D
 *
 * Evaluates teleportation and rush/dash position parameters:
 *   - tel3: Target-relative displacement / rush attack
 *   - tele: Ground-targeted teleportation / blink
 *   - tel2: Range-limited blink
 * Computes 3D displacement vector, enforces maximum range clamp,
 * adjusts for target bounding box padding, tests collision via
 * IRegionManager::QueryMovement (0x00CC387C), and allocates/stores
 * tagSkillPositionResult in the execution context.
 */
int32_t Skill_ApplyPositionEffect(
	const uint32_t* pTele,
	CGObjChar* pActor,
	tagActiveSkillInstance* pInstance,
	const uint32_t* pTel2,
	const uint32_t* pTel3
);

// Action Handler 1: Projectile Cast @ 0x005857B0 (2365 bytes)
int32_t SkillAction_Projectile(int32_t nEvent, CGObjChar* pCaster, void* pSkillActor, int32_t nReserved, void* pContext);

// Action Handler 3: Area Attack Cast @ 0x005830B0 (9247 bytes)
int32_t SkillAction_AreaAttack(int32_t nEvent, CGObjChar* pCaster, void* pSkillActor, int32_t nReserved, void* pContext);

// Action Handler 4: Continuous / Channel Cast @ 0x00587260 (80 bytes)
int32_t SkillAction_Continuous(int32_t nEvent, CGObjChar* pCaster, void* pSkillActor, int32_t nReserved, void* pContext);

// [RECONSTRUCTED - Native 0x00587210] (80 bytes)
// Cancels a continuous / channeling skill instance, clearing field 0xC34 on Dmgr and cancelling Real session
void* SkillAction_Continuous_Cancel(tagActiveSkillInstance* pActiveSkill, CGObjChar* pCaster);

// [RECONSTRUCTED - Native 0x00593540] (692 bytes)
// Executes authentic skill engagement, charges HP/MP, offsets SP, and processes effects
extern bool g_bDebugSkillActionHandler; // 0x00C82624

void SkillCombat_EngageSkill(CGObjChar* pCaster, tagActiveSkillInstance* pInstance);

/**
 * [PARTIAL - Native 0x0058E5F0] (8298 bytes)
 * SkillCombat_CalculateHitOutcome
 *
 * Rolls every hit of the cast against every target of the command and fills the execution context's result
 * batch, which the 0xB070 cast packet carries and SkillCombat_ApplyResultRecipients applies.
 */
int32_t SkillCombat_CalculateHitOutcome(
	CGObjChar* pCaster,
	Skill::sSkillPreEngageData* pCommand,
	tagSkillExecutionContext* pExec
);

/**
 * [PARTIAL - Native 0x00593800] (1731 bytes)
 * SkillCombat_ApplyResultRecipients
 *
 * Hands each record of the result batch to its target through CGObjChar::ApplyHit.
 */
void SkillCombat_ApplyResultRecipients(
	Skill::sSkillPreEngageData* pCommand,
	tagSkillExecutionContext* pExec,
	CGObjChar* pCaster
);

/**
 * [PARTIAL - Native 0x00593ED0] (341 bytes)
 * SkillCombat_ApplyStatusEffects
 *
 * Writes the statuses a hit rolled into the target's slots.
 */
void SkillCombat_ApplyStatusEffects(tagSkillTargetHitGroup* pRec, CGObjChar* pTarget);

/**
 * [PARTIAL - Native 0x00593F50] (2898 bytes)
 * SkillCombat_ApplySkillEffectsToTargets
 *
 * The summon / resurrect / teleport half of a cast.
 */
void SkillCombat_ApplySkillEffectsToTargets(tagActiveSkillInstance* pInstance);

/**
 * [PARTIAL - Native 0x0058E540] (168 bytes)
 * SkillCombat_IsTargetDamageable
 */
bool SkillCombat_IsTargetDamageable(CGObjChar* pCaster);

/**
 * [PARTIAL - Native 0x00590680] (11956 bytes)
 * SkillCombat_RollAbnormalStatus
 *
 * Rolls the abnormal statuses the skill carries against one target, appends a record per status that lands to
 * the hit group's list and returns the OR of their status bits.
 */
uint32_t SkillCombat_RollAbnormalStatus(
	CGObjChar* pCaster,
	CGObjChar* pTarget,
	tagSkillTargetHitGroup* pRec,
	const tagRefSkill* pRefSkill,
	const tagSkillHitResult* pHit
);


/**
 * [RECONSTRUCTED - Native 0x0058482E - 0x00584A50]
 * tagPersistentLink
 * Single-target tether or channel link maintained across ticks.
 */
struct tagPersistentLink {
	uint32_t m_dwUnknown00 = 0;     // +0x00
	uint32_t m_dwUnknown04 = 0;     // +0x04
	uint32_t m_dwLastTick = 0;      // +0x08: Timestamp of last pulse
	uint32_t m_dwSourceActorID = 0; // +0x0C: Caster entity ID
	uint32_t m_dwTargetActorID = 0; // +0x10: Target entity ID
};

/**
 * [RECONSTRUCTED - Native 0x00584A50 - 0x00585172]
 * tagPersistentAreaLink
 * Multi-target party aura / area tether maintained across ticks.
 */
struct tagPersistentAreaLink {
	uint32_t           m_dwUnknown00 = 0;     // +0x00
	uint32_t           m_dwUnknown04 = 0;     // +0x04
	float              m_fRadius = 0.0f;      // +0x08: Maximum tether radius
	uint32_t           m_dwLastTick = 0;      // +0x0C: Timestamp of last pulse
	uint32_t           m_dwSourceActorID = 0; // +0x10: Caster entity ID
	std::set<uint32_t> m_setMembers;          // +0x14: Recipient member entity IDs
};

/**
 * [RECONSTRUCTED - Native 0x0058489D / 0x00584A62]
 * Evaluates whether period ticks from 'Puls' parameter record have elapsed.
 */
bool CastLifecycle_PulseDue(
	uint32_t dwNowTick,
	uint32_t& dwLastTick,
	const tagRefSkill* pRefSkill
);

/**
 * [RECONSTRUCTED - Native 0x0058482E - 0x00584A50]
 * CastLifecycle_UpdateLinks
 *
 * Updates single-target persistent links across ticks. Validates distance,
 * line-of-sight, alive status, and executes pulse attacks.
 */
void CastLifecycle_UpdateLinks(
	CGObjChar* pCaster,
	void* pSkillActor,
	bool& bEnd
);

/**
 * [RECONSTRUCTED - Native 0x00584A50 - 0x00585172]
 * CastLifecycle_UpdateAreaLink
 *
 * Updates multi-target party aura links. Re-evaluates distance, cleans departed members,
 * discovers nearby party members within radius, and applies periodic healing or buffs.
 */
void CastLifecycle_UpdateAreaLink(
	CGObjChar* pCaster,
	void* pSkillActor,
	bool& bEnd
);

/**
 * [RECONSTRUCTED - Native 0x0058D7A0] (291 bytes)
 * Skill_ValidateTargetPermissions
 *
 * Validates whether caster is allowed to target candidate entity according to
 * skill target descriptor flags:
 *   - +0x9E: Bypass permission checks flag
 *   - +0x1C (IsPlayer): Caster / target player status
 *   - +0x24 (IsMonster): Monster filter
 *   - +0x99 / +0x9A: Player target requirement flags
 *   - +0x98: Self-target permission flag
 *   - +0x9F / +0xF8: Life state check (dead corpse vs alive)
 *   - +0x9B / +0x28: Non-player / NPC filter
 *   - +0x9C: Enemy / player filter
 *   - +0x1CB8 / CGObjChar_GetPartyID: Party membership check
 *   - +0xFC (GetMotionState) == 8 && +0x240 (Da parameter): Down/Prone attack filter
 */
bool Skill_ValidateTargetPermissions(
	const tagRefSkill* pSkill,
	CGObjChar* pCaster,
	CGObjChar* pTarget
);

/**
 * [RECONSTRUCTED - Native 0x00587630] (7509 bytes)
 * SkillGlobal_BuildParameterIndex
 *
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\skill\SkillGlobal.cpp
 * Called by: SkillGlobal_LoadReferenceData (0x005893E0) @ 0x005894E9
 *
 * Resets the 0x37C-byte parameter pointer table (+0x230 to +0x5AB) of tagRefSkill,
 * then iterates through the 49 raw reference parameter words (+0x16C).
 * Maps 158 FourCC tags to parameter pointer slots, normalizes target bitmasks,
 * and handles getv, efr, setv, reqi, and ssou records.
 */
void SkillGlobal_BuildParameterIndex(tagRefSkill* pRefSkill);

/**
 * [RECONSTRUCTED - Native 0x0059DC80] (334 bytes)
 * Skill_ValidateBuffExclusionList
 *
 * Validates active buffs and links against incoming skill exclusion rules:
 *   - Checks active links in CSkillManager::m_listActiveBuffs (+0x268)
 *   - Evaluates Lnks (+0x370) and Lks2 (+0x374) compatibility
 *   - Returns SKILL_ERR_SKILL_ON_COOLDOWN (0x300C), 0x3029, or 0x3037 on conflict
 */
uint16_t Skill_ValidateBuffExclusionList(
	CSkillManager* pSkillMgr,
	const tagRefSkill* pSkill,
	CGObjChar* pTarget
);

/**
 * [RECONSTRUCTED - Native 0x0058AF60] (856 bytes)
 * TargetSelection_Cone
 *
 * Tests whether candidate character lies within a directional cone/wedge sector:
 *   - Computes 3D relative displacement between caster and candidate via Pos_Relative3D
 *   - Checks max distance against (caster collision radius + direction length + candidate collision radius)
 *   - Computes lateral distance: sin(acos(dot(dir, offset))) * distance
 *   - Returns true if lateral distance < (candidate collision radius + dwWidth)
 */
bool TargetSelection_Cone(
	CGObjChar* pCaster,
	CGObjChar* pCandidate,
	float fDirX,
	float fDirY,
	float fDirZ,
	uint32_t dwWidth
);

/**
 * [RECONSTRUCTED - Native 0x005829D0] (1749 bytes)
 * SkillEffect_RetireContributionsAndLinks
 *
 * Cleans up and retires all active buffs and links associated with an area attack.
 */
void SkillEffect_RetireContributionsAndLinks(
	CGObjChar* pCaster,
	void* pSkillActor
);

/**
 * [RECONSTRUCTED - Native 0x00582750] (632 bytes)
 * Skill_ProcessPeriodicDamage
 *
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\skill\SkillGlobal.cpp
 * Called by: SkillAction_AreaAttack (0x005830B0) @ 0x00585382
 *
 * Validates periodic pulse target and tick interval against 'Summ' parameter record (+0x308),
 * queries attacker's mastery rank for the active weapon skill via CSkillManager_GetSkillMasteryRank (0x0059E770),
 * calculates combined magical & physical skill damage, applies special target multipliers (0x00615A20),
 * executes CGObjChar::ApplyHit (slot 319 @ +0x4FC), and broadcasts 0x30D1 packet to nearby sessions.
 */
int32_t Skill_ProcessPeriodicDamage(
	CGObjChar* pAttacker,
	tagActiveSkillInstance* pInstance
);

/**
 * [RECONSTRUCTED - Native 0x00584668 - 0x005847A0]
 * Skill_UpdateRampContributions
 *
 * Evaluates 'Mom' parameter record (+0x57C), increments ramp counter,
 * computes float value = (rampCount * ramp[3] * ramp[1]), and writes
 * 10 pairs of parameters to CGParamKeeper: (0x80,0), (0x81,0), (0x82,0),
 * (0x83,0), (5,1), (6,1), (9,0), (9,1), (0x0B,1), (0x0B,0).
 */
void Skill_UpdateRampContributions(
	CGObjChar* pActor,
	tagActiveSkillInstance* pInstance
);

// Serializes hit result data into CMsg packet buffer
// [RECONSTRUCTED - Native 0x005855F0] (203 bytes)
void SkillPacket_WriteHitResult(const tagSkillHitResult* pHit, BSLib::CPacket* pPacket);

// Serializes target ID and iterates list of hits into CMsg packet buffer
// [RECONSTRUCTED - Native 0x005856D0] (96 bytes)
void SkillPacket_WriteTargetHits(const tagSkillTargetHitGroup* pTargetHit, BSLib::CPacket* pPacket);

// Serializes batch kind, target count, and calls SkillPacket_WriteTargetHits
// [RECONSTRUCTED - Native 0x00585730] (124 bytes)
void SkillPacket_WriteResultBatch(const tagSkillResultBatch* pBatch, BSLib::CPacket* pPacket);

// Serializes position result coordinates (region + 3 floats) into CMsg packet buffer
// [RECONSTRUCTED - Native 0x005862B0] (39 bytes)
void SkillPacket_WritePositionResult(const tagSkillPositionResult* pPos, BSLib::CPacket* pPacket);

#endif // _SR_GAMESERVER_SKILL_SKILLGLOBAL_H_

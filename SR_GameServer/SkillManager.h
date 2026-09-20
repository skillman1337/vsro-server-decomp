/**
 * ============================================================================
 * Silkroad Online - Character Skill & Modifier Manager
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\SkillManager.h
 *
 * Implements:
 *   - CSkillManager::CSkillManager @ 0x00599F10 (372 bytes)
 *   - CSkillManager::~CSkillManager @ 0x0059A420 (442 bytes)
 *   - CSkillManager::GetSkillModifier @ 0x005A0330 (99 bytes)
 *   - CSkillManager::RegisterModifiers @ 0x005A02E0 (72 bytes)
 *   - CSkillManager::UnregisterModifiers @ 0x005A02A0 (56 bytes)
 *   - CSkillManager::GetDefaultAttackSkillByWeapon @ 0x0059E710 (50 bytes)
 *   - CSkillManager::FindActiveBuffBySkillID @ 0x0059EF90 (85 bytes)
 *   - CSkillManager::CancelActiveBuff @ 0x0059EFF0 (42 bytes)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_SKILLMANAGER_H_
#define _SR_GAMESERVER_SKILLMANAGER_H_

#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <list>
#include <optional>
#include "SkillRealModifiers.h"
#include "skill/SkillPreEngageData.h"

class CMsg;

namespace BSLib {
class CPacket;
typedef ::CMsg CMsg;
}

class CGObjChar;
class CGObjPC;
class CInstanceSkill;
class CInstanceSkillMastery;
struct tagRefSkill;
struct tagRefSkillMastery;

// Skill modifier record returned by CSkillManager::GetSkillModifier (0x005A0330)
// Native values are 3-word records: { key, argument1, argument2 }
struct tagSkillModifier {
	uint32_t dwModifierID; // +0x00: Modifier Param Key
	uint32_t dwValue;      // +0x04: Modifier Value (Percentage or integer bonus)
	uint32_t dwParam2;     // +0x08: Optional second parameter
};

// Skill position result record allocated by SkillPositionResult_Alloc (0x005AA270)
struct tagSkillPositionResult {
	uint32_t m_dwUnk0;       // +0x00
	uint8_t  m_bActive;      // +0x04: Active flag (1)
	uint8_t  m_pad05;        // +0x05
	uint16_t m_wRegionID;    // +0x06: Destination region ID
	int32_t  m_nX;           // +0x08: Encoded integer X coordinate
	int32_t  m_nY;           // +0x0C: Encoded integer Y coordinate
	int32_t  m_nZ;           // +0x10: Encoded integer Z coordinate
};

// CORRECTION (Claude): the 14-byte position record, the 8-byte target candidate and the command
// record that lived here (tagSkillCommand) are Skill::sSkillTargetPos, tagTargetCandidate and
// Skill::sSkillPreEngageData (RTTI .?AUsSkillPreEngageData@Skill@@), see skill/SkillPreEngageData.h.
// Offsets corrected there: +0x10 is the first target id, +0x18 the duration, and the vectors are
// objects at +0x3C / +0x4C; positions are floats.

// Skill reduction result status codes matching Joymax wire protocol (0x78XX)
enum SkillReductionResult : uint16_t {
	REDUCTION_SUCCESS             = 0x7800,
	REDUCTION_NOT_FOUND           = 0x7801,
	REDUCTION_RANK_NOT_LOWER      = 0x7802,
	REDUCTION_OPERATION_FAILED    = 0x7803,
	REDUCTION_INSUFFICIENT_GOLD   = 0x7804,
	REDUCTION_MISSING_ITEMS       = 0x7805,
	REDUCTION_DEPENDENCY          = 0x7806,
};

// Skill learning error codes returned by ValidateSkillLearning / Packet 0xB0A1 (0x34XX)
enum ESkillLearnError : uint16_t {
	SKILL_LEARN_SUCCESS                   = 0x3400,
	ERROR_SKILL_MASTERY_MISSING           = 0x3401,
	ERROR_SKILL_MASTERY_LEVEL_TOO_LOW     = 0x3402,
	ERROR_SKILL_REQ_STR_TOO_LOW           = 0x3403,
	ERROR_SKILL_REQ_INT_TOO_LOW           = 0x3404,
	ERROR_SKILL_COUNTRY_MISMATCH          = 0x3405,
	ERROR_SKILL_SEQUENCE_MISMATCH         = 0x3406,
	ERROR_SKILL_REQ_SKILL_LEVEL_TOO_LOW   = 0x3407,
	ERROR_SKILL_LEARN_FAILED              = 0x3409,
	ERROR_SKILL_INSUFFICIENT_SP           = 0x340A,
	ERROR_SKILL_INVALID_FIRST_RANK        = 0x340C
};

// Mastery advancement error codes returned by RaiseMastery / Packet 0xB0A2 (0x38XX)
enum EMasteryLearnError : uint16_t {
	MASTERY_LEVEL_UP_SUCCESS              = 0x3800,
	ERROR_MASTERY_INVALID_COUNTRY         = 0x3801,
	ERROR_MASTERY_INSUFFICIENT_SP         = 0x3802,
	ERROR_MASTERY_LEVEL_EXCEEDS_CHAR_LEVEL= 0x3803,
	ERROR_MASTERY_OPERATION_FAILED        = 0x3804,
	ERROR_MASTERY_TOTAL_CAP_EXCEEDED      = 0x3805
};

// Forward declarations for cast links
struct tagCastLink;
struct tagAreaLink;
struct tagSkillResultBatch;
struct tagSkillStatusEffect;

// Skill hit result record serialized by SkillPacket_WriteHitResult (0x005855F0)
struct tagSkillHitResult {
	uint8_t   m_pad0[5];      // +0x00 - +0x04
	uint8_t   m_byHeader;     // +0x05: Header byte (kind = byHeader & 0x7F)
	uint8_t   m_byFlags;      // +0x06: Flags byte
	uint8_t   m_pad07;        // +0x07
	uint32_t  m_dwExtra08;    // +0x08: Optional extra parameter / status
	uint16_t  m_wRegion;      // +0x0C: Region ID for position hits (kinds 4, 5)
	// CORRECTION (Claude): these were floats. SkillCombat_CalculateHitOutcome rounds the displacement with
	// CRT_ftol before storing it (0x005900C3 / 0x005900CF / 0x005900DB) and SkillPacket_WriteHitResult copies
	// the 12 bytes as they are (0x005856B8), so the client reads three integers.
	int32_t   m_nPosX;        // +0x0E
	int32_t   m_nPosY;        // +0x12
	int32_t   m_nPosZ;        // +0x16
	uint16_t  m_wExtra1A;     // +0x1A: Extra word for kind 7
	uint16_t  m_wExtra1C;     // +0x1C: Extra word for kind 7
	uint32_t  m_dwAmount;     // +0x20: Damage or recovery amount
};

// Status index to status bit (0x00C63EC8) and the growth curve status 2 reads (0x00C63C94). Both the skill
// code (0x005AA450) and CGObjChar (0x004A4365, 0x004A4500) index them.
extern const uint32_t g_adwAbnormalStatusBit[32];
extern const uint32_t g_adwBurnStatusRate[141];

/**
 * One abnormal status a hit inflicts, built by SkillCombat_RollAbnormalStatus (0x00590680) into the list the
 * hit group carries at +0x64 and handed to the target by CSkillManager::ProcessDamageEffects (0x005A0B80).
 * The payload words at +0x1C - +0x48 belong to individual statuses: each block of 0x00590680 copies its own
 * parameter record into its own slot, so only the status named on the field ever writes it.
 */
struct tagSkillStatusEffect {
	uint8_t  m_pad0[5];         // +0x00 - +0x04
	uint8_t  m_byStatusIndex;   // +0x05: index into g_adwAbnormalStatusBit (0x005AA452)
	uint8_t  m_byCategory;      // +0x06: 1 for the bits in 0x3F, 2 for the bits in 0x017FEFC0 (0x005AA460)
	uint8_t  m_pad07;           // +0x07
	uint32_t m_dwStatusBit;     // +0x08: g_adwAbnormalStatusBit[index] (0x005AA459)
	uint32_t m_dwDuration;      // +0x0C: duration in ms (0x005908xx / 0x005910AE)
	uint32_t m_dwOverlapLimit;  // +0x10: parameter 0x384, 2000 when the skill has none (0x00590BD3)
	uint32_t m_dwChance;        // +0x14: the probability the roll used (0x00590824)
	uint8_t  m_byGrade;         // +0x18: parameter word 2 for the control statuses (0x005910A5)
	uint8_t  m_pad19;           // +0x19
	uint16_t m_wLevel;          // +0x1A: strength of the damage over time statuses (0x00590817)
	uint32_t m_dwHiddenParam;   // +0x1C: status 0x18 (0x005934BA)
	float    m_fBurnRate;       // +0x20: status 2 (0x00590BF2)
	uint16_t m_wBurnEntry;      // +0x24: status 2, word from the table at 0x00C63C94 (0x00590C02)
	uint16_t m_pad26;           // +0x26
	uint32_t m_dwShockParam;    // +0x28: status 3 (0x00590AAA)
	uint32_t m_dwPanicParam;    // +0x2C: statuses 0x15 / 0x16 (0x00592F53 / 0x00593174)
	uint32_t m_pad30;           // +0x30
	uint32_t m_dwDecayParam;    // +0x34: statuses 0x11, 0x12, 0x15, 0x16 (0x00592B3F ..)
	uint32_t m_dwPoisonParam;   // +0x38: statuses 4, 0x0B, 0x13, 0x14, 0x15, 0x16 (0x00590D77 ..)
	uint32_t m_dwShortSightParam; // +0x3C: status 0x0A (0x005919C3)
	uint32_t m_dwBleedParam;    // +0x40: status 0x0B (0x00591C0C)
	uint32_t m_dwDiseaseParam;  // +0x44: status 0x0F (0x005922F3)
	uint32_t m_dwDarknessParam; // +0x48: status 0x0D (0x00591E15)
	uint32_t m_dwCasterID;      // +0x4C: caster game ID (0x0059083D)
	uint32_t m_dwTargetID;      // +0x50: target game ID (0x00590859)
	uint16_t m_wCasterTID;      // +0x54: caster TID (0x00590852)
	uint16_t m_pad56;           // +0x56
	uint32_t m_dwCasterJID;     // +0x58: the caster's account ID when it is a player (0x0059087A)
	uint32_t m_dw5C;            // +0x5C
};

// Target hit group serialized by SkillPacket_WriteTargetHits (0x005856D0). Only +0x08 and the hit list leave
// the server; the other fields are what SkillCombat_CalculateHitOutcome (0x0058E5F0) fills and
// SkillCombat_ApplyResultRecipients (0x00593800) reads back.
struct tagSkillTargetHitGroup {
	uint8_t                      m_pad0[5];         // +0x00 - +0x04
	uint8_t                      m_byDamageKind;    // +0x05: 0 weapon, 1 level scaled, 2 special target, 3 weapon power (0x0058E9BA..)
	uint8_t                      m_pad06[2];        // +0x06 - +0x07
	uint32_t                     m_dwTargetID;      // +0x08: Target Global Entity ID
	uint8_t                      m_byAttackRating;  // +0x0C: attack rating rolled against the target's parry (0x0058EA66)
	uint8_t                      m_byAbnormalKind;  // +0x0D: refSkill +0x23C[1] (0x0058EA63)
	uint8_t                      m_byEvasionRate;   // +0x0E: the target's evasion in percent (0x0058EAB9)
	uint8_t                      m_byParryRate;     // +0x0F: parry chance in percent (0x0058EC63)
	uint32_t                     m_dwIsMPDamage;    // +0x10: the hit drains MP, not HP (0x0058F10B in CSkillManager)
	uint8_t                      m_byEvadeCount;    // +0x14: stages the target evaded (0x0058F0F3)
	uint8_t                      m_byStageCount;    // +0x15: stages that reached the damage roll (0x0058F79F)
	uint8_t                      m_pad16[2];        // +0x16 - +0x17
	uint32_t                     m_dwTotalDamage;   // +0x18: damage ApplyResultRecipients hands to ApplyHit (0x00593xxx)
	uint32_t                     m_dwRecovery;      // +0x1C: the recovery the hit also carries, the second
	                                                //        value CGObjChar::ApplyHit takes (0x00593BB7)
	uint32_t                     m_dwStatusMask;    // +0x20: abnormal status bits the hit inflicted
	// +0x24 - +0x48 carry the displacement a knockdown or a pull gives the target (0x005900FE - 0x00590157)
	uint32_t                     m_dwHasDisplacement; // +0x24
	uint16_t                     m_wDisplaceRegion;   // +0x28
	uint16_t                     m_pad2A;             // +0x2A
	int32_t                      m_nDisplaceX;        // +0x2C
	int32_t                      m_nDisplaceY;        // +0x30
	int32_t                      m_nDisplaceZ;        // +0x34
	uint32_t                     m_dw38;              // +0x38: 1 with a displacement
	uint32_t                     m_dw3C;              // +0x3C: 1 with a displacement
	uint32_t                     m_dwTerminated;    // +0x40: the stage loop ended early - blocked or lethal (0x0058F778 / 0x005905B8)
	uint8_t                      m_byMotion;        // +0x44: 8 while the target is being pushed (0x0059011A)
	uint8_t                      m_by45;            // +0x45
	uint8_t                      m_pad46[2];        // +0x46 - +0x47
	float                        m_fDisplaceTime;   // +0x48: how long the push lasts (0x00590157)
	uint8_t                      m_pad4C[8];        // +0x4C - +0x53
	uint32_t                     m_dwKilled;        // +0x54: the hit took the target's last hit point (0x005905E0)
	std::list<tagSkillHitResult> m_listHits;        // +0x58 - +0x63: List of hits on this target
	std::list<tagSkillStatusEffect> m_listStatus;  // +0x64 - +0x6F: statuses the hit inflicted (0x00590805)
};

// Result batch record serialized by SkillPacket_WriteResultBatch (0x00585730)
struct tagSkillResultBatch {
	uint8_t                           m_pad0[5];       // +0x00 - +0x04
	uint8_t                           m_byKind;        // +0x05: hit stages per target (0x0058E7C3), at least 1
	uint8_t                           m_byDamageStages; // +0x06: stages that reached the damage roll (0x0058F743)
	uint8_t                           m_pad07;         // +0x07
	std::list<tagSkillTargetHitGroup> m_listTargets;   // +0x08 - +0x13: List of targets in batch
	uint8_t                           m_byTargetCount; // +0x10 (aliased in list)
};

// Companion / parasite tether link record serialized by SendLinkedEffectB0BE (0x0059AFF0)
struct tagCastLink {
    uint8_t m_pad0[8]{};             // Native pool header +00..07
    uint32_t m_dwLastTick = 0;       // +08: periodic source update
    uint32_t m_dwSourceActorID = 0;  // +0C: 584A2F resolves the source
    uint32_t m_dwTargetActorID = 0;  // +10: 58484D resolves the recipient
    uint32_t m_dwSourceContextID = 0;// +14: 5830B0 source context producer
    uint32_t m_dwTargetContextID = 0;// +18: 582AC9 recipient lookup token
    std::string m_strTargetName;    // Native object +1C (SSO buffer +20)
    void* m_pAssociatedTask = nullptr; // Native +38; source retirement owns cancellation
};

// Forward declarations for cast links
struct tagCastLink;
struct tagAreaLink;

// Periodic damage pulse descriptor (Execution + 0x64)
struct tagPeriodicDamagePulse {
	uint32_t m_dwPad00 = 0;        // +0x00
	uint32_t m_dwPad04 = 0;        // +0x04
	uint32_t m_dwStartedAt = 0;    // +0x08: Start timestamp (from GetTickCount)
	uint32_t m_dwLastTick = 0;     // +0x0C: Last damage tick timestamp
	uint32_t m_dwTargetID = 0;     // +0x10: Target entity ID
};

struct tagRepeatingCost {
	uint32_t m_dwChargedAt = 0;
};

// Queued skill damage operation (Native 0x0059B070)
struct tagQueuedSkillOperation {
	uint32_t              m_dwContextID = 0;      // +0x00: Execution context ID
	uint32_t              m_dwSourceActorID = 0;  // +0x10: Attacking entity ID
	uint32_t              m_dwDamage = 0;         // +0x20: Damage amount
	uint8_t               m_byFlags = 0;          // +0x2C: Damage flags (0x80 = lethal / fatal hit)
	std::vector<uint32_t> m_vecDamageWords;       // +0x30: Packed 3-byte damage component words
};

// Deferred skill status result record (Native 0x00593ED0)
struct tagDeferredStatusRecord {
	std::list<void*>      m_listStatusChanges;    // +0x64: Queued status change items
};

// Travel timer descriptor allocated for projectile / travelling skills (Native 0x005AA2D0)
struct tagSkillTravelTimer {
	void*    pNext = nullptr;        // +0x00: Pool node link
	uint32_t dwActive = 1;           // +0x04: Active state flag
	uint32_t dwStartedAt = 0;        // +0x08: Start tick timestamp (GetTickCount)
	uint32_t dwDuration = 0;         // +0x0C: Total flight duration in milliseconds
};

// Skill execution context record
struct tagSkillExecutionContext {
	// Native 5A9AE0 allocates a distinct execution identity and source mode 1.
	static tagSkillExecutionContext* Allocate();
	static void Release(tagSkillExecutionContext*& context);
	uint8_t                  m_pad0[8];
	const tagRefSkill*       m_pRefSkill;         // +0x08: Skill reference template
	uint32_t                 m_dwContextID;       // +0x0C: Execution context ID
	uint8_t                  m_pad10[0x10];       // +0x10 - +0x1F
	uint8_t                  m_byMode;            // +0x20: Context mode byte
	uint8_t                  m_pad21[3];          // +0x21 - +0x23
	uint32_t                 m_dwDurationBonus;   // +0x24: Duration bonus in ms
	uint32_t                 m_dwRampStartedAt;   // +0x28: Ramp rate started timestamp (proven @ 0x005846A2: +0x44)
	uint32_t                 m_dwRampCount;       // +0x2C: Ramp count accumulator (proven @ 0x005846A8: +0x48)
	uint32_t                 m_dwModifier2C;      // +0x2C
	uint32_t                 m_dwModifier30;      // +0x30
	uint32_t                 m_dwModifier34;      // +0x34
	uint32_t                 m_dwModifier38;      // +0x38
	uint32_t                 m_dwOwnedSelectors = 0; // +0x3C: Active selector ownership bitmask (Native 0x0059DE27, 0x00582AF0)
	uint8_t                  m_pad40[0x0C];          // +0x40 - +0x4B
	uint32_t                 m_dwResultFlags;     // +0x4C: Result bitmask (0x02 = tel2, 0x08 = tele/tel3)
	int32_t                  m_nCalculatedHPCost = 0;
	int32_t                  m_nCalculatedMPCost = 0;
	uint8_t                  m_byCalculatedBerserkCost = 0; // Native execution +18
	tagRepeatingCost*        m_pRepeatingCost = nullptr;
	union {
		uint32_t             m_dwPad50;           // +0x50
		tagSkillResultBatch* m_pResultBatch;      // +0x50: Attack / target result batch (proven @ 0x0059E924)
	};
	tagSkillPositionResult*  m_pPositionResult = nullptr;   // +0x54: Allocated position result
	tagSkillTravelTimer*     m_pTravelTimer = nullptr;      // +0x58: Allocated projectile travel timer (Native 0x00585E06)
	uint8_t                  m_pad5C[8];          // +0x5C - +0x63
	// 59371C allocates this solely for ref+308; 585378 dispatches 582750.
	// Area healing uses ref+298 and its area-link recipient selection, not this slot.
	tagPeriodicDamagePulse* m_pPeriodicDamage = nullptr; // Native +64
	tagCastLink*             m_pCastLink;         // +0x68: Paired companion / parasite link
	tagAreaLink*             m_pAreaLink;         // +0x6C: Area effect / aura link
};

// Active buff / skill instance record (Native slab @ 0x005AB940)
struct tagActiveSkillInstance {
	static tagActiveSkillInstance* Allocate();
	static void Release(tagActiveSkillInstance*& instance);
	uint8_t                    m_pad0[4];
	uint8_t                    m_bActive;      // +0x04
	uint8_t                    m_pad05;        // +0x05
	uint16_t                   m_wStatus;      // +0x06: Status word (e.g. 0x3000, proven @ 0x0058310B)
	uint32_t                   m_dwStartTime;  // +0x08
	union {
		uint32_t           m_dwMode;       // +0x0C: 0 = Begin, 1 = Activate, 2 = Active Buff / Tick
		uint8_t            m_byMode;       // +0x0C
	};
	uint32_t                   m_dwRetirement; // +0x10: Retirement marker (0 = retired/cancelled, 1 = active)
	Skill::sSkillPreEngageData*           m_pCommand;     // +0x14: Action command holding skill ID
	tagSkillExecutionContext*  m_pExecution;   // +0x18: Execution context holding ref skill & context ID
	Skill::sSkillPreEngageData* m_pSecondaryCommand = nullptr; // Native +1C
	tagSkillExecutionContext* m_pSecondaryExecution = nullptr; // Native +20

	// [RECONSTRUCTED - Native 0x0059EFF0] (42 bytes)
	// Clears retirement marker (+0x10) if bForce is true or if ref skill lacks nbuf parameter or mode is 1
	void RequestRetirement(bool bForce);
};

// Skill descriptor record stored in m_mapSkill (+0x228)
struct tagSkillData {
	uint32_t            m_dwVptr;      // +0x00
	uint32_t            m_dwRefCount;  // +0x04
	CInstanceSkill*     m_pRefRecord;  // +0x08: Persistent database record
	uint8_t             m_byStatus;    // +0x0C: 0 = Active, 1 = Pending, 2 = Inactive/Deleted, 3 = Siege
	uint8_t             m_pad0D[3];    // +0x0D - +0x0F
	const tagRefSkill*  m_pRefSkill;   // +0x10: Reference to skill template from g_pRefData + 0x888
};

// Mastery descriptor record stored in m_mapMastery (+0x234)
struct tagSkillMasteryData {
	uint32_t               m_dwVptr;      // +0x00
	uint32_t               m_dwRefCount;  // +0x04
	CInstanceSkillMastery* m_pRefRecord;  // +0x08: Persistent database record
	uint8_t                m_byStatus;    // +0x0C: 0 = Active, 1 = Pending, 3 = Inactive/Deleted
	uint8_t                m_pad0D[3];    // +0x0D - +0x0F
	const void*            m_pRefMastery; // +0x10: Reference to mastery template from g_pRefData + 0x87C
};

/**
 * ============================================================================
 * CZoeZoeRnd [RECONSTRUCTED - Native 0x00599BE0 - 0x00599E00]
 *
 * Silkroad Online Pseudo-Random Probability & Pity Threshold Manager
 * Native VTable @ 0x00AFE168 (RTTI: .?AVCZoeZoeRnd@@ @ 0x00B59968)
 * Embedded in CSkillManager at offset +0x04
 * ============================================================================
 */
class CZoeZoeRnd {
public:
	// [RECONSTRUCTED - Native 0x00599BE0] (81 bytes)
	CZoeZoeRnd();

	// [RECONSTRUCTED - Native 0x00599C40] (126 bytes)
	virtual ~CZoeZoeRnd();

	// [RECONSTRUCTED - Native 0x00599CC0] (273 bytes)
	// Dynamic probability roll using internal per-key threshold pity system
	bool Roll(int32_t nChance, uint32_t dwKey);

	// Clears all tracked probability thresholds
	void Clear();

	// Inspects currently tracked threshold for a key
	std::optional<int32_t> GetThreshold(uint32_t dwKey) const;

	size_t Size() const { return m_thresholds.size(); }

private:
	// Native 0x00599E00: std::map storing per-key dynamic success thresholds
	std::map<uint32_t, int32_t> m_thresholds; // +0x04
};

/**
 * [RECONSTRUCTED - 0x00599F10]
 * Character Skill & Modifier Manager (embedded in CGObjChar at offset +0xA30, size 0x300 bytes)
 */
class CSkillManager {
public:
	CSkillManager();
	~CSkillManager();

	// [RECONSTRUCTED - Native 0x005A1A20] (18 bytes)
	// Rolls dynamic probability for status effects, criticals, or skill procs
	bool RollProbability(int32_t nChance, uint32_t dwKey);

	CZoeZoeRnd& GetZoeZoeRnd() { return m_zoeRnd; }
	const CZoeZoeRnd& GetZoeZoeRnd() const { return m_zoeRnd; }

	// [RECONSTRUCTED - Native 0x0059D870] (665 bytes)
	// Validates whether new buff can replace or coexist with active buffs on character
	bool ValidateBuffReplacement(const tagRefSkill* pRefSkill, CGObjChar* pCaster);

	// [RECONSTRUCTED - Native 0x0059DC00] (111 bytes)
	// Updates 4 64-bit state words (+0x1B0) using 3 packed state indices (bits 0..23)
	uint32_t ChangeStates(uint32_t dwPackedStates, bool bRemove);

	// [RECONSTRUCTED - Native 0x0059DC80] (354 bytes)
	// Validates whether linked companion buffs or exclusion lists prevent casting
	uint16_t ValidateBuffExclusionList(const tagRefSkill* pRefSkill, CGObjChar* pTarget);

	// [RECONSTRUCTED - Native 0x0059DDF0] (11 bytes)
	// Returns condition flag (bit 0 of state word at offset +0x1D0)
	bool CheckOwnerCondition() const;

	// [RECONSTRUCTED - Native 0x0059DE00] (77 bytes)
	// Installs or removes execution selector bit in context (+0x3C) and manager (+0x1D0)
	int32_t InstallSelector(tagSkillExecutionContext* pContext, uint32_t dwValue);

	void AddActiveSkill(tagActiveSkillInstance* pInstance) {
		if (!pInstance) return;
		for (auto* existing : m_listActiveBuffs) if (existing == pInstance) return;
		m_listActiveBuffs.push_back(pInstance);
	}

	// 59DF20: real[0] mask, real[1] value, real[2] key, execution identity.
	void UpdateRealModifiers(const uint32_t* real, uint32_t context, bool remove) {
		if (real) m_realModifiers.Update(real[0], real[1], real[2], context, remove);
	}
	const SkillRealModifiers& GetRealModifiers() const { return m_realModifiers; }

	// [RECONSTRUCTED - 0x005A0330] (99 bytes)
	// Looks up active skill modifier in m_mapModifiers (+0x240) by modifier key
	const tagSkillModifier* GetSkillModifier(uint32_t dwModifierID) const;

	// [RECONSTRUCTED - 0x005A02E0] (72 bytes)
	// Registers Setv through Setv5 parameter records from pRefSkill into m_mapModifiers (+0x240)
	void RegisterModifiers(const tagRefSkill* pRefSkill);

	// [RECONSTRUCTED - 0x005A02A0] (56 bytes)
	// Unregisters Setv through Setv5 parameter records from m_mapModifiers (+0x240)
	void UnregisterModifiers(const tagRefSkill* pRefSkill);

	// Convenience overload for scalar registration
	void RegisterModifier(uint32_t dwModifierID, uint32_t dwValue, uint32_t dwParam2 = 0);

	// Convenience overload for single key removal
	void UnregisterModifier(uint32_t dwModifierID);

	// Clear all active modifiers
	void ClearModifiers();

	// [RECONSTRUCTED - 0x0059E710] (50 bytes) (esi = this)
	// Default attack skill for the owner's slot-6 weapon; 0 for non-players.
	// CORRECTION (Claude): takes no weapon argument; it reads the weapon itself (0x004EAD40).
	uint32_t GetDefaultAttackSkillByWeapon() const;

	// [RECONSTRUCTED - 0x0059EF90] (85 bytes)
	// Searches active buffs in m_listActiveBuffs (+0x268) by skill ID and optional context ID
	tagActiveSkillInstance* FindActiveBuffBySkillID(uint32_t dwSkillID, uint32_t dwContextID = 0) const;

	// Compatibility alias matching checkpoint05 interface
	tagActiveSkillInstance* FindActiveInstance(uint32_t dwSkillID, uint32_t dwContextID = 0) const {
		return FindActiveBuffBySkillID(dwSkillID, dwContextID);
	}

	// [RECONSTRUCTED - 0x0059F0C0] (27 bytes)
	// Cancels active buff by skill ID, marking it for retirement
	tagActiveSkillInstance* CancelBuffBySkillID(uint32_t dwSkillID);

	// [RECONSTRUCTED - Native 0x005A09F0] (396 bytes)
	// Applies heal recovery to target character based on skill parameters and weapon healing bonus
	void ApplyHealRecovery(CGObjChar* pTarget, const tagRefSkill* pRefSkill);

	// Convenience wrapper for legacy callers
	bool CancelActiveBuff(uint32_t dwRefSkillID, uint32_t dwRecordSkillID = 0);

	// [RECONSTRUCTED - 0x0059D680] (91 bytes)
	// Looks up learned skill in m_mapSkill (+0x228) by Skill ID (returns nullptr if status is 2)
	tagSkillData* FindSkillByID(uint32_t dwSkillID) const;

	// [RECONSTRUCTED - Native 0x0059D6F0] (99 bytes)
	// Searches active masteries in m_mapMastery (+0x234) by mastery ID (returns nullptr if status is 3)
	tagSkillMasteryData* FindMastery(uint32_t dwMasteryID) const;

	// Compatibility alias for legacy callers
	tagSkillMasteryData* FindMasteryEntry(uint32_t dwMasteryID) const {
		return FindMastery(dwMasteryID);
	}

	// [RECONSTRUCTED - Native 0x0059CDC0] (630 bytes)
	// Serializes active buff data into client packet buffer (spawn packet / status sync)
	void SerializeActiveBuffs(BSLib::CPacket* pPacket, uint32_t dwTimeMode);

	// [RECONSTRUCTED - Native 0x0059D040] (867 bytes)
	// Serializes pending learned masteries and skills updates with sentinels 0, 1, 2
	void SerializePendingLearnedSkills(BSLib::CPacket* pPacket);

	// [RECONSTRUCTED - Native 0x0059ECD0] (234 bytes)
	// Constructs and broadcasts Opcode 0xB072 for expired / cancelled skill buff instances
	void PublishRetiredBuffs();

	// [RECONSTRUCTED - Native 0x0059FF80] (374 bytes)
	// Cancels/retires non-persistent active buffs upon character death
	void RetireSkillsForDeath(bool bForceSelection);

	// [RECONSTRUCTED - Native 0x005A1AD0] (156 bytes)
	// Checks if target entity is hostile and eligible for attack
	uint32_t IsHostileTargetEligible(CGObjChar* pTarget);

	// [RECONSTRUCTED - Native 0x005A1B70] (60 bytes)
	// Checks if target entity is in the same party
	bool IsSameParty(CGObjChar* pTarget);

	// [RECONSTRUCTED - Native 0x0059F7B0] (235 bytes)
	// Consumes reduction items from player inventory using cursor slot 0x0D and storage virtual slots 137 / 140
	uint16_t ProcessReductionItems(const char* szItemCodeName, int32_t nCount, uint32_t dwControl = 1);

	// [RECONSTRUCTED - Native 0x0059F410] (501 bytes)
	// Validates skill reduction / recall prerequisites, gold, items, and skill dependency tree
	uint16_t ValidateSkillReduction(const char* szItemCodeName, uint32_t dwSkillID, uint8_t byRequestedRank, uint8_t byFlags);

	// [RECONSTRUCTED - Native 0x0059F610] (400 bytes)
	// Validates mastery reduction / recall prerequisites, gold, items, and mastery dependency tree
	uint16_t ValidateMasteryReduction(const char* szItemCodeName, uint32_t dwMasteryID, uint8_t byRequestedRank, uint8_t byFlags);

	// [RECONSTRUCTED - Native 0x0059F8B0] (1190 bytes)
	// Executes skill reduction, updates database, refunds SP, and broadcasts result
	uint16_t ExecuteSkillReduction(const char* szItemCodeName, uint32_t dwSkillID, uint8_t byRequestedRank, uint8_t byFlags);

	// [RECONSTRUCTED - Native 0x0059E450] (499 bytes)
	// Validates mastery requirements, stats, sequence, prerequisite skills, and SP cost for learning a skill
	uint16_t ValidateSkillLearning(uint32_t dwSkillID);

	// [RECONSTRUCTED - Native 0x0059B8D0] (676 bytes)
	// Executes an indirect / script / item triggered skill cast.
	// If skill has Cbuf (+0x358) and Dura (+0x280), creates an owner timed job.
	// Otherwise allocates command (requestMode = 0x20), validates cast prerequisites,
	// and executes cast begin (0xB070) + stage end (0xB071) or spawns entity with Lnks (0xB0BE).
	bool BeginIndirectSkill(
		const tagRefSkill* pRefSkill,
		uint32_t dwJobValue1 = 0,
		uint32_t dwJobValue2 = 0,
		uint32_t dwDurationOverride = 0
	);

	// [RECONSTRUCTED - Native 0x0059F0E0] (770 bytes)
	// Iterates active passive skills and validates equipment requirements (Reqi array at +0x3A0, Reqn at +0x3B4).
	// Automatically activates or deactivates passive bonuses upon equipment changes.
	void UpdatePassiveSkills();

	// [RECONSTRUCTED - Native 0x0059BFE0] (1275 bytes)
	// Executes skill learning, replaces previous rank, starts passive effects, updates DB, and debits SP
	bool LearnSkill(uint32_t dwSkillID);

	// [RECONSTRUCTED - Native 0x0059C4E0] (827 bytes)
	// Advances mastery level, enforces country mastery caps (China 330, Europe 240), updates DB, and debits SP
	bool RaiseMastery(uint32_t dwMasteryID, uint8_t byLevelIncrement);

	// [RECONSTRUCTED - Native 0x0059A680] (802 bytes)
	// Consumes persistent learned skills and masteries loaded from database
	bool LoadLearnedRecords(std::list<CInstanceSkill*>& listSkills, CGObjPC* pPC, std::list<CInstanceSkillMastery*>& listMasteries);

	// [RECONSTRUCTED - Native 0x0059D3B0] (579 bytes)
	// Rebuilds pending mastery and learned skill display queues
	void RebuildLearnedLists();

	// [RECONSTRUCTED - Native 0x0059D760] (250 bytes)
	// Finds the highest learned skill rank for a specified skill group
	const tagRefSkill* GetHighestRankSkill(uint32_t dwGroupID) const;

	// [RECONSTRUCTED - Native 0x0059E7C0] (123 bytes)
	// Calculates total accumulated mastery levels across all learned masteries
	uint32_t GetTotalMasteryLevel() const;

	// [RECONSTRUCTED - Native 0x0059E870] (85 bytes)
	// Scans active masteries in m_mapMastery (+0x234) and returns highest mastery level
	uint8_t GetMaxMasteryLevel() const;
	uint8_t GetSkillMasteryRank(const tagRefSkill* skill) const; // 59E770

	// [RECONSTRUCTED - Native 0x0059EA40] (112 bytes)
	// Registers siege weapon skill reference into learned skill map
	int32_t RegisterSiegeSkill(uint32_t dwSiegeSkillID);

	// [RECONSTRUCTED - Native 0x0064CC40] (16 bytes)
	// Zeroes 4 state words (+0x50 - +0x5F) on character subsystem
	static void ResetLearnedStateWords(void* pTarget);

	// [PARTIAL - Native 0x0059E650] (183 bytes) (eax = this, stack wWeaponTID & 0xF800)
	// CORRECTION (Claude): was GetAttackSkillByActionType and called GetDefaultAttackSkillByWeapon back,
	// which recursed forever. It indexes the per-weapon-kind table filled by SkillGlobal_LoadReferenceData.
	uint32_t GetAttackSkillByWeaponTID(uint16_t wWeaponTID) const;

	// Returns active skill MP cost reduction modifier (proven @ 0x005A0330)
	float GetSkillMPCostModifier() const { return 0.0f; }

	void AddLearnedSkill(uint32_t dwSkillID, tagSkillData* pData) {
		m_mapSkill[dwSkillID] = pData;
	}

	// [RECONSTRUCTED - Native 0x0059B7C0] (123 bytes) (ebx = this, edi = pMsg)
	// Internal 0x7070: unless the owner is dead or gone, knocked down (motion 0x12), or its running
	// instance's skill has +0x274, reads the pre-engage data and starts the skill use.
	int32_t OnMsgSkillAction(CMsg* pMsg);

	// [PARTIAL - Native 0x0059B480] (820 bytes) (stack pPreEngage, owned from here on)
	int32_t InitiateSkillCast(Skill::sSkillPreEngageData* pPreEngage);

	// [RECONSTRUCTED - Native 0x0049A1B0] (32 bytes) (eax = this)
	// Msid (+0x4A4) of the skill of the instance at +0x1F0; casting is locked while it is 1.
	int32_t IsCastingLocked() const;

	// [RECONSTRUCTED - Native 0x0059AD90] (121 bytes)
	// Sends Opcode 0xB070 skill error response to player or posts AI event 4 to monster
	int32_t SendSkillErrorResponseB070(uint16_t wErrorCode);

	// [RECONSTRUCTED - Native 0x0059AE10] (225 bytes)
	// Broadcasts Opcode 0xB071 stage end / cancellation packet to nearby sessions
	int32_t SendStageEndB071(uint8_t byKind, uint16_t wErrorCode, uint32_t dwContextID, bool bForceBroadcast = false);

	// [PARTIAL - Native 0x005A0B80] (2871 bytes)
	// Shares one landed hit with the party or the guild the caster belongs to.
	void ProcessDamageEffects(CGObjChar* pCaster, CGObjChar* pTarget, tagSkillTargetHitGroup* pRec,
		tagSkillHitResult* pHit);

	// [RECONSTRUCTED - Native 0x00586D61 - 0x00587005] the action stage of a delayed cast: the same 0xB071
	// the stage end uses, but carrying the target, the result flags and the hit batch the client draws.
	int32_t SendActionStageB071(tagActiveSkillInstance* pInstance);

	// [RECONSTRUCTED - Native 0x0059E8F0] (377 bytes)
	// Constructs and broadcasts Opcode 0xB070 Cast Begin to nearby sessions
	int32_t SendCastBeginB070(tagActiveSkillInstance* pInstance, uint16_t wStatus);

	// [RECONSTRUCTED - Native 0x0059AF00] (238 bytes)
	// Broadcasts Opcode 0xB0BD buff/effect added notification to nearby sessions
	int32_t SendEffectAddedB0BD(tagActiveSkillInstance* pInstance);

	// [RECONSTRUCTED - Native 0x0059AFF0] (120 bytes)
	// Sends Opcode 0xB0BE companion/parasite linked effect packet directly to owner
	int32_t SendLinkedEffectB0BE(tagCastLink* pLink, const tagRefSkill* pRefSkill);

	std::list<tagActiveSkillInstance*>& GetActiveBuffs() { return m_listActiveBuffs; }
	const std::list<tagActiveSkillInstance*>& GetActiveBuffs() const { return m_listActiveBuffs; }

	void SetOwner(CGObjChar* pOwner) { m_pOwner = pOwner; }
	CGObjChar* GetOwner() const { return m_pOwner; }

	// Effect instance slots filled when a buff with the given reference parameter is applied
	// (CSkillManager_ApplyBuffModifiersToActor 0x00594AC0) and cleared by 0x00582A20.
	tagActiveSkillInstance* GetInstance1DC() const { return m_pInstance1DC; } // ref +0x2B4 (0x00582DCE)
	tagActiveSkillInstance* GetInstance1F8() const { return m_pInstance1F8; } // ref +0x2CC (0x00594D22)
	tagActiveSkillInstance* GetInstance214() const { return m_pInstance214; } // ref +0x448 (0x00594F4D)

	// Current executing skill instance accessors (Native +0x1D8)
	tagActiveSkillInstance* GetCurrentInstance() const { return m_pCurrentInstance; }
	void SetCurrentInstance(tagActiveSkillInstance* pInstance) { m_pCurrentInstance = pInstance; }

	void AddActiveBuff(tagActiveSkillInstance* pInstance) { m_listActiveBuffs.push_back(pInstance); }

	// [RECONSTRUCTED - Native 0x0059FD60] (542 bytes)
	// Executes mastery reduction, updates database, refunds SP, and broadcasts result
	uint16_t ExecuteMasteryReduction(const char* szItemCodeName, uint32_t dwMasteryID, uint8_t byRequestedRank, uint8_t byFlags);

	// [RECONSTRUCTED - Native 0x0059D600] (118 bytes) (esi = this, edi = dwSkillID)
	// Reference skill for a cast: must be learned unless the owner rides a transport (slot 336) or
	// casting is locked by an Msid skill. CORRECTION (Claude): was ResolveCastReference, without the
	// slot 336 test.
	const tagRefSkill* GetSkillData(uint32_t dwSkillID);

	// [RECONSTRUCTED - Native 0x0059DB10] (236 bytes)
	// Checks if casting state words (+0x1B0) or active instance conflicts with packed state mask
	bool HasBlockedStates(uint32_t dwPackedStates) const;

	// [RECONSTRUCTED - Native 0x0059B070] (427 bytes)
	// Applies queued deferred damage and broadcasts packet 0xB0BC
	void ProcessQueuedDamage(tagQueuedSkillOperation& operation);

	// [RECONSTRUCTED - Native 0x00593ED0] (341 bytes)
	// Applies queued status changes and refreshes character status state
	void ProcessDeferredStatusResults(tagDeferredStatusRecord& record);

	// [RECONSTRUCTED - Native 0x0059ECD0] (109 bytes)
	// Broadcasts packet 0xB072 with all retired context IDs and clears queue
	void PublishRetiredContexts();

	// [RECONSTRUCTED - Native 0x0059EDC0] (445 bytes)
	// Sends operation success packet (0xB0A1, 0xB0A2, 0xB202, 0xB203) or failure notice (4)
	void SendOperationResult(uint32_t dwOperation, bool bSuccess, uint32_t dwID, uint32_t dwValue);

	// [RECONSTRUCTED - Native 0x0059C820] (1423 bytes)
	// Drains all runtime skill queues, frees persistent wrappers, and resets runtime slots
	void ClearRuntime();

	// [RECONSTRUCTED - Native 0x0059BB80] (1109 bytes)
	// Master simulation tick draining periodic effects, queued damage, deferred status, and active buffs
	void OnTick(float fDeltaSec);

	// [RECONSTRUCTED - Native 0x0059A5E0] (147 bytes)
	// Resets active runtime slot pointers and flags (+0x1D8..+0x224, +0x2F0..+0x2FC)
	void ResetRuntimeSlots();

	void QueueDamageOperation(tagQueuedSkillOperation* pOp) {
		if (pOp) m_listQueuedDamage.push_back(pOp);
	}

	void QueueDeferredStatus(tagDeferredStatusRecord* pRec) {
		if (pRec) m_listDeferredStatus.push_back(pRec);
	}

	void AddPeriodicEffect(void* pEffect) {
		if (pEffect) m_listPeriodicEffects.push_back(pEffect);
	}

private:
	CGObjChar*                            m_pOwner;                // +0x00: Owning character object
	CZoeZoeRnd                            m_zoeRnd;                // +0x04: Embedded dynamic probability tracker (Native 0x00599BE0)
	union {
		uint32_t                          m_dwReserved1B0[10];     // +0x1B0 - +0x1D4: 10 state words cleared on load
		struct {
			uint64_t                      m_loadStateWords[4];     // +0x1B0 - +0x1CF: 4 64-bit state words (Native 0x0059DC00)
			uint32_t                      m_dwStateBit1D0;         // +0x1D0: Selector and condition bitmask (Native 0x0059DDF0)
			uint32_t                      m_dwReserved1D4;         // +0x1D4
		};
	};
	tagActiveSkillInstance*               m_pCurrentInstance;      // +0x1D8: Currently executing skill instance
	tagActiveSkillInstance*               m_pInstance1DC = nullptr;        // +0x1DC: ref +0x2B4 effect instance
	tagActiveSkillInstance*               m_pRuntimeInstance1F0 = nullptr; // +0x1F0: Runtime override instance
	tagActiveSkillInstance*               m_pInstance1F8 = nullptr;        // +0x1F8: ref +0x2CC effect instance
	tagActiveSkillInstance*               m_pInstance214 = nullptr;        // +0x214: ref +0x448 effect instance ("StoneSkill")
	uint32_t                              m_dwOverrideAttackID = 0;        // +0x260: Override attack skill ID
	std::map<uint32_t, tagSkillData*>     m_mapSkill;              // +0x228: Learned skills map indexed by Skill ID
	std::map<uint32_t, tagSkillMasteryData*> m_mapMastery;         // +0x234: Learned masteries map indexed by Mastery ID
	std::map<uint32_t, tagSkillModifier>  m_mapModifiers;          // +0x240: Active skill parameter modifiers
	std::list<tagActiveSkillInstance*>    m_listActiveBuffs;       // +0x268: Active buffs / passive skill instances
	std::list<tagQueuedSkillOperation*>   m_listQueuedDamage;      // Queued deferred damage operations
	std::list<tagDeferredStatusRecord*>   m_listDeferredStatus;    // Queued deferred status effects
	std::list<void*>                      m_listPeriodicEffects;   // Active periodic effects (pulses)
	std::vector<uint32_t>                 m_vecRetiredContextIDs;  // +0x298: Queue of expired buff context IDs to publish
	std::vector<uint32_t>                 m_vecPendingMasteryIDs;  // +0x2A8: Queue of pending learned mastery updates
	std::vector<uint32_t>                 m_vecPendingSkillIDs;    // +0x2B8: Queue of pending learned skill updates
	uint32_t                              m_dwReserved2EC;         // +0x2EC: State word cleared on load
	SkillRealModifiers m_realModifiers;
};

// Global / helper forwarder for Formulae and external callers
const tagSkillModifier* CSkillManager_GetSkillModifier(const CSkillManager* pSkillManager, uint32_t dwModifierID);

#endif // _SR_GAMESERVER_SKILLMANAGER_H_

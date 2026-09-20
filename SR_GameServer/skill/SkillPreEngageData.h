/**
 * ============================================================================
 * Silkroad Online - Skill pre-engage data (targeting parameters of one skill use)
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\Skill*.h
 *
 * Skill::sSkillPreEngageData
 *   - Native VTable @ 0x00AFE2C8, RTTI .?AUsSkillPreEngageData@Skill@@
 *   - Constructor @ 0x005A9610, destructor @ 0x005AFA70 (deleting 0x005AA0F0), size 0x5C
 *   - Pool CChunkAllocatorST @ 0x00CE2B18: Allocate 0x005AA0B0, Release 0x005AA0C0
 *
 * CORRECTION (Claude): this is the object the port called both CActionTargetContext (actor side)
 * and tagSkillCommand (skill side). The command actor fills it from the client 0x7074 packet,
 * serialises it into the internal 0x7070 message, and the skill manager deserialises it again
 * and keeps it on the running skill instance (+0x14).
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_SKILL_SKILLPREENGAGEDATA_H_
#define _SR_GAMESERVER_SKILL_SKILLPREENGAGEDATA_H_

#include <cstdint>
#include <memory>
#include <vector>

class CMsg;

// Target entry (8 bytes, verified @ 0x0058CC70), appended by 0x0048DBB0.
// ReadFromMsg (0x005AA51E) stores mode 0 and attenuation 0x64.
struct tagTargetCandidate {
	uint32_t dwGlobalID;      // +0x00: Entity global ID
	union {
		struct {
			uint8_t byMode;        // +0x04: Target mode
			uint8_t byAttenuation; // +0x05: Power / attenuation level
			uint16_t wPad;         // +0x06 - +0x07
		};
		struct {
			uint8_t byAttenuationLegacy;
			uint8_t pad[3];
		};
	};

	tagTargetCandidate() : dwGlobalID(0), byMode(0), byAttenuation(0), wPad(0) {}
	tagTargetCandidate(uint32_t id, uint8_t att = 0) : dwGlobalID(id), byMode(0), byAttenuation(att), wPad(0) {}
	tagTargetCandidate(uint32_t id, uint8_t mode, uint8_t att) : dwGlobalID(id), byMode(mode), byAttenuation(att), wPad(0) {}
	operator uint32_t() const { return dwGlobalID; }
};

struct tagActiveSkillInstance;

namespace Skill {

// Native command +1C borrows a callback object (582F93, 650Fxx).
// Portable shared lifetime prevents the native pool-storage assumption from
// becoming a dangling pointer when a scheduler job is heap-deleted.
struct RetirementTrigger {
	virtual ~RetirementTrigger() = default;
	virtual void Notify(int32_t reason, tagActiveSkillInstance* instance) = 0;
};

// Target flags (+0x0C)
enum : uint8_t {
	SKILL_TARGET_FLAG_OBJECT       = 0x01, // one target object id follows
	SKILL_TARGET_FLAG_POSITION     = 0x02, // one region + position follows
	SKILL_TARGET_FLAG_BODY_MODE_6  = 0x10, // set by 0x004ACEAD when the caster's body mode is 6
};

#pragma pack(push, 1)

/**
 * Target position entry (0x0E bytes), appended by 0x005AA6F0.
 */
struct sSkillTargetPos {
	uint16_t m_wRegionID;     // +0x00
	float    m_fX;            // +0x02
	float    m_fY;            // +0x06
	float    m_fZ;            // +0x0A
};

#pragma pack(pop)

/**
 * [RECONSTRUCTED - see file header]
 */
struct sSkillPreEngageData {
	// [RECONSTRUCTED - 0x005A9610]
	sSkillPreEngageData();

	// [RECONSTRUCTED - 0x005AFA70]
	virtual ~sSkillPreEngageData();

	// [RECONSTRUCTED - 0x005AA0B0] Takes an item from the pool and marks it allocated (+0x04 = 1).
	static sSkillPreEngageData* Allocate();

	// [RECONSTRUCTED - 0x005AA0C0] (esi = ppData)
	// Clears and returns the item when it is still marked allocated; always nulls the pointer.
	static void Release(sSkillPreEngageData*& pData);

	// [RECONSTRUCTED - 0x005A95A0] (edi = this)
	void Clear();

	// [RECONSTRUCTED - 0x005AA480] (edi = pMsg, stack this, bSkipSkillID)
	// Reads [skill id unless bSkipSkillID][flags][target id if flag 1][region, x, y, z if flag 2].
	void ReadFromMsg(CMsg* pMsg, int32_t bSkipSkillID);

	// [RECONSTRUCTED - 0x005AA630] (eax = this, ebx = pMsg)
	void WriteToMsg(CMsg* pMsg) const;

	uint8_t                      m_bAllocated;       // +0x04
	uint32_t                     m_dwSkillID;        // +0x08
	uint8_t                      m_byTargetFlags;    // +0x0C
	uint32_t                     m_dwTargetObjID;    // +0x10: first target object id
	uint32_t                     m_dwTargetObjID14;  // +0x14: also the first target object id after ReadFromMsg
	uint32_t                     m_dwDuration;       // +0x18: effect duration (buff remaining time = start + this - now, 0x0059CF40)
	std::shared_ptr<RetirementTrigger> m_retirementTrigger; // Native +1C
	uint32_t                     m_dw20;             // +0x20
	uint32_t                     m_dw24;             // +0x24
	uint32_t                     m_dw28;             // +0x28
	uint16_t                     m_w2C;              // +0x2C
	float                        m_f30;              // +0x30
	float                        m_f34;              // +0x34
	float                        m_f38;              // +0x38
	std::vector<tagTargetCandidate> m_vecTargets;    // +0x3C
	std::vector<sSkillTargetPos> m_vecTargetPos;     // +0x4C
};

} // namespace Skill

#endif // _SR_GAMESERVER_SKILL_SKILLPREENGAGEDATA_H_

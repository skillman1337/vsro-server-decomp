/**
 * ============================================================================
 * Silkroad Online - Character Auto Command Actor (client action queue)
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GCharAutoCommandActor.h
 *
 * CGCharAutoCommandActor
 *   - Native VTable @ 0x00AECD0C, RTTI .?AVCGCharAutoCommandActor@@
 *   - Embedded at CGObjChar + 0x1BC0 (constructor 0x004AC9F0, destructor 0x004ACA90)
 *   - ProcessCommand @ 0x004ACC40 (client 0x7074), Update @ 0x004AD7A0 (CGObjChar::OnTick)
 *
 * The actor turns 0x7074 requests into queued sAutoCommand records. Each record runs a small
 * state machine in the handler for its type; the attack and skill handlers finish by posting the
 * internal 0x7070 message through the owner's command source, which the skill manager executes
 * on the next message pump.
 *
 * CORRECTION (Claude): the record is sAutoCommand (RTTI .?AUsAutoCommand@@, was CActionRecord),
 * its targeting data is Skill::sSkillPreEngageData (was CActionTargetContext), and the action
 * types are attack / pick up / trace / cast / dispel (types 2, 3 and 5 were labelled item use,
 * cancel and ground skill).
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GCHARAUTOCOMMANDACTOR_H_
#define _SR_GAMESERVER_GCHARAUTOCOMMANDACTOR_H_

#include <cstdint>
#include <list>
#include "skill/SkillPreEngageData.h"

class CGObjChar;
class CGCharAutoCommandActor;
class CMsg;
struct tagRefSkill;

// First byte of 0x7074
enum : uint8_t {
	AUTO_COMMAND_REQUEST_EXECUTE             = 1,
	AUTO_COMMAND_REQUEST_CANCEL              = 2, // 0x004ACCA2
	AUTO_COMMAND_REQUEST_CANCEL_AND_EXECUTE  = 3, // 0x004ACD1B
};

// sAutoCommand::m_byType (second byte of 0x7074), indexes the handler table at actor +0x18
enum : uint8_t {
	AUTO_COMMAND_ATTACK  = 1, // 0x004AD8E0
	AUTO_COMMAND_PICKUP  = 2, // 0x004AE0C0
	AUTO_COMMAND_TRACE   = 3, // 0x004AE3D0
	AUTO_COMMAND_CAST    = 4, // 0x004AE590
	AUTO_COMMAND_DISPEL  = 5, // 0x004AE520
};

// sAutoCommand::m_dwState
enum : uint32_t {
	AUTO_COMMAND_STATE_BEGIN    = 0, // validate and compute the engage range
	AUTO_COMMAND_STATE_APPROACH = 1, // pursue until in range
	AUTO_COMMAND_STATE_EXECUTE  = 2, // post the command message (0x7070 / 0x7034)
	AUTO_COMMAND_STATE_KEEP_UP  = 3, // wait for the action to play out, chain or repeat
	AUTO_COMMAND_STATE_QUEUED   = 5, // set by ProcessCommand; Update switches it to BEGIN
};

// Response types of 0xB074
enum : uint8_t {
	AUTO_COMMAND_RESPONSE_ACCEPTED = 1,
	AUTO_COMMAND_RESPONSE_FINISHED = 2,
	AUTO_COMMAND_RESPONSE_REFUSED  = 3,
};

/**
 * sAutoCommand
 * Native VTable @ 0x00AECD04, RTTI .?AUsAutoCommand@@, size 0x34
 * Pool CChunkAllocatorST @ 0x00CCF0B0: Allocate 0x004AC910, Release 0x004AC920
 */
struct sAutoCommand {
	// [RECONSTRUCTED - inlined by the block constructor 0x004AFD20]
	sAutoCommand();

	// [RECONSTRUCTED - 0x004AC980]
	virtual ~sAutoCommand();

	// [RECONSTRUCTED - 0x004AC910]
	static sAutoCommand* Allocate();

	// [RECONSTRUCTED - 0x004AC920] (stack ppCommand)
	// Releases the pre-engage data, resets the record and returns it to the pool when it is still
	// marked allocated; always nulls the pointer, so releasing twice is harmless.
	static void Release(sAutoCommand*& pCommand);

	// [RECONSTRUCTED - 0x004AC9E0] (eax = dwState, esi = this)
	void SetState(uint32_t dwState);

	uint8_t                      m_bAllocated;      // +0x04
	uint8_t                      m_byType;          // +0x05
	const tagRefSkill*           m_pRefSkill;       // +0x08
	uint32_t                     m_dwDispelParam;   // +0x0C: read after the skill id for dispels of skills with Lnks (+0x370)
	uint32_t                     m_dwStateTick;     // +0x10
	float                        m_fMinRange;       // +0x14
	float                        m_fMaxRange;       // +0x18
	uint32_t                     m_dwState;         // +0x1C
	const tagRefSkill*           m_pChainSkill;     // +0x20: current step of a chained skill
	uint32_t                     m_dwChainStepTime; // +0x24
	uint32_t                     m_dwLatencyBudget; // +0x28: 500 ms taken out of the chain step times
	uint32_t                     m_dwLatencyUsed;   // +0x2C: added back before chaining into an attack
	Skill::sSkillPreEngageData*  m_pPreEngage;      // +0x30
};

class CGCharAutoCommandActor {
public:
	typedef int32_t (CGCharAutoCommandActor::*PFN_HANDLER)(sAutoCommand* pCommand, uint16_t* pwErrorCode);

	// [RECONSTRUCTED - 0x004AC9F0]
	CGCharAutoCommandActor();

	// [RECONSTRUCTED - 0x004ACA90]
	virtual ~CGCharAutoCommandActor();

	// [RECONSTRUCTED - inlined by CGObjChar construction]
	void SetOwner(CGObjChar* pOwner) { m_pOwner = pOwner; }
	CGObjChar* GetOwner() const { return m_pOwner; }

	// [RECONSTRUCTED - 0x004ACC40] (ebx = this, stack pMsg, bRefuse)
	void ProcessCommand(CMsg* pMsg, int32_t bRefuse);

	// [RECONSTRUCTED - 0x004AD7A0] (esi = this, ebx = pwErrorCode)
	// Returns 0 when idle or the record was finished, 2 when the handler ended it, 1 otherwise.
	int32_t Update(uint16_t* pwErrorCode);

	// [RECONSTRUCTED - 0x004AD320] (eax = this, stack bStopMovement)
	void CancelActiveAction(int32_t bStopMovement);

	size_t GetCommandCount() const { return m_listCommands.size(); }

private:
	// [RECONSTRUCTED - 0x004AD270] (edi = this, stack byType, wErrorCode)
	void SendResponse(uint8_t byType, uint16_t wErrorCode);

	// [RECONSTRUCTED - 0x004AD390] (eax = this)
	void OnActionFinish();

	// [RECONSTRUCTED - 0x004AD630] (eax = this, stack pCommand)
	// Takes ownership of pCommand: queues it, or releases it when it was executed immediately.
	uint16_t ExecuteAction(sAutoCommand* pCommand);

	// [RECONSTRUCTED - 0x004AD870] (eax = pCommand, edi = this, ebx = pwErrorCode)
	int32_t CheckAndDispatchAction(sAutoCommand* pCommand, uint16_t* pwErrorCode);

	// [RECONSTRUCTED - 0x004AD030] (eax = pCommand, ebx = this)
	int32_t ResolveActionSkill(sAutoCommand* pCommand);

	// [RECONSTRUCTED - 0x004AD230] (eax = pCommand, stack this)
	int32_t HasLearnedSkill(sAutoCommand* pCommand);

	// [RECONSTRUCTED - 0x004AD4A0] (eax = dwTargetID, stack this, nRange)
	int32_t IsTargetInRange(uint32_t dwTargetID, int32_t nRange);

	// [RECONSTRUCTED - 0x004AD3E0] (ecx = pTarget, stack this)
	float GetDistanceToEntity(CGObjChar* pTarget);

	// [RECONSTRUCTED - 0x004AD780] (ecx = this, eax = bFlag)
	void StopMovement(int32_t bFlag);

	// [RECONSTRUCTED - 0x004AF3C0] (ecx = this, esi = pCommand)
	void ApplyWeaponStructureRange(sAutoCommand* pCommand);

	// [RECONSTRUCTED - 0x004AF430] (ecx = this, esi = pCommand)
	void ApplyWeaponRangeScale(sAutoCommand* pCommand);

	// [RECONSTRUCTED - inlined in 0x004ADA88, 0x004ADF13 and 0x004AE82E]
	void AddSkillEngageRange(sAutoCommand* pCommand, const tagRefSkill* pRefSkill);

	// [RECONSTRUCTED - inlined in 0x004ADD44 and 0x004AEB69]
	int32_t PostSkillActionMsg(sAutoCommand* pCommand);

	// [RECONSTRUCTED - 0x004AD8E0]
	int32_t Handler_Attack(sAutoCommand* pCommand, uint16_t* pwErrorCode);

	// [RECONSTRUCTED - 0x004AE0C0]
	int32_t Handler_Pickup(sAutoCommand* pCommand, uint16_t* pwErrorCode);

	// [RECONSTRUCTED - 0x004AE3D0]
	int32_t Handler_Trace(sAutoCommand* pCommand, uint16_t* pwErrorCode);

	// [RECONSTRUCTED - 0x004AE590]
	int32_t Handler_Cast(sAutoCommand* pCommand, uint16_t* pwErrorCode);

	// [RECONSTRUCTED - 0x004AE520]
	int32_t Handler_Dispel(sAutoCommand* pCommand, uint16_t* pwErrorCode);

private:
	std::list<sAutoCommand*> m_listCommands;      // +0x04 (size at +0x0C)
	CGObjChar*               m_pOwner;            // +0x10
	uint32_t                 m_dwLastUpdateTick;  // +0x14
	uint32_t                 m_dwLastCommandTick; // +0x18
	PFN_HANDLER              m_apfnHandler[6];    // +0x1C - +0x33: index = type - 1 (native reads [+0x18 + type * 4])
};

#endif // _SR_GAMESERVER_GCHARAUTOCOMMANDACTOR_H_

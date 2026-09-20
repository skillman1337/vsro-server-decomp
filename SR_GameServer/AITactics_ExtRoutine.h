/**
 * ============================================================================
 * Silkroad Online - AI Tactics Extended Routine
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\AITactics_ExtRoutine.h
 *
 * Implements:
 *   - AI::CTactics @ 0x00AF879C (Inherits from CCmdSource, Size: 0x1D0 = 464 bytes)
 *   - Extended target acquisition, party aggro distribution, and state hooks
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_AITACTICS_EXTROUTINE_H_
#define _SR_GAMESERVER_AITACTICS_EXTROUTINE_H_

#include <cstdint>
#include <cstring>
#include <vector>
#include "GObjChar.h"
#include "AIHive.h"
#include "../ServerCommon/ReferenceData.h"

class CGObjChar;
struct tagRefTactics;

namespace AI {
class CTactics;

class CAISkill {
public:
	virtual ~CAISkill() = default;
	virtual void* GetCandidateSkill() { return nullptr; }
	virtual void* SelectSkill(CTactics* /*pTactics*/, CGObjChar* /*pTarget*/) { return nullptr; }
	virtual bool ExecuteSkill(CTactics* /*pTactics*/, CGObjChar* /*pTarget*/, void* /*pSkillData*/) { return true; }
	virtual uint32_t GetInterval() const { return m_dwInterval; }

public:
	uint32_t m_dwInterval = 1500; // +0x48: Skill interval in ms
	uint8_t  m_byChannelFlag = 0; // +0x4C
};
} // namespace AI

/**
 * [RECONSTRUCTED - 0x00AF880C]
 * CAIState
 * Base finite state machine state controller.
 * Size: 0x1C (28 bytes)
 */
class CAIState {
public:
	virtual ~CAIState() = default;

	// Virtual slot 1 (+0x04) @ 0x0053DC40
	virtual int32_t GetStateId() const { return m_wStateId; }

	// Virtual slot 2 (+0x08) @ 0x00559C00
	virtual int32_t OnEnter(int32_t arg1, int32_t arg2) { (void)arg1; (void)arg2; return 0; }

	// Virtual slot 3 (+0x0C) @ 0x00559C40
	virtual int32_t OnTick() { return 0; }

	// Virtual slot 4 (+0x10) @ 0x00559C70
	virtual int32_t OnExit() { return 0; }

public:
	AI::CTactics* m_pTactics = nullptr; // +0x04: Owning tactics controller
	uint16_t      m_wStateId = 0;       // +0x08
	uint16_t      pad0A = 0;            // +0x0A
	uint32_t      m_dwTimer = 0;        // +0x0C
	uint32_t      pad10 = 0;            // +0x10
	uint32_t      pad14 = 0;            // +0x14
	uint32_t      m_nSubState = 0;      // +0x18
};

namespace AI {

using ::CAIState;

/**
 * [RECONSTRUCTED - 0x0054A710 / 0x00B5875C]
 * CFactoryObj
 * Native base for reference-counted objects managed by CRefClassFactory.
 * VTable: 0x00AF9668 (subobject base)
 */
class CFactoryObj {
public:
	CFactoryObj() : m_dwID(0), m_nRefCount(0), m_pFactory(nullptr) {}
	virtual ~CFactoryObj() = default;

	// Virtual slot 1 (+0x04) @ 0x0054A6C0: Reset
	virtual int32_t Reset() {
		m_dwID = 0;
		m_nRefCount = 0;
		m_pFactory = nullptr;
		return 0;
	}

	// Virtual slot 2 (+0x08) @ 0x0054A6D0: Release / DecRef
	virtual int32_t Release() {
		if (m_nRefCount > 0) {
			--m_nRefCount;
			if (m_nRefCount == 0) {
				m_pFactory = nullptr;
			}
			return m_nRefCount;
		}
		return 0;
	}

	void AddRef() { ++m_nRefCount; }

public:
	uint32_t m_dwID        = 0;       // +0x04
	uint32_t m_nRefCount   = 0;       // +0x08
	void*    m_pFactory    = nullptr; // +0x0C
};

/**
 * [RECONSTRUCTED - 0x00AF9668 / 0x0054A730 / 0x005534C0]
 * CAIMsg
 * Asynchronous AI message dispatched between tactical state machines.
 * Total size: 0x120 (288 bytes)
 */
class CAIMsg : public CFactoryObj {
public:
	CAIMsg()
		: CFactoryObj()
		, m_wMsgID(0)
		, m_wSubID(0)
		, m_nCapacity(0x100)
		, m_dwSenderID(0)
		, m_nPayloadSize(0) {
		memset(m_byPayload, 0, sizeof(m_byPayload));
	}
	virtual ~CAIMsg() override = default;

	// Virtual slot 3 (+0x0C) @ 0x0054A7F0: ResetPayload
	virtual int32_t ResetPayload() {
		m_dwSenderID = 0;
		m_nPayloadSize = 0;
		m_dwReadOffset = 0;
		m_wSubID = 0;
		return 0;
	}

	uint16_t GetMsgID() const { return m_wMsgID; }

	bool WritePayload(const void* pSrc, size_t nBytes) {
		if (pSrc == nullptr || m_nPayloadSize + nBytes > sizeof(m_byPayload)) {
			return false;
		}
		std::memcpy(m_byPayload + m_nPayloadSize, pSrc, nBytes);
		m_nPayloadSize += static_cast<uint32_t>(nBytes);
		return true;
	}

	bool ReadPayload(void* pDst, size_t nBytes) {
		if (pDst == nullptr || m_dwReadOffset + nBytes > m_nPayloadSize) {
			return false;
		}
		std::memcpy(pDst, m_byPayload + m_dwReadOffset, nBytes);
		m_dwReadOffset += static_cast<uint32_t>(nBytes);
		return true;
	}

	void ResetReadCursor() {
		m_dwReadOffset = 0;
	}

public:
	uint16_t m_wMsgID;             // +0x10
	uint16_t m_wSubID;             // +0x12
	uint32_t m_nCapacity;          // +0x14: 0x100 (256 bytes)
	union {
		uint32_t m_dwSenderID;     // +0x18
		uint32_t m_dwReadOffset;   // +0x18: Read cursor during stream reading
	};
	uint32_t m_nPayloadSize;       // +0x1C
	union {
		uint8_t  m_byPayload[0x100];   // +0x20 - +0x11F
		uint32_t m_dwTargetID;         // +0x20: Target GameID in payload
	};
};

/**
 * [RECONSTRUCTED - 0x0053D4B0]
 * tagAITimerEntry
 * Size: 0x0C (12 bytes)
 */
struct tagAITimerEntry {
	uint32_t m_dwTimerID    = 0; // +0x00
	uint32_t m_dwInterval   = 0; // +0x04: Period or timeout in milliseconds
	uint32_t m_dwTargetTime = 0; // +0x08: Expire timestamp
};

/**
 * [RECONSTRUCTED - 0x00AF86C8 / 0x0053D3B0 / 0x0053D420 / 0x0053D590]
 * CAITimeManager
 * VTable: 0x00AF86C8
 * Size: 0x2C (44 bytes)
 */
class CAITimeManager {
public:
	CAITimeManager()
		: m_dwMaxTimers(9)
		, m_vecActiveTimers()
		, m_vecPendingTimers()
		, m_nActiveTimerCount(0) {
		m_vecActiveTimers.resize(9);
	}
	virtual ~CAITimeManager() {
		ClearTimers();
	}

	void ClearTimers() {
		m_vecActiveTimers.clear();
		m_vecPendingTimers.clear();
		m_nActiveTimerCount = 0;
	}

	void SetTimer(uint32_t dwTimerID, uint32_t dwIntervalMs);
	bool IsTimerExpired(uint32_t dwTimerID, uint32_t dwCurTick);
	void ArmTimer(uint32_t dwTimerID, uint32_t dwInterval, uint32_t dwRandomRange, uint32_t dwCurTick, bool bSpecial);
	void ClearTimer(uint32_t dwTimerID);

public:
	uint32_t                     m_dwMaxTimers;       // +0x04: 9
	std::vector<tagAITimerEntry> m_vecActiveTimers;   // +0x08 - +0x17 (16 bytes)
	std::vector<tagAITimerEntry> m_vecPendingTimers;  // +0x18 - +0x27 (16 bytes)
	uint32_t                     m_nActiveTimerCount; // +0x28
};

// State transition handler function pointer
typedef int32_t (*PFN_AI_STATE_HANDLER)(CTactics* pTactics, void* pContext);

/**
 * [RECONSTRUCTED - 0x005473C0]
 * tagAggroRecord
 * Aggro tracking record for primary and secondary attackers.
 */
struct tagAggroRecord {
	uint32_t m_dwTargetID    = 0; // +0x00: Target GameID
	uint32_t m_dwTotalDamage = 0; // +0x04: Accumulated damage dealt
	int32_t  m_nAggroScore   = 0; // +0x08: Aggro threat score
	uint32_t m_dwLastHitTime = 0; // +0x0C: Timestamp of last damage dealt
};

/**
 * [RECONSTRUCTED - 0x00AF879C / 0x0053EA60 / 0x0053F130] (Size: 0x1D0 = 464 bytes)
 * CTactics
 * Monster AI tactics controller holding aggro, roaming, and combat states.
 * VTable: 0x00AF879C (26 virtual function slots)
 */
class CTactics {
public:
	// Native @ 0x0053EA60
	CTactics();

	// Native @ 0x0053F130 (Virtual slot 0 @ 0x0054E640)
	virtual ~CTactics();

	// Virtual slot 1 (+0x04) @ 0x0040B720
	virtual void PostCommand(void* pPacket);

	// Virtual slot 2 (+0x08) @ 0x0040A940
	virtual void SetOwnerChar(CGObjChar* pChar);

	// Virtual slot 3 (+0x0C) @ 0x009BF500
	virtual void StubSlot3() {}

	// Virtual slot 4 (+0x10) @ 0x0040AA50
	virtual void FlushCommands();

	// Virtual slot 5 (+0x14) @ 0x0053FD70
	virtual AI::CAIMsg* CreateAIMessage(uint16_t wMsgID);

	// Virtual slot 6 (+0x18) @ 0x0053FDA0
	virtual bool PostAIMessage(AI::CAIMsg* pMsg);

	// Virtual slot 7 (+0x1C) @ 0x0053FDD0
	virtual bool OnCurrentStateTick();

	// Virtual slot 8 (+0x20) @ 0x0053F3B0
	virtual void RequestRemoval();

	// Virtual slot 9 (+0x24) @ 0x0053D840
	virtual int32_t QueryPoolIndex() const { return GetPoolIndex(); }

	// Virtual slot 10 (+0x28) @ 0x005ECFD0
	virtual bool PreInitHook() { return true; }

	// Virtual slot 11 (+0x2C) @ 0x0040A950
	virtual void FlushAndClear() { FlushCommands(); }

	// Virtual slot 12 (+0x30) @ 0x0053F2A0 (267 bytes, retn 0x14)
	// CORRECTION (Claude): the third argument is the hatching AI::CNest (stored at +0x30 and read as a nest by
	// 0x0053FEE7, 0x0054135A, 0x00545FA9, 0x005461B4), and pPos is a 0x18-byte tagObjLocation.
	virtual bool BindContext(CGObjChar* pOwner, tagRefTactics* pRefTactics, CNest* pNest, const tagObjLocation* pPos, float fRoamRadius);

	// Virtual slot 13 (+0x34) @ 0x0053F3E0
	virtual void Shutdown();

	// Virtual slot 14 (+0x38) @ 0x0053FEA0
	virtual bool Update();

	// Virtual slot 15 (+0x3C) @ 0x0053F4D0
	virtual bool Initialize();

	// Virtual slot 16 (+0x40) @ 0x0053FB90
	virtual bool InitializeSkillsAndRadii();

	// Virtual slot 17 (+0x44) @ 0x0053FC00
	virtual bool InstallStrategyTable();

	// Virtual slot 18 (+0x48) @ 0x005471B0
	virtual int32_t ConfigureFollowState();

	// Virtual slot 19 (+0x4C) @ 0x00547200
	virtual int32_t ConfigureWanderState();

	// Virtual slot 20 (+0x50) @ 0x00547240
	virtual int32_t ResetStateHandlers();

	// Virtual slot 21 (+0x54) @ 0x00547280
	virtual void SetStateHook(int32_t nState);

	// Virtual slot 22 (+0x58) @ 0x0053D850
	virtual void SetPoolIndex(int32_t nIndex);

	// Virtual slot 23 (+0x5C) @ 0x00823DB0
	virtual int32_t GetPoolIndex() const;

	// Virtual slot 24 (+0x60) @ 0x0053FE00
	virtual void ReceiveAIMessage(CAIMsg* pMsg);

	// Virtual slot 25 (+0x64) @ 0x005400C0
	virtual float GetEffectiveLeashRadius() const;

	// [RECONSTRUCTED - Native 0x0053FD40] (47 bytes, esi = this)
	// CORRECTION (Claude): formerly CancelAction; it reports the owner to m_pNest->OnMonsterDead and clears the nest.
	void DetachNest();

	// [RECONSTRUCTED - Native 0x00540D20] (88 bytes)
	bool CheckAggroOrStateTransition();

	// [RECONSTRUCTED - Native 0x0053D860] (54 bytes, eax = this): -1 before the interval, else IsPCInOwnerMsgBlock()
	// CORRECTION (Claude): formerly CheckAggroSearchInterval; nothing here searches for an aggro target.
	int32_t CheckPCInOwnerMsgBlock();

	// [RECONSTRUCTED - Native 0x0053FE30] (72 bytes)
	bool ProcessPendingAIMessage();

	// [STUB - Native 0x00540E60] (117 bytes, esi = this)
	// CORRECTION (Claude): formerly SearchAggroTarget; the native counts players in the owner's CMsgBlock.
	bool IsPCInOwnerMsgBlock();

	// [RECONSTRUCTED - Native 0x0053FFE0] (116 bytes)
	bool SetCombatTarget(CGObjChar* pTarget, uint8_t byMode);

	// [RECONSTRUCTED - Native 0x0053FB40] (148 bytes)
	void ChangeState(int32_t nStateID, int32_t nSubStateID);

	// Supporting message handlers
	void OnMsg_Attacked(CAIMsg* pMsg);        // 0x00540120
	void OnMsg_SquadTargetHelp(CAIMsg* pMsg); // 0x00540340
	void OnMsg_Assist(CAIMsg* pMsg);          // 0x00540430
	void OnMsg_Command(CAIMsg* pMsg);         // 0x00540500

	// [RECONSTRUCTED - Native 0x00540CB0] (58 bytes)
	void UnregisterSquadTarget();

	// [RECONSTRUCTED - Native 0x00541DC0] (48 bytes)
	void UpdateSkillAttackRange(CGObjChar* pTarget, void* pSkillData);

	// [RECONSTRUCTED - Native 0x005411A0] (84 bytes)
	void BroadcastActionPacket(CGObjChar* pTarget, void* pSkillData);

	// [RECONSTRUCTED - Native 0x00541200]
	int32_t CallForHelp();

	// [RECONSTRUCTED - Native 0x005415A0]
	int32_t NotifyLinkedHelp();

public:
	CGObjChar*            m_pOwner;                // +0x04: Owned character instance
	uint8_t               pad_cmdqueue[0x20];      // +0x08 - +0x27: CCmdSource message queue
	int32_t               m_nPoolIndex;            // +0x28: Preallocated pool index (1 - 50,000)
	tagRefTactics*        m_pRefTactics;           // +0x2C: _RefTactics row
	CNest*                m_pNest;                 // +0x30: hatching nest (BindContext 0x0053F2C6); NULL for non-nest spawns
	CAITimeManager        m_timeManager;           // +0x34 - +0x5F: CAITimeManager timer instance
	std::vector<CAIState*> m_vecStates;            // +0x60 - +0x6F: 16 AI state objects
	std::vector<CAISkill*> m_vecSkills;            // +0x70 - +0x7F: Available monster skill objects
	std::vector<uint32_t> m_vecAggroTargets;       // +0x80 - +0x8F: Aggro candidate character IDs
	CAIState*             m_pCurrentState;         // +0x90: Active AI state object
	uint32_t              m_dwActiveStateFlag;     // +0x94: 1 if tactics is actively processing
	CAIMsg*               m_pPendingMsg;           // +0x98: Pending queued AI message
	CAISkill*             m_pActiveSkill;          // +0x9C: Currently executing skill routine
	CAISkill*             m_pBackupSkill;          // +0xA0: Backup / alternate skill routine
	uint8_t               pad_a4[0x18];            // +0xA4 - +0xBB
	tagAggroRecord        m_primaryAggro;          // +0xBC - +0xCF: Primary threat target
	tagAggroRecord        m_secondaryAggro;        // +0xD0 - +0xE3: Secondary threat target
	int32_t               m_nApproachSlot;         // +0xE4: Assigned surround approach slot (0..7, or -1 if none)
	uint8_t               pad_e8[4];               // +0xE8 - +0xEB
	PFN_AI_STATE_HANDLER  m_pfnApproach;           // +0xEC
	PFN_AI_STATE_HANDLER  m_pfnFollow;             // +0xF0
	uint8_t               pad_f4[4];               // +0xF4 - +0xF7
	PFN_AI_STATE_HANDLER  m_pfnWander;             // +0xF8
	PFN_AI_STATE_HANDLER  m_pfnFlee;               // +0x100
	PFN_AI_STATE_HANDLER  m_pfnReturnToSpawn;      // +0x108
	PFN_AI_STATE_HANDLER  m_pfnApproachTarget;     // +0x110
	PFN_AI_STATE_HANDLER  m_pfnCombatMelee;        // +0x114
	PFN_AI_STATE_HANDLER  m_pfnCombatRanged;       // +0x11C
	uint8_t               pad_120[0x28];           // +0x120 - +0x147
	uint8_t               m_byUnk148;              // +0x148: byte, cleared by DetachNest (0x0053FD67) and ChangeState (0x0053FF6A)
	uint8_t               pad_149[3];              // +0x149 - +0x14B
	uint32_t              m_dwPendingActionID;     // +0x14C
	uint32_t              m_dwNextSearchDelayMs;   // +0x150: Dynamic search delay interval
	uint32_t              m_dwLastSearchTimestamp; // +0x154: Timestamp of last target search
	float                 m_fAggroRadius;          // +0x158: Mode 0: Default Aggro Radius
	float                 m_fSearchRadius;         // +0x15C: Mode 1: Search / Detection Radius
	float                 m_fLeashRadius;          // +0x160: Mode 2: Max Roam / Leash Radius
	int32_t               m_nRoamDirection;        // +0x164: +1 or -1 wander direction
	uint8_t               m_byAggroMode;           // +0x168: Aggressive or passive flag
	uint8_t               pad_169[0x1B];           // +0x169 - +0x183
	float                 m_fSpawnX;               // +0x184: Spawn anchor X
	float                 m_fSpawnY;               // +0x188: Spawn anchor Y
	float                 m_fSpawnZ;               // +0x18C: Spawn anchor Z
	uint16_t              m_wSpawnRegionID;        // +0x190: Spawn anchor region
	uint8_t               pad_192[0x0A];           // +0x192 - +0x19B
	float                 m_fHomeX;                // +0x19C: Current home X
	float                 m_fHomeY;                // +0x1A0: Current home Y
	float                 m_fHomeZ;                // +0x1A4: Current home Z
	uint16_t              m_wHomeRegionID;         // +0x1A8: Current home region
	uint8_t               pad_1aa[0x0A];           // +0x1AA - +0x1B3
	tagObjLocation        m_NestPos;               // +0x1B4 - +0x1CB: m_pNest->m_Pos or the bind position (0x0053F2E0..0x0053F30C)
	float                 m_fRoamRadius;           // +0x1CC: 0x0053F36D fstp dword [esi+0x1CC]
};

// Extended routines from AITactics_ExtRoutine.cpp:
int32_t AI_CTactics_EvaluateCandidateTarget(CTactics* pTactics, uint8_t byFilter, void* pCandidateList, CGObjChar* pTarget, const float* pMaxDist, float* pOutDist); // 0x00547070
int32_t AI_CTactics_Battle_TryAction(CTactics* pTactics, CGObjChar* pTarget);                                                                                       // 0x005472A0
int32_t AI_CTactics_Aggro_RecordDamage(CTactics* pTactics, const uint32_t* pDamageInfo, uint8_t* pOutSlot);                                                        // 0x005473C0
int32_t AI_CTactics_PartyAggro_Distribute(CTactics* pTactics, const uint32_t* pTargetData);                                                                        // 0x00547570
int32_t AI_CTactics_SyncMovementSpeed(CTactics* pTactics);                                                                                                          // 0x00547860
int32_t AI_CTactics_ScanForTargets(CTactics* pTactics, void* pContext, uint32_t dwParam);                                                                          // 0x005478F0
int32_t AI_CTactics_ScanForTargets_Special(CTactics* pTactics, void* pContext, uint32_t dwParam);                                                                  // 0x00547AD0
int32_t AI_CTactics_StateIdleWander(CTactics* pTactics, void* pContext);                                                                                            // 0x00547C70
int32_t AI_CTactics_StateCustomSpawn(CTactics* pTactics, void* pContext);                                                                                           // 0x00547F40

} // namespace AI

#endif // _SR_GAMESERVER_AITACTICS_EXTROUTINE_H_

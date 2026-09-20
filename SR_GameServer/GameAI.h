/**
 * ============================================================================
 * Silkroad Online - Game Server AI Subsystem
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GameAI.h
 *
 * Implements CGameAI and AI subsystems:
 *   - CGameAI VTable @ 0x00AF968C (RTTI: .?AVCGameAI@@, inherits .?AV?$CSingletonT@VCGameAI@@@@)
 *   - AI::CTactics VTable @ 0x00AF879C (RTTI: .?AVCTactics@AI@@)
 *   - AI::CInstancePool<AI::CTactics> @ 0x0054D4F0 / 0x0054D560 / 0x0054E620
 *   - CScheduledCallbacker<CGameAI, float> @ 0x0054D810 / 0x0054D840 / 0x0054E6A0
 *   - CQuePool<AI::CAIMsg> @ 0x0054C950 / 0x0054CB90 / 0x0054E730
 *   - AI::CRefClassFactory<AI::CAIMsg> @ 0x0054D9E0 / 0x0054DA60 / 0x0054E6C0
 *   - AI::CSquadManager VTable @ 0x00AF881C @ 0x0054A890
 *   - CPositioner @ 0x0054DEA0 / 0x0054DEF0
 *   - Global singleton pointer g_pGameAI @ 0x00D6A96C
 *   - Global static instance g_gameAI @ 0x00CE1EC0
 *   - Global approach direction vector table g_vApproachDirections[36] @ 0x00CE2030
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GAMEAI_H_
#define _SR_GAMESERVER_GAMEAI_H_

#ifndef _D3DVECTOR_DEFINED
#define _D3DVECTOR_DEFINED
typedef struct _D3DVECTOR {
	float x;
	float y;
	float z;
} D3DVECTOR;
#endif

#include <cstdint>
#include <cstring>
#include <map>
#include <list>
#include <vector>
#include <string>
#include "GObjChar.h"
#include "ScheduledCallbacker.h"
#include "AIHive.h"
#include "AITactics_ExtRoutine.h"
#include "../JMX_Library/MathLib/FStream.h"

// ============================================================================
// CSingletonT Template (Native Joymax Base Template)
// ============================================================================
#ifndef _CSINGLETONT_DEFINED_
#define _CSINGLETONT_DEFINED_
template <typename T>
class CSingletonT {
public:
	CSingletonT() = default;
	virtual ~CSingletonT() = default;
	static T* GetInstance() {
		static T s_instance;
		return &s_instance;
	}
};
#endif

namespace AI {
	class CTactics;
	class CAIMsg;
	class CSquad;
	class CSquadManager;
}

/**
 * [RECONSTRUCTED - 0x007B1090]
 * tagQueBufferState
 * Internal ring buffer and synchronization state embedded in CQuePool.
 * Size: 28 bytes (0x1C)
 */
struct tagQueBufferState {
	int32_t  m_nLockState      = -1;      // +0x00 (+0x10 in CQuePool): Sync flag / spinlock
	void**   m_ppBuffers       = nullptr; // +0x04 (+0x14 in CQuePool): Chunk buffer table
	uint32_t m_nBufferCapacity = 0;       // +0x08 (+0x18 in CQuePool): Allocated chunk count
	uint32_t m_nHead           = 0;       // +0x0C (+0x1C in CQuePool): Head index
	uint32_t m_nItemCount      = 0;       // +0x10 (+0x20 in CQuePool): Active item count
	uint32_t m_nTail           = 0;       // +0x14 (+0x24 in CQuePool): Tail index
};

/**
 * [RECONSTRUCTED - 0x0054C950 / 0x0054CA20 / 0x0054E730]
 * CQuePool<T>
 * Named object pool. Size: 92 bytes (0x5C)
 * CORRECTION (Claude): this was declared as CQue<T>. The real CQue<T> (.?AV?$CQue@PAVCMsg@@@@, ctor 0x0040AAD0)
 * is {vftable, max count, std::deque<T>} (0x1C) and sits at +0x0C inside this pool (0x0054C980). The pool
 * has no RTTI, so CQuePool is a reconstruction name; the members below are not re-verified.
 */
template <typename T>
class CQuePool {
public:
	CQuePool(uint32_t dwFlags = 0, const char* szName = "Unknown")
		: m_dwFlags(dwFlags)
		, m_dwReserved1(0)
		, m_dwCapacity(500)
		, m_bufferState()
		, m_listItems()
		, m_strQueName(szName ? szName : "Unknown") {}

	virtual ~CQuePool() { Clear(); }

	bool Push(const T& item) {
		m_listItems.push_back(item);
		return true;
	}

	bool Pop(T& outItem) {
		if (m_listItems.empty()) {
			return false;
		}
		outItem = m_listItems.front();
		m_listItems.pop_front();
		return true;
	}

	void Clear() {
		m_listItems.clear();
		if (m_bufferState.m_ppBuffers != nullptr) {
			delete[] m_bufferState.m_ppBuffers;
			m_bufferState.m_ppBuffers = nullptr;
		}
		m_bufferState.m_nBufferCapacity = 0;
		m_bufferState.m_nItemCount = 0;
		m_bufferState.m_nHead = 0;
		m_bufferState.m_nTail = 0;
	}

public:
	uint32_t          m_dwFlags;       // +0x04
	uint32_t          m_dwReserved1;   // +0x08
	uint32_t          m_dwCapacity;    // +0x0C: Initialized to 500
	tagQueBufferState m_bufferState;   // +0x10 - +0x2B: Size 28 bytes
	std::list<T>      m_listItems;     // +0x2C - +0x37: Fallback item list
	std::string       m_strQueName;    // +0x38 - +0x5B: Queue debug identifier
};

// tagAIPatrolRegion: map of nests situated inside a specific region ID (Native 0x0054B710: operator new(0x0C))
typedef std::map<uint32_t, AI::CAIHive*> tagAIPatrolRegion;

namespace AI {

// AI::CTactics is canonically defined in AITactics_ExtRoutine.h

/**
 * [RECONSTRUCTED - 0x0054D4F0 / 0x0054D560 / 0x0054E620]
 * CInstancePool<CTactics>
 * Size: 0x24 (36 bytes)
 *
 * Native Memory Layout:
 *   +0x00: VTable pointer (0x00AF9694)
 *   +0x04: T* m_pPoolBuffer (Pointer to contiguous array of preallocated instances)
 *   +0x08: std::list<T*> m_listPool (Available free instances)
 *   +0x14: std::map<uint32_t, T*> m_mapActive (Currently active instances)
 *   +0x20: uint32_t m_nCapacity (Total capacity count: 50,000 / 0xC350)
 */
template <typename T>
class CInstancePool {
public:
	CInstancePool()
		: m_pPoolBuffer(nullptr)
		, m_listPool()
		, m_mapActive()
		, m_nCapacity(0) {}

	virtual ~CInstancePool() { Clear(); }

	bool Initialize(uint32_t nCapacity) {
		Clear();
		m_nCapacity = nCapacity;
		if (nCapacity == 0) {
			return true;
		}
		m_pPoolBuffer = new T[nCapacity];
		for (uint32_t i = 0; i < nCapacity; ++i) {
			T* pInstance = &m_pPoolBuffer[i];
			pInstance->SetPoolIndex(static_cast<int32_t>(i + 1));
			m_listPool.push_back(pInstance);
		}
		return true;
	}

	void Clear() {
		m_listPool.clear();
		m_mapActive.clear();
		if (m_pPoolBuffer != nullptr) {
			delete[] m_pPoolBuffer;
			m_pPoolBuffer = nullptr;
		}
		m_nCapacity = 0;
	}

	bool Release(T* pInstance) {
		if (pInstance == nullptr) {
			return false;
		}
		int32_t nIndex = pInstance->GetPoolIndex();
		m_mapActive.erase(static_cast<uint32_t>(nIndex));
		m_listPool.push_back(pInstance);
		return true;
	}

	T* Acquire() {
		if (m_listPool.empty()) {
			return nullptr;
		}
		T* pInstance = m_listPool.front();
		m_listPool.pop_front();
		return pInstance;
	}

	T* Acquire(uint32_t dwID) {
		if (m_listPool.empty()) {
			return nullptr;
		}
		T* pInstance = m_listPool.front();
		m_listPool.pop_front();
		m_mapActive[dwID] = pInstance;
		return pInstance;
	}

public:
	T*                     m_pPoolBuffer; // +0x04
	std::list<T*>          m_listPool;    // +0x08 - +0x13
	std::map<uint32_t, T*> m_mapActive;   // +0x14 - +0x1F
	uint32_t               m_nCapacity;   // +0x20
};

/**
 * [RECONSTRUCTED - 0x00AF9B30 / 0x0055DD60 / 0x0055DDD0 / 0x00421BD0]
 * CSquad
 * Coordinated monster squad/formation group.
 * Size: 0x40 (64 bytes)
 *
 * Native Memory Layout:
 *   +0x00: VTable pointer (0x00AF9B30)
 *   +0x04: uint32_t m_dwSquadID
 *   +0x08: uint32_t m_dwLeaderID
 *   +0x0C: uint32_t m_nActiveSlots
 *   +0x10: uint32_t m_dwUnk10
 *   +0x14: std::map<uint32_t, CTactics*> m_mapMembers (12 bytes)
 *   +0x20: uint32_t m_dwSlotMembers[8] (32 bytes: each approach slot candidate)
 */
class CSquad {
public:
	// Native @ 0x0055DD60 (97 bytes)
	CSquad();

	// Native @ 0x0055DDD0 (76 bytes)
	virtual ~CSquad();

	// Native @ 0x0055DE20 (62 bytes)
	bool Initialize(uint32_t dwLeaderID, uint32_t dwSquadID);

	// Native @ 0x0053D8A0 (153 bytes)
	bool AddMember(CTactics* pTactics);

	// Native @ 0x0055DC30 (141 bytes)
	bool RemoveMember(CTactics* pTactics);

	// Native @ 0x0055DF10 (371 bytes)
	uint32_t AssignApproachSlot(CTactics* pTactics);

	// Native @ 0x00545660 (30 bytes)
	void ReleaseApproachSlot(uint32_t dwSlotIndex, uint32_t dwExpectedID);

	// Native @ 0x0055DB40 (234 bytes)
	static int32_t NextApproachSlotCandidate(int32_t nCurSlot, int32_t* pStep);

	// Pool tracking
	void SetPoolIndex(int32_t nIndex) { m_nPoolIndex = nIndex; }
	int32_t GetPoolIndex() const { return m_nPoolIndex; }

public:
	uint32_t                    m_dwSquadID;         // +0x04
	uint32_t                    m_dwLeaderID;        // +0x08
	uint32_t                    m_nActiveSlots;      // +0x0C
	int32_t                     m_nPoolIndex;        // +0x10
	std::map<uint32_t, CTactics*> m_mapMembers;      // +0x14 - +0x1F
	uint32_t                    m_dwSlotMembers[8];  // +0x20 - +0x3F: 8 approach slots
};

/**
 * [RECONSTRUCTED - 0x00AF967C / 0x0054A890 / 0x0054A900 / 0x0054AAA0]
 * CSquadManager: AI Coordinated Squad Manager
 * Size: 0xCC (204 bytes)
 *
 * Native Memory Layout:
 *   +0x00: VTable pointer (0x00AF967C)
 *   +0x04: std::map<uint32_t, CSquad*> m_mapSquads (12 bytes)
 *   +0x10: CInstancePool<CSquad> m_poolSquads (92 bytes, capacity: 5,000)
 *   +0x6C: D3DVECTOR m_vApproachDirections[8] (96 bytes: 8 * 12 bytes @ 22.5°, 67.5°, 112.5°, etc.)
 */
class CSquadManager {
public:
	// Native @ 0x0054A890 (98 bytes)
	CSquadManager();

	// Native @ 0x0054A900 (139 bytes)
	virtual ~CSquadManager();

	// Native @ 0x0053D940 (259 bytes)
	CSquad* GetOrCreateSquad(uint32_t dwSquadID, uint32_t dwLeaderID, CTactics* pTactics, bool bParam = false);

	// Native @ 0x0055DCC0 (158 bytes)
	bool DestroySquad(uint32_t dwSquadID);

public:
	std::map<uint32_t, CSquad*> m_mapSquads;              // +0x04 - +0x0F
	CInstancePool<CSquad>       m_poolSquads;             // +0x10 - +0x6B
	D3DVECTOR                   m_vApproachDirections[8]; // +0x6C - +0xCB: 8 approach vectors
};

/**
 * [RECONSTRUCTED - 0x0054D9E0 / 0x0054DA70 / 0x0054E6C0]
 * CRefClassFactory<T>
 * Native VTable @ 0x00AF96A4 (RTTI: .?AV?$CRefClassFactory@VCAIMsg@AI@@@AI@@)
 * Size: 0x7C (124 bytes)
 *
 * Memory Layout:
 *   +0x00: VTable pointer (0x00AF96A4)
 *   +0x04: CQuePool<T*> m_que (92 bytes, 0x5C)
 *   +0x60: std::map<uint32_t, T*> m_mapRefs (12 bytes, 0x0C)
 *   +0x6C: std::list<T*> m_listInstances (12 bytes, 0x0C)
 *   +0x78: uint32_t m_dwAllocCount (4 bytes)
 */
template <typename T>
class CRefClassFactory {
public:
	CRefClassFactory()
		: m_que(0, "AI::CAIMsg")
		, m_mapRefs()
		, m_listInstances()
		, m_dwAllocCount(0) {}

	virtual ~CRefClassFactory() { DestroyAllMessages(); }

	bool Preallocate(uint32_t nCount) {
		if (nCount < 2) return false;
		for (uint32_t i = 0; i < nCount; ++i) {
			T* pObj = new T();
			pObj->m_dwID = static_cast<uint32_t>(m_listInstances.size() + 1);
			m_listInstances.push_back(pObj);
		}
		m_dwAllocCount += nCount;
		return true;
	}

	T* Alloc() {
		if (m_listInstances.empty()) {
			uint32_t nAlloc = m_dwAllocCount ? (m_dwAllocCount >> 1) : 2;
			if (nAlloc < 2) nAlloc = 2;
			if (!Preallocate(nAlloc)) {
				return nullptr;
			}
		}
		T* pObj = m_listInstances.front();
		m_listInstances.pop_front();
		pObj->Reset();
		pObj->m_pFactory = this;
		pObj->AddRef();
		m_mapRefs[pObj->m_dwID] = pObj;
		return pObj;
	}

	void Release(T* pObj) {
		if (pObj == nullptr) return;
		auto it = m_mapRefs.find(pObj->m_dwID);
		if (it != m_mapRefs.end()) {
			m_mapRefs.erase(it);
		}
		m_listInstances.push_back(pObj);
	}

	void DestroyAllMessages() {
		for (auto* pObj : m_listInstances) {
			if (pObj != nullptr) {
				delete pObj;
			}
		}
		for (auto& pair : m_mapRefs) {
			if (pair.second != nullptr) {
				delete pair.second;
			}
		}
		m_listInstances.clear();
		m_mapRefs.clear();
		m_que.Clear();
		m_dwAllocCount = 0;
	}

public:
	CQuePool<T*>           m_que;           // +0x04 - +0x5F
	std::map<uint32_t, T*> m_mapRefs;       // +0x60 - +0x6B
	std::list<T*>          m_listInstances; // +0x6C - +0x77
	uint32_t               m_dwAllocCount;  // +0x78 - +0x7B
};

} // namespace AI

/**
 * [RECONSTRUCTED - 0x0054BFA0 / 0x0054C3D0]
 * tagPositionerRegionNode
 * Holds active and standby positioner tactic lists for a given region.
 * Size: 0x20 (32 bytes)
 *
 * Memory Layout:
 *   +0x00: std::vector<tagRefTactics*> m_vecActive (16 bytes)
 *   +0x10: std::vector<tagRefTactics*> m_vecStandby (16 bytes)
 */
struct tagPositionerRegionNode {
	std::vector<tagRefTactics*> m_vecActive;  // +0x00 - +0x0C: Active tactics list
	std::vector<tagRefTactics*> m_vecStandby; // +0x10 - +0x1C: Standby tactics list

	tagPositionerRegionNode() = default;
	~tagPositionerRegionNode() = default;

	// [RECONSTRUCTED - 0x0054C1C0] (148 bytes)
	tagRefTactics* SelectRandomTactics(bool bStandby);
};

/**
 * [RECONSTRUCTED - 0x0054DEA0 / 0x0054DEF0]
 * CPositionerRegionMap
 * Region waypoint and nest tactics lookup map.
 * In native binary, this is stdext::hash_map<uint32_t, tagPositionerRegionNode*>
 * Size: 0x28 (40 bytes in 32-bit MSVC: list + vector + _Mask + _Maxidx)
 */
class CPositionerRegionMap : public std::map<uint32_t, tagPositionerRegionNode*> {
public:
	CPositionerRegionMap() = default;
	~CPositionerRegionMap() { Clear(); }

	void Clear();
	tagPositionerRegionNode* FindRegionNode(uint32_t dwRegionID);
	tagPositionerRegionNode* InsertRegionNode(uint32_t dwRegionID, tagPositionerRegionNode* pNode);
};

/**
 * [RECONSTRUCTED - 0x00AF96B4 / 0x005552E0]
 * CPositioner: Combat surround and approach slot manager
 * Native VTable @ 0x00AF96B4 (RTTI: .?AVCPositioner@@)
 * Size: 0x28 (40 bytes)
 *
 * Memory Layout:
 *   +0x00: VTable pointer (0x00AF96B4)
 *   +0x04: uint32_t m_nOccupiedSlots (count of occupied slots, 0..8)
 *   +0x08: AI::CTactics* m_pMonsters[8] (8 surround slots spaced 45 deg apart)
 */
class CPositioner {
public:
	// Native @ 0x005552E0 (54 bytes)
	CPositioner();

	// Native @ 0x00555300 (23 bytes)
	virtual ~CPositioner();

	// Native @ 0x00555330 (16 bytes)
	void Reset();

	// Native @ 0x00555340 (274 bytes)
	int32_t AssignApproachSlot(const void* pTargetPos, AI::CTactics* pTactics);

	// Native @ 0x00555460 (32 bytes)
	int32_t AssignApproachSlotHelper(AI::CTactics* pTactics);

	// Native @ 0x00555480 (19 bytes)
	void ClearSlots();

	// Native @ 0x005554A0 (23 bytes)
	void SetSlot(uint32_t nSlot, AI::CTactics* pTactics);

	// Native @ 0x005554B0 (32 bytes)
	void ReleaseSlot(uint32_t nSlot, AI::CTactics* pTactics);

	// Native @ 0x005554D0 (107 bytes)
	int32_t FindAlternativeSlot(uint32_t* pSearchState, uint32_t nPreferredSlot);

public:
	uint32_t       m_nOccupiedSlots; // +0x04
	AI::CTactics*  m_pMonsters[8];   // +0x08 - +0x27: 8 surround slots
};

/**
 * [RECONSTRUCTED - 0x00AF968C / 0x0054AC20]
 * CGameAI
 * Central AI subsystem manager for SR_GameServer.
 *
 * Native Memory Layout (Total Size: 0x16C = 364 bytes):
 *   +0x000: CSingletonT<CGameAI> VTable @ 0x00AF968C
 *   +0x004: AI::CInstancePool<AI::CTactics> m_poolTactics (36 bytes)
 *   +0x028: CScheduledCallbacker<CGameAI, float> m_callbacker (20 bytes)
 *   +0x03C: AI::CSquadManager* m_pSquadManager (4 bytes, proven from 0x0054B1D0)
 *   +0x040: CQuePool<AI::CAIMsg> m_queAIMsg (92 bytes, name: "AI::CMsg")
 *   +0x09C: AI::CRefClassFactory<AI::CAIMsg> m_factoryAIMsg (124 bytes)
 *   +0x118: std::map<uint16_t, tagAIPatrolRegion*> m_mapPatrolRegions (12 bytes)
 *   +0x124: std::map<uint32_t, AI::CTactics*> m_mapActiveAI (12 bytes)
 *   +0x134: std::vector<uint32_t> m_vecTacticsToRemove (12 bytes)
 *   +0x140: CPositioner m_positioner (40 bytes)
 *   +0x168: uint32_t m_dwLastTickTime (4 bytes)
 */
class CGameAI : public CSingletonT<CGameAI> {
public:
	// Native @ 0x0054AC20 (257 bytes)
	CGameAI();

	// Native @ 0x0054AD30 / 0x0054AE40 (245 bytes)
	virtual ~CGameAI() override;

	// Native @ 0x0054B1D0 (bootstrapped via CGame::InitializeAISubsystem @ 0x00413670)
	bool Initialize();

	// Native @ 0x0054AF40 (454 bytes)
	void CleanUp();

	// Native @ 0x0054C6E0 (scheduled patrol callback)
	void OnScheduledPatrolTick();

	// Native @ 0x0054B300 (1026 bytes) - per-tick update
	uint32_t OnTick(float fDeltaTime = 0.0f);

	// Supporting native routines
	bool LoadPositionerData();       // 0x0054BDE0
	tagPositionerRegionNode* FindPositionerRegionNode(uint32_t dwRegionID); // 0x0054C2A0
	tagRefTactics* SelectSpawnTactics(void* pContext, bool bStandby);   // 0x0054C0E0
	void ClearPositionerData();      // 0x0054C300
	bool InitializePatrolRegions();  // 0x0054B710
	bool PopulateRegionNests(uint16_t wRegionID, std::map<uint32_t, AI::CAIHive*>* pMapHives, tagAIPatrolRegion* pPatrolRegion); // 0x0054B8D0
	bool BindNestTactics(uint16_t wGameWorldID, std::map<uint32_t, AI::CAIHive*>* pTempHiveMap, tagAIPatrolRegion* pPatrolRegion); // 0x0054BB60
	bool ValidateAllNestPaths();     // 0x0054C000
	// CORRECTION (Claude): the hive map key is tagRefHive::m_dwHiveID and CAIHive::FindNest (0x0055F080)
	// is keyed by tagRefNest::m_dwNestID; the old parameter names called these nest and tactics IDs.
	AI::CNest* FindNestInRegion(uint16_t wRegionID, uint32_t dwHiveID, uint32_t dwNestID); // 0x0054C450
	AI::CNest* FindNestInAnyHive(uint16_t wRegionID, uint32_t dwNestID); // 0x0054C4F0
	AI::CAIHive* FindHiveInRegion(uint16_t wRegionID, uint32_t dwHiveID); // 0x0054C5B0
	bool PostAIMessage(AI::CAIMsg* pMsg); // 0x0054C660
	AI::CAIMsg* CreateAIMessage(uint16_t wMsgID); // 0x0054C6A0
	bool MarkTacticsForRemoval(uint32_t dwPoolIndex); // 0x0053DA50

public:
	AI::CInstancePool<AI::CTactics>          m_poolTactics;         // +0x004
	CScheduledCallbacker<CGameAI, float>     m_callbacker;          // +0x028
	AI::CSquadManager*                       m_pSquadManager;       // +0x03C
	CQuePool<AI::CAIMsg*>                    m_queAIMsg;            // +0x040
	AI::CRefClassFactory<AI::CAIMsg>         m_factoryAIMsg;        // +0x09C
	std::map<uint16_t, tagAIPatrolRegion*>   m_mapPatrolRegions;    // +0x118
	std::map<uint32_t, AI::CTactics*>        m_mapActiveAI;         // +0x124: Active tactics map
	std::vector<uint32_t>                    m_vecTacticsToRemove;  // +0x134: Pending tactics release queue
	CPositionerRegionMap                     m_positioner;          // +0x140: Spawn tactics positioner map
	uint32_t                                 m_dwLastTickTime;      // +0x168: Last update timestamp
};

// Global singleton instances matching native 0x00CE1EC0 / 0x00D6A96C
extern CGameAI  g_gameAI;
extern CGameAI* g_pGameAI;

// Native 0x00C82620: GetTickCount() cached by CGameAI::Initialize (0x0054B1FB) and CGameAI::OnTick
// (0x0054B315). AIHive timers compare against it, never against a fresh GetTickCount().
extern uint32_t g_dwGameAICurrentTick;

// Global 36 direction vector table @ 0x00CE2030 (computed in 0x0054AB10)
extern D3DVECTOR g_vApproachDirections[36];

// ============================================================================
// AI State Management & Allocation (Native 0x0055B090 / 0x0055B120 / 0x0055B250)
// ============================================================================

/**
 * [RECONSTRUCTED - 0x00AF880C / 0x0055B030]
 * CAIState_SPAWN
 * Native VTable @ 0x00AF880C
 * Base state for monster spawn behaviors. Size: 0x1C (28 bytes)
 */
class CAIState_SPAWN : public CAIState {
public:
	CAIState_SPAWN();
	virtual ~CAIState_SPAWN() override = default;

	virtual int32_t GetStateId() const override { return m_wStateId; }
};

/**
 * [RECONSTRUCTED - 0x00AF88D4 / 0x0055B090]
 * CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN
 * Native VTable @ 0x00AF88D4 (RTTI: .?AVCAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN@@)
 * Special Shytan custom monster summon spawn state. State ID = 0x11 (17).
 * Size: 0x1C (28 bytes)
 */
class CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN : public CAIState_SPAWN {
public:
	CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN();
	virtual ~CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN() override = default;

	virtual int32_t OnTick() override;
};

/**
 * [RECONSTRUCTED - 0x00AF9A8C / 0x0055B0F0 / 0x0055B100]
 * CAIStateAllocator
 * Native VTable @ 0x00AF9A8C (RTTI: .?AVCAIStateAllocator@@)
 * Abstract base allocator interface for AI state machines.
 */
class CAIStateAllocator {
public:
	CAIStateAllocator() = default;
	virtual ~CAIStateAllocator() = default;

	virtual void* Allocate() = 0;
	virtual void Free(void* pState) = 0;
};

/**
 * [RECONSTRUCTED - 0x00AF9A9C / 0x0055B120 / 0x0055B180]
 * CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN
 * Native VTable @ 0x00AF9A9C (RTTI: .?AVCAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN@@)
 * Size: 0x60 (96 bytes)
 *
 * Memory Layout:
 *   +0x00: VTable pointer (0x00AF9A9C)
 *   +0x04: CQuePool<CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN*> m_queStates
 */
class CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN : public CAIStateAllocator {
public:
	CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN();
	virtual ~CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN() override;

	virtual void* Allocate() override;
	virtual void Free(void* pState) override;

public:
	CQuePool<CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN*>    m_queStates;
	std::vector<CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN*> m_allocatedBlocks;
};

/**
 * [RECONSTRUCTED - 0x00AF9AB4 / 0x0055B250 / 0x0055B2E0 / 0x0055B300]
 * CAIStateManager
 * Native VTable @ 0x00AF9AB4 (RTTI: .?AVCAIStateManager@@, inherits CSingletonT<CAIStateManager>)
 * Manages registered custom AI state allocators. Size: 0x10 (16 bytes)
 */
class CAIStateManager : public CSingletonT<CAIStateManager> {
public:
	CAIStateManager();
	virtual ~CAIStateManager() override;

	void CleanUp();
	void ClearAllocators();

	void RegisterAllocator(uint32_t dwStateId, CAIStateAllocator* pAllocator);
	CAIStateAllocator* GetAllocator(uint32_t dwStateId);

public:
	std::vector<CAIStateAllocator*> m_vecAllocators; // +0x04 - +0x0F
};

extern CAIStateManager  g_aiStateManager;
extern CAIStateManager* g_pAIStateManager;

// ============================================================================
// AI Navigation & Dungeon Mesh Data (Native 0x00555F80 / 0x00555A20 / 0x00555530)
// ============================================================================

struct tagBlockMapEntry {
	uint32_t m_dwKey;  // +0x00
	uint16_t m_wVal1; // +0x04
	uint16_t m_wVal2; // +0x06
	uint16_t m_wVal3; // +0x08
	uint16_t m_wPad;  // +0x0A
};

/**
 * [RECONSTRUCTED - 0x00AF992C / 0x00555530]
 * CRefBlock: Navigation collision mesh block
 * Size: 0x24 (36 bytes)
 *
 * VTable @ 0x00AF992C (8 slots):
 *   Slot 0 (+0x00): ~CRefBlock @ 0x005559B0
 *   Slot 1 (+0x04): Initialize @ 0x005555F0
 *   Slot 2 (+0x08): CleanUp @ 0x005556B0
 *   Slot 3 (+0x0C): SetCell @ 0x00555710
 *   Slot 4 (+0x10): SetEntry @ 0x00555740
 *   Slot 5 (+0x14): GetCustomEdge @ 0x00555780
 *   Slot 6 (+0x18): GetCell @ 0x005557C0
 *   Slot 7 (+0x1C): FindEntry @ 0x005557F0
 */
class CRefBlock {
public:
	CRefBlock();
	virtual ~CRefBlock();

	virtual bool Initialize(uint32_t dwIndex, uint32_t dwDimension, uint32_t dwTotalContainEdgeCount);
	virtual void CleanUp();
	virtual bool SetCell(uint32_t x, uint32_t z, uint32_t dwValue);
	virtual bool SetEntry(uint32_t dwIndex, const D3DVECTOR& vEntry);
	virtual D3DVECTOR* GetCustomEdge(uint32_t dwCustomEdgeID);
	virtual uint32_t GetCell(uint32_t x, uint32_t z);
	virtual void* FindEntry(uint32_t dwEntryID);

public:
	uint32_t                             m_dwIndex;                 // +0x04
	uint32_t                             m_dwDimension;             // +0x08
	uint32_t                             m_dwTotalContainEdgeCount; // +0x0C
	uint32_t*                            m_pCells;                  // +0x10: Cell grid [dimension * dimension]
	D3DVECTOR*                           m_pEntries;                // +0x14: Entry points [edgeCount + 1]
	std::map<uint32_t, tagBlockMapEntry> m_mapEntries;              // +0x18 - +0x23
};

/**
 * [RECONSTRUCTED - 0x00AF9950 / 0x00555A20]
 * CRefDungeon: Navigation dungeon sector containing blocks
 * Size: 0x1C (28 bytes)
 *
 * VTable @ 0x00AF9950 (6 slots):
 *   Slot 0 (+0x00): ~CRefDungeon @ 0x00555DB0
 *   Slot 1 (+0x04): Initialize @ 0x00555AF0
 *   Slot 2 (+0x08): CleanUp @ 0x00555B40
 *   Slot 3 (+0x0C): Stub @ 0x00559000
 *   Slot 4 (+0x10): GetBlock @ 0x00555C40
 *   Slot 5 (+0x14): GetCell @ 0x00555CA0
 */
class CRefDungeon {
public:
	CRefDungeon();
	virtual ~CRefDungeon();

	virtual bool Initialize(uint16_t wRegionID, uint32_t dwCellCount);
	virtual void CleanUp();
	virtual uint32_t Stub();
	virtual CRefBlock* GetBlock(uint32_t dwBlockIndex);
	virtual uint32_t GetCell(uint32_t x, uint32_t z);

	bool ReadNavData(FILE* fp);

public:
	uint16_t                m_wRegionID;   // +0x04 (low word)
	uint16_t                pad06;         // +0x06
	uint32_t                m_dwDungeonID; // +0x08 (or cell count)
	std::vector<CRefBlock*> m_vecBlocks;   // +0x0C - +0x17
	uint16_t*               m_pNavGrid;    // +0x18: 2 bytes per cell
};

/**
 * [RECONSTRUCTED]
 * tagSimpleDungeonBlock: Single navigation block in a simple dungeon
 * Layout:
 *   +0x00: uint32_t   m_dwEntryCount
 *   +0x04: D3DVECTOR* m_pEntries (array of 12-byte float vertices)
 */
struct tagSimpleDungeonBlock {
	uint32_t   m_dwEntryCount; // +0x00
	D3DVECTOR* m_pEntries;     // +0x04
};

/**
 * [RECONSTRUCTED - 0x00555DD0 / 0x0052FF00]
 * CSimpleDungeon: Simple dungeon navigation container
 * Size: 0x10 (16 bytes)
 * Layout:
 *   +0x00: uint16_t                            m_wRegionID
 *   +0x02: uint16_t                            m_wPad
 *   +0x04: std::vector<tagSimpleDungeonBlock*> m_vecBlocks
 */
class CSimpleDungeon {
public:
	// Native @ 0x00555DD0 (36 bytes)
	CSimpleDungeon();

	// Native @ 0x0052FF00 (63 bytes)
	~CSimpleDungeon();

	// Native @ 0x0052FF70 (113 bytes)
	void CleanUp();

	// Native @ 0x00555E20 (140 bytes)
	void* AddBlock(uint32_t dwBlockIndex, uint32_t dwEntryCount, D3DVECTOR* pEntries);

public:
	uint16_t                            m_wRegionID; // +0x00
	uint16_t                            m_wPad;      // +0x02
	std::vector<tagSimpleDungeonBlock*> m_vecBlocks; // +0x04 - +0x0F
};

// Global simple dungeon registry @ 0x00CD7DF0
extern std::map<uint16_t, CSimpleDungeon*> g_mapSimpleDungeons;

// Supporting native simple dungeon manager routines
CSimpleDungeon* CSimpleDungeonManager_Find(uint16_t wRegionID); // 0x0053B220
void CSimpleDungeonManager_CleanUp();                           // 0x00530070
bool CSimpleDungeon_Register(CSimpleDungeon* pDungeon);         // 0x00555F20

/**
 * [RECONSTRUCTED - 0x00AF996C / 0x00555F80]
 * CAINavDataManager: AI Navigation Data Manager singleton
 * Size: 0x10 (16 bytes)
 */
class CAINavDataManager : public CSingletonT<CAINavDataManager> {
public:
	CAINavDataManager();
	virtual ~CAINavDataManager() override;

	CRefDungeon* GetOrCreateDungeonNavData(uint16_t wRegionID, uint32_t dwDungeonID);
	CRefDungeon* FindDungeon(uint16_t wRegionID);
	void ClearDungeons();

	bool LoadNavDataFiles();
	bool LoadNavDataBinaryFile(const char* szFilePath);
	bool LoadSimpleDungeonData();
	bool LoadSimpleDungeonData(const std::vector<uint16_t>& vecRegionIDs);

public:
	std::map<uint32_t, CRefDungeon*> m_mapDungeons; // +0x04 - +0x0F
};

extern CAINavDataManager  g_aiNavDataManager;
extern CAINavDataManager* g_pAINavDataManager;

// AI Initialization & Helper routines
void AI_BuildDirectionVectorTable();
void AI_BuildApproachDirectionTable(AI::CSquadManager* pSquadMgr);
int32_t AI_ApproachSlotFromVector(const D3DVECTOR* pDiff);
int32_t AI_GetApproachDirectionIndexFromVector(const D3DVECTOR* pVec); // 0x0053DAD0
float Vec3_AngleBetween(const D3DVECTOR* pV1, const D3DVECTOR* pV2);   // 0x00545450
bool AI_LoadNavDataBinaryFile(const char* szFilePath);
bool AI_LoadAINavDataFiles();
bool AI_CAIStateAllocator_Initialize(CAIStateManager* pStateManager = nullptr);

// CORRECTION (Claude): removed the undefined free CGameAI_PopulateRegionNests / CGameAI_BindNestTactics
// declarations; both are the CGameAI members above.
class CGameWorldLayer;
bool AI_InitHiveSpawnStates(CGameWorldLayer* pLayer, std::map<uint32_t, AI::CAIHive*>* pHiveMap); // 0x0054BBF0

#endif // _SR_GAMESERVER_GAMEAI_H_

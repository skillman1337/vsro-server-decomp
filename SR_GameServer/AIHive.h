/**
 * ============================================================================
 * Silkroad Online - Game Server AI Hive / Nest
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\AIHive.h
 *
 * RTTI-proven classes (SR_GameServer.exe, BN snapshot 18):
 *   - AI::CAIHive  .?AVCAIHive@AI@@  vftable 0x00AF9B80 (1 slot)  sizeof 0x38 (new @ 0x0054B9A1)
 *   - AI::CNest    .?AVCNest@AI@@    vftable 0x00AF9C3C (5 slots) sizeof 0x34 (new @ 0x0055E807)
 *
 * Per-layer runtime records (no RTTI; the names are reconstruction names):
 *   - tagHiveSpawnState (0x40): CGameWorldLayer+0x18 vector, indexed by CAIHive::m_dwIndex
 *   - tagNestSpawnState (0x20): tagHiveSpawnState+0x18 vector, indexed by CNest::m_dwIndex
 *
 * World classes: CGameWorldMgr (GameWorldMgr.h), CGameWorld and tagRegionContext (GameWorld.h),
 * CGameWorldLayer (GameWorldLayer.h), CMap (Map.h), CWorldMap (WorldMap.h), CRegion (Region.h).
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_AIHIVE_H_
#define _SR_GAMESERVER_AIHIVE_H_

#include <cstdint>
#include <vector>
#include <map>
#include "GlobalPos.h"
#include "GameWorld.h"

struct tagRefHive;
struct tagRefNest;
struct tagRefTactics;
struct tagRefObjCommon;
class CGObjChar;

namespace AI {
	class CAIHive;
	class CNest;
}

/**
 * [RECONSTRUCTED - 0x00560380 / 0x0055EA90 / 0x0054BD2F] (Size: 0x20)
 * tagNestSpawnState
 * Hatch timer and counters of one nest inside one world layer.
 */
struct tagNestSpawnState {
	uint32_t dwLastHatchTime;  // +0x00: g_dwGameAICurrentTick at the last hatch/restart
	uint32_t dwHatchDelay;     // +0x04: rolled delay minus dwDelayReduction (ms)
	uint32_t dwCurCount;       // +0x08: live monsters from this nest
	float    fIncreaseRate;    // +0x0C: percent taken off the rolled delay
	uint32_t dwDelayReduction; // +0x10
	uint32_t bAutoHatch;       // +0x14: tagRefNest::m_btType == 0 (0x0054BD42)
	uint32_t dwRespawn;        // +0x18: tagRefNest::m_btRespawn (0x0054BD49)
	int32_t  nRemainCount;     // +0x1C: signed budget for non-respawning nests (0x0055EB23 jle)
};

/**
 * [RECONSTRUCTED - 0x0054A800 / 0x0055F190 / 0x0055EEC0] (Size: 0x40)
 * tagHiveSpawnState
 * Hatch state of one hive inside one world layer. Constructor 0x0054A800 is in the GameAI
 * address range and is defined in GameAI.cpp.
 */
struct tagHiveSpawnState {
	tagHiveSpawnState();

	uint32_t                       dwSampleSum;     // +0x00: PC counts summed over the current sample pair
	uint32_t                       dwSampleCount;   // +0x04
	uint32_t                       dwAverage;       // +0x08: mean of dwHistory
	int32_t                        nHistoryIndex;   // +0x0C: (index + 1) % 2
	uint32_t                       dwHistory[2];    // +0x10
	std::vector<tagNestSpawnState> vecNest;         // +0x18 (VC8 layout: first +0x1C)
	uint32_t                       dwLastHatchTime; // +0x28
	uint32_t                       dwLastAverage;   // +0x2C
	uint32_t                       dwCurCount;      // +0x30: live monsters across the hive
	uint32_t                       dwUnk34;         // +0x34: zeroed by 0x0054A86C and 0x0054BC96, never read here
	float                          fIncreaseRate;   // +0x38
	AI::CNest*                     pSelectedNest;   // +0x3C: HatchOverwriteTotal target
};

namespace AI {

/**
 * [RECONSTRUCTED - 0x00560450, vftable 0x00AF9C3C] (Size: 0x34)
 * CNest
 * One _RefNest spawn point of a hive.
 */
class CNest {
public:
	// Native 0x00560450 (39 bytes)
	CNest();

	// vftable[0]: scalar deleting destructor 0x00560480, body 0x005604A0
	virtual ~CNest();

	// vftable[1] 0x005604B0 (189 bytes)
	virtual bool Initialize(CAIHive* pHive, tagRefNest* pRefNest, uint32_t dwIndex);

	// vftable[2] 0x00455EB0 (B0 01 C3, shared COMDAT). Called by CAIHive::Update between hatches.
	virtual bool Update();

	// vftable[3] 0x00455EB0 (B0 01 C3, shared COMDAT). Called by CAIHive::DeleteNests before delete.
	virtual bool Release();

	// vftable[4] 0x00560D00 (177 bytes, retn 8)
	virtual bool OnMonsterDead(CGObjChar* pObj, uint32_t dwTacticsPoolIndex); // 2nd arg: CTactics pool index from DetachNest 0x0053FD54, unused here

	// Native 0x00560570 (131 bytes)
	bool BindRefData();

	// Native 0x005606C0 (235 bytes): 0xFF for non-NPC objects
	uint8_t HatchNPC(uint16_t wGameWorldID);

	// Native 0x005607B0 (1355 bytes): returns the spawned object's ID or 0
	uint32_t Hatch(uint16_t wGameWorldID, uint16_t wLayerID, tagNestSpawnState* pState);

	// Native 0x00560E40 (14 bytes)
	uint32_t RollHatchDelay(tagNestSpawnState* pState);

	// Native 0x00560E90 (175 bytes)
	uint32_t GetPCCount(tagRegionContext* pContext, tagNestSpawnState* pState);

public:
	CAIHive*         m_pHive;       // +0x04
	tagRefNest*      m_pRefNest;    // +0x08
	tagRefTactics*   m_pRefTactics; // +0x0C: BindRefData
	tagObjLocation   m_Pos;         // +0x10 - +0x27
	tagRefObjCommon* m_pRefObj;     // +0x28: BindRefData
	uint32_t         m_dwIndex;     // +0x2C: tagHiveSpawnState::vecNest index
	uint32_t         m_bPartyHatch; // +0x30: == 1 permits the party rarity roll; cleared after a party hatch
};

/**
 * [RECONSTRUCTED - 0x0055E440, vftable 0x00AF9B80] (Size: 0x38)
 * CAIHive
 * Owner of the CNest instances created from one _RefHive row.
 */
class CAIHive {
public:
	// m_pfnHatch element: 4-byte single-inheritance member pointer, thiscall retn 0xC
	typedef int32_t (CAIHive::*HATCH_FUNC)(uint16_t wGameWorldID, uint16_t wLayerID, tagHiveSpawnState* pState);

	// Native 0x0055E440 (114 bytes)
	CAIHive();

	// vftable[0]: scalar deleting destructor 0x0055E4C0, body 0x0055E530
	virtual ~CAIHive();

	// Native 0x0055E5B0 (444 bytes)
	bool Initialize(tagRefHive* pRefHive, uint16_t wGameWorldID, uint32_t dwIndex);

	// Native 0x0055E770 (78 bytes)
	bool SetRefHive(tagRefHive* pRefHive);

	// Native 0x0055E7C0 (324 bytes): 0 = created, 1 = no ref, 2 = region missing, 3 = Initialize failed
	int32_t CreateNest(tagRefNest* pRefNest, uint16_t wGameWorldID, uint32_t dwIndex);

	// Native 0x0055E910 (154 bytes)
	int32_t Update(tagHiveSpawnState* pState, uint16_t wGameWorldID, uint16_t wLayerID);

	// Native 0x0055E9B0 (99 bytes)
	uint32_t GetMaxTotalCount();

	// Native 0x0055EA20 (100 bytes)
	bool HatchNPCs(uint16_t wGameWorldID);

	// m_pfnHatch[0], native 0x0055EA90 (381 bytes)
	int32_t HatchPerNest(uint16_t wGameWorldID, uint16_t wLayerID, tagHiveSpawnState* pState);

	// m_pfnHatch[1], native 0x0055EC10 (403 bytes)
	int32_t HatchOverwriteTotal(uint16_t wGameWorldID, uint16_t wLayerID, tagHiveSpawnState* pState);

	// Native 0x0055EDB0 (14 bytes)
	void Release();

	// Native 0x0055EDC0 (250 bytes)
	void DeleteNests();

	// Native 0x0055EEC0 (279 bytes)
	bool OnMonsterDead(tagHiveSpawnState* pState);

	// Native 0x0055EFE0 (152 bytes)
	CNest* SelectRandomNest();

	// Native 0x0055F080 (82 bytes)
	CNest* FindNest(uint32_t dwNestID);

	// Native 0x0055F0E0 (173 bytes)
	bool UpdateIncreaseRate(tagRegionContext* pContext, tagHiveSpawnState* pState);

	// Native 0x0055F190 (490 bytes)
	void CalcIncreaseRate(uint32_t dwPCCount, tagHiveSpawnState* pState);

public:
	tagRefHive*             m_pRefHive;         // +0x04
	HATCH_FUNC              m_pfnHatch[2];      // +0x08, +0x0C: written by Initialize only
	std::map<uint32_t, CNest*> m_mapNest;       // +0x10 - +0x1B: keyed by tagRefNest::m_dwNestID
	std::vector<CNest*>     m_vecNest;          // +0x1C - +0x2B: m_mapNest values in key order
	uint32_t                m_dwMaxTotalCount;  // +0x2C
	uint8_t                 m_btHatchType;      // +0x30: m_pfnHatch index
	uint32_t                m_dwIndex;          // +0x34: CGameWorldLayer hive-state index
};

} // namespace AI

// Native 0x00560600 (142 bytes)
float AI_GetMonsterRarityRate(const uint8_t* pbtRarity);

// Nest spawn-state helpers
uint32_t tagNestSpawnState_SetHatchDelay(uint32_t dwDelayTimeMinSec, uint32_t dwDelayTimeMaxSec, tagNestSpawnState* pState); // 0x00560380
void     tagNestSpawnState_IncCount(tagNestSpawnState* pState);                                                         // 0x00560DC0
void     tagNestSpawnState_DecCount(tagNestSpawnState* pState);                                                         // 0x00560DF0
int32_t  tagNestSpawnState_HalveHatchDelay(tagNestSpawnState* pState);                                                  // 0x00560E10
uint32_t tagNestSpawnState_RestartHatchTimer(tagNestSpawnState* pState, AI::CNest* pNest);                              // 0x00560E50
uint32_t tagNestSpawnState_SetIncreaseRate(tagNestSpawnState* pState, AI::CNest* pNest, float fRate);                   // 0x00560E70

#endif // _SR_GAMESERVER_AIHIVE_H_

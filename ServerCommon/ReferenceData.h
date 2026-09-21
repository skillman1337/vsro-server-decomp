/**
 * ============================================================================
 * Silkroad Online - Reference Data Database Manager
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\ServerCommon\ReferenceData.h
 *
 * Implements:
 *   - CReferenceData @ 0x006A3D50
 *   - Static reference data caching (_RefObjCommon, _RefSkill, _RefDropItem)
 *   - Global pointer g_pRefData @ 0x00D6AA14 (1,411 references in binary)
 *   - Accessor GetRefData() @ 0x00404CA0 (192 callers in binary)
 * ============================================================================
 */

#ifndef _SERVERCOMMON_REFERENCEDATA_H_
#define _SERVERCOMMON_REFERENCEDATA_H_

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <map>
#include "RefInstanceGenerator.h"

/**
 * Reference Object Common Database record (_RefObjCommon)
 * Offset +0x08: m_dwRefObjID
 * Offset +0x80: m_wTypeID (TID)
 * Offset +0x88: m_byExpShare
 */
struct tagRefObjCommon {
	uint8_t     pad00[8];
	uint32_t    m_dwRefObjID;        // +0x08: Reference object template ID
	uint8_t     pad0C[4];            // +0x0C - +0x0F
	std::string m_strCodeName;       // +0x10: Internal code name string
	uint8_t     pad2C[0x54];         // +0x2C - +0x7F
	union {
		uint16_t m_wTypeID;          // +0x80: 16-bit TypeID
		struct {
			uint16_t m_tid;
		};
	};
	uint8_t     pad82[6];            // +0x82 - +0x87
	union {
		uint8_t     m_byExpShare;        // +0x88: Experience sharing flag (0x01 = shared party exp)
		uint8_t     m_byCountry;         // +0x88: Country code (0 = China, 1 = Europe, proven @ 0x0059A7EA)
	};
	union {
		uint8_t     m_byCOS_Rarity;      // +0x89: COS Rarity (1 = 40 slots, 2 = 30 slots)
		uint8_t     m_byRank;            // +0x89: Monster rarity. CORRECTION (Claude): 0x00560600 rates it 0->x1, 1->x2, 4->x20, 5->x100, 6->x4, 7->x30 (high nibble 1 = party, base x10); the old "1=Normal, 3=Champion" legend contradicted those bytes
	};
	uint8_t     pad8A[2];            // +0x8A - +0x8B
	uint32_t    m_dwTypeDetailFlags; // +0x8C: Capability / permission detail bitmask
	uint8_t     pad90[12];           // +0x90 - +0x9B
	uint32_t    m_dwCountry;         // +0x9C: Country code (0 = China, 1 = Europe)
	uint8_t     padA0[0x46];         // +0xA0 - +0xE5
	uint16_t    m_wCOSSlotCapacity;  // +0xE6: COS Slot capacity fallback
	uint8_t     padE8[6];            // +0xE8 - +0xED
	union {
		uint8_t  m_byCollisionRadius; // +0xEE: Object collision / bounding radius (proven @ 0x004A68C0)
		uint16_t m_wCollisionRadius;
	};
	uint8_t     padF0[0xA8];         // +0xF0 - +0x197
	union {
		uint32_t    m_dwMaxStack;        // +0x198: Max stack capacity (proven @ 0x00459D80)
		uint8_t     m_byLevel;           // +0x198: Monster / character level
	};
	uint8_t     pad19C[0xB4];        // +0x19C - +0x24F
	uint32_t    m_dwRewardExp;       // +0x250: Monster / quest reward experience
	// Portable field for native item-reference +340. This host structure includes
	// std::string and is NOT a packed native layout; never read it at raw +340.
	uint8_t m_byItemFlags340 = 0;
};

typedef tagRefObjCommon CRefObjCommon;

// Per-level reference data record in CReferenceData::m_vecRefLevelData
struct tagRefLevelData {
	uint8_t  pad00[0x10];    // +0x00 - +0x0F
	union {
		uint32_t dwGoldCost;     // +0x10: Mastery downgrade gold cost factor
		uint32_t dwMasterySPCost;// +0x10: SP cost for mastery level increment (proven @ 0x0059C66B)
	};
	uint8_t  pad14[0x08];    // +0x14 - +0x1B
	uint32_t dwLevelTotalExp;// +0x1C: Total level experience / reference SP denominator
};

struct tagRefSkill;

/**
 * CReferenceData
 * Global reference game dataset manager
 * Native pointer g_pRefData @ 0x00D6AA14 (1,411 references in binary)
 * Accessor GetRefData() @ 0x00404CA0 (192 callers in binary)
 */
// CORRECTION (2026-09-17, Claude): the three hatching records below were previously named
// one step off. The record read through g_pRefData+0x244 is the _RefNest row, +0x238 is
// _RefTactics, and +0x250 is _RefHive (see AIHive.cpp). RTTI also contains loader classes
// CRefHive / CRefNest / CRefTactics (vftables 0x00B0C5E8 / 0x00B0C638 / 0x00B0C610,
// CRefNest element size 0x30); these plain records are what those loaders store.

/**
 * [RECONSTRUCTED - 0x00560590 / 0x00560968 / 0x0053F2A0 / 0x0053FA4C] _RefTactics row
 * CReferenceData+0x238 std::map<DWORD, tagRefTactics*>, keyed by dwTacticsID.
 * Every named field is read by the cited instruction. Names follow the _RefTactics column order
 * (..., btTraceType, btTraceBoundary, TraceData, btHomingType, HomingData, btAggressTypeOnHoming,
 * btFleeType, dwChampionTacticsID, AdditionOptionFlag); +0x84 is placed by that order, not by a name.
 * CORRECTION (Claude): the old +0x20 "m_dwRadius" had no reader through this record. The +0x20 read in
 * BindContext / CallForHelp is tagRefNest::m_nRadius via CTactics::m_pNest, and the search radius
 * adds +0x14.
 */
struct tagRefTactics {
	uint32_t m_dwTacticsID          = 0;   // +0x00 (map key)
	uint32_t m_dwObjID              = 0;   // +0x04: _RefObjCommon ID (0x005605B3)
	uint8_t  pad08[0x0C]            = {0}; // +0x08 - +0x13
	int32_t  m_nSightRange          = 0;   // +0x14: 0x0053FA4C fiadd st0, dword [eax+0x14]
	uint8_t  pad18[0x09]            = {0}; // +0x18 - +0x20
	uint8_t  m_btHelpRequestTo      = 0;   // +0x21: CallForHelp exits when == 2 (0x00541211) and sends it (0x00541253); name inferred
	uint8_t  pad22[0x57]            = {0}; // +0x22 - +0x78
	uint8_t  m_btTraceBoundary      = 0;   // +0x79: 0x0055A1DD cmp byte [edx+0x79], 2
	uint8_t  pad7A[0x0A]            = {0}; // +0x7A - +0x83
	int32_t  m_nHomingData          = 0;   // +0x84: roam radius when > 0 (0x0053F353)
	uint8_t  pad88[0x04]            = {0}; // +0x88 - +0x8B
	uint32_t m_dwChampionTacticsID  = 0;   // +0x8C: 0x005608FC cmp [edx+0x8C], 0
	uint8_t  m_btAdditionOptionFlag = 0;   // +0x90: 0x00540D26 test byte [eax+0x90], 0x84
};

/**
 * [RECONSTRUCTED - 0x0055E640 / 0x005604B0 / 0x0054BD35] _RefNest row
 * CReferenceData+0x244 std::map<DWORD, tagRefNest*>, keyed by dwNestID.
 */
struct tagRefNest {
	uint32_t m_dwNestID                = 0;    // +0x00: CAIHive::m_mapNest key (0x0055E8AF)
	uint32_t m_dwHiveID                = 0;    // +0x04: must equal tagRefHive::m_dwHiveID (0x0055E64E)
	uint32_t m_dwTacticsID             = 0;    // +0x08: _RefTactics key (0x0056057B)
	uint16_t m_wRegionDBID             = 0;    // +0x0C (0x005604D8 mov cx, word [eax+0xC])
	uint16_t pad0E                     = 0;    // +0x0E
	float    m_fLocalPosX              = 0.0f; // +0x10
	float    m_fLocalPosY              = 0.0f; // +0x14
	float    m_fLocalPosZ              = 0.0f; // +0x18
	uint16_t m_wInitialDir             = 0;    // +0x1C: 0..65535 -> 0..360 degrees (0x005609C2)
	uint16_t pad1E                     = 0;    // +0x1E
	int32_t  m_nRadius                 = 0;    // +0x20: display radius (0x00567BD5)
	int32_t  m_nGenerateRadius         = 0;    // +0x24: spawn scatter radius (0x00560C51 fild [esi+0x24])
	int32_t  m_nChampionGenPercentage  = 0;    // +0x28 (0x00560919 cmp edx, [eax+0x28])
	uint32_t m_dwDelayTimeMin          = 0;    // +0x2C: seconds (0x00560E43)
	uint32_t m_dwDelayTimeMax          = 0;    // +0x30: seconds (0x00560E46)
	uint32_t m_dwMaxTotalCount         = 0;    // +0x34 (0x0055E66F)
	uint8_t  m_btFlag                  = 0;    // +0x38
	uint8_t  m_btRespawn               = 0;    // +0x39 (0x0054BD45)
	uint8_t  m_btType                  = 0;    // +0x3A: 0 enables per-nest auto hatch (0x0054BD3A)
};

/**
 * [RECONSTRUCTED - 0x0055E770 / 0x0055E5B0 / 0x0054B98C] _RefHive row
 * CReferenceData+0x250 std::map<DWORD, tagRefHive*>, keyed by dwHiveID.
 */
struct tagRefHive {
	uint32_t              m_dwHiveID                 = 0;    // +0x00
	uint8_t               m_btKeepMonsterCountType   = 0;    // +0x04: 1 = scale hatch delay by PC count
	uint8_t               pad05[3]                   = {0};  // +0x05 - +0x07
	uint32_t              m_dwOverwriteMaxTotalCount = 0;    // +0x08: > 0 selects CAIHive::HatchOverwriteTotal
	float                 m_fMonsterCountPerPC       = 0.0f; // +0x0C: divides CAIHive::m_dwMaxTotalCount (0x0055E6A8)
	uint32_t              m_dwSpawnSpeedIncreaseRate = 0;    // +0x10 (0x0055F22C imul)
	uint32_t              m_dwMaxIncreaseRate        = 0;    // +0x14: CLAMP upper bound (0x0055F2BF)
	uint8_t               m_btFlag                   = 0;    // +0x18
	uint8_t               pad19                      = 0;    // +0x19
	uint16_t              m_wGameWorldID             = 0;    // +0x1A (0x0054B98C cmp word [edi+0x1A], bx)
	uint16_t              m_wHatchObjType            = 0;    // +0x1C (0x0054B996 cmp word [edi+0x1C], 2)
	uint16_t              pad1E                      = 0;    // +0x1E
	std::vector<uint32_t> m_vecNestID;                       // +0x20 (VC8 layout: first +0x24, last +0x28)
};

/**
 * [PARTIAL - 0x006E45B0 / 0x005EC640 / 0x005F85A0 / 0x00609760] _REFGAME_WORLD row (0x50)
 * Data part of CRefGameWorld (.?AVCRefGameWorld@@, record 0x68 with this at +0x18; table name 0x00B11510).
 * CGameWorld+0x04 points here. The loader is not ported.
 */
struct tagRefGameWorld {
	uint16_t    m_wGameWorldID  = 0;    // +0x00: 0x005307D3 movzx ebp, word [eax]
	uint8_t     pad02[0x02]     = {0};  // +0x02
	std::string m_strUnk04;             // +0x04: std::string constructed at 0x006E45DB
	uint8_t     m_btUnk20       = 0;    // +0x20: 0x005F85F6 == 0; 0x00608682 maps 0/1 onto CGameWorld+0x3C
	uint8_t     pad21           = 0;    // +0x21
	uint16_t    m_wMaxLayerID   = 0;    // +0x22: CGameWorld::GetLayer bound (0x005EC644)
	uint8_t     pad24[0x10]     = {0};  // +0x24 - +0x33
	std::string m_strWorldCodeName;     // +0x34: logged as GameWorldName (0x006097DB); world config key (0x0060936A)
};

class CReferenceData {
public:
	CReferenceData();
	virtual ~CReferenceData();

	// [RECONSTRUCTED - Native 0x006A3D50]
	// Loads static reference objects from database or text data
	bool LoadReferenceData();

	const tagRefObjCommon* GetRefObjCommon(uint32_t dwRefID) const;

	const tagRefObjCommon* FindRefObjCommon(uint32_t dwRefID) const {
		return GetRefObjCommon(dwRefID);
	}
	tagRefObjCommon* FindRefObjCommon(uint32_t dwRefID) {
		return const_cast<tagRefObjCommon*>(GetRefObjCommon(dwRefID));
	}
	// [RECONSTRUCTED - COMDAT 0x00501AC0 on +0x238] find(key) == end ? NULL : second
	tagRefTactics* FindRefTactics(uint32_t dwTacticsID) {
		auto it = m_mapRefTactics.find(dwTacticsID);
		if (it != m_mapRefTactics.end()) {
			return it->second;
		}
		return nullptr;
	}

	// [RECONSTRUCTED - COMDAT 0x00560250 on +0x244] find(key) == end ? NULL : second
	tagRefNest* FindRefNest(uint32_t dwNestID) {
		auto it = m_mapRefNest.find(dwNestID);
		if (it != m_mapRefNest.end()) {
			return it->second;
		}
		return nullptr;
	}

	// [RECONSTRUCTED - Native 0x006BEF30]
	// Builds the map of teleport buildings indexed by CodeName (assert @ line 2526 of RefInstanceGenerator.cpp)
	bool BuildTeleportCodeNameIndex();

	// [RECONSTRUCTED - Native 0x006BF080]
	// Retrieves teleport descriptor by CodeName (e.g. "GATE_CH_JANGAN")
	tagRefTeleport* GetTeleportByCodeName(const std::string& strCodeName);

	// [RECONSTRUCTED - Native 0x006F68F0]
	// Builds the map of skills indexed by Basic_Code (assert @ line 2383 of RefInstanceGenerator.h)
	bool BuildSkillCodeNameIndex();

	size_t GetLoadedTableCount() const;
	void SetLoadedTableCount(size_t nCount);

	static CReferenceData* GetInstance();

	// [RECONSTRUCTED - Native 0x0046C1F0]
	const tagRefSkill* FindSkill(uint32_t dwSkillID) const {
		auto it = m_mapRefSkill.find(dwSkillID);
		if (it != m_mapRefSkill.end()) {
			return reinterpret_cast<const tagRefSkill*>(it->second);
		}
		return nullptr;
	}

	const void* FindMastery(uint32_t dwMasteryID) const {
		auto it = m_mapRefMastery.find(dwMasteryID);
		if (it != m_mapRefMastery.end()) {
			return it->second;
		}
		return nullptr;
	}

public:
	uint8_t                       pad04[0x10];       // +0x04 - +0x13
	std::vector<tagRefLevelData*> m_vecRefLevelData; // +0x14: Vector of level records
	uint8_t                       pad20[0x9C];       // +0x20 - +0xBB

	// +0xBC: Vector of teleport building descriptors
	std::vector<tagRefTeleport*>              m_vecRefTeleport;
	// +0xC4: Map of teleport buildings indexed by code name string
	std::map<std::string, tagRefTeleport*>    m_RefTeleportByCodeName;

	uint8_t                       padD0[0x188];      // +0xD0 - +0x257

	// +0x258: World region mapping list
	std::vector<tagRefInstanceWorldRegion*>   m_vecRefInstanceWorldRegion;
	// +0x25C: World start spawn position list
	std::vector<tagRefInstanceWorldStartPos*> m_vecRefInstanceWorldStartPos;

	uint8_t                       pad268[0x614];     // +0x268 - +0x87B

	// +0x87C: Map of mastery templates indexed by MasteryID
	std::map<uint32_t, void*>                 m_mapRefMastery;
	// +0x888: Map of skill templates indexed by SkillID
	std::map<uint32_t, void*>                 m_mapRefSkill;
	// +0x894: Map of skill templates indexed by Basic_Code string
	std::map<std::string, void*>              m_RefSkillByCodename;
	// +0x238: _RefTactics rows indexed by TacticsID (0x00560585 add eax, 0x238)
	std::map<uint32_t, tagRefTactics*>        m_mapRefTactics;
	// +0x244: _RefNest rows indexed by NestID (0x0055E63B add eax, 0x244)
	std::map<uint32_t, tagRefNest*>           m_mapRefNest;
	// +0x250: _RefHive rows indexed by HiveID (0x0054B918 lea ecx, [eax+0x250])
	std::map<uint32_t, tagRefHive*>           m_mapRefHive;

private:
	size_t                              m_nLoadedTableCount = 0;
	std::map<uint32_t, tagRefObjCommon> m_mapRefObjects;
};

typedef CReferenceData CRefData;

// Global reference dataset pointer matching native 0x00D6AA14
extern CReferenceData  g_referenceData;
extern CReferenceData* g_pRefData;

inline CReferenceData* CShardDB_GetReferenceData() { return CReferenceData::GetInstance(); }

// Native 0x00C82538: server config "GiantMonster_SpawnRatio" (percent)
extern uint16_t g_wGiantMonsterSpawnRatio;

/**
 * [RECONSTRUCTED - 0x00404CA0]
 * Returns global reference dataset instance (m_pRefData)
 */
CReferenceData* GetRefData();

#endif // _SERVERCOMMON_REFERENCEDATA_H_

/**
 * ============================================================================
 * Silkroad Online - Game Server AI Subsystem Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GameAI.cpp
 *
 * Implements:
 *   - CGameAI::CGameAI @ 0x0054AC20
 *   - CGameAI::~CGameAI @ 0x0054AD30 / 0x0054AE40
 *   - CGameAI::Initialize @ 0x0054B1D0
 *   - CGameAI::CleanUp @ 0x0054AF40
 *   - CGameAI::OnScheduledPatrolTick @ 0x0054C6E0
 *   - CGameAI::OnTick @ 0x0054B300
 *   - CGameAI::LoadPositionerData @ 0x0054BDE0
 *   - CGameAI::SelectSpawnTactics @ 0x0054C0E0
 *   - tagPositionerRegionNode::SelectRandomTactics @ 0x0054C1C0
 *   - CGameAI::FindPositionerRegionNode @ 0x0054C2A0
 *   - CGameAI::ClearPositionerData @ 0x0054C300
 *   - CGameAI::FindNestInRegion @ 0x0054C450
 *   - CGameAI::FindNestInAnyHive @ 0x0054C4F0
 *   - CGameAI::FindHiveInRegion @ 0x0054C5B0
 *   - CGameAI::PostAIMessage @ 0x0054C660
 *   - CGameAI::InitializePatrolRegions @ 0x0054B710
 *   - CGameAI::PopulateRegionNests @ 0x0054B8D0
 *   - CGameAI::BindNestTactics @ 0x0054BB60
 *   - AI_InitHiveSpawnStates @ 0x0054BBF0 (tagHiveSpawnState ctor @ 0x0054A800)
 *   - CGameAI::ValidateAllNestPaths @ 0x0054C000
 *   - CGameAI::CreateAIMessage @ 0x0054C6A0
 *   - AI_BuildDirectionVectorTable @ 0x0054AB10
 *   - AI_BuildApproachDirectionTable @ 0x0054A990
 *   - AI_LoadAINavDataFiles @ 0x005560E0
 *   - AI_LoadNavDataBinaryFile @ 0x00556240
 *   - AI_CAIStateAllocator_Initialize @ 0x0055B380
 *   - CAINavDataManager implementations @ 0x00555F80 / 0x00556070 / 0x00556400 / 0x00556550
 *   - CAINavDataManager::LoadSimpleDungeonData @ 0x00556610
 *   - CSimpleDungeon::CSimpleDungeon @ 0x00555DD0
 *   - CSimpleDungeon::~CSimpleDungeon @ 0x0052FF00
 *   - CSimpleDungeon::CleanUp @ 0x0052FF70
 *   - CSimpleDungeon::AddBlock @ 0x00555E20
 *   - CSimpleDungeon_Register @ 0x00555F20
 *   - CSimpleDungeonManager_Find @ 0x0053B220
 *   - CSimpleDungeonManager_CleanUp @ 0x00530070
 *   - CGameAI::MarkTacticsForRemoval @ 0x0053DA50
 *   - AI_GetApproachDirectionIndexFromVector @ 0x0053DAD0
 *   - Vec3_AngleBetween @ 0x00545450
 *   - CPositioner implementations @ 0x0054DEA0 / 0x0054DEF0 / 0x0054DF70 / 0x0054F780
 *   - Global instances: g_gameAI @ 0x00CE1EC0, g_pGameAI @ 0x00D6A96C
 *   - Global nav manager: g_aiNavDataManager @ 0x00CE21E0, g_pAINavDataManager @ 0x00D6A974
 *   - Global state manager: g_aiStateManager @ 0x00CE21F0, g_pAIStateManager @ 0x00D6A978
 *   - Global approach table: g_vApproachDirections[36] @ 0x00CE2030
 * ============================================================================
 */

#include "GameAI.h"
#include "GameWorldMgr.h"
#include "GameWorld.h"
#include "GameWorldLayer.h"
#include "AIHive.h"
#include "../ServerCommon/ReferenceData.h"
#include "../JMX_ServerFramework/ServerFramework/ServerConfig.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <windows.h>

extern CGObjChar* CGame_FindObjectByID(uint32_t dwGameID);

// Native static instance @ 0x00CE1EC0
CGameAI g_gameAI;

// Native global pointer @ 0x00D6A96C
CGameAI* g_pGameAI = &g_gameAI;

// Native global @ 0x00C82620 (GetTickCount cache written by Initialize / OnTick)
uint32_t g_dwGameAICurrentTick = 0;

// Native global state manager @ 0x00CE21F0 / 0x00D6A978
CAIStateManager  g_aiStateManager;
CAIStateManager* g_pAIStateManager = &g_aiStateManager;

// Native global navigation data manager @ 0x00CE21E0 / 0x00D6A974
CAINavDataManager  g_aiNavDataManager;
CAINavDataManager* g_pAINavDataManager = &g_aiNavDataManager;

// Native 36 approach direction vector table @ 0x00CE2030
D3DVECTOR g_vApproachDirections[36];

// ============================================================================
// Math Helper Routines (Native 0x0054AB10 / 0x0054A990)
// ============================================================================

static void Vec3_Normalize(D3DVECTOR* pOut) {
	if (pOut == nullptr) return;
	float fLen = std::sqrt(pOut->x * pOut->x + pOut->y * pOut->y + pOut->z * pOut->z);
	if (fLen > 0.00001f) {
		pOut->x /= fLen;
		pOut->y /= fLen;
		pOut->z /= fLen;
	}
}

/**
 * [RECONSTRUCTED - 0x0054AB10]
 * AI_BuildDirectionVectorTable
 * Native implementation @ 0x0054AB10 (221 bytes)
 *
 * Populates 36 discrete normalized unit direction vectors spaced 10 degrees
 * apart (5, 15, 25 ... 355 deg) into g_vApproachDirections (@ 0x00CE2030).
 */
void AI_BuildDirectionVectorTable() {
	const float kDegToRad = 0.01745329238474369f; // PI / 180.0f
	int32_t nIdx = 0;
	for (int32_t iDeg = 5; iDeg < 365 && nIdx < 36; iDeg += 10, ++nIdx) {
		float fRad = static_cast<float>(iDeg) * kDegToRad;
		float fSin = std::sin(fRad);
		float fCos = std::cos(fRad);

		g_vApproachDirections[nIdx].x = fCos;
		g_vApproachDirections[nIdx].y = 0.0f;
		g_vApproachDirections[nIdx].z = -fSin;
		Vec3_Normalize(&g_vApproachDirections[nIdx]);
	}
}

/**
 * [RECONSTRUCTED - 0x0054A990]
 * AI_BuildApproachDirectionTable
 * Native implementation @ 0x0054A990 (214 bytes)
 *
 * Populates 8 discrete approach direction vectors spaced 45 degrees apart
 * (22, 67, 112, 157, 202, 247, 292, 337 deg) into CSquadManager's table at +0x6C.
 */
void AI_BuildApproachDirectionTable(AI::CSquadManager* pSquadMgr) {
	if (pSquadMgr == nullptr) return;

	const float kDegToRad = 0.01745329238474369f;
	int32_t nIdx = 0;
	for (int32_t iDeg = 22; iDeg < 382 && nIdx < 8; iDeg += 45, ++nIdx) {
		float fRad = static_cast<float>(iDeg) * kDegToRad;
		float fSin = std::sin(fRad);
		float fCos = std::cos(fRad);

		pSquadMgr->m_vApproachDirections[nIdx].x = fCos;
		pSquadMgr->m_vApproachDirections[nIdx].y = 0.0f;
		pSquadMgr->m_vApproachDirections[nIdx].z = -fSin;
		Vec3_Normalize(&pSquadMgr->m_vApproachDirections[nIdx]);
	}
}

// ============================================================================
// AI::CSquad & AI::CSquadManager Implementation (Native 0x0055DD60 / 0x0054A890)
// ============================================================================

namespace AI {

/**
 * [RECONSTRUCTED - 0x0055DD60]
 * CSquad::CSquad
 * Native implementation @ 0x0055DD60 (97 bytes)
 */
CSquad::CSquad()
	: m_dwSquadID(0)
	, m_dwLeaderID(0)
	, m_nActiveSlots(0)
	, m_nPoolIndex(0)
	, m_mapMembers() {
	std::memset(m_dwSlotMembers, 0, sizeof(m_dwSlotMembers));
}

/**
 * [RECONSTRUCTED - 0x0055DDD0]
 * CSquad::~CSquad
 * Native implementation @ 0x0055DDD0 (76 bytes)
 */
CSquad::~CSquad() {
	m_mapMembers.clear();
}

/**
 * [RECONSTRUCTED - 0x0055DE20]
 * CSquad::Initialize
 * Native implementation @ 0x0055DE20 (62 bytes)
 */
bool CSquad::Initialize(uint32_t dwLeaderID, uint32_t dwSquadID) {
	if (CGame_FindObjectByID(dwLeaderID) == nullptr) {
		return false;
	}
	m_dwLeaderID = dwLeaderID;
	m_dwSquadID = dwSquadID;
	m_nActiveSlots = 0;
	std::memset(m_dwSlotMembers, 0, sizeof(m_dwSlotMembers));
	return true;
}

/**
 * [RECONSTRUCTED - 0x0055DB40]
 * CSquad::NextApproachSlotCandidate
 * Native implementation @ 0x0055DB40 (234 bytes)
 */
int32_t CSquad::NextApproachSlotCandidate(int32_t nCurSlot, int32_t* pStep) {
	if (nCurSlot != -1 && pStep != nullptr) {
		*pStep += 1;
		int32_t nNext = (nCurSlot + *pStep) & 7;
		if (nNext < 0 || nNext >= 8) {
			ServerFramework::ServerFramework_GenerateMiniDump();
		}
		return nNext;
	}
	return nCurSlot;
}

/**
 * [RECONSTRUCTED - 0x0055DF10]
 * CSquad::AssignApproachSlot
 * Native implementation @ 0x0055DF10 (371 bytes)
 */
uint32_t CSquad::AssignApproachSlot(CTactics* pTactics) {
	if (pTactics == nullptr) {
		return 0xFFFFFFFF;
	}

	CGObjChar* pLeader = CGame_FindObjectByID(m_dwLeaderID);
	if (pLeader == nullptr) {
		BSLib::Log_Printf(0x2000001, "Why not exist SquadTargetCharID!! SquadTargetCharID : %d\n", m_dwLeaderID);
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0xFFFFFFFF;
	}

	if (m_nActiveSlots >= 8) {
		return 0xFFFFFFFF;
	}

	// Find the first free approach slot
	for (uint32_t i = 0; i < 8; ++i) {
		if (m_dwSlotMembers[i] == 0) {
			m_dwSlotMembers[i] = reinterpret_cast<uintptr_t>(pTactics);
			++m_nActiveSlots;
			return i;
		}
	}

	return 0xFFFFFFFF;
}

/**
 * [RECONSTRUCTED - 0x00545660]
 * CSquad::ReleaseApproachSlot
 * Native implementation @ 0x00545660 (30 bytes)
 */
void CSquad::ReleaseApproachSlot(uint32_t dwSlotIndex, uint32_t dwExpectedID) {
	if (dwSlotIndex <= 7 && m_dwSlotMembers[dwSlotIndex] == dwExpectedID) {
		m_dwSlotMembers[dwSlotIndex] = 0;
		if (m_nActiveSlots > 0) {
			--m_nActiveSlots;
		}
	}
}

/**
 * [RECONSTRUCTED - 0x0053D8A0]
 * CSquad::AddMember
 * Native implementation @ 0x0053D8A0 (153 bytes)
 */
bool CSquad::AddMember(CTactics* pTactics) {
	if (pTactics == nullptr) {
		return false;
	}

	uint32_t nSlot = AssignApproachSlot(pTactics);
	if (nSlot == 0xFFFFFFFF) {
		return false;
	}

	m_mapMembers[reinterpret_cast<uintptr_t>(pTactics)] = pTactics;
	return true;
}

/**
 * [RECONSTRUCTED - 0x0055DC30]
 * CSquad::RemoveMember
 * Native implementation @ 0x0055DC30 (141 bytes)
 */
bool CSquad::RemoveMember(CTactics* pTactics) {
	if (pTactics == nullptr) {
		return false;
	}

	auto it = m_mapMembers.find(reinterpret_cast<uintptr_t>(pTactics));
	if (it == m_mapMembers.end()) {
		return false;
	}

	m_mapMembers.erase(it);

	for (uint32_t i = 0; i < 8; ++i) {
		if (m_dwSlotMembers[i] == reinterpret_cast<uintptr_t>(pTactics)) {
			m_dwSlotMembers[i] = 0;
			if (m_nActiveSlots > 0) {
				--m_nActiveSlots;
			}
			break;
		}
	}
	return true;
}

/**
 * [RECONSTRUCTED - 0x0054A890]
 * CSquadManager::CSquadManager
 * Native implementation @ 0x0054A890 (98 bytes)
 */
CSquadManager::CSquadManager()
	: m_mapSquads()
	, m_poolSquads() {
	std::memset(m_vApproachDirections, 0, sizeof(m_vApproachDirections));
	m_poolSquads.Initialize(5000);
}

/**
 * [RECONSTRUCTED - 0x0054A900]
 * CSquadManager::~CSquadManager
 * Native implementation @ 0x0054A900 (139 bytes)
 */
CSquadManager::~CSquadManager() {
	m_mapSquads.clear();
	m_poolSquads.Clear();
}

/**
 * [RECONSTRUCTED - 0x0053D940]
 * CSquadManager::GetOrCreateSquad
 * Native implementation @ 0x0053D940 (259 bytes)
 */
CSquad* CSquadManager::GetOrCreateSquad(uint32_t dwSquadID, uint32_t dwLeaderID, CTactics* pTactics, bool bParam) {
	(void)bParam;
	if (dwLeaderID == 0 || pTactics == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return nullptr;
	}

	auto it = m_mapSquads.find(dwSquadID);
	if (it != m_mapSquads.end()) {
		CSquad* pSquad = it->second;
		if (pSquad != nullptr) {
			pSquad->AddMember(pTactics);
			return pSquad;
		}
	}

	CSquad* pNewSquad = m_poolSquads.Acquire();
	if (pNewSquad == nullptr) {
		return nullptr;
	}

	if (!pNewSquad->Initialize(dwLeaderID, dwSquadID)) {
		m_poolSquads.Release(pNewSquad);
		return nullptr;
	}

	if (!pNewSquad->AddMember(pTactics)) {
		m_poolSquads.Release(pNewSquad);
		return nullptr;
	}

	m_mapSquads[dwSquadID] = pNewSquad;
	return pNewSquad;
}

/**
 * [RECONSTRUCTED - 0x0055DCC0]
 * CSquadManager::DestroySquad
 * Native implementation @ 0x0055DCC0 (158 bytes)
 */
bool CSquadManager::DestroySquad(uint32_t dwSquadID) {
	auto it = m_mapSquads.find(dwSquadID);
	if (it == m_mapSquads.end()) {
		return false;
	}

	CSquad* pSquad = it->second;
	if (pSquad != nullptr) {
		m_poolSquads.Release(pSquad);
	}

	m_mapSquads.erase(it);
	return true;
}

} // namespace AI

// ============================================================================
// CRefBlock Implementation (Native 0x00555530 - 0x005557F0)
// VTable @ 0x00AF992C (8 slots)
// ============================================================================

/**
 * [RECONSTRUCTED - 0x00555530] (136 bytes)
 * CRefBlock::CRefBlock
 */
CRefBlock::CRefBlock()
	: m_dwIndex(0)
	, m_dwDimension(0)
	, m_dwTotalContainEdgeCount(0)
	, m_pCells(nullptr)
	, m_pEntries(nullptr)
	, m_mapEntries() {
}

/**
 * [RECONSTRUCTED - 0x005559B0] (19 bytes)
 * CRefBlock::~CRefBlock
 */
CRefBlock::~CRefBlock() {
	CleanUp();
}

/**
 * [RECONSTRUCTED - 0x005555F0] (187 bytes)
 * CRefBlock::Initialize (VTable Slot 1 @ +0x04)
 */
bool CRefBlock::Initialize(uint32_t dwIndex, uint32_t dwDimension, uint32_t dwTotalContainEdgeCount) {
	m_dwIndex = dwIndex;
	m_dwDimension = dwDimension;
	m_dwTotalContainEdgeCount = dwTotalContainEdgeCount;

	m_pEntries = new D3DVECTOR[dwTotalContainEdgeCount + 1];
	std::memset(m_pEntries, 0, (dwTotalContainEdgeCount + 1) * sizeof(D3DVECTOR));

	m_pCells = new uint32_t[dwDimension * dwDimension];
	std::memset(m_pCells, 0, dwDimension * dwDimension * sizeof(uint32_t));
	return true;
}

/**
 * [RECONSTRUCTED - 0x005556B0] (86 bytes)
 * CRefBlock::CleanUp (VTable Slot 2 @ +0x08)
 */
void CRefBlock::CleanUp() {
	if (m_pCells != nullptr) {
		delete[] m_pCells;
		m_pCells = nullptr;
	}
	if (m_pEntries != nullptr) {
		delete[] m_pEntries;
		m_pEntries = nullptr;
	}
	m_mapEntries.clear();
	m_dwIndex = 0;
	m_dwDimension = 0;
	m_dwTotalContainEdgeCount = 0;
}

/**
 * [RECONSTRUCTED - 0x00555710] (48 bytes)
 * CRefBlock::SetCell (VTable Slot 3 @ +0x0C)
 */
bool CRefBlock::SetCell(uint32_t x, uint32_t z, uint32_t dwValue) {
	if (x >= m_dwDimension || z >= m_dwDimension || m_pCells == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return false;
	}
	m_pCells[z * m_dwDimension + x] = dwValue;
	return true;
}

/**
 * [RECONSTRUCTED - 0x00555740] (49 bytes)
 * CRefBlock::SetEntry (VTable Slot 4 @ +0x10)
 */
bool CRefBlock::SetEntry(uint32_t dwIndex, const D3DVECTOR& vEntry) {
	if (dwIndex > m_dwTotalContainEdgeCount || m_pEntries == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return false;
	}
	m_pEntries[dwIndex] = vEntry;
	return true;
}

/**
 * [RECONSTRUCTED - 0x00555780] (50 bytes)
 * CRefBlock::GetCustomEdge (VTable Slot 5 @ +0x14)
 * Line 252 assert in AIDataCommon.h
 */
D3DVECTOR* CRefBlock::GetCustomEdge(uint32_t dwCustomEdgeID) {
	if (m_dwTotalContainEdgeCount >= dwCustomEdgeID && m_pEntries != nullptr) {
		return &m_pEntries[dwCustomEdgeID];
	}
	BSLib::Log_Printf(0x2000000, "Assertion failed in AIDataCommon.h: m_dwTotalContainEdgeCount >= dwCustomEdgeID");
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x005557C0] (44 bytes)
 * CRefBlock::GetCell (VTable Slot 6 @ +0x18)
 */
uint32_t CRefBlock::GetCell(uint32_t x, uint32_t z) {
	if (x < m_dwDimension && z < m_dwDimension && m_pCells != nullptr) {
		return m_pCells[z * m_dwDimension + x];
	}
	ServerFramework::ServerFramework_GenerateMiniDump();
	return x;
}

/**
 * [RECONSTRUCTED - 0x005557F0] (48 bytes)
 * CRefBlock::FindEntry (VTable Slot 7 @ +0x1C)
 */
void* CRefBlock::FindEntry(uint32_t dwEntryID) {
	auto it = m_mapEntries.find(dwEntryID);
	if (it != m_mapEntries.end()) {
		return &it->second;
	}
	return nullptr;
}

// ============================================================================
// CRefDungeon Implementation (Native 0x00555A20 / 0x00555AF0 / 0x00555B40 / 0x00555C40)
// ============================================================================

CRefDungeon::CRefDungeon()
	: m_wRegionID(0)
	, pad06(0)
	, m_dwDungeonID(0)
	, m_vecBlocks()
	, m_pNavGrid(nullptr) {
}

CRefDungeon::~CRefDungeon() {
	CleanUp();
}

bool CRefDungeon::Initialize(uint16_t wRegionID, uint32_t dwCellCount) {
	m_wRegionID = wRegionID;
	m_dwDungeonID = dwCellCount;
	m_vecBlocks.resize(dwCellCount, nullptr);

	m_pNavGrid = new uint16_t[dwCellCount * dwCellCount];
	std::memset(m_pNavGrid, 0, dwCellCount * dwCellCount * sizeof(uint16_t));
	return true;
}

void CRefDungeon::CleanUp() {
	for (auto* pBlock : m_vecBlocks) {
		if (pBlock != nullptr) {
			delete pBlock;
		}
	}
	m_vecBlocks.clear();

	if (m_pNavGrid != nullptr) {
		delete[] m_pNavGrid;
		m_pNavGrid = nullptr;
	}
	m_wRegionID = 0;
	m_dwDungeonID = 0;
}

/**
 * [NATIVE STUB - 0x00559000] (3 bytes: xor eax, eax; ret)
 * CRefDungeon::Stub (VTable Slot 3 @ +0x0C)
 */
uint32_t CRefDungeon::Stub() {
	return 0;
}

CRefBlock* CRefDungeon::GetBlock(uint32_t dwBlockIndex) {
	if (dwBlockIndex >= m_vecBlocks.size()) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return nullptr;
	}
	return m_vecBlocks[dwBlockIndex];
}

/**
 * [RECONSTRUCTED - 0x00555CA0] (60 bytes)
 * CRefDungeon::GetCell (VTable Slot 5 @ +0x14)
 */
uint32_t CRefDungeon::GetCell(uint32_t x, uint32_t z) {
	uint32_t dwDim = static_cast<uint32_t>(m_vecBlocks.size());
	if (dwDim != 0 && x < dwDim && z < dwDim && m_pNavGrid != nullptr) {
		return m_pNavGrid[z * dwDim + x];
	}
	ServerFramework::ServerFramework_GenerateMiniDump();
	return 0xFFFFFFFF;
}

bool CRefDungeon::ReadNavData(FILE* fp) {
	if (fp == nullptr) {
		return false;
	}

	uint32_t dwBlockCount = static_cast<uint32_t>(m_vecBlocks.size());
	for (uint32_t i = 0; i < dwBlockCount; ++i) {
		CRefBlock* pBlock = new CRefBlock();

		uint32_t dwIndex = 0;
		uint32_t dwDimension = 0;
		uint32_t dwEntryCount = 0;

		if (std::fread(&dwIndex, 4, 1, fp) != 1 ||
			std::fread(&dwDimension, 4, 1, fp) != 1 ||
			std::fread(&dwEntryCount, 4, 1, fp) != 1) {
			delete pBlock;
			return false;
		}

		pBlock->Initialize(dwIndex, dwDimension, dwEntryCount);

		if (dwDimension > 0) {
			std::fread(pBlock->m_pCells, 4, dwDimension * dwDimension, fp);
		}

		uint32_t dwEntriesToRead = 0;
		std::fread(&dwEntriesToRead, 4, 1, fp);
		for (uint32_t e = 0; e < dwEntriesToRead; ++e) {
			uint32_t dwEntryIdx = 0;
			uint16_t wX = 0, wY = 0, wZ = 0;
			std::fread(&dwEntryIdx, 4, 1, fp);
			std::fread(&wX, 2, 1, fp);
			std::fread(&wY, 2, 1, fp);
			std::fread(&wZ, 2, 1, fp);

			D3DVECTOR vEntry = { static_cast<float>(wX), static_cast<float>(wY), static_cast<float>(wZ) };
			pBlock->SetEntry(dwEntryIdx, vEntry);
		}

		m_vecBlocks[i] = pBlock;
	}

	if (m_dwDungeonID > 0 && m_pNavGrid != nullptr) {
		std::fread(m_pNavGrid, 1, m_dwDungeonID * m_dwDungeonID * 2, fp);
	}

	return true;
}

// ============================================================================
// CAINavDataManager Implementation (Native 0x00555F80 / 0x00556070 / 0x00556400 / 0x005560E0 / 0x00556240)
// ============================================================================

CAINavDataManager::CAINavDataManager()
	: CSingletonT<CAINavDataManager>()
	, m_mapDungeons() {
	if (g_pAINavDataManager != nullptr && g_pAINavDataManager != this) {
		ServerFramework::ServerFramework_GenerateMiniDump();
	}
	g_pAINavDataManager = this;
}

CAINavDataManager::~CAINavDataManager() {
	ClearDungeons();
	g_pAINavDataManager = nullptr;
}

CRefDungeon* CAINavDataManager::GetOrCreateDungeonNavData(uint16_t wRegionID, uint32_t dwDungeonID) {
	auto it = m_mapDungeons.find(wRegionID);
	if (it != m_mapDungeons.end()) {
		return it->second;
	}

	CRefDungeon* pDungeon = new CRefDungeon();
	pDungeon->Initialize(wRegionID, dwDungeonID);
	m_mapDungeons[wRegionID] = pDungeon;
	return pDungeon;
}

/**
 * [RECONSTRUCTED - 0x005564F0] (48 bytes)
 * CAINavDataManager::FindDungeon
 */
CRefDungeon* CAINavDataManager::FindDungeon(uint16_t wRegionID) {
	auto it = m_mapDungeons.find(wRegionID);
	if (it != m_mapDungeons.end()) {
		return it->second;
	}
	return nullptr;
}

void CAINavDataManager::ClearDungeons() {
	for (auto& pair : m_mapDungeons) {
		if (pair.second != nullptr) {
			delete pair.second;
		}
	}
	m_mapDungeons.clear();
}

// ============================================================================
// CSimpleDungeon & Global Dungeon Registry Implementation
// Native addresses: 0x00555DD0, 0x0052FF00, 0x0052FF70, 0x00555E20, 0x00555F20
// ============================================================================

// Global simple dungeon registry @ 0x00CD7DF0
std::map<uint16_t, CSimpleDungeon*> g_mapSimpleDungeons;

/**
 * [RECONSTRUCTED - 0x00555DD0] (36 bytes)
 * CSimpleDungeon::CSimpleDungeon
 */
CSimpleDungeon::CSimpleDungeon()
	: m_wRegionID(0)
	, m_wPad(0)
	, m_vecBlocks() {
}

/**
 * [RECONSTRUCTED - 0x0052FF00] (63 bytes)
 * CSimpleDungeon::~CSimpleDungeon
 */
CSimpleDungeon::~CSimpleDungeon() {
	CleanUp();
}

/**
 * [RECONSTRUCTED - 0x0052FF70] (113 bytes)
 * CSimpleDungeon::CleanUp
 */
void CSimpleDungeon::CleanUp() {
	for (tagSimpleDungeonBlock* pBlock : m_vecBlocks) {
		if (pBlock != nullptr) {
			if (pBlock->m_pEntries != nullptr) {
				delete[] pBlock->m_pEntries;
				pBlock->m_pEntries = nullptr;
			}
			delete pBlock;
		}
	}
	m_vecBlocks.clear();
}

/**
 * [RECONSTRUCTED - 0x00555E20] (140 bytes)
 * CSimpleDungeon::AddBlock
 */
void* CSimpleDungeon::AddBlock(uint32_t dwBlockIndex, uint32_t dwEntryCount, D3DVECTOR* pEntries) {
	if (dwBlockIndex >= m_vecBlocks.size()) {
		m_vecBlocks.resize(dwBlockIndex + 1, nullptr);
	}

	if (m_vecBlocks[dwBlockIndex] != nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return nullptr;
	}

	tagSimpleDungeonBlock* pBlock = new tagSimpleDungeonBlock();
	pBlock->m_dwEntryCount = dwEntryCount;
	pBlock->m_pEntries = pEntries;

	m_vecBlocks[dwBlockIndex] = pBlock;
	return pBlock;
}

/**
 * [RECONSTRUCTED - 0x0053B220] (58 bytes)
 * CSimpleDungeonManager_Find
 */
CSimpleDungeon* CSimpleDungeonManager_Find(uint16_t wRegionID) {
	auto it = g_mapSimpleDungeons.find(wRegionID);
	if (it == g_mapSimpleDungeons.end()) {
		return nullptr;
	}
	return it->second;
}

/**
 * [RECONSTRUCTED - 0x00530070] (96 bytes)
 * CSimpleDungeonManager_CleanUp
 */
void CSimpleDungeonManager_CleanUp() {
	for (auto& pair : g_mapSimpleDungeons) {
		if (pair.second != nullptr) {
			delete pair.second;
			pair.second = nullptr;
		}
	}
	g_mapSimpleDungeons.clear();
}

/**
 * [RECONSTRUCTED - 0x00555F20] (84 bytes)
 * CSimpleDungeon_Register
 */
bool CSimpleDungeon_Register(CSimpleDungeon* pDungeon) {
	if (pDungeon == nullptr) {
		return false;
	}
	g_mapSimpleDungeons[pDungeon->m_wRegionID] = pDungeon;
	return true;
}

/**
 * [RECONSTRUCTED - 0x00556610] (568 bytes)
 * CAINavDataManager::LoadSimpleDungeonData (Primary overload)
 */
bool CAINavDataManager::LoadSimpleDungeonData() {
	// NOTE (Claude): renamed from CWorldManager::m_mapRegions, which was this game world map. Whether 0x00556610
	// takes its IDs from here is not verified.
	std::vector<uint16_t> vecRegionIDs;
	if (g_pGameWorldMgr != nullptr) {
		for (const auto& pair : g_pGameWorldMgr->m_mapGameWorld) {
			vecRegionIDs.push_back(static_cast<uint16_t>(pair.first));
		}
	}
	return LoadSimpleDungeonData(vecRegionIDs);
}

/**
 * [RECONSTRUCTED - 0x00556610] (568 bytes)
 * CAINavDataManager::LoadSimpleDungeonData (Native vector overload)
 */
bool CAINavDataManager::LoadSimpleDungeonData(const std::vector<uint16_t>& vecRegionIDs) {
	char szDir[MAX_PATH];
	::GetCurrentDirectoryA(sizeof(szDir), szDir);

	char szNavPath[MAX_PATH];
	std::snprintf(szNavPath, sizeof(szNavPath), "%s\\DATA\\navmesh", szDir);

	CFStream stream;
	uint8_t byVersion = 0;
	uint32_t dwOffset = 0;

	for (size_t i = 0; i < vecRegionIDs.size(); ++i) {
		uint16_t wRegionID = vecRegionIDs[i];
		if (g_mapSimpleDungeons.find(wRegionID) != g_mapSimpleDungeons.end()) {
			continue;
		}

		char szFileName[MAX_PATH];
		std::snprintf(szFileName, sizeof(szFileName), "AINavData_%d.DAT", static_cast<int32_t>(wRegionID));

		char szFilePath[MAX_PATH];
		std::snprintf(szFilePath, sizeof(szFilePath), "%s\\%s", szNavPath, szFileName);

		if (!stream.OpenFile(szFilePath)) {
			BSLib::Log_Printf(2, "Failed!! to Load SimpleDungeonData file [%s]!\n", szFileName);
			return false;
		}

		stream.ReadBytes(1, &byVersion);
		stream.ReadBytes(4, &dwOffset);

		if (stream.GetFileSize() <= dwOffset) {
			ServerFramework::ServerFramework_GenerateMiniDump();
			BSLib::Log_Printf(2, "SimpleDungeonData[%s] offset is Invalid!!\n", szFileName);
			return false;
		}

		stream.Seek(static_cast<int32_t>(dwOffset), SEEK_SET);

		uint16_t wDungeonRegionID = 0;
		uint32_t dwBlockCount = 0;
		stream.ReadBytes(2, &wDungeonRegionID);
		stream.ReadBytes(4, &dwBlockCount);

		if (CSimpleDungeonManager_Find(wDungeonRegionID) != nullptr) {
			ServerFramework::ServerFramework_GenerateMiniDump();
		}

		CSimpleDungeon* pDungeon = new CSimpleDungeon();
		pDungeon->m_wRegionID = wDungeonRegionID;
		pDungeon->m_vecBlocks.resize(dwBlockCount, nullptr);

		for (uint32_t dwBlock = 0; dwBlock < dwBlockCount; ++dwBlock) {
			uint32_t dwEntryCount = 0;
			stream.ReadBytes(4, &dwEntryCount);

			D3DVECTOR* pEntries = nullptr;
			if (dwEntryCount > 0) {
				pEntries = new D3DVECTOR[dwEntryCount];
				stream.ReadBytes(dwEntryCount * sizeof(D3DVECTOR), pEntries);
			}

			pDungeon->AddBlock(dwBlock, dwEntryCount, pEntries);
		}

		uint32_t dwTrailer = 0;
		stream.ReadBytes(4, &dwTrailer);
		if (dwTrailer != 0) {
			ServerFramework::ServerFramework_GenerateMiniDump();
			stream.Close();
			BSLib::Log_Printf(0x2000000, "Invalid SimpleDungeonData File [%s]\n", szFileName);
			CSimpleDungeonManager_CleanUp();
			return false;
		}

		BSLib::Log_Printf(0, "SimpleDungeonData Data [%s] Loaded\n", szFileName);
		CSimpleDungeon_Register(pDungeon);
		stream.Close();
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x00556240]
 * CAINavDataManager::LoadNavDataBinaryFile
 * Native implementation @ 0x00556240 (436 bytes)
 */
bool CAINavDataManager::LoadNavDataBinaryFile(const char* szFilePath) {
	if (szFilePath == nullptr) {
		return false;
	}

	FILE* fp = std::fopen(szFilePath, "rb");
	if (fp == nullptr) {
		BSLib::Log_Printf(0x2000000, "Load NavData [%s] Failed!!!\n", szFilePath);
		return false;
	}

	uint8_t byVersion = 0;
	if (std::fread(&byVersion, 1, 1, fp) != 1) {
		std::fclose(fp);
		BSLib::Log_Printf(0x2000000, "Load NavData [%s] Failed!!!\n", szFilePath);
		return false;
	}

	if (byVersion != 1) {
		std::fclose(fp);
		BSLib::Log_Printf(0x2000000, "AINavData Version is not match!!!\n");
		return false;
	}

	uint32_t dwHeaderPad = 0;
	std::fread(&dwHeaderPad, 4, 1, fp);

	uint16_t wRegionID = 0;
	uint32_t dwDungeonID = 0;
	if (std::fread(&wRegionID, 2, 1, fp) != 1 ||
		std::fread(&dwDungeonID, 4, 1, fp) != 1) {
		std::fclose(fp);
		BSLib::Log_Printf(0x2000000, "Invalid NavDataFile [%s]\n", szFilePath);
		return false;
	}

	CRefDungeon* pDungeon = GetOrCreateDungeonNavData(wRegionID, dwDungeonID);
	if (pDungeon == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		std::fclose(fp);
		BSLib::Log_Printf(0x2000000, "Create New Dungeon failed!!! file [%s]\n", szFilePath);
		return false;
	}

	pDungeon->ReadNavData(fp);

	uint32_t dwTailCheck = 0;
	std::fread(&dwTailCheck, 4, 1, fp);
	if (dwTailCheck != 0) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		std::fclose(fp);
		BSLib::Log_Printf(0x2000000, "Invalid NavDataFile [%s]\n", szFilePath);
		return false;
	}

	std::fclose(fp);
	return true;
}

/**
 * [RECONSTRUCTED - 0x005560E0]
 * CAINavDataManager::LoadNavDataFiles
 * Native implementation @ 0x005560E0 (326 bytes)
 */
bool CAINavDataManager::LoadNavDataFiles() {
	WIN32_FIND_DATAA findData;
	HANDLE hFind = ::FindFirstFileA("DATA\\navmesh\\AINavData*.*", &findData);
	if (hFind == INVALID_HANDLE_VALUE) {
		BSLib::Log_Printf(0x1000000, "There are no AI_NAVIGATION data files..\n");
		return true;
	}

	do {
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			char szFullPath[MAX_PATH];
			std::snprintf(szFullPath, sizeof(szFullPath), "DATA\\navmesh\\%s", findData.cFileName);
			if (!LoadNavDataBinaryFile(szFullPath)) {
				BSLib::Log_Printf(2, "Failed To Load AI_NAVIGATION Data File! [%s]\n", findData.cFileName);
				::FindClose(hFind);
				return false;
			}
			BSLib::Log_Printf(0, "AI_NAVIGATION Data [%s] Loaded\n", findData.cFileName);
		}
	} while (::FindNextFileA(hFind, &findData));

	::FindClose(hFind);
	return true;
}

bool AI_LoadNavDataBinaryFile(const char* szFilePath) {
	if (g_pAINavDataManager != nullptr) {
		return g_pAINavDataManager->LoadNavDataBinaryFile(szFilePath);
	}
	return false;
}

bool AI_LoadAINavDataFiles() {
	if (g_pAINavDataManager != nullptr) {
		return g_pAINavDataManager->LoadNavDataFiles();
	}
	return false;
}

// ============================================================================
// AI State Management & Allocation Implementation (Native 0x0055B030 - 0x0055B420)
// ============================================================================

/**
 * [RECONSTRUCTED - 0x0055B030]
 * CAIState_SPAWN::CAIState_SPAWN
 * Native implementation @ 0x0055B030 (83 bytes)
 */
CAIState_SPAWN::CAIState_SPAWN()
	: CAIState() {
	m_wStateId = 0x00;
}

/**
 * [RECONSTRUCTED - 0x0055B090]
 * CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN::CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN
 * Native implementation @ 0x0055B090 (93 bytes)
 */
CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN::CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN()
	: CAIState_SPAWN() {
	m_wStateId = 0x11;
}

/**
 * [RECONSTRUCTED - 0x0055B010]
 * CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN::OnTick
 * Native implementation @ 0x0055B010 (26 bytes)
 */
int32_t CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN::OnTick() {
	if (m_pTactics != nullptr) {
		m_pTactics->ChangeState(1, 0); // State 1 = IDLE
	}
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0055B120]
 * CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN::CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN
 * Native implementation @ 0x0055B120 (95 bytes)
 */
CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN::CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN()
	: CAIStateAllocator()
	, m_queStates(500, "CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN")
	, m_allocatedBlocks() {
}

/**
 * [RECONSTRUCTED - 0x0055B180]
 * CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN::~CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN
 * Native implementation @ 0x0055B180 (94 bytes)
 */
CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN::~CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN() {
	m_queStates.Clear();
	for (auto* pBlock : m_allocatedBlocks) {
		delete[] pBlock;
	}
	m_allocatedBlocks.clear();
}

/**
 * [RECONSTRUCTED - 0x0055B1E0 / 0x0055C5F0]
 * CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN::Allocate
 * Native implementation @ 0x0055B1E0 (8 bytes) / 0x0055C5F0 (352 bytes)
 */
void* CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN::Allocate() {
	CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN* pState = nullptr;
	if (m_queStates.Pop(pState) && pState != nullptr) {
		return pState;
	}

	constexpr size_t kChunkCount = 500;
	CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN* pChunk = new CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN[kChunkCount];
	m_allocatedBlocks.push_back(pChunk);
	for (size_t i = 1; i < kChunkCount; ++i) {
		m_queStates.Push(&pChunk[i]);
	}
	return &pChunk[0];
}

/**
 * [RECONSTRUCTED - 0x0055B1F0]
 * CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN::Free
 * Native implementation @ 0x0055B1F0 (63 bytes)
 */
void CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN::Free(void* pState) {
	if (pState == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return;
	}
	m_queStates.Push(static_cast<CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN*>(pState));
}

/**
 * [RECONSTRUCTED - 0x0055B250]
 * CAIStateManager::CAIStateManager
 * Native implementation @ 0x0055B250 (129 bytes)
 */
CAIStateManager::CAIStateManager()
	: CSingletonT<CAIStateManager>()
	, m_vecAllocators() {
	if (g_pAIStateManager != nullptr && g_pAIStateManager != this) {
		ServerFramework::ServerFramework_GenerateMiniDump();
	}
	g_pAIStateManager = this;
	m_vecAllocators.resize(2, nullptr);
}

/**
 * [RECONSTRUCTED - 0x0055B2E0 / 0x0055B300]
 * CAIStateManager::~CAIStateManager
 * Native implementation @ 0x0055B2E0 (30 bytes) / 0x0055B300 (126 bytes)
 */
CAIStateManager::~CAIStateManager() {
	CleanUp();
	g_pAIStateManager = nullptr;
}

/**
 * [RECONSTRUCTED - 0x0055B300]
 * CAIStateManager::CleanUp
 * Native implementation @ 0x0055B300 (126 bytes)
 */
void CAIStateManager::CleanUp() {
	ClearAllocators();
	m_vecAllocators.clear();
}

/**
 * [RECONSTRUCTED - 0x0055B420]
 * CAIStateManager::ClearAllocators
 * Native implementation @ 0x0055B420 (95 bytes)
 */
void CAIStateManager::ClearAllocators() {
	for (auto* pAlloc : m_vecAllocators) {
		if (pAlloc != nullptr) {
			delete pAlloc;
		}
	}
	m_vecAllocators.assign(2, nullptr);
}

void CAIStateManager::RegisterAllocator(uint32_t dwStateId, CAIStateAllocator* pAllocator) {
	if (dwStateId < 0x10) {
		return;
	}
	uint32_t nIdx = dwStateId - 0x10;
	if (nIdx >= m_vecAllocators.size()) {
		m_vecAllocators.resize(nIdx + 1, nullptr);
	}
	m_vecAllocators[nIdx] = pAllocator;
}

CAIStateAllocator* CAIStateManager::GetAllocator(uint32_t dwStateId) {
	if (dwStateId < 0x10) {
		return nullptr;
	}
	uint32_t nIdx = dwStateId - 0x10;
	if (nIdx < m_vecAllocators.size()) {
		return m_vecAllocators[nIdx];
	}
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x0055B380]
 * AI_CAIStateAllocator_Initialize
 * Native implementation @ 0x0055B380 (149 bytes)
 *
 * Registers custom state allocators (CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN)
 * into the AI state allocator registry vector at slot 1.
 */
bool AI_CAIStateAllocator_Initialize(CAIStateManager* pStateManager) {
	if (pStateManager == nullptr) {
		pStateManager = g_pAIStateManager;
	}
	if (pStateManager == nullptr) {
		return false;
	}

	if (pStateManager->m_vecAllocators.size() <= 1) {
		pStateManager->m_vecAllocators.resize(2, nullptr);
	}
	pStateManager->m_vecAllocators[1] = new CAllocator_CAISTATE_CUSTOM_SPAWN_SUMMON_SHYTAN();
	return true;
}

// ============================================================================
// tagPositionerRegionNode & CPositioner Implementation
// ============================================================================

/**
 * [RECONSTRUCTED - 0x0054C1C0] (148 bytes)
 * tagPositionerRegionNode::SelectRandomTactics
 *
 * Selects a random tactic from standby (if bStandby=true) or active list.
 * If list has only 1 element, returns front directly without rand().
 */
tagRefTactics* tagPositionerRegionNode::SelectRandomTactics(bool bStandby) {
	if (bStandby) {
		size_t nCount = m_vecStandby.size();
		if (nCount == 0) {
			return nullptr;
		}
		if (nCount == 1) {
			return m_vecStandby.front();
		}
		size_t nIdx = static_cast<size_t>(std::rand()) % nCount;
		return m_vecStandby[nIdx];
	} else {
		size_t nCount = m_vecActive.size();
		if (nCount == 0) {
			return nullptr;
		}
		if (nCount == 1) {
			return m_vecActive.front();
		}
		size_t nIdx = static_cast<size_t>(std::rand()) % nCount;
		return m_vecActive[nIdx];
	}
}

/**
 * [RECONSTRUCTED - 0x0054C300]
 * CPositionerRegionMap::Clear
 *
 * Destroys all allocated tagPositionerRegionNode instances and clears the map.
 */
void CPositionerRegionMap::Clear() {
	for (auto& pair : *this) {
		if (pair.second != nullptr) {
			delete pair.second;
		}
	}
	clear();
}

/**
 * [RECONSTRUCTED - 0x0054F780]
 * CPositionerRegionMap::FindRegionNode
 */
tagPositionerRegionNode* CPositionerRegionMap::FindRegionNode(uint32_t dwRegionID) {
	auto it = find(dwRegionID);
	if (it != end()) {
		return it->second;
	}
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x0054DF70]
 * CPositionerRegionMap::InsertRegionNode
 */
tagPositionerRegionNode* CPositionerRegionMap::InsertRegionNode(uint32_t dwRegionID, tagPositionerRegionNode* pNode) {
	(*this)[dwRegionID] = pNode;
	return pNode;
}

// ============================================================================
// CPositioner Implementation (Native 0x005552E0 - 0x005554D0)
// VTable @ 0x00AF96B4 (RTTI: .?AVCPositioner@@)
// Combat surround and approach slot manager
// ============================================================================

/**
 * [RECONSTRUCTED - 0x005552E0] (54 bytes)
 * CPositioner::CPositioner
 */
CPositioner::CPositioner()
	: m_nOccupiedSlots(0) {
	std::memset(m_pMonsters, 0, sizeof(m_pMonsters));
}

/**
 * [RECONSTRUCTED - 0x00555300] (23 bytes)
 * CPositioner::~CPositioner
 */
CPositioner::~CPositioner() {
	Reset();
}

/**
 * [RECONSTRUCTED - 0x00555330] (16 bytes)
 * CPositioner::Reset
 */
void CPositioner::Reset() {
	ClearSlots();
}

/**
 * [RECONSTRUCTED - 0x00555340] (274 bytes)
 * CPositioner::AssignApproachSlot
 */
int32_t CPositioner::AssignApproachSlot(const void* pTargetPos, AI::CTactics* pTactics) {
	if (pTactics == nullptr || pTargetPos == nullptr) {
		return -1;
	}

	if (m_nOccupiedSlots >= 8) {
		return -1;
	}

	CGObjChar* pOwner = pTactics->m_pOwner;
	if (pOwner == nullptr) {
		return -1;
	}

	const D3DVECTOR* pTgtPos = static_cast<const D3DVECTOR*>(pTargetPos);
	D3DVECTOR diff;
	diff.x = pOwner->m_fPosX - pTgtPos->x;
	diff.y = pOwner->m_fPosY - pTgtPos->y;
	diff.z = pOwner->m_fPosZ - pTgtPos->z;

	int32_t nPreferredSlot = AI_ApproachSlotFromVector(&diff);
	if (nPreferredSlot < 0 || nPreferredSlot >= 8) {
		nPreferredSlot = 0;
	}

	if (m_pMonsters[nPreferredSlot] == nullptr) {
		SetSlot(static_cast<uint32_t>(nPreferredSlot), pTactics);
		return nPreferredSlot;
	}

	uint32_t dwSearchState = 0;
	int32_t nAltSlot = FindAlternativeSlot(&dwSearchState, static_cast<uint32_t>(nPreferredSlot));
	if (nAltSlot >= 0 && nAltSlot < 8) {
		SetSlot(static_cast<uint32_t>(nAltSlot), pTactics);
		return nAltSlot;
	}

	return -1;
}

/**
 * [RECONSTRUCTED - 0x00555460] (32 bytes)
 * CPositioner::AssignApproachSlotHelper
 */
int32_t CPositioner::AssignApproachSlotHelper(AI::CTactics* pTactics) {
	if (pTactics == nullptr) {
		return -1;
	}
	CGObjChar* pOwner = pTactics->m_pOwner;
	if (pOwner == nullptr) {
		return -1;
	}
	D3DVECTOR pos = { pOwner->m_fPosX, pOwner->m_fPosY, pOwner->m_fPosZ };
	return AssignApproachSlot(&pos, pTactics);
}

/**
 * [RECONSTRUCTED - 0x00555480] (19 bytes)
 * CPositioner::ClearSlots
 */
void CPositioner::ClearSlots() {
	std::memset(m_pMonsters, 0, sizeof(m_pMonsters));
	m_nOccupiedSlots = 0;
}

/**
 * [RECONSTRUCTED - 0x005554A0] (23 bytes)
 * CPositioner::SetSlot
 */
void CPositioner::SetSlot(uint32_t nSlot, AI::CTactics* pTactics) {
	if (nSlot < 8) {
		if (m_pMonsters[nSlot] == nullptr) {
			m_nOccupiedSlots++;
		}
		m_pMonsters[nSlot] = pTactics;
	}
}

/**
 * [RECONSTRUCTED - 0x005554B0] (32 bytes)
 * CPositioner::ReleaseSlot
 */
void CPositioner::ReleaseSlot(uint32_t nSlot, AI::CTactics* pTactics) {
	if (nSlot < 8 && m_pMonsters[nSlot] == pTactics) {
		m_pMonsters[nSlot] = nullptr;
		if (m_nOccupiedSlots > 0) {
			m_nOccupiedSlots--;
		}
	}
}

/**
 * [RECONSTRUCTED - 0x005554D0] (107 bytes)
 * CPositioner::FindAlternativeSlot
 */
int32_t CPositioner::FindAlternativeSlot(uint32_t* pSearchState, uint32_t nPreferredSlot) {
	if (pSearchState == nullptr || nPreferredSlot >= 8) {
		return -1;
	}

	for (int32_t i = 1; i <= 4; ++i) {
		int32_t slotPlus = (nPreferredSlot + i) & 7;
		if (m_pMonsters[slotPlus] == nullptr) {
			return slotPlus;
		}
		int32_t slotMinus = (nPreferredSlot - i) & 7;
		if (m_pMonsters[slotMinus] == nullptr) {
			return slotMinus;
		}
	}

	return -1;
}

/**
 * [RECONSTRUCTED - 0x0055E3B0] (86 bytes)
 * AI_ApproachSlotFromVector
 *
 * Converts a 2D/3D directional difference vector into an approach slot (0..7).
 * Angle = atan2(diff.z, diff.x). Maps 360 degrees into 8 sectors of 45 deg.
 */
int32_t AI_ApproachSlotFromVector(const D3DVECTOR* pDiff) {
	if (pDiff == nullptr) {
		return 0;
	}
	float fAngle = std::atan2(pDiff->z, pDiff->x);
	if (fAngle < 0.0f) {
		fAngle += 6.283185307f; // 2 * PI
	}
	int32_t nSlot = static_cast<int32_t>((fAngle + 0.392699081f) / 0.785398163f) & 7;
	return nSlot;
}

// ============================================================================
// CGameAI Implementation
// ============================================================================

/**
 * [RECONSTRUCTED - 0x0054AC20]
 * CGameAI::CGameAI
 * Native implementation @ 0x0054AC20 (257 bytes)
 *
 * Sequence:
 *   1. Asserts singleton uniqueness (if g_pGameAI != nullptr -> GenerateMiniDump)
 *   2. Registers singleton instance g_pGameAI = this
 *   3. Initializes sub-objects:
 *        - m_poolTactics (+0x04)
 *        - m_callbacker (+0x28)
 *        - m_pSquadManager = nullptr (+0x3C)
 *        - m_queAIMsg (+0x40, name: "AI::CMsg")
 *        - m_factoryAIMsg (+0x9C, name: "AI::CAIMsg")
 *        - m_mapPatrolRegions (+0x118)
 *        - m_mapActiveAI (+0x124)
 *        - m_vecTacticsToRemove (+0x134)
 *        - m_positioner (+0x140)
 *        - m_dwLastTickTime = 0 (+0x168)
 */
CGameAI::CGameAI()
	: CSingletonT<CGameAI>()
	, m_poolTactics()
	, m_callbacker()
	, m_pSquadManager(nullptr)
	, m_queAIMsg(0, "AI::CMsg")
	, m_factoryAIMsg()
	, m_mapPatrolRegions()
	, m_mapActiveAI()
	, m_vecTacticsToRemove()
	, m_positioner()
	, m_dwLastTickTime(0) {
	if (g_pGameAI != nullptr && g_pGameAI != this) {
		ServerFramework::ServerFramework_GenerateMiniDump();
	}
	g_pGameAI = this;
}

/**
 * [RECONSTRUCTED - 0x0054AD30 / 0x0054AE40]
 * CGameAI::~CGameAI
 * Native implementation @ 0x0054AD30 (26 bytes) / 0x0054AE40 (245 bytes)
 */
CGameAI::~CGameAI() {
	CleanUp();
	if (g_pGameAI == this) {
		g_pGameAI = nullptr;
	}
}

/**
 * [RECONSTRUCTED - 0x0054B1D0]
 * CGameAI::Initialize
 * Native implementation @ 0x0054B1D0 (350 bytes)
 *
 * Bootstrapped during game startup by CGame::InitializeAISubsystem (@ 0x00413670).
 */
bool CGameAI::Initialize() {
	m_dwLastTickTime = ::GetTickCount();
	g_dwGameAICurrentTick = m_dwLastTickTime; // 0x0054B1FB mov dword [0xC82620], eax

	// Step 1: Load positioner data (0x0054BDE0)
	LoadPositionerData();

	// Step 2: Initialize patrol regions (0x0054B710)
	InitializePatrolRegions();

	// Step 3: Preallocate message instances (0x0054F0A0, 10,000 count)
	if (!m_factoryAIMsg.Preallocate(10000)) {
		return false;
	}

	// Step 4: Allocate squad manager (0x0054A890, 0xCC bytes)
	m_pSquadManager = new AI::CSquadManager();
	AI_BuildApproachDirectionTable(m_pSquadManager);

	// Step 5: Initialize tactics instance pool (0x0054D5E0, 50,000 capacity)
	m_poolTactics.Initialize(50000);

	// Step 6: Load AI Navigation Data (0x005560E0)
	if (g_pAINavDataManager == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
	}
	AI_LoadAINavDataFiles();

	// Step 7: Initialize AI state allocator (0x0055B380)
	if (g_pAIStateManager == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
	}
	AI_CAIStateAllocator_Initialize(g_pAIStateManager);

	// Step 8: Register periodic patrol callback (0x0054D8D0, 30.0s interval)
	m_callbacker.RegisterCallback(this, 30.0f, [](CGameAI* pAI) {
		if (pAI != nullptr) {
			pAI->OnScheduledPatrolTick();
		}
	}, 0);

	m_dwLastTickTime = 0;

	// Step 9: Build 36 approach direction vector lookup table (0x0054AB10)
	AI_BuildDirectionVectorTable();

	// Step 10: Validate all nest paths across regions (0x0054C000)
	return ValidateAllNestPaths();
}

/**
 * [RECONSTRUCTED - 0x0054AF40]
 * CGameAI::CleanUp
 * Native implementation @ 0x0054AF40 (454 bytes)
 */
void CGameAI::CleanUp() {
	// Step 1: Clean and free patrol regions (Native 0x0054AF77 - 0x0054AFC8)
	for (auto& pair : m_mapPatrolRegions) {
		if (pair.second != nullptr) {
			for (auto& nestPair : *pair.second) {
				if (nestPair.second != nullptr) {
					delete nestPair.second;
				}
			}
			delete pair.second;
		}
	}
	m_mapPatrolRegions.clear();

	// Step 2: Clean and free active AI tactics
	for (auto& pair : m_mapActiveAI) {
		if (pair.second != nullptr) {
			delete pair.second;
		}
	}
	m_mapActiveAI.clear();

	// Step 3: Destroy all AI messages and clear message factory (0x0054B080 - 0x0054B130)
	m_factoryAIMsg.DestroyAllMessages();

	// Step 4: Clear pending tactics release queue
	m_vecTacticsToRemove.clear();

	// Step 5: Clear tactics instance pool (0x0054B148)
	m_poolTactics.Clear();

	// Step 6: Delete squad manager (Native 0x0054AF70: (**ecx_12)(1))
	if (m_pSquadManager != nullptr) {
		delete m_pSquadManager;
		m_pSquadManager = nullptr;
	}

	// Step 7: Clear positioner data (Native 0x0054C300 called at 0x0054AF40)
	ClearPositionerData();

	m_dwLastTickTime = 0;
}

/**
 * [RECONSTRUCTED - 0x0054BDE0]
 * CGameAI::LoadPositionerData
 * Native implementation @ 0x0054BDE0 (440 bytes)
 *
 * Iterates through g_pRefData's nest tactics (at +0x238 in CReferenceData) and organizes them into
 * active and standby waypoint queues per region inside m_positioner.
 */
bool CGameAI::LoadPositionerData() {
	if (g_pRefData == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return false;
	}

	for (auto& pair : g_pRefData->m_mapRefTactics) {
		tagRefTactics* pTactics = reinterpret_cast<tagRefTactics*>(pair.second);
		if (pTactics == nullptr) {
			continue;
		}

		uint32_t dwTargetID = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pTactics) + 0x04);
		if (dwTargetID != 0 || pTactics->m_dwTacticsID == 3000) {
			tagPositionerRegionNode* pNode = m_positioner.FindRegionNode(dwTargetID);
			if (pNode == nullptr) {
				pNode = new tagPositionerRegionNode();
				m_positioner.InsertRegionNode(dwTargetID, pNode);
			}

			if (pTactics->m_dwChampionTacticsID == 0) { // tagRefTactics +0x8C (renamed from m_dwReplaceTacticsID)
				pNode->m_vecStandby.push_back(pTactics);
			} else {
				pNode->m_vecActive.push_back(pTactics);
			}
		}
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x0054C2A0] (57 bytes)
 * CGameAI::FindPositionerRegionNode
 *
 * Looks up region node in m_positioner hash map.
 */
tagPositionerRegionNode* CGameAI::FindPositionerRegionNode(uint32_t dwRegionID) {
	return m_positioner.FindRegionNode(dwRegionID);
}

/**
 * [RECONSTRUCTED - 0x0054C0E0] (223 bytes)
 * CGameAI::SelectSpawnTactics
 *
 * Resolves spawn tactics for given nest/region context.
 * Probes primary ID (+0x08) then secondary ID (+0x0C).
 * Alternates between standby and active queues based on requested bStandby flag.
 */
tagRefTactics* CGameAI::SelectSpawnTactics(void* pContext, bool bStandby) {
	if (pContext != nullptr) {
		uint32_t dwPrimaryID = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pContext) + 0x08);
		tagPositionerRegionNode* pNode = FindPositionerRegionNode(dwPrimaryID);
		if (pNode != nullptr) {
			tagRefTactics* pTactics = pNode->SelectRandomTactics(bStandby);
			if (pTactics != nullptr) {
				return pTactics;
			}
			return pNode->SelectRandomTactics(!bStandby);
		}

		uint32_t dwSecondaryID = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pContext) + 0x0C);
		pNode = FindPositionerRegionNode(dwSecondaryID);
		if (pNode != nullptr) {
			tagRefTactics* pTactics = pNode->SelectRandomTactics(bStandby);
			if (pTactics != nullptr) {
				return pTactics;
			}
			return pNode->SelectRandomTactics(!bStandby);
		}
	} else {
		tagPositionerRegionNode* pNode = FindPositionerRegionNode(0);
		if (pNode != nullptr && !pNode->m_vecStandby.empty()) {
			return pNode->m_vecStandby.front();
		}
	}
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x0054C300] (192 bytes)
 * CGameAI::ClearPositionerData
 *
 * Destroys all allocated region node entries and empties the positioner map.
 */
void CGameAI::ClearPositionerData() {
	m_positioner.Clear();
}

/**
 * [RECONSTRUCTED - 0x0054A800] (137 bytes)
 * tagHiveSpawnState::tagHiveSpawnState
 * Declared in AIHive.h; the body sits in the GameAI address range.
 */
tagHiveSpawnState::tagHiveSpawnState()
	: dwSampleSum(0)
	, dwSampleCount(0)
	, dwAverage(0)
	, nHistoryIndex(0)
	, dwLastHatchTime(g_dwGameAICurrentTick)
	, dwLastAverage(0)
	, dwCurCount(0)
	, dwUnk34(0)
	, fIncreaseRate(0.0f)
	, pSelectedNest(nullptr) {
	dwHistory[0] = 0;
	dwHistory[1] = 0;
}

/**
 * [RECONSTRUCTED - 0x0054BBF0] (481 bytes, eax = layer, retn 4)
 * AI_InitHiveSpawnStates
 * CORRECTION (Claude): replaces AI_CAIHive_BindTacticsMap, which copied the wrong _RefNest fields.
 * Nest states are filled by loop index over m_vecNest (NestID order) while CAIHive and CNest read
 * them by CNest::m_dwIndex (creation order). This original mismatch is kept.
 */
bool AI_InitHiveSpawnStates(CGameWorldLayer* pLayer, std::map<uint32_t, AI::CAIHive*>* pHiveMap) {
	pLayer->m_vecHiveState.resize(pHiveMap->size());

	for (std::map<uint32_t, AI::CAIHive*>::iterator it = pHiveMap->begin(); it != pHiveMap->end(); ++it) {
		AI::CAIHive* pHive = it->second;
		tagHiveSpawnState* pHiveState = &pLayer->m_vecHiveState[pHive->m_dwIndex];

		tagNestSpawnState nestState;
		nestState.dwLastHatchTime = 0;
		nestState.dwHatchDelay = 0;
		nestState.dwCurCount = 0;
		nestState.fIncreaseRate = 0.0f;
		nestState.dwDelayReduction = 0;
		nestState.bAutoHatch = 1;
		nestState.dwRespawn = 1;
		nestState.nRemainCount = 0;

		pHiveState->dwUnk34 = 0;
		pHiveState->vecNest.resize(pHive->m_mapNest.size(), nestState);

		for (uint32_t i = 0; i < pHive->m_mapNest.size(); ++i) {
			AI::CNest* pNest = pHive->m_vecNest[i];
			tagNestSpawnState* pNestState = &pHiveState->vecNest[i];

			pNestState->dwLastHatchTime = 0;
			pNestState->bAutoHatch = (pNest->m_pRefNest->m_btType == 0);
			pNestState->dwRespawn = pNest->m_pRefNest->m_btRespawn;
			pNestState->nRemainCount = pNest->m_pRefNest->m_dwMaxTotalCount;
			pNest->RollHatchDelay(pNestState);
		}

		pHiveState->pSelectedNest = pHive->SelectRandomNest();
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x0054B8D0]
 * CGameAI::PopulateRegionNests
 * Native implementation @ 0x0054B8D0 (637 bytes)
 *
 * Scans g_pRefData->m_mapRefNest for region templates, instantiating
 * event hives (type 2) into both temp map and patrol region, and
 * standard hives (type 1) into patrol region.
 */
bool CGameAI::PopulateRegionNests(uint16_t wRegionID, std::map<uint32_t, AI::CAIHive*>* pTempHiveMap, tagAIPatrolRegion* pPatrolRegion) {
	if (g_pRefData == nullptr || pPatrolRegion == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return false;
	}

	// CORRECTION (Claude): these rows are _RefHive (g_pRefData+0x250); the filter is GameWorldID (+0x1A)
	// and HatchObjType (+0x1C), and Initialize takes (pRefHive, wGameWorldID, dwIndex) (0x0054B9C9..0x0054B9D2).
	// Pass 1 (0x0054B98C..0x0054BA22) indexes by pTempHiveMap->size() and inserts into both maps;
	// pass 2 (0x0054BAA4..0x0054BB1C) tests HatchObjType 1 and indexes/inserts into pPatrolRegion only.
	// Pass 1: HatchObjType 2
	for (auto& pair : g_pRefData->m_mapRefHive) {
		tagRefHive* pRefHive = pair.second;
		if (pRefHive != nullptr && pRefHive->m_wGameWorldID == wRegionID && pRefHive->m_wHatchObjType == 2) {
			AI::CAIHive* pHive = new AI::CAIHive();
			uint32_t dwIndex = static_cast<uint32_t>(pTempHiveMap->size()); // 0x0054B9C9 mov ecx, dword [ebp+0x8]
			pHive->Initialize(pRefHive, wRegionID, dwIndex);

			if (!pHive->m_mapNest.empty()) {
				if (pTempHiveMap != nullptr) {
					(*pTempHiveMap)[pRefHive->m_dwHiveID] = pHive;
				}
				(*pPatrolRegion)[pRefHive->m_dwHiveID] = pHive;
			} else {
				delete pHive;
			}
		}
	}

	// Pass 2: HatchObjType 1
	for (auto& pair : g_pRefData->m_mapRefHive) {
		tagRefHive* pRefHive = pair.second;
		if (pRefHive != nullptr && pRefHive->m_wGameWorldID == wRegionID && pRefHive->m_wHatchObjType == 1) {
			AI::CAIHive* pHive = new AI::CAIHive();
			uint32_t dwIndex = static_cast<uint32_t>(pPatrolRegion->size());
			pHive->Initialize(pRefHive, wRegionID, dwIndex);

			if (!pHive->m_mapNest.empty()) {
				(*pPatrolRegion)[pRefHive->m_dwHiveID] = pHive;
			} else {
				delete pHive;
			}
		}
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x0054BB60] (136 bytes, retn 0xC; `this` is not used)
 * CGameAI::BindNestTactics
 * Layer 0 gets spawn states for the HatchObjType 2 hives (pTempHiveMap); every layer in
 * CGameWorld::m_listLayer gets states for all hives (pPatrolRegion).
 * CORRECTION (Claude): 0x0054BB76..0x0054BBC4 take (wGameWorldID, pTempHiveMap, pPatrolRegion), give layer 0
 * the temp map and walk the layer list; the previous version gave layer 0 the patrol map, skipped the list
 * and returned true for a missing layer 0.
 */
bool CGameAI::BindNestTactics(uint16_t wGameWorldID, std::map<uint32_t, AI::CAIHive*>* pTempHiveMap, tagAIPatrolRegion* pPatrolRegion) {
	ASSERT(g_pGameWorldMgr);
	CGameWorld* pGameWorld = g_pGameWorldMgr->FindGameWorld(wGameWorldID);
	if (pGameWorld == nullptr) {
		return false;
	}

	CGameWorldLayer* pLayer = pGameWorld->GetLayer(0);
	if (pLayer == nullptr) {
		return false;
	}

	AI_InitHiveSpawnStates(pLayer, pTempHiveMap);

	for (std::list<CGameWorldLayer*>::iterator it = pGameWorld->m_listLayer.begin(); it != pGameWorld->m_listLayer.end(); ++it) {
		AI_InitHiveSpawnStates(*it, pPatrolRegion);
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x0054B710] (443 bytes)
 * CGameAI::InitializePatrolRegions
 * One patrol hive map per game world, keyed by its _RefGame_World ID. A duplicate ID is asserted and
 * the first map stays (insert).
 * CORRECTION (Claude): 0x0054B747..0x0054B8B1 walk CGameWorldMgr::m_mapGameWorld by count, skip NULL worlds,
 * key by tagRefGameWorld::m_wGameWorldID and insert; the previous version walked an invented region map
 * and overwrote duplicates.
 */
bool CGameAI::InitializePatrolRegions() {
	std::map<uint32_t, AI::CAIHive*> mapTempHive;

	ASSERT(g_pGameWorldMgr);
	uint16_t wGameWorldCount = static_cast<uint16_t>(g_pGameWorldMgr->m_mapGameWorld.size());
	std::map<uint32_t, CGameWorld*>::iterator it = g_pGameWorldMgr->m_mapGameWorld.begin();
	for (uint16_t i = 0; i < wGameWorldCount; ++i, ++it) {
		CGameWorld* pGameWorld = it->second;
		if (pGameWorld == nullptr) {
			continue;
		}

		uint16_t wGameWorldID = pGameWorld->m_pRefGameWorld->m_wGameWorldID;
		mapTempHive.clear();

		tagAIPatrolRegion* pPatrolRegion = new tagAIPatrolRegion;
		PopulateRegionNests(wGameWorldID, &mapTempHive, pPatrolRegion);
		BindNestTactics(wGameWorldID, &mapTempHive, pPatrolRegion);

		ASSERT(m_mapPatrolRegions.find(wGameWorldID) == m_mapPatrolRegions.end());
		m_mapPatrolRegions.insert(std::map<uint16_t, tagAIPatrolRegion*>::value_type(wGameWorldID, pPatrolRegion));
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x0054C000]
 * CGameAI::ValidateAllNestPaths
 * Native implementation @ 0x0054C000 (208 bytes)
 *
 * Validates path connectivity and performs initial monster spawns
 * for all nests across all active patrol regions.
 */
bool CGameAI::ValidateAllNestPaths() {
	for (auto& regionPair : m_mapPatrolRegions) {
		tagAIPatrolRegion* pZoneMap = regionPair.second;
		if (pZoneMap == nullptr) {
			continue;
		}

		for (auto& nestPair : *pZoneMap) {
			AI::CAIHive* pHive = nestPair.second;
			if (pHive == nullptr) {
				continue;
			}

			if (!pHive->HatchNPCs(regionPair.first)) { // 0x0054C076 mov eax, ebx (world ID)
				return false;
			}
		}
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x0054C450] (145 bytes)
 * CGameAI::FindNestInRegion
 *
 * Locates a hive by HiveID in the patrol region, then its nest by NestID (CAIHive::FindNest 0x0055F080).
 */
AI::CNest* CGameAI::FindNestInRegion(uint16_t wRegionID, uint32_t dwHiveID, uint32_t dwNestID) {
	auto itRegion = m_mapPatrolRegions.find(wRegionID);
	if (itRegion == m_mapPatrolRegions.end() || itRegion->second == nullptr) {
		return nullptr;
	}

	auto itHive = itRegion->second->find(dwHiveID);
	if (itHive == itRegion->second->end() || itHive->second == nullptr) {
		return nullptr;
	}

	return itHive->second->FindNest(dwNestID);
}

/**
 * [RECONSTRUCTED - 0x0054C4F0] (177 bytes)
 * CGameAI::FindNestInAnyHive
 *
 * Scans all hives in the patrol region for the nest keyed by dwNestID.
 */
AI::CNest* CGameAI::FindNestInAnyHive(uint16_t wRegionID, uint32_t dwNestID) {
	auto itRegion = m_mapPatrolRegions.find(wRegionID);
	if (itRegion == m_mapPatrolRegions.end() || itRegion->second == nullptr) {
		return nullptr;
	}

	for (auto& hivePair : *itRegion->second) {
		AI::CAIHive* pHive = hivePair.second;
		if (pHive != nullptr) {
			AI::CNest* pNest = pHive->FindNest(dwNestID);
			if (pNest != nullptr) {
				return pNest;
			}
		} else {
			ServerFramework::ServerFramework_GenerateMiniDump();
		}
	}

	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x0054C5B0] (153 bytes)
 * CGameAI::FindHiveInRegion
 *
 * Looks up CAIHive instance by hive ID within the specified patrol region.
 */
AI::CAIHive* CGameAI::FindHiveInRegion(uint16_t wRegionID, uint32_t dwHiveID) {
	auto itRegion = m_mapPatrolRegions.find(wRegionID);
	if (itRegion == m_mapPatrolRegions.end() || itRegion->second == nullptr) {
		return nullptr;
	}

	auto itHive = itRegion->second->find(dwHiveID);
	if (itHive != itRegion->second->end()) {
		return itHive->second;
	}

	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x0054C660] (61 bytes)
 * CGameAI::PostAIMessage
 *
 * Enqueues an asynchronous AI message into m_queAIMsg.
 */
bool CGameAI::PostAIMessage(AI::CAIMsg* pMsg) {
	if (pMsg == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return false;
	}
	return m_queAIMsg.Push(pMsg);
}

/**
 * [RECONSTRUCTED - 0x0054C6A0]
 * CGameAI::CreateAIMessage
 * Native implementation @ 0x0054C6A0 (48 bytes)
 *
 * Allocates and initializes an asynchronous AI message from m_factoryAIMsg.
 */
AI::CAIMsg* CGameAI::CreateAIMessage(uint16_t wMsgID) {
	AI::CAIMsg* pMsg = m_factoryAIMsg.Alloc();
	if (pMsg == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return nullptr;
	}
	pMsg->m_wMsgID = wMsgID;
	pMsg->ResetPayload();
	return pMsg;
}

/**
 * [RECONSTRUCTED - 0x0053DA50] (68 bytes)
 * CGameAI::MarkTacticsForRemoval
 *
 * Validates tactics membership in m_mapActiveAI and schedules it
 * for cleanup in m_vecTacticsToRemove.
 */
bool CGameAI::MarkTacticsForRemoval(uint32_t dwPoolIndex) {
	auto it = m_mapActiveAI.find(dwPoolIndex);
	if (it == m_mapActiveAI.end()) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		BSLib::Log_Printf(0x2000000, "Trying to delete tactics is already deleted!!!\n");
		return false;
	}

	m_vecTacticsToRemove.push_back(dwPoolIndex);
	return true;
}

/**
 * [RECONSTRUCTED - 0x0054C6E0] (471 bytes)
 * CGameAI::OnScheduledPatrolTick
 * Periodic PC-count sampling for every hive on every instance layer (index 1..n).
 * CORRECTION (Claude): the world is CGameWorldMgr::FindGameWorld and the layers are CGameWorld::m_vecLayer.
 * The native starts the context at {1, 1} (0x0054C708), has no manager NULL return or NULL tests on the
 * patrol map, hive or ref hive, and subscripts the hive state without a range test (0x0054C81E..0x0054C832).
 */
void CGameAI::OnScheduledPatrolTick() {
	tagRegionContext context;
	context.wGameWorldID = 1;
	context.wLayerID = 1;

	for (std::map<uint16_t, tagAIPatrolRegion*>::iterator it = m_mapPatrolRegions.begin(); it != m_mapPatrolRegions.end(); ++it) {
		context.wGameWorldID = it->first;
		tagAIPatrolRegion* pPatrolRegion = it->second;

		ASSERT(g_pGameWorldMgr);
		CGameWorld* pGameWorld = g_pGameWorldMgr->FindGameWorld(it->first);
		if (pGameWorld == nullptr) {
			ASSERT(false);
			continue;
		}

		uint16_t wLayerCount = static_cast<uint16_t>(pGameWorld->m_vecLayer.size());
		for (uint16_t wLayer = 1; wLayer < wLayerCount; ++wLayer) {
			CGameWorldLayer* pLayer = pGameWorld->m_vecLayer[wLayer];
			if (pLayer == nullptr) {
				continue;
			}

			context.wLayerID = pLayer->m_wLayerID;
			for (tagAIPatrolRegion::iterator itHive = pPatrolRegion->begin(); itHive != pPatrolRegion->end(); ++itHive) {
				AI::CAIHive* pHive = itHive->second;
				if (pHive->m_pRefHive->m_wHatchObjType == 2) { // 0x0054C817
					continue;
				}
				pHive->UpdateIncreaseRate(&context, &pLayer->m_vecHiveState[pHive->m_dwIndex]);
			}
		}
	}
}

/**
 * [RECONSTRUCTED - 0x0054B300]
 * CGameAI::OnTick
 * Native implementation @ 0x0054B300 (1026 bytes)
 *
 * Main AI Subsystem frame tick routine.
 */
uint32_t CGameAI::OnTick(float /*fDeltaTime*/) {
	uint32_t dwNow = ::GetTickCount();
	g_dwGameAICurrentTick = dwNow; // 0x0054B315 mov dword [0xC82620], eax
	uint32_t dwDeltaMs = dwNow - m_dwLastTickTime;
	float fDeltaSec = static_cast<float>(dwDeltaMs) / 1000.0f;
	if (fDeltaSec < 0.0f) {
		fDeltaSec = 0.0f;
	} else if (fDeltaSec > 0.2f) {
		fDeltaSec = 0.2f;
	}

	// ------------------------------------------------------------------------
	// Phase 1: hive hatching on every instance layer (0x0054B368 - 0x0054B515)
	// CORRECTION (Claude): the world is CGameWorldMgr::FindGameWorld and the layers are CGameWorld::m_vecLayer
	// from index 1. The native has no NULL tests on the patrol map, hive or ref hive, and an out-of-range
	// hive index is asserted and then subscripted (0x0054B4A5..0x0054B4CE), not skipped.
	// ------------------------------------------------------------------------
	for (std::map<uint16_t, tagAIPatrolRegion*>::iterator it = m_mapPatrolRegions.begin(); it != m_mapPatrolRegions.end(); ++it) {
		uint16_t wGameWorldID = it->first;
		tagAIPatrolRegion* pPatrolRegion = it->second;

		ASSERT(g_pGameWorldMgr);
		CGameWorld* pGameWorld = g_pGameWorldMgr->FindGameWorld(wGameWorldID);
		if (pGameWorld == nullptr) {
			ASSERT(false);
			continue;
		}

		for (tagAIPatrolRegion::iterator itHive = pPatrolRegion->begin(); itHive != pPatrolRegion->end(); ++itHive) {
			AI::CAIHive* pHive = itHive->second;
			if (pHive->m_pRefHive->m_wHatchObjType == 2) {
				continue;
			}

			uint32_t dwIndex = pHive->m_dwIndex;
			uint16_t wLayerCount = static_cast<uint16_t>(pGameWorld->m_vecLayer.size());
			for (uint16_t wLayer = 1; wLayer < wLayerCount; ++wLayer) {
				CGameWorldLayer* pLayer = pGameWorld->m_vecLayer[wLayer];
				if (pLayer == nullptr) {
					continue;
				}

				ASSERT(dwIndex < pLayer->m_vecHiveState.size());
				pHive->Update(&pLayer->m_vecHiveState[dwIndex], wGameWorldID, pLayer->m_wLayerID); // 0x0054B4E5
			}
		}
	}

	// ------------------------------------------------------------------------
	// Phase 2: Tactics Release & Garbage Collection (0x0054B455 - 0x0054B544)
	// ------------------------------------------------------------------------
	if (!m_vecTacticsToRemove.empty()) {
		for (uint32_t dwTacticsID : m_vecTacticsToRemove) {
			auto it = m_mapActiveAI.find(dwTacticsID);
			if (it != m_mapActiveAI.end()) {
				AI::CTactics* pTactics = it->second;
				m_mapActiveAI.erase(it);
				if (pTactics != nullptr) {
					m_poolTactics.Release(pTactics);
				}
			}
		}
		m_vecTacticsToRemove.clear();
	}

	// ------------------------------------------------------------------------
	// Phase 3: Active Tactics Update (0x0054B545 - 0x0054B5AF)
	// ------------------------------------------------------------------------
	static bool s_bNullTacticsLogged = false;
	for (auto it = m_mapActiveAI.begin(); it != m_mapActiveAI.end(); ) {
		AI::CTactics* pTactics = it->second;
		if (pTactics != nullptr) {
			// Virtual slot 14 (+0x38): Update() @ 0x0053FEA0
			pTactics->Update();
			++it;
		} else {
			if (!s_bNullTacticsLogged) {
				BSLib::Log_Printf(0x2000001,
					"update tactics::tactics is null!! active tactics count(%d)",
					static_cast<uint32_t>(m_mapActiveAI.size()));
				s_bNullTacticsLogged = true;
			}
			it = m_mapActiveAI.erase(it);
		}
	}

	// ------------------------------------------------------------------------
	// Phase 4: Scheduled Callbacker & Completion (0x0054B5B0 - 0x0054B5D6)
	// ------------------------------------------------------------------------
	m_callbacker.Update(fDeltaSec);
	m_dwLastTickTime = dwNow;
	return dwNow;
}

/**
 * [RECONSTRUCTED - 0x00545450]
 * Vec3_AngleBetween
 *
 * Computes angle in radians between two 3D vectors via dot product:
 * cos(theta) = (v1 . v2) / (|v1| * |v2|)
 */
float Vec3_AngleBetween(const D3DVECTOR* pV1, const D3DVECTOR* pV2) {
	if (pV1 == nullptr || pV2 == nullptr) {
		return 0.0f;
	}

	float dot = (pV1->x * pV2->x) + (pV1->y * pV2->y) + (pV1->z * pV2->z);
	float lenSq1 = (pV1->x * pV1->x) + (pV1->y * pV1->y) + (pV1->z * pV1->z);
	float lenSq2 = (pV2->x * pV2->x) + (pV2->y * pV2->y) + (pV2->z * pV2->z);

	float denom = std::sqrt(lenSq1) * std::sqrt(lenSq2);
	if (denom <= 0.00001f) {
		return 0.0f;
	}

	float cosTheta = dot / denom;
	if (cosTheta < -1.0f) cosTheta = -1.0f;
	if (cosTheta > 1.0f) cosTheta = 1.0f;

	return std::acos(cosTheta);
}

/**
 * [RECONSTRUCTED - 0x0053DAD0] (148 bytes)
 * AI_GetApproachDirectionIndexFromVector
 *
 * Calculates the approach direction slot index [0..35] from a 3D direction vector:
 * 1. Inverts vector (-x, 0, -z)
 * 2. Computes angle relative to reference (1, 0, 0)
 * 3. Converts to degrees (rad * 57.29578)
 * 4. Divides by 10.0 degrees per slot -> 36 total slots
 */
int32_t AI_GetApproachDirectionIndexFromVector(const D3DVECTOR* pVec) {
	if (pVec == nullptr) {
		return 0;
	}

	D3DVECTOR vNeg;
	vNeg.x = -pVec->x;
	vNeg.y = 0.0f;
	vNeg.z = -pVec->z;

	D3DVECTOR vRef = { 1.0f, 0.0f, 0.0f };

	float fAngleDeg = Vec3_AngleBetween(&vRef, &vNeg) * 57.295780181884766f;

	int32_t nSlot = static_cast<int32_t>(fAngleDeg / 10.0f);
	if (nSlot < 0 || nSlot >= 36) {
		ServerFramework::ServerFramework_GenerateMiniDump();
	}
	if (nSlot < 0) {
		return 0;
	}
	if (nSlot >= 35) {
		return 35;
	}
	return nSlot;
}

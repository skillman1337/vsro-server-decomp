/**
 * ============================================================================
 * Silkroad Online - Game Server AI Hive / Nest Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\AIHive.cpp
 *
 * Compilation unit 0x0055E440 - 0x0056105F (65 functions, BN snapshot 18). Implements:
 *   - AI::CAIHive  0x0055E440 ctor, 0x0055E530 dtor, 0x0055E5B0 Initialize, 0x0055E770 SetRefHive,
 *                  0x0055E7C0 CreateNest, 0x0055E910 Update, 0x0055E9B0 GetMaxTotalCount,
 *                  0x0055EA20 HatchNPCs, 0x0055EA90 HatchPerNest, 0x0055EC10 HatchOverwriteTotal,
 *                  0x0055EDB0 Release, 0x0055EDC0 DeleteNests, 0x0055EEC0 OnMonsterDead,
 *                  0x0055EFE0 SelectRandomNest, 0x0055F080 FindNest, 0x0055F0E0 UpdateIncreaseRate,
 *                  0x0055F190 CalcIncreaseRate
 *   - AI::CNest    0x00560450 ctor, 0x005604A0 dtor, 0x005604B0 Initialize, 0x00560570 BindRefData,
 *                  0x005606C0 HatchNPC, 0x005607B0 Hatch, 0x00560D00 OnMonsterDead,
 *                  0x00560E40 RollHatchDelay, 0x00560E90 GetPCCount
 *   - tagNestSpawnState helpers 0x00560380 / 0x00560DC0 / 0x00560DF0 / 0x00560E10 / 0x00560E50 / 0x00560E70
 *   - AI_GetMonsterRarityRate 0x00560600
 * The remaining 34 functions of the range are VC8 std::map<DWORD, CNest*> / std::vector<CNest*>
 * instantiations and their EH funclets. TID_IsMonsterTypeID4_4 (0x00561020) is in GObj.h;
 * CMonster_SpawnInstance (0x005F6EB0) moved to GameWorldMgr.cpp.
 *
 * VC8 checked iterators (_SECURE_SCL) throw on every out-of-range vector subscript and map iterator
 * below; the port's operator[] does not.
 *
 * CORRECTION (2026-09-17, Claude): rewritten from machine bytes. The previous reconstruction mixed
 * up the _RefHive/_RefNest/_RefTactics records, the per-layer state layout (+0x2C/+0x34/+0x38) and
 * the vftable slots, and invented a spawn-delay clamp, an entity list and a champion constant.
 * ============================================================================
 */

#include "AIHive.h"
#include "GameAI.h"
#include "Game.h"
#include "GObjMob.h"
#include "GameWorldMgr.h"
#include "GameWorld.h"
#include "GameWorldLayer.h"
#include "Map.h"
#include "WorldMap.h"
#include "Region.h"
#include "SpecialTargetManager.h"
#include "../ServerCommon/ReferenceData.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_Library/NavMesh_new/RegionManagerBody.h"
#include "../JMX_Library/NavMesh_new/RTNavCell.h"
#include <cstdlib>

namespace AI {

/*
================================================================================
AI::CAIHive
================================================================================
*/

/**
 * [RECONSTRUCTED - 0x0055E440] (114 bytes)
 * CAIHive::CAIHive
 * m_pfnHatch is left uninitialized; Initialize is its only writer.
 */
CAIHive::CAIHive() {
	m_dwMaxTotalCount = 0;
	m_btHatchType = 0;
	m_dwIndex = 0;
	m_pRefHive = nullptr;
}

/**
 * [RECONSTRUCTED - 0x0055E530] (126 bytes)
 * CAIHive::~CAIHive
 */
CAIHive::~CAIHive() {
	Release();
}

/**
 * [RECONSTRUCTED - 0x0055E5B0] (444 bytes)
 * CAIHive::Initialize
 * A nest whose hive ID does not match is only asserted; CreateNest still runs for it.
 */
bool CAIHive::Initialize(tagRefHive* pRefHive, uint16_t wGameWorldID, uint32_t dwIndex) {
	if (pRefHive == nullptr) {
		ASSERT(false);
		Release();
		return false;
	}

	if (!SetRefHive(pRefHive)) {
		return false;
	}

	m_dwIndex = dwIndex;
	m_dwMaxTotalCount = 0;

	uint32_t dwNestIndex = 0;
	for (std::vector<uint32_t>::iterator it = pRefHive->m_vecNestID.begin(); it != pRefHive->m_vecNestID.end(); ++it) {
		ASSERT(g_pRefData);
		tagRefNest* pRefNest = g_pRefData->FindRefNest(*it);
		ASSERT(pRefNest != nullptr && pRefNest->m_dwHiveID == m_pRefHive->m_dwHiveID);

		if (CreateNest(pRefNest, wGameWorldID, dwNestIndex) == 0) {
			m_dwMaxTotalCount += pRefNest->m_dwMaxTotalCount;
			++dwNestIndex;
		}
	}

	if (m_pRefHive->m_btKeepMonsterCountType == 1) {
		m_dwMaxTotalCount = static_cast<uint32_t>(m_dwMaxTotalCount / m_pRefHive->m_fMonsterCountPerPC);
	}

	if (m_mapNest.size() == 0) {
		return false;
	}

	m_vecNest.reserve(m_mapNest.size());
	for (std::map<uint32_t, CNest*>::iterator it = m_mapNest.begin(); it != m_mapNest.end(); ++it) {
		m_vecNest.push_back(it->second);
	}

	m_pfnHatch[0] = &CAIHive::HatchPerNest;
	m_pfnHatch[1] = &CAIHive::HatchOverwriteTotal;

	if (m_pRefHive->m_dwOverwriteMaxTotalCount > 0) {
		m_btHatchType = 1;
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x0055E770] (78 bytes)
 * CAIHive::SetRefHive
 */
bool CAIHive::SetRefHive(tagRefHive* pRefHive) {
	m_pRefHive = pRefHive;

	if (pRefHive->m_btKeepMonsterCountType == 1 && pRefHive->m_fMonsterCountPerPC == 0.0f) {
		return false;
	}

	ASSERT(m_pRefHive->m_btKeepMonsterCountType < 2);
	if (m_pRefHive->m_btKeepMonsterCountType >= 2) {
		return false;
	}

	ASSERT(m_pRefHive->m_dwSpawnSpeedIncreaseRate <= m_pRefHive->m_dwMaxIncreaseRate);
	if (m_pRefHive->m_dwSpawnSpeedIncreaseRate > m_pRefHive->m_dwMaxIncreaseRate) {
		m_pRefHive->m_dwSpawnSpeedIncreaseRate = m_pRefHive->m_dwMaxIncreaseRate;
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x0055E7C0] (324 bytes)
 * CAIHive::CreateNest
 * The nest's region must exist in the game world's map and must not be a CRgnProxyTerrain
 * (CRegion+0x0C == 0). The world map itself is used without a NULL check.
 * CORRECTION (Claude): 0x0055E834..0x0055E864 go through g_pMap->FindWorldMap(wGameWorldID); the previous
 * version looked the region ID up as a game world and tested sub-zone containers that do not exist.
 */
int32_t CAIHive::CreateNest(tagRefNest* pRefNest, uint16_t wGameWorldID, uint32_t dwIndex) {
	if (pRefNest == nullptr) {
		ASSERT(false);
		return 1;
	}

	CNest* pNest = new CNest;

	ASSERT(g_pMap);
	CRegion* pRegion = g_pMap->FindWorldMap(wGameWorldID)->FindRegion(pRefNest->m_wRegionDBID);
	if (pRegion == nullptr || pRegion->m_dwUnk0C == 0) {
		delete pNest;
		return 2;
	}

	if (!pNest->Initialize(this, pRefNest, dwIndex)) {
		delete pNest;
		return 3;
	}

	ASSERT(pNest->m_pRefNest);
	m_mapNest.insert(std::map<uint32_t, CNest*>::value_type(pNest->m_pRefNest->m_dwNestID, pNest));
	return 0;
}

/**
 * [RECONSTRUCTED - 0x0055E910] (154 bytes)
 * CAIHive::Update
 */
int32_t CAIHive::Update(tagHiveSpawnState* pState, uint16_t wGameWorldID, uint16_t wLayerID) {
	if (g_dwGameAICurrentTick - pState->dwLastHatchTime >= 1000) {
		ASSERT(m_btHatchType < 2);
		(this->*m_pfnHatch[m_btHatchType])(wGameWorldID, wLayerID, pState);
		return 0;
	}

	if (m_mapNest.size() != 0) {
		for (std::vector<CNest*>::iterator it = m_vecNest.begin(); it != m_vecNest.end(); ++it) {
			ASSERT(*it);
			(*it)->Update();
		}
	}

	return 0;
}

/**
 * [RECONSTRUCTED - 0x0055E9B0] (99 bytes)
 * CAIHive::GetMaxTotalCount
 * Sums the unscaled per-nest limits; m_dwMaxTotalCount may also be divided by fMonsterCountPerPC.
 */
uint32_t CAIHive::GetMaxTotalCount() {
	uint32_t dwMaxTotalCount = 0;
	for (std::map<uint32_t, CNest*>::iterator it = m_mapNest.begin(); it != m_mapNest.end(); ++it) {
		dwMaxTotalCount += it->second->m_pRefNest->m_dwMaxTotalCount;
	}
	return dwMaxTotalCount;
}

/**
 * [RECONSTRUCTED - 0x0055EA20] (100 bytes)
 * CAIHive::HatchNPCs
 */
bool CAIHive::HatchNPCs(uint16_t wGameWorldID) {
	for (std::vector<CNest*>::iterator it = m_vecNest.begin(); it != m_vecNest.end(); ++it) {
		if ((*it)->HatchNPC(wGameWorldID) == 0) {
			return false;
		}
	}
	return true;
}

/**
 * [RECONSTRUCTED - 0x0055EA90] (381 bytes)
 * CAIHive::HatchPerNest (m_pfnHatch[0])
 */
int32_t CAIHive::HatchPerNest(uint16_t wGameWorldID, uint16_t wLayerID, tagHiveSpawnState* pState) {
	pState->dwLastHatchTime = g_dwGameAICurrentTick;

	for (std::vector<CNest*>::iterator it = m_vecNest.begin(); it != m_vecNest.end(); ++it) {
		CNest* pNest = *it;
		tagNestSpawnState* pNestState = &pState->vecNest[pNest->m_dwIndex];

		if (!pNestState->bAutoHatch) {
			continue;
		}
		if (!pNestState->dwRespawn && pNestState->nRemainCount <= 0) {
			continue;
		}
		if (g_dwGameAICurrentTick < pNestState->dwLastHatchTime + pNestState->dwHatchDelay) {
			continue;
		}
		if (pNestState->dwCurCount >= pNest->m_pRefNest->m_dwMaxTotalCount) {
			continue;
		}

		uint32_t dwObjID = pNest->Hatch(wGameWorldID, wLayerID, pNestState);
		if (dwObjID == 0) {
			continue;
		}

		ASSERT(pState->dwCurCount < 0xFFFFFFFF);
		if (pState->dwCurCount < 0xFFFFFFFF) {
			++pState->dwCurCount;
		}

		if (pNestState->dwRespawn) {
			continue;
		}

		ASSERT(g_pGame);
		CGObjChar* pObj = g_pGame->FindObjectByID(dwObjID);
		if (pObj != nullptr && pObj->IsMonster() == true) {
			uint32_t dwRefObjID = pObj->GetRefObjID();
			if (!GetGlobalSpecialTargetManager()->ContainsTargetID(dwRefObjID)) {
				--pNestState->nRemainCount;
			}
		}
	}

	return 0;
}

/**
 * [RECONSTRUCTED - 0x0055EC10] (403 bytes)
 * CAIHive::HatchOverwriteTotal (m_pfnHatch[1])
 * Ignores bAutoHatch, dwRespawn and nRemainCount.
 */
int32_t CAIHive::HatchOverwriteTotal(uint16_t wGameWorldID, uint16_t wLayerID, tagHiveSpawnState* pState) {
	if (m_pRefHive->m_dwOverwriteMaxTotalCount <= pState->dwCurCount) {
		return 0;
	}

	pState->dwLastHatchTime = g_dwGameAICurrentTick;

	CNest* pSelectedNest = pState->pSelectedNest;
	if (pSelectedNest != nullptr) {
		tagNestSpawnState* pNestState = &pState->vecNest[pSelectedNest->m_dwIndex];
		if (g_dwGameAICurrentTick < pNestState->dwLastHatchTime + pNestState->dwHatchDelay) {
			return 0;
		}
		if (pNestState->dwCurCount >= pSelectedNest->m_pRefNest->m_dwMaxTotalCount) {
			return 0;
		}
		if (pSelectedNest->Hatch(wGameWorldID, wLayerID, pNestState) == 0) {
			return 0;
		}

		ASSERT(pState->dwCurCount < 0xFFFFFFFF);
		if (pState->dwCurCount < 0xFFFFFFFF) {
			++pState->dwCurCount;
		}
		return 0;
	}

	for (std::vector<CNest*>::iterator it = m_vecNest.begin(); it != m_vecNest.end(); ++it) {
		CNest* pNest = *it;
		tagNestSpawnState* pNestState = &pState->vecNest[pNest->m_dwIndex];

		if (g_dwGameAICurrentTick < pNestState->dwLastHatchTime + pNestState->dwHatchDelay) {
			continue;
		}
		if (pNestState->dwCurCount >= pNest->m_pRefNest->m_dwMaxTotalCount) {
			continue;
		}

		if (pNest->Hatch(wGameWorldID, wLayerID, pNestState) != 0) {
			ASSERT(pState->dwCurCount < 0xFFFFFFFF);
			if (pState->dwCurCount < 0xFFFFFFFF) {
				++pState->dwCurCount;
			}
		}

		if (m_pRefHive->m_dwOverwriteMaxTotalCount <= pState->dwCurCount) {
			return 0;
		}
	}

	return 0;
}

/**
 * [RECONSTRUCTED - 0x0055EDB0] (14 bytes)
 * CAIHive::Release
 */
void CAIHive::Release() {
	DeleteNests();
	m_pRefHive = nullptr;
}

/**
 * [RECONSTRUCTED - 0x0055EDC0] (250 bytes)
 * CAIHive::DeleteNests
 */
void CAIHive::DeleteNests() {
	for (std::vector<CNest*>::iterator it = m_vecNest.begin(); it != m_vecNest.end(); ++it) {
		CNest* pNest = *it;
		pNest->Release();
		delete pNest;
	}

	m_mapNest.clear();
	m_vecNest.clear();
}

/**
 * [RECONSTRUCTED - 0x0055EEC0] (279 bytes)
 * CAIHive::OnMonsterDead
 * At the overwrite limit every nest restarts its timer. With an overwrite limit of 1 the random
 * pick (upper bound folded onto the last nest) becomes the next pSelectedNest.
 */
bool CAIHive::OnMonsterDead(tagHiveSpawnState* pState) {
	if (m_btHatchType == 1 && pState->dwCurCount == m_pRefHive->m_dwOverwriteMaxTotalCount) {
		uint32_t dwNestCount = static_cast<uint32_t>(m_mapNest.size());
		uint32_t dwRand = static_cast<uint32_t>(abs(rand())) % (dwNestCount + 1);
		if (dwRand >= dwNestCount - 1) {
			dwRand = dwNestCount - 1;
		}

		uint32_t i = 0;
		for (std::vector<CNest*>::iterator it = m_vecNest.begin(); it != m_vecNest.end(); ++it, ++i) {
			CNest* pNest = *it;
			tagNestSpawnState* pNestState = &pState->vecNest[pNest->m_dwIndex];

			if (dwRand == i && m_pRefHive->m_dwOverwriteMaxTotalCount == 1) {
				pState->pSelectedNest = pNest;
				pNest->RollHatchDelay(pNestState);
			}

			tagNestSpawnState_RestartHatchTimer(pNestState, pNest);
		}
	}

	ASSERT(pState->dwCurCount > 0);
	if (pState->dwCurCount > 0) {
		--pState->dwCurCount;
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x0055EFE0] (152 bytes)
 * CAIHive::SelectRandomNest
 * ORIGINAL DEFECT, kept: `i == nRand` is only compared before the first increment (LLIL #47; the
 * loop #49..#58 never re-tests it), so any hive with more than one nest gets NULL.
 */
CNest* CAIHive::SelectRandomNest() {
	uint32_t dwNestCount = static_cast<uint32_t>(m_mapNest.size());
	if (m_pRefHive == nullptr || m_pRefHive->m_dwOverwriteMaxTotalCount != 1 || dwNestCount < 1) {
		return nullptr;
	}

	if (dwNestCount == 1) {
		return *m_vecNest.begin();
	}

	int32_t nRand = static_cast<int32_t>(static_cast<uint32_t>(abs(rand())) % dwNestCount);
	int32_t i = 0;
	for (std::vector<CNest*>::iterator it = m_vecNest.begin(); i < nRand; ++it, ++i) {
		if (i == nRand) {
			return *it;
		}
	}

	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x0055F080] (82 bytes)
 * CAIHive::FindNest
 */
CNest* CAIHive::FindNest(uint32_t dwNestID) {
	std::map<uint32_t, CNest*>::iterator it = m_mapNest.find(dwNestID);
	if (it == m_mapNest.end()) {
		return nullptr;
	}
	return it->second;
}

/**
 * [RECONSTRUCTED - 0x0055F0E0] (173 bytes)
 * CAIHive::UpdateIncreaseRate
 */
bool CAIHive::UpdateIncreaseRate(tagRegionContext* pContext, tagHiveSpawnState* pState) {
	if (m_pRefHive->m_btKeepMonsterCountType == 1) {
		uint32_t dwPCCount = 0;
		for (std::vector<CNest*>::iterator it = m_vecNest.begin(); it != m_vecNest.end(); ++it) {
			CNest* pNest = *it;
			dwPCCount += pNest->GetPCCount(pContext, &pState->vecNest[pNest->m_dwIndex]);
		}
		CalcIncreaseRate(dwPCCount, pState);
	}
	return true;
}

/**
 * [RECONSTRUCTED - 0x0055F190] (490 bytes)
 * CAIHive::CalcIncreaseRate
 * Two samples are averaged per history slot. The load ratio is truncated to an integer (fistp)
 * before it is multiplied, so it contributes only while the average reaches m_dwMaxTotalCount.
 */
void CAIHive::CalcIncreaseRate(uint32_t dwPCCount, tagHiveSpawnState* pState) {
	++pState->dwSampleCount;
	pState->dwSampleSum += dwPCCount;

	if (pState->dwSampleCount >= 2) {
		pState->dwHistory[pState->nHistoryIndex] = pState->dwSampleSum / 2;
		pState->nHistoryIndex = (pState->nHistoryIndex + 1) % 2;
		pState->dwAverage = (pState->dwHistory[0] + pState->dwHistory[1]) / 2;
		pState->dwSampleSum = 0;
		pState->dwSampleCount = 0;
	}

	pState->dwLastAverage = pState->dwAverage;

	uint32_t dwLoadRatio = static_cast<uint32_t>(static_cast<int64_t>(static_cast<double>(pState->dwAverage) / static_cast<double>(m_dwMaxTotalCount)));
	float fIncreaseRate = static_cast<float>(dwLoadRatio * m_pRefHive->m_dwSpawnSpeedIncreaseRate);
	CLAMP(fIncreaseRate, 0.0f, static_cast<float>(m_pRefHive->m_dwMaxIncreaseRate));

	if (fIncreaseRate != pState->fIncreaseRate) {
		for (std::vector<CNest*>::iterator it = m_vecNest.begin(); it != m_vecNest.end(); ++it) {
			CNest* pNest = *it;
			tagNestSpawnState_SetIncreaseRate(&pState->vecNest[pNest->m_dwIndex], pNest, fIncreaseRate);
		}
		pState->fIncreaseRate = fIncreaseRate;
	}
}

/*
================================================================================
AI::CNest
================================================================================
*/

/**
 * [RECONSTRUCTED - 0x00560450] (39 bytes)
 * CNest::CNest
 * m_pRefObj, m_dwIndex and m_bPartyHatch are left uninitialized, as in the native constructor.
 */
CNest::CNest()
	: m_pHive(nullptr)
	, m_pRefNest(nullptr)
	, m_pRefTactics(nullptr) {
}

/**
 * [RECONSTRUCTED - 0x005604A0] (7 bytes)
 * CNest::~CNest
 */
CNest::~CNest() {
}

/**
 * [RECONSTRUCTED - 0x005604B0] (189 bytes)
 * CNest::Initialize (vftable[1])
 * Binds reference data, copies initial nest position into m_Pos, validates the point
 * with g_pRegionManager->CheckPointValid, and verifies cell NPC suitability via
 * pNavCell->IsTerrainCell() (slot 2 @ 0x0056052D).
 * Preserves the native compiler/logic defect: if IsTerrainCell is false, it tests
 * `dwTacticsID < 1000 && dwTacticsID >= 2000` (81 F9 E8 03 00 00 / 73 1E / 81 F9 D0 07 00 00 / 72 16),
 * which is never true, so success follows.
 */
bool CNest::Initialize(CAIHive* pHive, tagRefNest* pRefNest, uint32_t dwIndex) {
	m_pHive = pHive;
	m_pRefNest = pRefNest;
	m_dwIndex = dwIndex;

	if (!BindRefData()) {
		return false;
	}

	m_Pos.wRegionID = m_pRefNest->m_wRegionDBID;
	m_Pos.fPosX = m_pRefNest->m_fLocalPosX;
	m_Pos.fPosY = m_pRefNest->m_fLocalPosY;
	m_Pos.fPosZ = m_pRefNest->m_fLocalPosZ;

	if (!NavMesh::g_pRegionManager->CheckPointValid(&m_Pos, 0)) {
		// TRACE(m_pRefNest->m_dwHiveID, m_pRefNest->m_dwNestID): compiled to the empty call 0x0066B100
		// CORRECTION (Claude): 0x00560510..0x00560517 pushes NestID then HiveID, so cdecl arg1 is HiveID here.
		return false;
	}

	NavMesh::CRTNavCell* pCell = m_Pos.pNavCell;
	// CORRECTION (Claude): slot 2 is CRTNavCell::IsTerrainCell (false for object triangles), not CanSpawnNPC.
	if (pCell != nullptr && !pCell->IsTerrainCell()) {
		uint32_t dwTacticsID = m_pRefNest->m_dwTacticsID;
		if (dwTacticsID < 1000 && dwTacticsID >= 2000) {
			// TRACE(m_pRefNest->m_dwNestID, m_pRefNest->m_dwHiveID): compiled to 0x0066B100
			return false;
		}
	}

	m_bPartyHatch = 0;
	return true;
}

/**
 * [RECONSTRUCTED - Shared 0x00455EB0] (3 bytes: B0 01 C3)
 * CNest::Update (vftable[2])
 */
bool CNest::Update() {
	return true;
}

/**
 * [RECONSTRUCTED - Shared 0x00455EB0] (3 bytes: B0 01 C3)
 * CNest::Release (vftable[3])
 */
bool CNest::Release() {
	return true;
}

/**
 * [RECONSTRUCTED - 0x00560570] (131 bytes)
 * CNest::BindRefData
 */
bool CNest::BindRefData() {
	uint32_t dwTacticsID = m_pRefNest->m_dwTacticsID;

	ASSERT(g_pRefData);
	tagRefTactics* pRefTactics = g_pRefData->FindRefTactics(dwTacticsID);
	if (pRefTactics == nullptr || pRefTactics->m_dwObjID == 0) {
		BSLib::Log_Printf(0x1000000, "Invalid Object Hatching Nest checked!! NESTID[%d] Hatching TacticsID[%d]",
			m_pRefNest->m_dwNestID, dwTacticsID);
		return false;
	}

	m_pRefTactics = pRefTactics;

	ASSERT(g_pRefData);
	m_pRefObj = g_pRefData->FindRefObjCommon(m_pRefTactics->m_dwObjID);
	if (m_pRefObj == nullptr) {
		ASSERT(false);
		return false;
	}

	return true;
}

/**
 * [RECONSTRUCTED - 0x005606C0] (235 bytes)
 * CNest::HatchNPC
 * Hatches NPC objects (TypeID3 2) on layer 0 at world load; every other object type answers 0xFF.
 * The layer pointer is asserted but used regardless.
 */
uint8_t CNest::HatchNPC(uint16_t wGameWorldID) {
	tagTID tid(m_pRefObj->m_wTypeID);
	if (!tid.IsNPC()) {
		return 0xFF;
	}

	ASSERT(g_pGameWorldMgr);
	CGameWorld* pGameWorld = g_pGameWorldMgr->FindGameWorld(wGameWorldID);
	if (pGameWorld == nullptr) {
		ASSERT(false);
		return 1;
	}

	CGameWorldLayer* pLayer = pGameWorld->GetLayer(0);
	ASSERT(pLayer);

	tagHiveSpawnState* pHiveState = &pLayer->m_vecHiveState[m_pHive->m_dwIndex];
	return Hatch(wGameWorldID, 0, &pHiveState->vecNest[m_dwIndex]) != 0;
}

/**
 * [RECONSTRUCTED - 0x005607B0] (1355 bytes)
 * CNest::Hatch
 * Rarity: an authored rarity of 1,3,4,5,6,7,8 is kept. Otherwise the party (0x10) roll runs when
 * m_bPartyHatch is set, and a champion-tactics nest rolls champion (1) or giant (4). An enabled
 * special-target entry may then replace the object and its rarity nibble.
 */
uint32_t CNest::Hatch(uint16_t wGameWorldID, uint16_t wLayerID, tagNestSpawnState* pState) {
	if (m_pRefObj == nullptr) {
		ASSERT(false);
		return 0;
	}

	ASSERT(g_pGameWorldMgr);
	if (g_pGameWorldMgr->IsDoNotSpawnMonsterOverMaxServiceLevel(wGameWorldID)) {
		if (m_pRefObj->m_byLevel > 110) {
			return 0;
		}
	}

	ASSERT(m_Pos.wRegionID != 0);

	uint16_t wTypeID = m_pRefObj->m_wTypeID;
	tagTID tid(wTypeID);
	tagRefTactics* pRefTactics = m_pRefTactics;
	uint8_t btRarity = 0;

	if (tid.IsMonster()) {
		uint8_t btRefRarity = m_pRefObj->m_byRank;
		if (btRefRarity == 5) {
			btRarity = 5;
		} else if (btRefRarity == 3) {
			btRarity = 3;
		} else if (btRefRarity == 8) {
			btRarity = 8;
		} else if (btRefRarity == 6) {
			btRarity = 6;
		} else if (btRefRarity == 7) {
			btRarity = 7;
		} else if (btRefRarity == 4) {
			btRarity = 4;
		} else if (btRefRarity == 1) {
			btRarity = 1;
		} else {
			if (m_bPartyHatch == 1 && rand() % 101 < 50 && !TID_IsMonsterTypeID4_4(&wTypeID)) {
				btRarity = 0x10;
			}

			if (m_pRefTactics->m_dwChampionTacticsID != 0 && rand() % 101 < m_pRefNest->m_nChampionGenPercentage) {
				if (rand() % 101 <= g_wGiantMonsterSpawnRatio) {
					btRarity = (btRarity & 0xF4) | 4;
				} else {
					btRarity = (btRarity & 0xF1) | 1;
				}

				pRefTactics = GetRefData()->FindRefTactics(m_pRefTactics->m_dwChampionTacticsID);
				if (pRefTactics == nullptr) {
					ASSERT(false);
					btRarity &= 0xF0;
					pRefTactics = m_pRefTactics;
				}
			}
		}
	}

	float fAngle;
	if (tid.IsMonster() && m_pRefObj->m_byRank != 3) {
		float fRand = static_cast<float>(rand() / 32767.0);
		fAngle = fRand * 6.2831855f;
	} else {
		fAngle = static_cast<float>(m_pRefNest->m_wInitialDir / 65535.0 * 360.0 * 0.017453292f);
	}

	tagObjLocation pos = m_Pos;
	const tagRefObjCommon* pRefObj = m_pRefObj;
	uint32_t dwObjID = 0;
	int32_t bHalveHatchDelay = 0;

	ASSERT(g_pSpecialTargetManager);
	CSpecialTargetManager* pSpecialTargetMgr = g_pSpecialTargetManager;
	float fRand = static_cast<float>(rand() / 32767.0);
	float fReplaceRoll = static_cast<float>(pSpecialTargetMgr->CalculateTotalSpawnRatio() * fRand);
	float fReplaceSum = 0.0f;

	ASSERT(g_pGameWorldMgr);
	if (g_pGameWorldMgr->IsRefGameWorldByte20Zero(wGameWorldID) == 1) {
		for (std::vector<CSpecialTargetManager::TargetEntry*>::iterator it = pSpecialTargetMgr->m_vecTargets.begin();
			it != pSpecialTargetMgr->m_vecTargets.end(); ++it) {
			CSpecialTargetManager::TargetEntry* pEntry = *it;
			if (pEntry->m_dwType != 1 || pEntry->m_dwMode != 1 || static_cast<uint32_t>(btRarity & 0x0F) != pEntry->m_dwRarity) {
				continue;
			}

			fReplaceSum += pEntry->m_fRatio;
			if (fReplaceSum >= fReplaceRoll) {
				btRarity = (btRarity & 0xF0) | (pEntry->m_btReplaceRarity & 0x0F);

				const tagRefObjCommon* pRefSpawnChar = GetRefData()->FindRefObjCommon(pEntry->m_dwTargetID);
				pRefObj = pRefSpawnChar;
				if (pRefSpawnChar == nullptr) {
					BSLib::Log_Printf(0x2000001, "pRefSpawnChar == REFDATA_MGR.GetRefObj(%d)", pEntry->m_dwTargetID);
					ASSERT(false);
					pRefObj = m_pRefObj;
				}
				pRefTactics = nullptr;
				break;
			}
		}
	}

	ASSERT(g_pGameWorldMgr);
	ASSERT(wGameWorldID < 0xFFFF);
	tagRegionContext context;
	context.wGameWorldID = wGameWorldID;
	context.wLayerID = wLayerID;

	ASSERT(g_pGameWorldMgr);
	int32_t bNoControlNotifySpawnUniqueMsg = (g_pGameWorldMgr->IsControlNotifySpawnUniqueMonsterMsg(wGameWorldID) == 0);

	CGObjMob* pMonster = CMonster_SpawnInstance(&pos, g_pGameWorldMgr, context, 0, pRefObj, m_pRefNest, pRefTactics,
		fAngle, static_cast<float>(m_pRefNest->m_nGenerateRadius), btRarity, 0, &bHalveHatchDelay, 0.0f, bNoControlNotifySpawnUniqueMsg);

	if (pMonster != nullptr) {
		ASSERT(pState);
		tagNestSpawnState_IncCount(pState);
		pState->dwLastHatchTime = g_dwGameAICurrentTick;
		RollHatchDelay(pState);

		dwObjID = pMonster->GetGameID(); // native reads CGObj+0x08
		if ((btRarity & 0xF0) == 0x10) {
			m_bPartyHatch = 0;
		}
		return dwObjID;
	}

	if (bHalveHatchDelay == 1 && pState != nullptr) {
		tagNestSpawnState_HalveHatchDelay(pState);
		return 0;
	}

	return dwObjID;
}

/**
 * [RECONSTRUCTED - 0x00560D00] (177 bytes, retn 8)
 * CNest::OnMonsterDead (vftable[4])
 * m_pHive is dereferenced before its assertion, as in the native.
 */
bool CNest::OnMonsterDead(CGObjChar* pObj, uint32_t /*dwTacticsPoolIndex*/) {
	if (pObj->IsNPC() == true) {
		return true;
	}

	CGameWorldLayer* pLayer = pObj->m_pGameWorldLayer;
	if (pLayer == nullptr) {
		ASSERT(false);
		return false;
	}

	tagHiveSpawnState* pHiveState = &pLayer->m_vecHiveState[m_pHive->m_dwIndex];
	tagNestSpawnState* pNestState = &pHiveState->vecNest[m_dwIndex];
	ASSERT(m_pHive);

	if (pNestState->dwCurCount == m_pRefNest->m_dwMaxTotalCount) {
		pNestState->dwLastHatchTime = g_dwGameAICurrentTick;
	}

	if (m_pHive != nullptr) {
		m_pHive->OnMonsterDead(pHiveState);
	}

	tagNestSpawnState_DecCount(pNestState);
	return true;
}

/**
 * [RECONSTRUCTED - 0x00560E40] (14 bytes)
 * CNest::RollHatchDelay
 */
uint32_t CNest::RollHatchDelay(tagNestSpawnState* pState) {
	return tagNestSpawnState_SetHatchDelay(m_pRefNest->m_dwDelayTimeMin, m_pRefNest->m_dwDelayTimeMax, pState);
}

/**
 * [RECONSTRUCTED - 0x00560E90] (175 bytes)
 * CNest::GetPCCount
 * Players on the context's layer in the CMsgBlock that holds the nest's reference position.
 * pState is only NULL-tested.
 * CORRECTION (Claude): 0x00560EA2..0x00560F32 call CMap::FindRegion and CRegion::GetPCCount; the previous
 * version called the invented CWorldManager::IsOutdoorRegion / CountOutdoorEntitiesInTactics and added a
 * m_pRefNest NULL test the native does not have.
 */
uint32_t CNest::GetPCCount(tagRegionContext* pContext, tagNestSpawnState* pState) {
	if (pState == nullptr) {
		return 0;
	}

	ASSERT(g_pMap);
	CRegion* pRegion = g_pMap->FindRegion(pContext, m_pRefNest->m_wRegionDBID);
	if (pRegion == nullptr) {
		static bool s_bNestRegionInvalidLogged = false; // 0x00D6A97C
		if (!s_bNestRegionInvalidLogged) {
			ASSERT(false);
			BSLib::Log_Printf(0x2000001, "Nest Region is invalid!!! NestID[%d] NestRegionID[%d]",
				m_pRefNest->m_dwNestID, m_pRefNest->m_wRegionDBID);
			s_bNestRegionInvalidLogged = true;
		}
		return 0;
	}

	tagObjLocation pos;
	pos.pNavCell = nullptr;
	pos.pNavMeshInst = nullptr;
	pos.wRegionID = m_pRefNest->m_wRegionDBID;
	pos.fPosX = m_pRefNest->m_fLocalPosX;
	pos.fPosY = m_pRefNest->m_fLocalPosY;
	pos.fPosZ = m_pRefNest->m_fLocalPosZ;
	return pRegion->GetPCCount(*pContext, pos);
}

} // namespace AI

/*
================================================================================
Rarity rate and nest spawn-state helpers
================================================================================
*/

/**
 * [RECONSTRUCTED - 0x00560600] (142 bytes)
 * AI_GetMonsterRarityRate
 * High nibble: 0 -> x1, 1 (party) -> x10, else ASSERT. Low nibble via jump table 0x00560690.
 */
float AI_GetMonsterRarityRate(const uint8_t* pbtRarity) {
	float fRate = 1.0f;

	switch (*pbtRarity >> 4) {
	case 0:
		break;
	case 1:
		fRate = 10.0f;
		break;
	default:
		ASSERT(false);
		break;
	}

	switch (*pbtRarity & 0x0F) {
	case 1:
		fRate *= 2.0;
		break;
	case 4:
		fRate *= 20.0;
		break;
	case 5:
		fRate *= 100.0;
		break;
	case 6:
		fRate *= 4.0;
		break;
	case 7:
		fRate *= 30.0;
		break;
	default:
		break;
	}

	return fRate;
}

/**
 * [RECONSTRUCTED - 0x00560380] (193 bytes)
 * tagNestSpawnState_SetHatchDelay
 * Rolls [min, max) seconds, then takes fIncreaseRate percent off. Both conversions truncate (fistp qword).
 */
uint32_t tagNestSpawnState_SetHatchDelay(uint32_t dwDelayTimeMinSec, uint32_t dwDelayTimeMaxSec, tagNestSpawnState* pState) {
	uint32_t dwDelayMin = dwDelayTimeMinSec * 1000;
	uint32_t dwDelayMax = dwDelayTimeMaxSec * 1000;
	uint32_t dwDelay;

	if (dwDelayMax > dwDelayMin) {
		float fRand = static_cast<float>(rand() / 32767.0);
		dwDelay = static_cast<uint32_t>(static_cast<int64_t>(fRand * static_cast<double>(dwDelayMax - dwDelayMin))) + dwDelayMin;
	} else {
		dwDelay = dwDelayMin;
	}

	pState->dwDelayReduction = static_cast<uint32_t>(static_cast<int64_t>(pState->fIncreaseRate * static_cast<double>(dwDelay) / 100.0));
	pState->dwHatchDelay = dwDelay - pState->dwDelayReduction;
	return pState->dwHatchDelay;
}

/**
 * [RECONSTRUCTED - 0x00560DC0] (39 bytes)
 * tagNestSpawnState_IncCount
 */
void tagNestSpawnState_IncCount(tagNestSpawnState* pState) {
	if (pState == nullptr) {
		ASSERT(false);
		return;
	}

	ASSERT(pState->dwCurCount < 0xFFFFFFFF);
	if (pState->dwCurCount < 0xFFFFFFFF) {
		++pState->dwCurCount;
	}
}

/**
 * [RECONSTRUCTED - 0x00560DF0] (25 bytes)
 * tagNestSpawnState_DecCount
 */
void tagNestSpawnState_DecCount(tagNestSpawnState* pState) {
	ASSERT(pState->dwCurCount > 0);
	if (pState->dwCurCount > 0) {
		--pState->dwCurCount;
	}
}

/**
 * [RECONSTRUCTED - 0x00560E10] (37 bytes)
 * tagNestSpawnState_HalveHatchDelay
 */
int32_t tagNestSpawnState_HalveHatchDelay(tagNestSpawnState* pState) {
	if (pState != nullptr) {
		pState->dwHatchDelay >>= 1;
		pState->dwLastHatchTime = g_dwGameAICurrentTick;
		pState->dwHatchDelay = (pState->dwHatchDelay > 1000) ? pState->dwHatchDelay : 1000;
	}
	return 0;
}

/**
 * [RECONSTRUCTED - 0x00560E50] (23 bytes)
 * tagNestSpawnState_RestartHatchTimer
 */
uint32_t tagNestSpawnState_RestartHatchTimer(tagNestSpawnState* pState, AI::CNest* pNest) {
	pState->dwLastHatchTime = g_dwGameAICurrentTick;
	return pNest->RollHatchDelay(pState);
}

/**
 * [RECONSTRUCTED - 0x00560E70] (19 bytes)
 * tagNestSpawnState_SetIncreaseRate
 */
uint32_t tagNestSpawnState_SetIncreaseRate(tagNestSpawnState* pState, AI::CNest* pNest, float fRate) {
	pState->fIncreaseRate = fRate;
	return pNest->RollHatchDelay(pState);
}

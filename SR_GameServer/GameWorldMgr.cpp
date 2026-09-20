/**
 * ============================================================================
 * Silkroad Online - Game Server Game World Manager Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GameWorldMgr.cpp
 *
 * Implements:
 *   - CGameWorldMgr::CGameWorldMgr @ 0x005F5950
 *   - CGameWorldMgr::FindGameWorld @ 0x005F6B80
 *   - CGameWorldMgr::IsRefGameWorldByte20Zero @ 0x005F85A0
 *   - CGameWorldMgr::IsControlNotifySpawnUniqueMonsterMsg @ 0x005F9910
 *   - CGameWorldMgr::IsDoNotSpawnMonsterOverMaxServiceLevel @ 0x005F9930
 *   - CMonster_SpawnInstance @ 0x005F6EB0
 *   - Static instance 0x00D0B380, g_pGameWorldMgr @ 0x00D6A9A4
 *
 * CORRECTION (Claude): replaces WorldManager.cpp. The removed CWorldManager::GetInstance (0x004039D0) had
 * no callers, CWorldManager::ValidatePath and CWorldSubZone::CountOutdoorEntitiesInTactics were invented,
 * and CWorldManager::IsOutdoorRegion is CMap::FindRegion (Map.cpp).
 * ============================================================================
 */

#include "GameWorldMgr.h"
#include "GameWorld.h"
#include "GameWorldLayer.h"
#include "Game.h"
#include "GObjMob.h"
#include "../ServerCommon/ReferenceData.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_Library/NavMesh_new/RegionManagerBody.h"
#include <cmath>
#include <cstdlib>

CGameWorldMgr  g_GameWorldMgr;                    // 0x00D0B380
CGameWorldMgr* g_pGameWorldMgr = &g_GameWorldMgr; // 0x00D6A9A4

/**
 * [PARTIAL - 0x005F5950]
 * CGameWorldMgr::CGameWorldMgr
 * The containers at +0x10 (0x005F9D20), +0x38 (0x00602E30) and +0x48 (0x005F9BC0) are not modeled.
 */
CGameWorldMgr::CGameWorldMgr() {
}

/**
 * [STUB - 0x005F5AC0]
 * CGameWorldMgr::~CGameWorldMgr
 * The native body is not reconstructed; the worlds in m_mapGameWorld are not released.
 */
CGameWorldMgr::~CGameWorldMgr() {
}

/**
 * [RECONSTRUCTED - 0x005F6B80] (105 bytes)
 * CGameWorldMgr::FindGameWorld
 * IDs are 1-based and dense: anything outside [1, world count] or missing asserts.
 */
CGameWorld* CGameWorldMgr::FindGameWorld(uint16_t wGameWorldID) {
	if (wGameWorldID >= 1 && wGameWorldID <= m_mapGameWorld.size()) {
		std::map<uint32_t, CGameWorld*>::iterator it = m_mapGameWorld.find(wGameWorldID);
		if (it != m_mapGameWorld.end()) {
			return it->second;
		}
	}

	ASSERT(false);
	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x005F85A0] (116 bytes)
 * CGameWorldMgr::IsRefGameWorldByte20Zero
 * CNest::Hatch only rolls special-target replacements when this is TRUE.
 */
int32_t CGameWorldMgr::IsRefGameWorldByte20Zero(uint16_t wGameWorldID) {
	CGameWorld* pGameWorld = FindGameWorld(wGameWorldID);
	if (pGameWorld == nullptr) {
		return 0;
	}
	return pGameWorld->m_pRefGameWorld->m_btUnk20 == 0;
}

/**
 * [RECONSTRUCTED - 0x005F88F0] (35 bytes)
 * CGameWorldMgr::ReducesRangedWeaponRange
 */
int32_t CGameWorldMgr::ReducesRangedWeaponRange(uint16_t wGameWorldID) {
	CGameWorld* pGameWorld = FindGameWorld(wGameWorldID);
	if (pGameWorld == nullptr) {
		ASSERT(false);
		return 0;
	}
	return pGameWorld->ReducesRangedWeaponRange();
}

/**
 * [RECONSTRUCTED - 0x005F9910] (22 bytes)
 * CGameWorldMgr::IsControlNotifySpawnUniqueMonsterMsg
 */
int32_t CGameWorldMgr::IsControlNotifySpawnUniqueMonsterMsg(uint16_t wGameWorldID) {
	CGameWorld* pGameWorld = FindGameWorld(wGameWorldID);
	if (pGameWorld == nullptr) {
		return 0;
	}
	return pGameWorld->IsControlNotifySpawnUniqueMonsterMsg();
}

/**
 * [RECONSTRUCTED - 0x005F9930] (22 bytes)
 * CGameWorldMgr::IsDoNotSpawnMonsterOverMaxServiceLevel
 */
int32_t CGameWorldMgr::IsDoNotSpawnMonsterOverMaxServiceLevel(uint16_t wGameWorldID) {
	CGameWorld* pGameWorld = FindGameWorld(wGameWorldID);
	if (pGameWorld == nullptr) {
		return 0;
	}
	return pGameWorld->IsDoNotSpawnMonsterOverMaxServiceLevel();
}

/**
 * [RECONSTRUCTED - Native 0x00531240] (354 bytes)
 * Pos_ScatterAroundGenerateRadius
 * Scatters a spawn location radially within fGenerateRadius around the center point,
 * dividing the area into 3 distance bands and normalizing sector boundaries.
 */
static void Pos_ScatterAroundGenerateRadius(tagObjLocation* pPos, float fGenerateRadius) {
	if (!pPos || fGenerateRadius <= 1e-6f) {
		return;
	}

	if (fGenerateRadius < 1.0f) {
		fGenerateRadius = 1.0f;
	}

	float fRing = fGenerateRadius / 3.0f;
	float fDist = 0.0f;
	int32_t nRoll = std::rand() % 101;
	if (nRoll < 70) {
		fDist = fRing * 2.0f;
	} else if (nRoll >= 90) {
		fDist = 0.0f;
	} else {
		fDist = fRing;
	}

	float fFrac = static_cast<float>(std::rand()) / 32767.0f;
	fDist += fFrac * fRing;
	if (fDist > fGenerateRadius) {
		fDist = fGenerateRadius;
	}

	float fAngle = (static_cast<float>(std::rand()) / 32767.0f) * 6.283185307179586f;
	pPos->fPosX += std::cos(fAngle) * fDist;
	pPos->fPosZ += std::sin(fAngle) * fDist;

	SRO_Vector3D vec(pPos->fPosX, pPos->fPosY, pPos->fPosZ);
	Pos_NormalizeOutdoorRegion(&pPos->wRegionID, &vec);
	pPos->fPosX = vec.x;
	pPos->fPosY = vec.y;
	pPos->fPosZ = vec.z;
}

/**
 * [RECONSTRUCTED - 0x005F6EB0] (1141 bytes, retn 0x34)
 * CMonster_SpawnInstance
 * Spawns a monster instance at or around the given location.
 * Implements radial generation scatter, movement collision validation,
 * hatch delay halving reporting (*pbHalveHatchDelay), and object registration.
 */
CGObjMob* CMonster_SpawnInstance(tagObjLocation* pPos, CGameWorldMgr* pGameWorldMgr, tagRegionContext context,
	uint32_t /*dwReserved08*/, const tagRefObjCommon* pRefObj, tagRefNest* /*pRefNest*/, tagRefTactics* /*pRefTactics*/,
	float /*fAngle*/, float fGenerateRadius, uint8_t btRarity, uint32_t /*dwReserved24*/, int32_t* pbHalveHatchDelay,
	float /*fReserved2C*/, int32_t /*bNoControlNotifySpawnUniqueMsg*/) {
	if (pbHalveHatchDelay != nullptr) {
		*pbHalveHatchDelay = 0;
	}

	if (pPos == nullptr || pGameWorldMgr == nullptr || pRefObj == nullptr) {
		return nullptr;
	}

	tagObjLocation spawnPos = *pPos;
	if (fGenerateRadius > 1e-6f) {
		Pos_ScatterAroundGenerateRadius(&spawnPos, fGenerateRadius);
	}

	// Validate pathing and movement collision via RegionManager
	if (NavMesh::g_pRegionManager != nullptr) {
		int32_t nMoveResult = NavMesh::g_pRegionManager->QueryMovement(0, 1, pPos, &spawnPos, 0, nullptr);
		if ((nMoveResult & 0x10000000) != 0) {
			if (pbHalveHatchDelay != nullptr && (nMoveResult & 1) != 0) {
				*pbHalveHatchDelay = 1;
			}
			return nullptr;
		}

		if (!NavMesh::g_pRegionManager->CheckPointValid(&spawnPos, 0)) {
			return nullptr;
		}
	}

	CGObjMob* pMob = new CGObjMob();
	pMob->m_fPosX = spawnPos.fPosX;
	pMob->m_fPosY = spawnPos.fPosY;
	pMob->m_fPosZ = spawnPos.fPosZ;
	pMob->m_wRegionID = spawnPos.wRegionID;

	// CORRECTION (Claude): world lookup renamed from CWorldManager::FindRegion / CWorldRegion::GetSubZoneByIndex.
	CGameWorld* pGameWorld = pGameWorldMgr->FindGameWorld(context.wGameWorldID);
	pMob->m_pGameWorldLayer = (pGameWorld != nullptr) ? pGameWorld->GetLayer(context.wLayerID) : nullptr;
	pMob->SetMobRank(btRarity);

	if (g_pGame != nullptr) {
		g_pGame->RegisterObject(pMob);
	}
	return pMob;
}

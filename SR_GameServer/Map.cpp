/**
 * ============================================================================
 * Silkroad Online - Game Server Map Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\Map.cpp
 *
 * Implements:
 *   - CMap::CMap @ 0x00530240
 *   - CMap::~CMap @ 0x005303A0
 *   - CMap::Load @ 0x005304E0
 *   - CMap::FindWorldMap @ 0x00530C10
 *   - CMap::FindRegion @ 0x00530C70
 *   - g_pMap @ 0x00D6A968
 * ============================================================================
 */

#include "Map.h"
#include "GameWorld.h"
#include "GameWorldMgr.h"
#include "WorldMap.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_Library/NavMesh_new/MapLoader.h"
#include "../JMX_Library/NavMesh_new/RegionManagerBody.h"
#include <cstring>

CMap* g_pMap = nullptr; // 0x00D6A968

/**
 * [RECONSTRUCTED - 0x00530240] (146 bytes)
 * CMap::CMap
 * The map constructed at +0x10 (0x00531460) is kept as padding.
 */
CMap::CMap()
	: m_pUnk1C(nullptr)
	, m_pUnk20(nullptr) {
	ASSERT(g_pMap == nullptr);
	g_pMap = this;

	std::memset(pad10, 0, sizeof(pad10));
	std::memset(pad24, 0, sizeof(pad24));
}

/**
 * [STUB - 0x005303A0]
 * CMap::~CMap
 * The native body is not reconstructed; the world maps are not released.
 */
CMap::~CMap() {
}

/**
 * [PARTIAL - 0x005304E0] (1101 bytes)
 * CMap::Load
 * Native order: install the region manager event callback (slot 7, 0x005301B0), initialize the manager with the
 * map data path (slot 1, "<path>data\"), create one CWorldMap per game world (0x00530720 -> CWorldMap::LoadRegions
 * 0x0053B4B0 -> 0x0053BAC0, which calls AddRegion for every region of the _RefRegion table), link the regions
 * (slot 3), reset the event state (slot 26) and register every reference structure (slot 9).
 *
 * Ported: the region manager initialization, the region set and LinkAllRegions. The region set comes from the
 * map data itself (mapinfo, the same bits CMapLoader::IsRegionEnabled tests) because the _RefRegion table and the
 * CWorldMap / CRegion objects it keys are not ported; on the live data that is the 2123 outdoor regions. The
 * structure registration (slot 9, the GATE / OBJECT flags) needs the reference structure table and is left out,
 * so no siege structure is known to the navmesh.
 */
int32_t CMap::Load(const char* pszPath) {
	if (pszPath == nullptr || *pszPath == '\0') {
		ASSERT(pszPath != nullptr && *pszPath != '\0'); // 0x005304FB: lstrlenA(path) <= 0
		return 0;
	}

	// Slot 1 (0x0098A660): no file manager, so the loader roots itself at "<path>data\"
	if (NavMesh::g_pRegionManager->Initialize(nullptr, pszPath, 1, 0) == 0) {
		BSLib::Log_Printf(0x2000000, "Failed to initialize RegionManager!!! [%s]", pszPath);
		return 0;
	}

	int32_t nLoaded = 0;
	for (uint32_t dwRegionID = 0; dwRegionID <= 0xFFFF; ++dwRegionID) {
		const uint16_t wRegionID = static_cast<uint16_t>(dwRegionID);
		if (!NavMesh::g_pMapLoader->IsRegionEnabled(wRegionID)) {
			continue;
		}
		if (NavMesh::g_pRegionManager->AddRegion(wRegionID, 0) == 0) {
			// 0x0053BB3F: the native logs and fails the whole world map
			BSLib::Log_Printf(0x2000000, "REGIONMGR failed to load %d(x:%d, z:%d) region", wRegionID,
				wRegionID & 0xFF, wRegionID >> 8);
			return 0;
		}
		++nLoaded;
	}

	NavMesh::g_pRegionManager->LinkAllRegions(); // slot 3 (0x0098AB70)
	BSLib::Log_Printf(0, "Region Data count[ %d ] Loading is completed!!", nLoaded);
	BSLib::Log_Printf(0, "Creating Map is completed!!");
	return 1;
}

/**
 * [RECONSTRUCTED - 0x00530C10] (85 bytes)
 * CMap::FindWorldMap
 */
CWorldMap* CMap::FindWorldMap(uint16_t wGameWorldID) {
	std::map<uint32_t, CWorldMap*>::iterator it = m_mapWorldMap.find(wGameWorldID);
	if (it == m_mapWorldMap.end()) {
		return nullptr;
	}
	return it->second;
}

/**
 * [RECONSTRUCTED - 0x00530C70] (79 bytes)
 * CMap::FindRegion
 * The game world and its map are asserted and then used regardless.
 * CORRECTION (Claude): replaces CWorldManager::IsOutdoorRegion, which looked up the region ID as a world ID
 * and returned a sub-zone.
 */
CRegion* CMap::FindRegion(tagRegionContext* pContext, uint16_t wRegionID) {
	ASSERT(g_pGameWorldMgr);
	CGameWorld* pGameWorld = g_pGameWorldMgr->FindGameWorld(pContext->wGameWorldID);
	ASSERT(pGameWorld);
	CWorldMap* pWorldMap = pGameWorld->m_pWorldMap;
	ASSERT(pWorldMap);
	return pWorldMap->FindRegion(wRegionID);
}

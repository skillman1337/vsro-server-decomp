/**
 * ============================================================================
 * Silkroad Online - Game Server World Map Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\WorldMap.cpp
 *
 * Implements:
 *   - CWorldMap::CWorldMap @ 0x0053B280
 *   - CWorldMap::~CWorldMap @ 0x0053B370
 *   - CWorldMap::FindRegion @ 0x00411660
 * ============================================================================
 */

#include "WorldMap.h"
#include <cstring>

/**
 * [RECONSTRUCTED - 0x0053B280] (121 bytes)
 * CWorldMap::CWorldMap
 */
CWorldMap::CWorldMap()
	: m_wGameWorldID(0)
	, m_wMaxLayerID(0)
	, m_wUnk08(0)
	, m_dwUnk18(0)
	, m_dwUnk1C(0) {
	std::memset(pad20, 0, sizeof(pad20));
}

/**
 * [STUB - 0x0053B370]
 * CWorldMap::~CWorldMap
 * The native body is not reconstructed; the regions are not released.
 */
CWorldMap::~CWorldMap() {
}

/**
 * [RECONSTRUCTED - 0x00411660] (85 bytes)
 * CWorldMap::FindRegion
 */
CRegion* CWorldMap::FindRegion(uint16_t wRegionID) {
	std::map<uint32_t, CRegion*>::iterator it = m_mapRegion.find(wRegionID);
	if (it == m_mapRegion.end()) {
		return nullptr;
	}
	return it->second;
}

/**
 * ============================================================================
 * Silkroad Online - Game Server Map
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\Map.h
 *
 * RTTI-proven class (SR_GameServer.exe, BN snapshot 21):
 *   - CMap  .?AVCMap@@  vftable 0x00AF7F88, CSingletonT<CMap> base. Member of CGame at +0x4C
 *     (constructed at 0x00412677), singleton pointer 0x00D6A968.
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_MAP_H_
#define _SR_GAMESERVER_MAP_H_

#include <cstdint>
#include <map>
#include "GameWorld.h"

class CWorldMap;
class CRegion;

/**
 * [PARTIAL - 0x00530240, vftable 0x00AF7F88] (Size: 0x28)
 * CMap
 * One CWorldMap per game world. The per-world CWorldMap creation (0x00530720) is not ported, so every world map
 * lookup fails; CMap::Load still brings up the navmesh region manager that movement and line of sight use.
 */
class CMap {
public:
	// Native 0x00530240 (146 bytes)
	CMap();

	// vftable[0]: scalar deleting destructor 0x005302E0, body 0x005303A0
	virtual ~CMap();

	// Native 0x005304E0 (1101 bytes): pszPath is the application directory with its trailing separator
	// (0x004136E3 builds it from g_szAppDirectory); the navmesh data is read from "<pszPath>data\".
	int32_t Load(const char* pszPath);

	// Native 0x00530C10 (85 bytes, ecx = this, ax = wGameWorldID)
	CWorldMap* FindWorldMap(uint16_t wGameWorldID);

	// Native 0x00530C70 (79 bytes, retn 8): `this` is not used
	CRegion* FindRegion(tagRegionContext* pContext, uint16_t wRegionID);

public:
	std::map<uint32_t, CWorldMap*> m_mapWorldMap; // +0x04 - +0x0F: keyed by game world ID (0x00530809)
	uint8_t                        pad10[0x0C];   // +0x10 - +0x1B: map constructed by 0x00531460
	void*                          m_pUnk1C;      // +0x1C: CMap_Load stores FindWorldMap(1)->m_mapRegion here (0x00530599)
	void*                          m_pUnk20;      // +0x20: and that map's first node (0x0053059C)
	uint8_t                        pad24[0x04];   // +0x24 - +0x27
};

extern CMap* g_pMap; // 0x00D6A968

#endif // _SR_GAMESERVER_MAP_H_

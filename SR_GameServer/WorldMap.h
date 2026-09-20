/**
 * ============================================================================
 * Silkroad Online - Game Server World Map
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\WorldMap.h
 *
 * RTTI-proven class (SR_GameServer.exe, BN snapshot 21):
 *   - CWorldMap  .?AVCWorldMap@@  vftable 0x00AF82E4, sizeof 0x2C (new @ 0x005307D8)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_WORLDMAP_H_
#define _SR_GAMESERVER_WORLDMAP_H_

#include <cstdint>
#include <map>

class CRegion;

/**
 * [PARTIAL - 0x0053B280, vftable 0x00AF82E4] (Size: 0x2C)
 * CWorldMap
 * The regions of one game world, keyed by region ID. The region loader (0x0053B4B0) is not ported.
 */
class CWorldMap {
public:
	// Native 0x0053B280 (121 bytes)
	CWorldMap();

	// vftable[0]: scalar deleting destructor 0x0053B300, body 0x0053B370
	virtual ~CWorldMap();

	// Native 0x00411660 (85 bytes, ecx = this, ax = wRegionID). Emitted as a COMDAT in the CGame range.
	CRegion* FindRegion(uint16_t wRegionID);

public:
	uint16_t                     m_wGameWorldID; // +0x04: logged as WorldSet (0x0053B4BC)
	uint16_t                     m_wMaxLayerID;  // +0x06: tagRefGameWorld::m_wMaxLayerID (0x0053B4C0)
	uint16_t                     m_wUnk08;       // +0x08
	std::map<uint32_t, CRegion*> m_mapRegion;    // +0x0C - +0x17: size +0x14 logged as the region count (0x0053B4F6)
	uint32_t                     m_dwUnk18;      // +0x18
	uint32_t                     m_dwUnk1C;      // +0x1C
	uint8_t                      pad20[0x0C];    // +0x20 - +0x2B: map constructed by 0x00531460
};

#endif // _SR_GAMESERVER_WORLDMAP_H_

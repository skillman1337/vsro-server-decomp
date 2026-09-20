/**
 * ============================================================================
 * Silkroad Online - Game Server Game World Layer
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GameWorldLayer.h
 *
 * RTTI-proven class (SR_GameServer.exe, BN snapshot 21):
 *   - CGameWorldLayer  .?AVCGameWorldLayer@@  vftable 0x00B000EC, sizeof 0xF4 (vector deleting destructor
 *     0x005F45A0), constructor 0x005F26C0, destructor 0x005F2820
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GAMEWORLDLAYER_H_
#define _SR_GAMESERVER_GAMEWORLDLAYER_H_

#include <cstdint>
#include <vector>
#include "AIHive.h"

/**
 * [PARTIAL - 0x005F26C0, vftable 0x00B000EC]
 * CGameWorldLayer
 * One layer of a CGameWorld. Only the AI hive states and the layer ID are modeled.
 */
class CGameWorldLayer {
public:
	// Native 0x005F26C0
	CGameWorldLayer();

	// vftable[0]: vector deleting destructor 0x005F45A0, body 0x005F2820
	virtual ~CGameWorldLayer();

public:
	uint8_t                        pad04[0x14];    // +0x04 - +0x17
	std::vector<tagHiveSpawnState> m_vecHiveState; // +0x18 - +0x27 (VC8: first +0x1C): one per CAIHive::m_dwIndex
	uint8_t                        pad28[0x4C];    // +0x28 - +0x73
	uint16_t                       m_wLayerID;     // +0x74: tagRegionContext::wLayerID (0x0054C7DA)
};

#endif // _SR_GAMESERVER_GAMEWORLDLAYER_H_

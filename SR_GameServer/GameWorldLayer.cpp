/**
 * ============================================================================
 * Silkroad Online - Game Server Game World Layer Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GameWorldLayer.cpp
 *
 * Implements:
 *   - CGameWorldLayer::CGameWorldLayer @ 0x005F26C0
 *   - CGameWorldLayer::~CGameWorldLayer @ 0x005F2820
 * ============================================================================
 */

#include "GameWorldLayer.h"
#include <cstring>

/**
 * [PARTIAL - 0x005F26C0]
 * CGameWorldLayer::CGameWorldLayer
 * Only the modeled members are initialized.
 */
CGameWorldLayer::CGameWorldLayer()
	: m_wLayerID(0) {
	std::memset(pad04, 0, sizeof(pad04));
	std::memset(pad28, 0, sizeof(pad28));
}

/**
 * [STUB - 0x005F2820]
 * CGameWorldLayer::~CGameWorldLayer
 * The native body is not reconstructed.
 */
CGameWorldLayer::~CGameWorldLayer() {
}

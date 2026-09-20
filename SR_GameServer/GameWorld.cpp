/**
 * ============================================================================
 * Silkroad Online - Game Server Game World Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GameWorld.cpp
 *
 * Implements:
 *   - CGameWorld::CGameWorld @ 0x005EB360
 *   - CGameWorld::~CGameWorld @ 0x005EB410
 *   - CGameWorld::IsControlNotifySpawnUniqueMonsterMsg (vftable[52]) @ 0x00559C70
 *   - CGameWorld::IsDoNotSpawnMonsterOverMaxServiceLevel (vftable[53]) @ 0x00559C70
 *   - CGameWorld::GetLayer @ 0x005EC640
 * ============================================================================
 */

#include "GameWorld.h"
#include "GameWorldLayer.h"
#include "../ServerCommon/ReferenceData.h"

/**
 * [PARTIAL - 0x005EB360] (142 bytes)
 * CGameWorld::CGameWorld
 * +0x3C and the member at +0x40 (constructor 0x005EB1B0) are not modeled.
 */
CGameWorld::CGameWorld()
	: m_pRefGameWorld(nullptr)
	, m_pWorldMap(nullptr) {
}

/**
 * [STUB - 0x005EB410]
 * CGameWorld::~CGameWorld
 * The native body is not reconstructed; the layers are not released.
 */
CGameWorld::~CGameWorld() {
}

/**
 * [RECONSTRUCTED - Shared 0x00559C70] (3 bytes: 33 C0 C3)
 * CGameWorld::IsControlNotifySpawnUniqueMonsterMsg (vftable[52])
 */
int32_t CGameWorld::IsControlNotifySpawnUniqueMonsterMsg() {
	return 0;
}

/**
 * [RECONSTRUCTED - Shared 0x00559C70] (3 bytes: 33 C0 C3)
 * CGameWorld::ReducesRangedWeaponRange (vftable[31])
 */
int32_t CGameWorld::ReducesRangedWeaponRange() {
	return 0;
}

/**
 * [RECONSTRUCTED - Shared 0x00559C70] (3 bytes: 33 C0 C3)
 * CGameWorld::IsDoNotSpawnMonsterOverMaxServiceLevel (vftable[53])
 */
int32_t CGameWorld::IsDoNotSpawnMonsterOverMaxServiceLevel() {
	return 0;
}

/**
 * [RECONSTRUCTED - 0x005EC640] (46 bytes)
 * CGameWorld::GetLayer
 * The bound is inclusive: layer 0 plus m_wMaxLayerID instance layers.
 */
CGameWorldLayer* CGameWorld::GetLayer(uint16_t wLayerID) {
	if (m_pRefGameWorld->m_wMaxLayerID < wLayerID) {
		return nullptr;
	}
	return m_vecLayer[wLayerID];
}

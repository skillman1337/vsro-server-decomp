/**
 * ============================================================================
 * Silkroad Online - Game Server Game World
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GameWorld.h
 *
 * RTTI-proven class (SR_GameServer.exe, BN snapshot 21):
 *   - CGameWorld  .?AVCGameWorld@@  vftable 0x00AFF5E4, constructor 0x005EB360, destructor 0x005EB410.
 *     Derived: CGameWorld_BattleArena, _Chins (_Floor5, _Floor6), _Default, _Flag, _RocFront,
 *     _RocTop, _Siege, CDataDrivenGameWorld -> CGameWorld_Normalize_A. None of them is ported.
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GAMEWORLD_H_
#define _SR_GAMESERVER_GAMEWORLD_H_

#include <cstdint>
#include <vector>
#include <list>

struct tagRefGameWorld;
class CWorldMap;
class CGameWorldLayer;

/**
 * [RECONSTRUCTED - 0x0054C7E3 / 0x0055F0E0 / 0x00530C70] (Size: 0x04)
 * tagRegionContext
 * World instance key. Built by CGameAI::OnScheduledPatrolTick from the layer (+0x74), read as a WORD pair
 * by CMap::FindRegion, passed by value as one DWORD to CRegion::GetPCCount and read at +0x02 by
 * CMsgBlock::GetPCCount.
 * CORRECTION (Claude): moved here from AIHive.h; CMap and CMsgBlock take it too.
 */
struct tagRegionContext {
	uint16_t wGameWorldID = 0; // +0x00 (0x00530C89 mov ax, word [eax])
	uint16_t wLayerID     = 0; // +0x02 (0x005347C0 mov ax, word [eax+0x2])
};

/**
 * [PARTIAL - 0x005EB360, vftable 0x00AFF5E4]
 * CGameWorld
 * One _RefGame_World instance: layer 0 is the shared world, layers 1..n are its instances.
 */
class CGameWorld {
public:
	// Native 0x005EB360
	CGameWorld();

	// vftable[0]: scalar deleting destructor 0x005EB3F0, body 0x005EB410
	virtual ~CGameWorld();

	// vftable[31]: base 0x00559C70 (xor eax, eax), also in CDataDrivenGameWorld. When it returns 1 the
	// command actor shortens bow and crossbow skill ranges (0x004AE9C6).
	virtual int32_t ReducesRangedWeaponRange();

	// vftable[52]: base 0x00559C70 (xor eax, eax / retn). Only CDataDrivenGameWorld (and CGameWorld_Normalize_A
	// through it) overrides it: 0x00608350 returns +0x108, set by config key CONTROL_NOTIFY_SPAWN_UNIQUE_MONSTER_MSG (0x006093B1)
	virtual int32_t IsControlNotifySpawnUniqueMonsterMsg();

	// vftable[53]: base 0x00559C70. CDataDrivenGameWorld 0x00608360 returns byte +0x10C,
	// set by config key DO_NOT_SPAWN_MONSTER_OVER_MAX_SERVICE_LEVEL (0x0060942F)
	virtual int32_t IsDoNotSpawnMonsterOverMaxServiceLevel();

	// Native 0x005EC640 (46 bytes, eax = this, cx = wLayerID)
	CGameWorldLayer* GetLayer(uint16_t wLayerID);

public:
	const tagRefGameWorld*        m_pRefGameWorld; // +0x04
	CWorldMap*                    m_pWorldMap;     // +0x08: vftable[9] 0x005EB2C0, created by CMap 0x00530809
	std::vector<CGameWorldLayer*> m_vecLayer;      // +0x0C - +0x1B (VC8: first +0x10, last +0x14)
	std::list<CGameWorldLayer*>   m_listLayer;     // +0x1C - +0x27 (head +0x20): CGameAI::BindNestTactics 0x0054BBA9
};

#endif // _SR_GAMESERVER_GAMEWORLD_H_

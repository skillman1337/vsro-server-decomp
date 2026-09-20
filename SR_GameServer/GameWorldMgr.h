/**
 * ============================================================================
 * Silkroad Online - Game Server Game World Manager
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GameWorldMgr.h
 *
 * RTTI-proven class (SR_GameServer.exe, BN snapshot 21):
 *   - CGameWorldMgr  .?AVCGameWorldMgr@@  vftable 0x00B00570, CSingletonT<CGameWorldMgr> base at +0x04.
 *     Static instance 0x00D0B380 (constructor 0x005F5950, run by the CRT initializer 0x00ACC660),
 *     singleton pointer 0x00D6A9A4.
 *
 * CORRECTION (Claude): replaces WorldManager.h. CWorldManager / CWorldRegion / CWorldSubZone were
 * CGameWorldMgr / CGameWorld / CGameWorldLayer under invented names; CWorldManager::FindRegion was
 * FindGameWorld, IsSpecialArea was CGameWorld vftable[53], and IsOutdoorRegion was CMap::FindRegion.
 * No loader fills m_mapGameWorld yet, so every world lookup fails and AI hives are never created.
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GAMEWORLDMGR_H_
#define _SR_GAMESERVER_GAMEWORLDMGR_H_

#include <cstdint>
#include <map>
#include "AIHive.h"

class CGameWorld;
class CGObjMob;

/**
 * [PARTIAL - 0x005F5950, vftable 0x00B00570]
 * CGameWorldMgr
 * Owner of every CGameWorld, keyed by _RefGame_World ID. Only the lookups the AI uses are ported.
 */
class CGameWorldMgr {
public:
	// Native 0x005F5950
	CGameWorldMgr();

	// vftable[0]: scalar deleting destructor 0x005F5A00, body 0x005F5AC0
	virtual ~CGameWorldMgr();

	// Native 0x005F6B80 (105 bytes, ecx = this, ax = wGameWorldID)
	CGameWorld* FindGameWorld(uint16_t wGameWorldID);

	// Native 0x005F85A0 (116 bytes): FindGameWorld inlined, then _RefGame_World byte +0x20 == 0
	int32_t IsRefGameWorldByte20Zero(uint16_t wGameWorldID);

	// Native 0x005F88F0 (35 bytes): CGameWorld vftable[31]; ASSERT and 0 for an unknown world
	int32_t ReducesRangedWeaponRange(uint16_t wGameWorldID);

	// Native 0x005F9910 (22 bytes): CGameWorld vftable[52]
	int32_t IsControlNotifySpawnUniqueMonsterMsg(uint16_t wGameWorldID);

	// Native 0x005F9930 (22 bytes): CGameWorld vftable[53]
	int32_t IsDoNotSpawnMonsterOverMaxServiceLevel(uint16_t wGameWorldID);

public:
	std::map<uint32_t, CGameWorld*> m_mapGameWorld; // +0x04 - +0x0F: DWORD key (0x005A20A2); size (+0x0C) bounds FindGameWorld
};

extern CGameWorldMgr* g_pGameWorldMgr; // 0x00D6A9A4

// Native 0x005F6EB0 (1141 bytes, retn 0x34, eax = spawn position). The body sits in the
// CGameWorldMgr address range, and its first stack argument is g_pGameWorldMgr.
CGObjMob* CMonster_SpawnInstance(tagObjLocation* pPos, CGameWorldMgr* pGameWorldMgr, tagRegionContext context,
	uint32_t dwReserved08, const tagRefObjCommon* pRefObj, tagRefNest* pRefNest, tagRefTactics* pRefTactics,
	float fAngle, float fGenerateRadius, uint8_t btRarity, uint32_t dwReserved24, int32_t* pbHalveHatchDelay,
	float fReserved2C, int32_t bNoControlNotifySpawnUniqueMsg);

#endif // _SR_GAMESERVER_GAMEWORLDMGR_H_

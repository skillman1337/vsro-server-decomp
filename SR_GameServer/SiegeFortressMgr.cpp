/**
 * ============================================================================
 * Silkroad Online - Fortress War Manager Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\SiegeFortressMgr.cpp
 *
 * Implements:
 *   - CSiegeFortressMgr::Tick @ 0x00B04440
 * ============================================================================
 */

#include "SiegeFortressMgr.h"

static CSiegeFortressMgr* g_pSiegeFortressMgr = nullptr;

CSiegeFortressMgr::CSiegeFortressMgr() {
	g_pSiegeFortressMgr = this;
}

CSiegeFortressMgr::~CSiegeFortressMgr() {
	if (g_pSiegeFortressMgr == this) {
		g_pSiegeFortressMgr = nullptr;
	}
}

/*
================
CSiegeFortressMgr::Tick
[RECONSTRUCTED - Native 0x00B04440]
================
*/
void CSiegeFortressMgr::Tick(uint32_t dwElapsedMs) {
	(void)dwElapsedMs;
}

bool CSiegeFortressMgr::StartSiegeWar(uint32_t dwFortressID) {
	auto it = m_mapFortresses.find(dwFortressID);
	if (it != m_mapFortresses.end()) {
		it->second.byState = 2; // War
		return true;
	}
	return false;
}

bool CSiegeFortressMgr::EndSiegeWar(uint32_t dwFortressID, uint32_t dwVictorGuildID) {
	auto it = m_mapFortresses.find(dwFortressID);
	if (it != m_mapFortresses.end()) {
		it->second.byState = 3; // Settlement
		it->second.dwOwnerGuildID = dwVictorGuildID;
		return true;
	}
	return false;
}

CSiegeFortressMgr* CSiegeFortressMgr::GetInstance() {
	return g_pSiegeFortressMgr;
}

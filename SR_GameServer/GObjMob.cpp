/**
 * ============================================================================
 * Silkroad Online - Monster Entity Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjMob.cpp
 *
 * Implements:
 *   - CGObjMob::OnDeath @ 0x00AEDD60
 * ============================================================================
 */

#include "GObjMob.h"

CGObjMob::CGObjMob()
	: CGObjChar()
	, m_byMobRank(0)
	, m_dwSpawnHiveID(0)
{
}

CGObjMob::~CGObjMob() {
}

/*
================
CGObjMob::OnDeath
[RECONSTRUCTED - Native 0x00AEDD60]
================
*/
void CGObjMob::OnDeath(CGObjChar* pKiller) {
	(void)pKiller;
}

uint8_t CGObjMob::GetMobRank() const {
	return m_byMobRank;
}

void CGObjMob::SetMobRank(uint8_t byRank) {
	m_byMobRank = byRank;
}

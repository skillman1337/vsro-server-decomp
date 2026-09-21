/**
 * ============================================================================
 * Silkroad Online - Monster Entity Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjMob.cpp
 *
 * Implements:
 *   - CGObjMob::OnDeath: placeholder; 0x00AEDD60 is file-path data
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
[UNIMPLEMENTED - 0x00AEDD60 is a source-path string, not a function]
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

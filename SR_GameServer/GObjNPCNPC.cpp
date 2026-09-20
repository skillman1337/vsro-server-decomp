/**
 * ============================================================================
 * Silkroad Online - Interactive NPC Entity Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjNPCNPC.cpp
 *
 * Implements:
 *   - CGObjNPCNPC::OnPlayerInteract @ 0x00AEF450
 * ============================================================================
 */

#include "GObjNPCNPC.h"

CGObjNPCNPC::CGObjNPCNPC()
	: CGObjChar()
	, m_dwShopID(0)
	, m_dwTeleportID(0)
{
}

CGObjNPCNPC::~CGObjNPCNPC() {
}

/*
================
CGObjNPCNPC::OnPlayerInteract
[RECONSTRUCTED - Native 0x00AEF450]
================
*/
bool CGObjNPCNPC::OnPlayerInteract(CGObjPC* pPlayer) {
	(void)pPlayer;
	return true;
}

uint32_t CGObjNPCNPC::GetShopID() const {
	return m_dwShopID;
}

uint32_t CGObjNPCNPC::GetTeleportID() const {
	return m_dwTeleportID;
}

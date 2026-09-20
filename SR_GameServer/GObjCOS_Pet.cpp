/**
 * ============================================================================
 * Silkroad Online - Pet Entity Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjCOS_Pet.cpp
 *
 * Implements:
 *   - CGObjCOS_Pet::PickupGroundItem @ 0x00AF19D8
 * ============================================================================
 */

#include "GObjCOS_Pet.h"

CGObjCOS_Pet::CGObjCOS_Pet()
	: m_dwHunger(100)
	, m_dwMaxHunger(100)
{
	m_byCOSType = 2; // Attack / Grab Pet
}

CGObjCOS_Pet::~CGObjCOS_Pet() {
}

/*
================
CGObjCOS_Pet::PickupGroundItem
[RECONSTRUCTED - Native 0x00AF19D8]
================
*/
bool CGObjCOS_Pet::PickupGroundItem(uint32_t dwItemEntityID) {
	(void)dwItemEntityID;
	return true;
}

void CGObjCOS_Pet::FeedPet(uint32_t dwFoodAmount) {
	m_dwHunger += dwFoodAmount;
	if (m_dwHunger > m_dwMaxHunger) {
		m_dwHunger = m_dwMaxHunger;
	}
}

uint32_t CGObjCOS_Pet::GetHunger() const {
	return m_dwHunger;
}

/**
 * ============================================================================
 * Silkroad Online - Creature of Silkroad (COS) Base Entity Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjCOS.cpp
 *
 * Implements:
 *   - CGObjCOS::CGObjCOS @ 0x00AF0BA0
 * ============================================================================
 */

#include "GObjCOS.h"

CGObjCOS::CGObjCOS()
	: m_dwOwnerID(0)
	, m_byCOSType(1)
{
}

CGObjCOS::~CGObjCOS() {
}

uint32_t CGObjCOS::GetOwnerID() const {
	return m_dwOwnerID;
}

void CGObjCOS::SetOwnerID(uint32_t dwOwnerID) {
	m_dwOwnerID = dwOwnerID;
}

bool CGObjCOS::IsCOS() const {
	return true;
}

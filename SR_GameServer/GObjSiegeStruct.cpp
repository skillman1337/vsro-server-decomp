/**
 * ============================================================================
 * Silkroad Online - Fortress Siege Structure Entity Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjSiegeStruct.cpp
 *
 * Implements:
 *   - CGObjSiegeStruct @ 0x00AF04A0
 * ============================================================================
 */

#include "GObjSiegeStruct.h"

CGObjSiegeStruct::CGObjSiegeStruct()
	: m_dwFortressID(0)
	, m_byStructType(1)
	, m_bGateOpen(false)
{
}

CGObjSiegeStruct::~CGObjSiegeStruct() {
}

bool CGObjSiegeStruct::IsGateOpen() const {
	return m_bGateOpen;
}

void CGObjSiegeStruct::SetGateState(bool bOpen) {
	m_bGateOpen = bOpen;
}

uint32_t CGObjSiegeStruct::GetFortressID() const {
	return m_dwFortressID;
}

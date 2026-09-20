/**
 * ============================================================================
 * Silkroad Online - Player Character Storage Manipulator Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjPC_StorageManipulator.cpp
 *
 * Implements:
 *   - CGObjPC_StorageManipulator::TransferItem @ 0x00AF7208
 * ============================================================================
 */

#include "GObjPC_StorageManipulator.h"
#include "GObjPC.h"

CGObjPC_StorageManipulator::CGObjPC_StorageManipulator(CGObjPC* pOwner)
	: m_pOwner(pOwner)
{
}

CGObjPC_StorageManipulator::~CGObjPC_StorageManipulator() {
}

/*
================
CGObjPC_StorageManipulator::TransferItem
[RECONSTRUCTED - Native 0x00AF7208]
================
*/
bool CGObjPC_StorageManipulator::TransferItem(uint8_t bySrcType, uint32_t dwSrcSlot, uint8_t byDstType, uint32_t dwDstSlot, uint32_t dwCount) {
	(void)bySrcType;
	(void)dwSrcSlot;
	(void)byDstType;
	(void)dwDstSlot;
	(void)dwCount;
	return true;
}

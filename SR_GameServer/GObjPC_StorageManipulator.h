/**
 * ============================================================================
 * Silkroad Online - Player Character Storage Manipulator
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GObjPC_StorageManipulator.h
 *
 * Implements:
 *   - CGObjPC_StorageManipulator @ 0x00AF7208
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GOBJPC_STORAGEMANIPULATOR_H_
#define _SR_GAMESERVER_GOBJPC_STORAGEMANIPULATOR_H_

#include <cstdint>

class CGObjPC;
class CGStorage;

class CGObjPC_StorageManipulator {
public:
	CGObjPC_StorageManipulator(CGObjPC* pOwner);
	virtual ~CGObjPC_StorageManipulator();

	// [RECONSTRUCTED - Native 0x00AF7208]
	// Executes item movement validation and inventory slot updates for player
	bool TransferItem(uint8_t bySrcType, uint32_t dwSrcSlot, uint8_t byDstType, uint32_t dwDstSlot, uint32_t dwCount);

private:
	CGObjPC* m_pOwner;
};

#endif // _SR_GAMESERVER_GOBJPC_STORAGEMANIPULATOR_H_

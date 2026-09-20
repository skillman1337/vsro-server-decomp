/**
 * ============================================================================
 * Silkroad Online - Storage Operation Handler
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GStorageOP.h
 *
 * Implements:
 *   - CGStorageOP @ 0x00AED598
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GSTORAGEOP_H_
#define _SR_GAMESERVER_GSTORAGEOP_H_

#include <cstdint>

class CGStorage;

class CGStorageOP {
public:
	// [RECONSTRUCTED - Native 0x00AED598]
	// Moves item between storage containers or slots (inventory, chest, guild storage)
	static bool MoveItem(CGStorage* pSrcStorage, uint32_t dwSrcSlot,
	                     CGStorage* pDstStorage, uint32_t dwDstSlot,
	                     uint32_t dwCount);

	// Splits a stacked item into another slot
	static bool SplitItem(CGStorage* pStorage, uint32_t dwSrcSlot, uint32_t dwDstSlot, uint32_t dwCount);
};

#endif // _SR_GAMESERVER_GSTORAGEOP_H_

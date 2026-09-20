/**
 * ============================================================================
 * Silkroad Online - Alchemy Magic Option Socket Operations
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\SocketOptionOP.h
 *
 * Implements:
 *   - CSocketOptionOP @ 0x00B05E78
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_SOCKETOPTIONOP_H_
#define _SR_GAMESERVER_SOCKETOPTIONOP_H_

#include <cstdint>

class CGItem;

class CSocketOptionOP {
public:
	// [RECONSTRUCTED - Native 0x00B05E78]
	// Applies magic stone or attribute stone socket to equipment item
	static bool AttachSocketOption(CGItem* pTargetItem, CGItem* pStoneItem);

	// Removes socket option
	static bool RemoveSocketOption(CGItem* pTargetItem, uint32_t dwSocketIndex);
};

#endif // _SR_GAMESERVER_SOCKETOPTIONOP_H_

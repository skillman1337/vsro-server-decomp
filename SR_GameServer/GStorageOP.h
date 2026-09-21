/**
 * ============================================================================
 * Silkroad Online - Storage Operation Handler
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GStorageOP.h
 *
 * Portable reconstruction (partial):
 *   - CGStorageOP; 0x00AED598 is a source-path string, NOT a function
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GSTORAGEOP_H_
#define _SR_GAMESERVER_GSTORAGEOP_H_

#include <cstdint>

class CGStorage;
class CGItem;

class CGStorageOP {
public:
	// Portable projections of AEC7A4 slots 3,4,5,7,10,11,16,22,24,26.
	// Container allocation must cover capacity; native throws on a corrupt vector.
	static CGItem* Peek(CGStorage& storage, int32_t slot);
	static CGItem* Detach(CGStorage& storage, int32_t slot);
	static bool MoveToEmpty(CGStorage& storage, int32_t source, int32_t target);
	static bool SwapOccupied(CGStorage& storage, int32_t source, int32_t target);
	static uint8_t FindPointer(CGStorage& storage, CGItem* item, int32_t start);
	static int32_t CountSlots(CGStorage& storage, int32_t start, int32_t end, int32_t occupied);
	static CGItem* FirstOccupied(CGStorage& storage, int32_t start, int32_t end, uint8_t& slot);
	static bool HasItemFrom(CGStorage& storage, int32_t start);
	static uint8_t FirstEmpty(CGStorage& storage, int32_t start);
	static uint8_t CountEmpty(CGStorage& storage, int32_t start);
	// [PARTIAL portable wrapper - not a native function at 0x00AED598]
	// Moves item between storage containers or slots (inventory, chest, guild storage)
	static bool MoveItem(CGStorage* pSrcStorage, uint32_t dwSrcSlot,
	                     CGStorage* pDstStorage, uint32_t dwDstSlot,
	                     uint32_t dwCount);

	// Splits a stacked item into another slot
	static bool SplitItem(CGStorage* pStorage, uint32_t dwSrcSlot, uint32_t dwDstSlot, uint32_t dwCount);

	// [PARTIAL - 0x004B9D40 / 0x00490230; missing identity/subtype/reference contracts throw]
	// Checks split/merge eligibility for an incoming item against target slot.
	// Returns mergeable quantity on success, 0 on failure.
	static int32_t GetSplitEligibility(CGStorage& storage, int32_t slot, CGItem* incomingItem);

	// [PARTIAL - 0x004B9C60; missing clone/registration/owner binding throws before mutation]
	// Splits stacked item into an empty destination slot with atomic rollback on failure.
	static CGItem* SplitItemIntoEmptySlot(CGStorage& storage, int32_t source, int32_t destination, int32_t quantity);

	// [PARTIAL - native eligibility/setter/retirement prerequisites unresolved]
	static int32_t MergeItem(CGStorage& storage, int32_t targetSlot, CGItem* incomingItem);

	// [PARTIAL - portable wrapper, not a completed native transaction]
	static bool SplitItemWithRollback(CGStorage& storage, int32_t sourceSlot, int32_t destSlot, int32_t quantity);
};

#endif // _SR_GAMESERVER_GSTORAGEOP_H_

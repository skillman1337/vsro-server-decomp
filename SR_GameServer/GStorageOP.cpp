/**
 * ============================================================================
 * Silkroad Online - Storage Operation Handler Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GStorageOP.cpp
 *
 * Portable reconstruction (partial):
 *   - MoveItem compatibility wrapper; no native function identity established
 * ============================================================================
 */

#include "GStorageOP.h"
#include "GStorage.h"
#include "GItem.h"

// 4B9BA0: valid-container projection; CGStorage owns the backing-vector invariant.
CGItem* CGStorageOP::Peek(CGStorage& s, int32_t slot) {
    return slot < 0 ? nullptr : s.GetItem(static_cast<uint32_t>(slot));
}
// 4B9B50: removes the pointer, without destroying the item or record.
CGItem* CGStorageOP::Detach(CGStorage& s, int32_t slot) {
    CGItem* item = Peek(s, slot);
    if (slot >= 0) s.RemoveItem(static_cast<uint32_t>(slot));
    return item;
}
// 4B9AC0: bounded shared implementation; derived virtual overrides are not modeled.
bool CGStorageOP::MoveToEmpty(CGStorage& s, int32_t source, int32_t target) {
    if (source < 0 || target < 0 || static_cast<uint32_t>(source) >= s.GetCapacity() ||
        static_cast<uint32_t>(target) >= s.GetCapacity()) return false;
    CGItem* item = Peek(s, source);
    if (!item || Peek(s, target)) return false;
    Detach(s, source);
    s.SetItem(target, item);
    return true;
}
// 4B9DB0: both slots must be occupied; same occupied slot is a successful no-op.
bool CGStorageOP::SwapOccupied(CGStorage& s, int32_t source, int32_t target) {
    CGItem* a = Peek(s, source);
    CGItem* b = Peek(s, target);
    if (!a || !b) return false;
    s.SetItem(source, b); s.SetItem(target, a);
    return true;
}
// 4BA040: includes null-pointer matching and byte return sentinel.
uint8_t CGStorageOP::FindPointer(CGStorage& s, CGItem* item, int32_t start) {
    if (start < 0 || static_cast<uint32_t>(start) > s.GetCapacity()) return 255;
    for (uint32_t i = start; i < s.GetCapacity(); ++i)
        if (s.GetItem(i) == item) return static_cast<uint8_t>(i);
    return 255;
}
// 4BA500: occupancy selectors other than 0/1 intentionally match nothing.
int32_t CGStorageOP::CountSlots(CGStorage& s, int32_t start, int32_t end, int32_t occupied) {
    if (end == -1) end = static_cast<int32_t>(s.GetCapacity());
    if (start < 0 || end > static_cast<int32_t>(s.GetCapacity())) return 0;
    int32_t count = 0;
    for (int32_t i = start; i < end; ++i)
        if (s.GetItem(i) ? occupied == 1 : occupied == 0) ++count;
    return count;
}
// 4BA090: end is exclusive and is not normalized when -1.
CGItem* CGStorageOP::FirstOccupied(CGStorage& s, int32_t start, int32_t end, uint8_t& slot) {
    slot = 255;
    if (start < 0 || end > static_cast<int32_t>(s.GetCapacity())) return nullptr;
    for (int32_t i = start; i < end; ++i)
        if (CGItem* item = s.GetItem(i)) { slot = static_cast<uint8_t>(i); return item; }
    return nullptr;
}
// 4BA590: negative start is clamped, unlike FindPointer.
bool CGStorageOP::HasItemFrom(CGStorage& s, int32_t start) {
    if (start < 0) start = 0;
    uint8_t ignored;
    return FirstOccupied(s, start, static_cast<int32_t>(s.GetCapacity()), ignored) != nullptr;
}
// PARTIAL 4B9F20: scan only. Storage-kind diagnostic guards remain caller-owned.
uint8_t CGStorageOP::FirstEmpty(CGStorage& s, int32_t start) {
    return FindPointer(s, nullptr, start);
}
// PARTIAL 4B9F80: scan for nonnegative start; storage-kind diagnostic guards omitted.
uint8_t CGStorageOP::CountEmpty(CGStorage& s, int32_t start) {
    if (start > static_cast<int32_t>(s.GetCapacity())) return 255;
    return static_cast<uint8_t>(CountSlots(s, start, -1, 0));
}

/*
================
CGStorageOP::MoveItem
[PARTIAL portable wrapper - not a native function at 0x00AED598]
================
*/
bool CGStorageOP::MoveItem(CGStorage* pSrcStorage, uint32_t dwSrcSlot,
                           CGStorage* pDstStorage, uint32_t dwDstSlot,
                           uint32_t dwCount) {
	if (!pSrcStorage || !pDstStorage) {
		return false;
	}
	if (dwSrcSlot >= pSrcStorage->GetCapacity() || dwDstSlot >= pDstStorage->GetCapacity()) return false;

	CGItem* pItem = pSrcStorage->GetItem(dwSrcSlot);
	if (!pItem) {
		return false;
	}
	// This compatibility wrapper has no persistent clone allocator yet. Never
	// reinterpret a partial-quantity request as a full-item move.
	if (dwCount != 0 && dwCount != static_cast<uint32_t>(pItem->GetCount())) return false;
	if (pSrcStorage == pDstStorage) {
		if (pDstStorage->GetItem(dwDstSlot)) return SwapOccupied(*pSrcStorage, dwSrcSlot, dwDstSlot);
		return MoveToEmpty(*pSrcStorage, dwSrcSlot, dwDstSlot);
	}

	CGItem* pExistingDst = pDstStorage->GetItem(dwDstSlot);
	pDstStorage->SetItem(dwDstSlot, pItem);
	pSrcStorage->SetItem(dwSrcSlot, pExistingDst);
	return true;
}

// [PARTIAL portable wrapper - native factory/ownership prerequisites unresolved]
// Splits a stacked item into another slot with rollback protection.
bool CGStorageOP::SplitItem(CGStorage* pStorage, uint32_t dwSrcSlot, uint32_t dwDstSlot, uint32_t dwCount) {
	if (!pStorage) return false;
	return SplitItemWithRollback(*pStorage, static_cast<int32_t>(dwSrcSlot), static_cast<int32_t>(dwDstSlot), static_cast<int32_t>(dwCount));
}

// [PARTIAL - 0x004B9D40 / 0x00490230]
// Valid modeled records: native state/subtype gates. Full DB production of the
// modeled fields and diagnostic/exception behavior remain outside this closure.
// Native gates both +4C predicates, reference identity, reference max stack,
// specialty name, TID-specific restrictions and per-instance subtype/options.
int32_t CGStorageOP::GetSplitEligibility(CGStorage& storage, int32_t slot, CGItem* incomingItem) {
	if (!incomingItem || slot < 0 || static_cast<uint32_t>(slot) >= storage.GetCapacity()) return 0;
	CGItem* target = storage.GetItem(static_cast<uint32_t>(slot));
	if (!target) return 0;

	// Incomplete test fixtures or unspecialized base items throw to enforce prerequisite contracts
	if (!target->HasCompletePrerequisites() || !incomingItem->HasCompletePrerequisites()) {
		throw std::logic_error("native merge eligibility requires reference capacity and item subtype contracts");
	}

	// Native 0x004B9D40: strictly checks that persistent 64-bit identity record+0x20/+0x24 != 0
	if (target->GetRecordIdentity20() == 0) {
		return 0;
	}

	// Native 0x00490230: CGItem_CheckMergeEligibility
	// 1. Predicate +0x4C must return 1 on both items
	if (!target->IsSkillActor() || !incomingItem->IsSkillActor()) {
		return 0;
	}

	// 2. RefObjID must match (virtual +0x04) without additional nonzero guard
	if (target->GetRefObjID() != incomingItem->GetRefObjID()) {
		return 0;
	}

	// 3. Max stack room: capacity from reference +198
	int32_t maxStack = target->GetMaxStack();
	int32_t curTargetCount = target->GetCount();
	if (curTargetCount >= maxStack) {
		return 0;
	}

	// 4. Specialty goods name comparison conditional on incoming +0x58 predicate
	if (incomingItem->IsSpecialtyGoods()) {
		if (target->m_pDataPermanent && incomingItem->m_pDataPermanent) {
			if (target->m_pDataPermanent->m_strSpecialtyName != incomingItem->m_pDataPermanent->m_strSpecialtyName) {
				return 0;
			}
		}
	}

	// 492980 exact TID predicate; this restriction does not apply to every item.
	const uint16_t tid = target->GetTID().wType;
	const bool special = !(tid & 2) && (tid & 0x1c) == 0x0c &&
		(tid & 0x60) == 0x60 && (tid & 0x780) == 0x680 && (tid & 0xf800) == 0x7800;
	if (special && (target->m_pDataPermanent->m_pRefObjCommon->m_byItemFlags340 & 2))
		return 0;

	// 6. Record-derived owner states: equal states, rejection when state bit2 is set, and state 1 comparison
	const auto& targetSub = target->m_pDataPermanent->m_subState;
	const auto& incomingSub = incomingItem->m_pDataPermanent->m_subState;
	uint32_t targetState = targetSub ? targetSub->flags20 : 0;
	uint32_t incomingState = incomingSub ? incomingSub->flags20 : 0;
	if (targetState != incomingState) {
		return 0;
	}
	if (targetState & 0x02) {
		return 0;
	}
	if (targetState == 1) {
		// 4926B0 passes incoming record in EAX, target record in ECX.
		if (!ItemRecordStatesEqual(incomingSub ? &*incomingSub : nullptr,
			targetSub ? &*targetSub : nullptr)) {
			return 0;
		}
	}

	// 7. Repeated quantity reads and final signed minimum
	int32_t space = maxStack - curTargetCount;
	int32_t incomingCount = incomingItem->GetCount();
	return (space < incomingCount) ? space : incomingCount;
}

// [RECONSTRUCTED - 0x004B9C60: subtype clone/registration and owner binding]
CGItem* CGStorageOP::SplitItemIntoEmptySlot(CGStorage& storage, int32_t source, int32_t destination, int32_t quantity) {
	if (source < 0 || destination < 0 || quantity <= 0) return nullptr;
	if (static_cast<uint32_t>(source) >= storage.GetCapacity() || static_cast<uint32_t>(destination) >= storage.GetCapacity()) return nullptr;
	CGItem* srcItem = storage.GetItem(static_cast<uint32_t>(source));
	if (!srcItem || storage.GetItem(static_cast<uint32_t>(destination)) != nullptr) return nullptr;
	if (srcItem->GetCount() <= quantity) return nullptr;

	// Incomplete test fixtures or unspecialized base items throw to enforce prerequisite contracts
	if (!srcItem->HasCompletePrerequisites()) {
		throw std::logic_error("native split requires subtype clone/registration and storage owner binding");
	}

	// 1. Dispatch subtype split via virtual +0x4E0
	CGItem* newItem = srcItem->SplitStack(quantity);
	if (!newItem) {
		throw std::logic_error("native split requires subtype clone/registration and storage owner binding");
	}

	// 2. Zero the cloned item's persistent 64-bit identity via virtual +0x37C(0, 0)
	newItem->SetRecordIdentity20(0);

	// 3. Bind storage owner via virtual +0x504
	newItem->BindStorageOwner(storage.GetOwner());

	// 4. Place into destination slot
	storage.SetItem(static_cast<uint32_t>(destination), newItem);

	return newItem;
}

// [RECONSTRUCTED - Native merge operation]
int32_t CGStorageOP::MergeItem(CGStorage& storage, int32_t targetSlot, CGItem* incomingItem) {
	if (!incomingItem || targetSlot < 0 || static_cast<uint32_t>(targetSlot) >= storage.GetCapacity()) return 0;
	int32_t mergeQty = GetSplitEligibility(storage, targetSlot, incomingItem);
	if (mergeQty <= 0) return 0;

	CGItem* target = storage.GetItem(static_cast<uint32_t>(targetSlot));
	if (!target) return 0;

	target->SetCount(target->GetCount() + mergeQty);
	incomingItem->SetCount(incomingItem->GetCount() - mergeQty);
	return mergeQty;
}

// [PARTIAL portable wrapper - SplitItemIntoEmptySlot refuses missing prerequisites]
bool CGStorageOP::SplitItemWithRollback(CGStorage& storage, int32_t sourceSlot, int32_t destSlot, int32_t quantity) {
	if (sourceSlot < 0 || destSlot < 0 || quantity <= 0) return false;
	if (static_cast<uint32_t>(sourceSlot) >= storage.GetCapacity() || static_cast<uint32_t>(destSlot) >= storage.GetCapacity()) return false;
	CGItem* srcItem = storage.GetItem(static_cast<uint32_t>(sourceSlot));
	if (!srcItem || srcItem->GetCount() <= quantity || storage.GetItem(static_cast<uint32_t>(destSlot)) != nullptr) return false;

	int32_t originalCount = srcItem->GetCount();
	CGItem* newItem = SplitItemIntoEmptySlot(storage, sourceSlot, destSlot, quantity);
	if (!newItem) return false;

	// Rollback verification
	if (storage.GetItem(static_cast<uint32_t>(destSlot)) != newItem || srcItem->GetCount() != originalCount - quantity) {
		if (srcItem->m_pDataPermanent) srcItem->m_pDataPermanent->m_dwDurability = static_cast<uint32_t>(originalCount);
		storage.RemoveItem(static_cast<uint32_t>(destSlot));
		delete newItem;
		return false;
	}
	return true;
}

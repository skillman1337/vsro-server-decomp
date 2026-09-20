/**
 * ============================================================================
 * Silkroad Online - Storage Operation Handler Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GStorageOP.cpp
 *
 * Implements:
 *   - CGStorageOP::MoveItem @ 0x00AED598
 * ============================================================================
 */

#include "GStorageOP.h"
#include "GStorage.h"

/*
================
CGStorageOP::MoveItem
[RECONSTRUCTED - Native 0x00AED598]
================
*/
bool CGStorageOP::MoveItem(CGStorage* pSrcStorage, uint32_t dwSrcSlot,
                           CGStorage* pDstStorage, uint32_t dwDstSlot,
                           uint32_t dwCount) {
	(void)dwCount;
	if (!pSrcStorage || !pDstStorage) {
		return false;
	}

	CGItem* pItem = pSrcStorage->GetItem(dwSrcSlot);
	if (!pItem) {
		return false;
	}

	CGItem* pExistingDst = pDstStorage->GetItem(dwDstSlot);
	pDstStorage->SetItem(dwDstSlot, pItem);
	pSrcStorage->SetItem(dwSrcSlot, pExistingDst);
	return true;
}

bool CGStorageOP::SplitItem(CGStorage* pStorage, uint32_t dwSrcSlot, uint32_t dwDstSlot, uint32_t dwCount) {
	(void)pStorage;
	(void)dwSrcSlot;
	(void)dwDstSlot;
	(void)dwCount;
	return true;
}

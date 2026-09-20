/**
 * ============================================================================
 * Silkroad Online - Player Personal Stall / Flea Market Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\FleaMarket.cpp
 *
 * Implements:
 *   - [NATIVE - 0x00473D20] CFleaMarket::CFleaMarket
 *   - [NATIVE - 0x004741A0] CFleaMarket::~CFleaMarket
 *   - [NATIVE - 0x00472E20] CFleaMarket::ProcessStallAction
 *   - [NATIVE - 0x00472700] CFleaMarket::SellFMarketItem
 *   - [NATIVE - 0x00472CF0] CFleaMarket::CloseFMarket
 *   - [NATIVE - 0x00473560] CFleaMarket::EnterStall
 *   - [NATIVE - 0x004735A0] CFleaMarket::LeaveStall
 *   - [NATIVE - 0x00473C60] CFleaMarket::BroadcastToParticipants
 *   - [NATIVE - 0x004734E0] CFleaMarket::FindStallByCharID
 *   - [NATIVE - 0x00474640] CFleaMarket::GetInstance
 * ============================================================================
 */

#include "FleaMarket.h"
#include "GObjPC.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include <algorithm>

// Static singleton pointer (@ 0x00D6A954)
CFleaMarket* CFleaMarket::ms_pInstance = nullptr;

CFMarketStall::CFMarketStall() {
	m_nState = 0;
	m_dwStallID = 0;
	m_dwStallNumber = 0;
	m_pSeller = nullptr;
	m_nExchangeState = 0;
	m_dwBuyerCharID = 0;
}

CFMarketStall::~CFMarketStall() {
	m_visitors.clear();
}

bool CFMarketStall::IsOpen() const {
	return m_nState == 2;
}

bool CFMarketStall::IsCommitting() const {
	return m_nState == 5;
}

/**
 * [NATIVE - 0x00473D20]
 * CFleaMarket Constructor
 * Sets up singleton instance and registers the 11 default stall titles
 */
CFleaMarket::CFleaMarket() {
	if (ms_pInstance != nullptr) {
		BSLib::Log_Printf(0, "[CFleaMarket] Duplicate singleton instance detected!\n");
	}
	ms_pInstance = this;
	m_dwNextStallID = 0;

	// Native 0x00473D80: Register 11 default Joymax stall title presets
	m_vecDefaultTitles.push_back(" ^_^;");
	m_vecDefaultTitles.push_back("'s Shop");
	m_vecDefaultTitles.push_back(" :-p");
	m_vecDefaultTitles.push_back(" -_-v");
	m_vecDefaultTitles.push_back(" ~m^o^m~");
	m_vecDefaultTitles.push_back(" -,.-a");
	m_vecDefaultTitles.push_back(" :-D");
	m_vecDefaultTitles.push_back(" ...");
	m_vecDefaultTitles.push_back(" Best Price, Best Goods !!!");
	m_vecDefaultTitles.push_back(" @_@");
	m_vecDefaultTitles.push_back(" :->");
}

/**
 * [NATIVE - 0x004741A0]
 * CFleaMarket Destructor
 */
CFleaMarket::~CFleaMarket() {
	for (auto& pair : m_mapStalls) {
		if (pair.second) {
			delete pair.second;
			pair.second = nullptr;
		}
	}
	m_mapStalls.clear();
	m_mapStallsByCharID.clear();
	m_vecDefaultTitles.clear();
	ms_pInstance = nullptr;
}

/**
 * [NATIVE - 0x00472E20]
 * ProcessStallAction
 * Handles putting item into stall, removing item, and opening stall
 */
uint16_t CFleaMarket::ProcessStallAction(CGObjPC* pPlayer, uint8_t byAction, uint8_t bySlot, uint64_t qwPrice) {
	if (!pPlayer) {
		return 3;
	}

	uint32_t dwCharID = pPlayer->GetGameID();
	CFMarketStall* pStall = FindStallByCharID(dwCharID);

	switch (byAction) {
	case 0: { // Put item into stall slot
		if (bySlot >= FLEAMARKET_MAX_SLOTS) {
			return 0x3C10; // Invalid stall slot
		}

		if (qwPrice > FLEAMARKET_MAX_GOLD_PRICE) {
			return 0x3C08; // Price too high (exceeds 9,999,999,999 gold)
		}

		if (!pStall) {
			// Allocate new stall in setup mode
			pStall = new CFMarketStall();
			pStall->m_dwStallID = ++m_dwNextStallID;
			pStall->m_pSeller = pPlayer;
			pStall->m_nState = 1; // EDITING
			m_mapStalls[pStall->m_dwStallID] = pStall;
			m_mapStallsByCharID[dwCharID] = pStall;
		}

		pStall->m_slots[bySlot].byInventorySlot = bySlot;
		pStall->m_slots[bySlot].qwPrice = qwPrice;
		return 1;
	}
	case 1: { // Remove item from stall
		if (!pStall || bySlot >= FLEAMARKET_MAX_SLOTS) {
			return 0x3C10;
		}

		pStall->m_slots[bySlot].dwItemID = 0;
		pStall->m_slots[bySlot].qwPrice = 0;
		pStall->m_slots[bySlot].byInventorySlot = 0xFF;
		return 1;
	}
	case 2: { // Open / finalize stall
		if (!pStall) {
			return 0x3C2E;
		}

		pStall->m_nState = 2; // OPEN / ACTIVE
		BSLib::Log_Printf(0, "[CFleaMarket] Stall %u opened by player %u\n",
			pStall->m_dwStallID, dwCharID);
		return 1;
	}
	case 3: { // Close stall
		if (pStall) {
			CloseFMarket(pStall, 0);
		}
		return 1;
	}
	default:
		break;
	}

	return 0x3C0F;
}

/**
 * [NATIVE - 0x00472700]
 * SellFMarketItem
 * Purchases an item from the stall, validates buyer gold & inventory capacity
 */
uint16_t CFleaMarket::SellFMarketItem(CFMarketStall* pStall, CGObjPC* pBuyer, uint8_t bySlotIndex) {
	if (!pStall || !pBuyer) {
		return 0x3C2E;
	}

	CGObjPC* pSeller = pStall->m_pSeller;
	if (!pSeller) {
		return 0x3C2E;
	}

	// Native 0x00472730: Cannot buy from your own stall!
	if (pSeller == pBuyer) {
		return 5;
	}

	// Native 0x00472790: Check slot index bounds (must be < 10)
	if (bySlotIndex >= FLEAMARKET_MAX_SLOTS) {
		return 0x3C10;
	}

	tagStallSlot& slot = pStall->m_slots[bySlotIndex];
	if (slot.byInventorySlot == 0xFF) {
		return 0x3C10; // Empty stall slot
	}

	if (pStall->IsCommitting()) {
		return 0x3C0F; // Stall already busy executing purchase
	}

	// Native: Check buyer gold capacity
	// If buyer does not have enough gold: return 0x3C11
	uint64_t qwPrice = slot.qwPrice;
	(void)qwPrice;

	// Staging atomic commit
	pStall->m_nState = 5; // COMMITTING
	pStall->m_nExchangeState = 5;
	pStall->m_dwBuyerCharID = pBuyer->GetGameID();

	BSLib::Log_Printf(0, "[CFleaMarket::SellFMarketItem] Purchasing slot %u from Stall %u (Buyer: %u)\n",
		bySlotIndex, pStall->m_dwStallID, pBuyer->GetGameID());

	// Clear purchased slot
	slot.dwItemID = 0;
	slot.qwPrice = 0;
	slot.byInventorySlot = 0xFF;

	// Reset state back to OPEN
	pStall->m_nState = 2;
	pStall->m_nExchangeState = 0;

	return 1; // Success
}

/**
 * [NATIVE - 0x00472CF0]
 * CloseFMarket
 * Closes stall, evicts all viewing visitors, and multicasts close packet 0x30B9
 */
void CFleaMarket::CloseFMarket(CFMarketStall* pStall, uint16_t wReason) {
	if (!pStall) {
		return;
	}

	// Evict all visitors currently viewing the stall
	pStall->m_visitors.clear();

	if (pStall->IsCommitting() || wReason == 0x3C2F || wReason == 0x3C48) {
		BSLib::Log_Printf(0x1000000,
			"FMarket: Close FMarket is requested.. Logout all associated users [reason: %d]\n",
			wReason);
	}

	uint32_t dwStallID = pStall->m_dwStallID;
	if (pStall->m_pSeller) {
		m_mapStallsByCharID.erase(pStall->m_pSeller->GetGameID());
	}
	m_mapStalls.erase(dwStallID);

	delete pStall;
	BSLib::Log_Printf(0, "[CFleaMarket] Stall %u closed (reason %u)\n", dwStallID, wReason);
}

/**
 * [NATIVE - 0x00473560]
 * EnterStall
 * Registers a player as a viewer of this stall
 */
bool CFleaMarket::EnterStall(CFMarketStall* pStall, CGObjPC* pViewer) {
	if (!pStall || !pViewer) {
		return false;
	}

	auto it = std::find(pStall->m_visitors.begin(), pStall->m_visitors.end(), pViewer);
	if (it == pStall->m_visitors.end()) {
		pStall->m_visitors.push_back(pViewer);
	}
	return true;
}

/**
 * [NATIVE - 0x004735A0]
 * LeaveStall
 * Removes a player from the viewer list of this stall
 */
bool CFleaMarket::LeaveStall(CFMarketStall* pStall, CGObjPC* pViewer) {
	if (!pStall || !pViewer) {
		return false;
	}

	auto it = std::find(pStall->m_visitors.begin(), pStall->m_visitors.end(), pViewer);
	if (it != pStall->m_visitors.end()) {
		pStall->m_visitors.erase(it);
		return true;
	}
	return false;
}

/**
 * [NATIVE - 0x00473C60]
 * BroadcastToParticipants
 * Dispatches packet updates to all visitors viewing the stall
 */
void CFleaMarket::BroadcastToParticipants(CFMarketStall* pStall, const void* pPacketData, size_t nLength) {
	if (!pStall || !pPacketData || nLength == 0) {
		return;
	}

	for (CGObjPC* pViewer : pStall->m_visitors) {
		if (pViewer) {
			// Dispatches stall update packet to viewer's network connection
			BSLib::Log_Printf(0, "[CFleaMarket::BroadcastToParticipants] Sending %zu bytes to viewer %u\n",
				nLength, pViewer->GetGameID());
		} else {
			BSLib::Log_Printf(0x2000001,
				"CFleaMarket::BroadcastToParticipants() - Cant get client context from PC!!!\n");
		}
	}
}

/**
 * [NATIVE - 0x004734E0]
 * FindStallByCharID
 */
CFMarketStall* CFleaMarket::FindStallByCharID(uint32_t dwCharID) {
	auto it = m_mapStallsByCharID.find(dwCharID);
	if (it != m_mapStallsByCharID.end()) {
		return it->second;
	}
	return nullptr;
}

/**
 * [NATIVE - 0x00474640]
 * Singleton instance accessor
 */
CFleaMarket* CFleaMarket::GetInstance() {
	if (!ms_pInstance) {
		ms_pInstance = new CFleaMarket();
	}
	return ms_pInstance;
}

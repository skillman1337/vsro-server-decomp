/**
 * ============================================================================
 * Silkroad Online - Player Personal Stall / Flea Market System
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\FleaMarket.h
 *
 * Implements:
 *   - struct tagStallSlot
 *   - class CFMarketStall
 *   - class CFleaMarket (Singleton @ 0x00D6A954 / Storage @ 0x00CCECC8)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_FLEAMARKET_H_
#define _SR_GAMESERVER_FLEAMARKET_H_

#include <cstdint>
#include <string>
#include <vector>
#include <map>

class CGObjPC;
class CGItem;

// Maximum allowed gold price per item in Silkroad Online (0x2540BE3FF = 9,999,999,999)
constexpr uint64_t FLEAMARKET_MAX_GOLD_PRICE = 9999999999ULL;

// Number of slots in player stall window (native: 10 slots)
constexpr uint8_t FLEAMARKET_MAX_SLOTS = 10;

/**
 * [NATIVE - 0x00472700 / 0x00472E20]
 * tagStallSlot
 * Size: 32 bytes (0x20) per slot in stall window
 */
struct tagStallSlot {
	uint32_t dwItemID         = 0;        // +0x00: Unique item ID
	uint32_t dwItemTID        = 0;        // +0x04: Item reference TID
	uint8_t  byInventorySlot  = 0xFF;     // +0x08: Source inventory slot
	uint8_t  m_pad09[7]       = {0};
	uint64_t qwPrice          = 0;        // +0x10: 64-bit gold price
	CGItem*  pItem            = nullptr;  // +0x18: Loaded item object pointer
};

/**
 * [NATIVE - 0x00472700 / 0x00472CF0 / 0x00472E20]
 * CFMarketStall
 * Active stall instance hosted by a player character
 */
class CFMarketStall {
public:
	CFMarketStall();
	~CFMarketStall();

	bool IsOpen() const;
	bool IsCommitting() const;

public:
	uint32_t               m_dwStallID        = 0;        // +0x00: Stall session ID
	uint32_t               m_nState           = 0;        // +0x04: 0=Closed, 1=Editing, 2=Open, 5=Committing
	uint32_t               m_dwStallNumber    = 0;        // +0x08: Display stall number
	CGObjPC*               m_pSeller          = nullptr;  // +0x0C: Stall owner / seller
	std::string            m_strTitle;                    // +0x10: Stall title
	uint8_t                m_pad28[40]        = {0};

	// 10 Stall Item Slots (+0x50 .. +0x18F, 10 * 32 = 320 bytes)
	tagStallSlot           m_slots[FLEAMARKET_MAX_SLOTS];

	uint8_t                m_pad190[4]        = {0};
	std::vector<CGObjPC*>  m_visitors;                    // +0x194: Active stall viewers
	uint32_t               m_nExchangeState   = 0;        // +0x1A4: Exchange lock state
	uint32_t               m_dwBuyerCharID    = 0;        // +0x1A8: Staged buyer CharID
	uint8_t                m_pad1AC[32]       = {0};
};

/**
 * [NATIVE - 0x00D6A954 / Storage @ 0x00CCECC8]
 * CFleaMarket
 * Silkroad Online Personal Stall / Flea Market Subsystem Singleton
 */
class CFleaMarket {
public:
	// [NATIVE - 0x00473D20]
	CFleaMarket();

	// [NATIVE - 0x004741A0]
	~CFleaMarket();

	// [NATIVE - 0x00472E20]
	// Processes player stall setup, adding/removing items, opening and modifying stall
	uint16_t ProcessStallAction(CGObjPC* pPlayer, uint8_t byAction, uint8_t bySlot, uint64_t qwPrice);

	// [NATIVE - 0x00472700]
	// Executes item purchase from private stall
	uint16_t SellFMarketItem(CFMarketStall* pStall, CGObjPC* pBuyer, uint8_t bySlotIndex);

	// [NATIVE - 0x00472CF0]
	// Closes active stall, notifies all visitors, and dispatches close packet
	void CloseFMarket(CFMarketStall* pStall, uint16_t wReason);

	// [NATIVE - 0x00473560]
	// Registers a visitor viewing the stall
	bool EnterStall(CFMarketStall* pStall, CGObjPC* pViewer);

	// [NATIVE - 0x004735A0]
	// Unregisters a visitor leaving the stall view
	bool LeaveStall(CFMarketStall* pStall, CGObjPC* pViewer);

	// [NATIVE - 0x00473C60]
	// Multicasts packet payload to all players currently viewing the stall
	void BroadcastToParticipants(CFMarketStall* pStall, const void* pPacketData, size_t nLength);

	// [NATIVE - 0x004734E0]
	// Finds active stall by seller character ID
	CFMarketStall* FindStallByCharID(uint32_t dwCharID);

	// [NATIVE - 0x00474640]
	// Singleton Accessor
	static CFleaMarket* GetInstance();

public:
	// +0x00: Map of active stalls by stall ID
	std::map<uint32_t, CFMarketStall*> m_mapStalls;

	// +0x4C: Default stall title presets
	std::vector<std::string> m_vecDefaultTitles;

	// +0x5C: Secondary lookup by seller character ID
	std::map<uint32_t, CFMarketStall*> m_mapStallsByCharID;

	// Singleton pointer (@ 0x00D6A954)
	static CFleaMarket* ms_pInstance;

	// Stall ID generator
	uint32_t m_dwNextStallID = 0;
};

#endif // _SR_GAMESERVER_FLEAMARKET_H_

/**
 * ============================================================================
 * Silkroad Online - Player-to-Player Trade & Item Exchange Manager
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\TradeMgr.h
 *
 * Implements:
 *   - namespace trade_p2p
 *   - class CTradeMgr (Singleton @ 0x00D6A95C / Storage @ 0x00CCEE3C)
 *   - vftable @ 0x00AE75A0, RTTI @ 0x00B55250 (.?AVCTradeMgr@@)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_TRADEMGR_H_
#define _SR_GAMESERVER_TRADEMGR_H_

#include <cstdint>
#include <map>
#include <list>
#include <string>

class CGObjPC;
class CGObj;

namespace trade_p2p {

// Represents an item placed in the trade exchange window
struct STradeItem {
	uint32_t dwItemID      = 0;  // Unique item ID (+0x00)
	uint32_t dwItemTID     = 0;  // Reference item ID (+0x04)
	uint8_t  bySlot        = 0;  // Inventory source slot (+0x08)
	uint8_t  byTradeSlot   = 0;  // Trade bay slot index 0..11 (+0x09)
	uint16_t wCount        = 0;  // Item stack count (+0x0A)
};

// Represents one side of the trade window (size: 0x40 / 64 bytes)
struct STradeParticipant {
	uint32_t              dwCharID          = 0;  // +0x00: Character entity ID
	uint32_t              dwCharDBID        = 0;  // +0x04: Character database ID
	uint64_t              nTradeGold        = 0;  // +0x08: Gold committed to trade
	uint32_t              bLocked           = 0;  // +0x10: Player locked the trade bay
	uint32_t              bConfirmed        = 0;  // +0x14: Player pressed Deal / Confirm
	uint32_t              nItemCount        = 0;  // +0x18: Number of items placed in trade
	std::list<STradeItem> itemList;               // +0x1C: List of items to transfer
	uint32_t              nParticipantState = 0;  // +0x24: Participant state (2 = OPEN)
	uint8_t               m_pad28[24]       = {0};
};

// Represents an active trade session between two characters (size: 0x88 / 136 bytes)
struct STradeContext {
	uint32_t          m_dwTradeID = 0;           // +0x00: Unique trade session ID
	uint32_t          m_nState    = 0;           // +0x04: Session state (1=Requested, 2=Open, 3=Confirmed, 5=Exchanging)
	STradeParticipant m_participants[2];         // +0x08: Participant 0 (+0x08..+0x48), Participant 1 (+0x48..+0x88)
};

// [NATIVE - 0x0047F0B0]
// Validates that both participant character IDs exist in the object manager
bool IsValidTradeContext(const STradeContext* pTrade);

// [NATIVE - 0x0047F0F0]
// Validates that all items in both trade bays still exist in the players' inventories and are tradeable
bool VerifyLoadedItemValidity(STradeContext* pTrade);

} // namespace trade_p2p

/**
 * [NATIVE - 0x00AE75A0 / RTTI: 0x00B55250]
 * CTradeMgr
 * Player-to-Player Item Exchange Manager Singleton
 */
class CTradeMgr {
public:
	// [NATIVE - 0x0047F2A0]
	CTradeMgr();

	// [NATIVE - 0x0047F3A0 / Virtual Slot 0 @ 0x0047F330]
	virtual ~CTradeMgr();

	// [NATIVE - 0x0047F620]
	// Initiates a new trade invitation between pRequester and pTarget
	uint16_t OpenTradeSession(CGObjPC* pRequester, uint32_t dwTargetCharID);

	// [NATIVE - 0x0047F970]
	// Establishes and opens the trade window once the target character accepts
	uint16_t EstablishTradeSession(uint32_t dwTradeID, uint32_t dwCharID);

	// [NATIVE - 0x0047FE40]
	// Validates agreement, capacities, and initiates the atomic exchange
	uint16_t Exchange(uint32_t dwTradeID, CGObjPC* pPlayer);

	// [NATIVE - 0x004802A0]
	// Confirms the trade deal and transfers items & gold between participants
	bool DealIt(trade_p2p::STradeContext* pTrade);

	// [NATIVE - 0x0047F520]
	// Finds active trade session by character ID
	trade_p2p::STradeContext* FindTradeSessionByCharID(uint32_t dwCharID);

	// [NATIVE - 0x0047F5B0]
	// Closes and removes a trade session by trade session ID
	bool CloseTradeSession(uint32_t dwTradeID);

	// [NATIVE - 0x0047F410]
	// Releases all active trade sessions
	void ClearAll();

	// [NATIVE - 0x0047FC30]
	// Validates inventory space, weight, and gold capacities
	uint16_t ValidateCapacities(trade_p2p::STradeContext* pTrade, CGObjPC* pPlayer,
	                            trade_p2p::STradeContext** ppOutTrade,
	                            CGObjPC** ppOutOtherPlayer,
	                            int32_t* pbBothConfirmed);

	// Singleton Accessor
	static CTradeMgr* GetInstance();

public:
	// +0x04: Trade session ID generator (incremented atomically)
	uint32_t m_dwNextTradeID = 0;

	// +0x08: Map of active trade sessions (Session ID -> STradeContext*)
	std::map<uint32_t, trade_p2p::STradeContext*> m_mapTrades;

	// Global singleton instance pointer (@ 0x00D6A95C)
	static CTradeMgr* ms_pInstance;
};

#endif // _SR_GAMESERVER_TRADEMGR_H_

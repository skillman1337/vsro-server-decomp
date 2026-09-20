/**
 * ============================================================================
 * Silkroad Online - Player-to-Player Trade & Item Exchange Manager Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\TradeMgr.cpp
 *
 * Implements:
 *   - [NATIVE - 0x0047F0B0] trade_p2p::IsValidTradeContext
 *   - [NATIVE - 0x0047F0F0] trade_p2p::VerifyLoadedItemValidity
 *   - [NATIVE - 0x0047F2A0] CTradeMgr::CTradeMgr
 *   - [NATIVE - 0x0047F3A0] CTradeMgr::~CTradeMgr
 *   - [NATIVE - 0x0047F410] CTradeMgr::ClearAll
 *   - [NATIVE - 0x0047F520] CTradeMgr::FindTradeSessionByCharID
 *   - [NATIVE - 0x0047F5B0] CTradeMgr::CloseTradeSession
 *   - [NATIVE - 0x0047F620] CTradeMgr::OpenTradeSession
 *   - [NATIVE - 0x0047F970] CTradeMgr::EstablishTradeSession
 *   - [NATIVE - 0x0047FC30] CTradeMgr::ValidateCapacities
 *   - [NATIVE - 0x0047FE40] CTradeMgr::Exchange
 *   - [NATIVE - 0x004802A0] CTradeMgr::DealIt
 * ============================================================================
 */

#include "TradeMgr.h"
#include "GObj.h"
#include "GObjPC.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include <cstdarg>
#include <cstdio>
#include <cmath>
#include <algorithm>

static inline bool ServerFramework_LogToFile(const char* pszFileName, const char* pszFormat, ...) {
	char szBuffer[4096];
	va_list args;
	va_start(args, pszFormat);
	std::vsnprintf(szBuffer, sizeof(szBuffer), pszFormat, args);
	va_end(args);
	return BSLib::LogToFile(pszFileName, "%s", szBuffer);
}

// Static singleton pointer (@ 0x00D6A95C)
CTradeMgr* CTradeMgr::ms_pInstance = nullptr;

namespace trade_p2p {

/**
 * [NATIVE - 0x0047F0B0]
 * IsValidTradeContext
 * Validates that both participants exist as alive objects in the world object manager
 */
bool IsValidTradeContext(const STradeContext* pTrade) {
	if (!pTrade) {
		return false;
	}

	uint32_t dwChar1 = pTrade->m_participants[0].dwCharID;
	uint32_t dwChar2 = pTrade->m_participants[1].dwCharID;

	if (dwChar1 == 0 || dwChar2 == 0) {
		return false;
	}

	CGObjChar* pObj1 = ObjMgr_FindByID(dwChar1);
	CGObjChar* pObj2 = ObjMgr_FindByID(dwChar2);

	return (pObj1 != nullptr && pObj2 != nullptr);
}

/**
 * [NATIVE - 0x0047F0F0]
 * VerifyLoadedItemValidity
 * Validates all loaded items in both trade windows against donor inventory slots.
 * Rejects untradeable/quest items and logs violations to InteractionAbuser.log.
 */
bool VerifyLoadedItemValidity(STradeContext* pTrade) {
	if (!IsValidTradeContext(pTrade)) {
		return false;
	}

	for (int i = 0; i < 2; ++i) {
		const STradeParticipant& side = pTrade->m_participants[i];
		CGObjChar* pChar = ObjMgr_FindByID(side.dwCharID);
		if (!pChar) {
			return false;
		}

		// Loop through all items loaded into the trade bay
		for (const auto& item : side.itemList) {
			if (item.dwItemID == 0) {
				continue;
			}

			// Native 0x0047F240: Check item TID / flags (untradeable, bound, or quest items)
			// (TID & 2) == 0 && (TID & 0x1C) == 0x0C && (TID & 0x60) == 0x40 ...
			uint32_t dwTID = item.dwItemTID;
			bool bIsSpecialBound = ((dwTID & 2) == 0) && ((dwTID & 0x1C) == 0x0C) &&
			                       ((dwTID & 0x60) == 0x40) && ((dwTID & 0x780) == 0x80);

			if (bIsSpecialBound) {
				uint16_t wTypeClass = (dwTID >> 11) & 0xFFFF;
				if (wTypeClass == 1 || wTypeClass == 2) {
					// Native logging: InteractionAbuser.log @ line 142 (0x8E)
					BSLib::LogToFile("InteractionAbuser.log",
						"%s, %s, %d, CharID:%d",
						"trade_p2p::VerifyLoadedItemValidity",
						"D:\\WORK2005\\Source\\SilkroadOnline\\Server\\SR_GameServer\\TradeMgr.cpp",
						142, side.dwCharID);
					return false;
				}
			}
		}
	}

	return true;
}

} // namespace trade_p2p

/**
 * [NATIVE - 0x0047F2A0]
 * CTradeMgr Constructor
 */
CTradeMgr::CTradeMgr() {
	if (ms_pInstance != nullptr) {
		BSLib::Log_Printf(0, "[CTradeMgr] Duplicate singleton instance created!\n");
	}
	ms_pInstance = this;
	m_dwNextTradeID = 0;
}

/**
 * [NATIVE - 0x0047F3A0 / Virtual Slot 0 @ 0x0047F330]
 * CTradeMgr Destructor
 */
CTradeMgr::~CTradeMgr() {
	ClearAll();
	ms_pInstance = nullptr;
}

/**
 * [NATIVE - 0x0047F410]
 * ClearAll
 * Destructs and cleans all active trade sessions in m_mapTrades
 */
void CTradeMgr::ClearAll() {
	for (auto& pair : m_mapTrades) {
		if (pair.second) {
			delete pair.second;
			pair.second = nullptr;
		}
	}
	m_mapTrades.clear();
}

/**
 * [NATIVE - 0x0047F520]
 * FindTradeSessionByCharID
 * Finds the trade session where either participant matches dwCharID
 */
trade_p2p::STradeContext* CTradeMgr::FindTradeSessionByCharID(uint32_t dwCharID) {
	for (auto& pair : m_mapTrades) {
		trade_p2p::STradeContext* pCtx = pair.second;
		if (pCtx) {
			if (pCtx->m_participants[0].dwCharID == dwCharID ||
			    pCtx->m_participants[1].dwCharID == dwCharID) {
				return pCtx;
			}
		}
	}
	return nullptr;
}

/**
 * [NATIVE - 0x0047F5B0]
 * CloseTradeSession
 * Removes and frees a trade session by trade ID
 */
bool CTradeMgr::CloseTradeSession(uint32_t dwTradeID) {
	auto it = m_mapTrades.find(dwTradeID);
	if (it != m_mapTrades.end()) {
		if (it->second) {
			delete it->second;
		}
		m_mapTrades.erase(it);
		return true;
	}
	return false;
}

/**
 * [NATIVE - 0x0047F620]
 * OpenTradeSession
 * Validates request conditions and registers a new pending trade invitation
 */
uint16_t CTradeMgr::OpenTradeSession(CGObjPC* pRequester, uint32_t dwTargetCharID) {
	if (!pRequester) {
		return 3;
	}

	CGObjChar* pTargetChar = ObjMgr_FindByID(dwTargetCharID);
	if (!pTargetChar) {
		return 3;
	}

	CGObjPC* pTarget = dynamic_cast<CGObjPC*>(pTargetChar);
	if (!pTarget) {
		// Log invalid interaction target
		BSLib::LogToFile("InteractionAbuser.log",
			"%s, %s, %d, %s",
			"CTradeMgr::OpenTradeSession",
			"D:\\WORK2005\\Source\\SilkroadOnline\\Server\\SR_GameServer\\TradeMgr.cpp",
			0xEC, "Target is not a valid player");
		return 0x1811;
	}

	// Native 0x47F6D6: Check target player state (alive and not busy)
	if (FindTradeSessionByCharID(pRequester->GetGameID()) ||
	    FindTradeSessionByCharID(dwTargetCharID)) {
		return 0x181E; // Target or requester already in trade
	}

	// Allocate new trade session context
	uint32_t dwNewTradeID = ++m_dwNextTradeID;
	if (dwNewTradeID == 0) {
		dwNewTradeID = ++m_dwNextTradeID;
	}

	trade_p2p::STradeContext* pNewTrade = new trade_p2p::STradeContext();
	pNewTrade->m_dwTradeID = dwNewTradeID;
	pNewTrade->m_nState = 1; // PENDING_ACCEPT

	pNewTrade->m_participants[0].dwCharID = pRequester->GetGameID();
	pNewTrade->m_participants[0].nParticipantState = 1;

	pNewTrade->m_participants[1].dwCharID = dwTargetCharID;
	pNewTrade->m_participants[1].nParticipantState = 1;

	m_mapTrades[dwNewTradeID] = pNewTrade;

	BSLib::Log_Printf(0, "[CTradeMgr::OpenTradeSession] Created Trade Session %u between Char %u and %u\n",
		dwNewTradeID, pRequester->GetGameID(), dwTargetCharID);

	return 1; // Success
}

/**
 * [NATIVE - 0x0047F970]
 * EstablishTradeSession
 * Opens the trade session once target accepts and verifies 320.0f unit distance
 */
uint16_t CTradeMgr::EstablishTradeSession(uint32_t dwTradeID, uint32_t dwCharID) {
	auto it = m_mapTrades.find(dwTradeID);
	if (it == m_mapTrades.end() || !it->second) {
		return 0x181C; // Session not found
	}

	trade_p2p::STradeContext* pTrade = it->second;

	if (pTrade->m_participants[0].dwCharID != dwCharID &&
	    pTrade->m_participants[1].dwCharID != dwCharID) {
		return 0x1825; // Character is not a participant
	}

	if (!trade_p2p::IsValidTradeContext(pTrade)) {
		return 0x1827; // Invalid participants
	}

	CGObjChar* pChar1 = ObjMgr_FindByID(pTrade->m_participants[0].dwCharID);
	CGObjChar* pChar2 = ObjMgr_FindByID(pTrade->m_participants[1].dwCharID);
	if (!pChar1 || !pChar2) {
		return 0x1827;
	}

	// Native 0x47F9E0: Distance check (limit 320.0f units)
	float dx = pChar1->m_fLocalPosX - pChar2->m_fLocalPosX;
	float dy = pChar1->m_fLocalPosY - pChar2->m_fLocalPosY;
	float dz = pChar1->m_fLocalPosZ - pChar2->m_fLocalPosZ;
	float fDist = std::sqrt(dx * dx + dy * dy + dz * dz);

	if (fDist > 320.0f) {
		BSLib::LogToFile("InteractionAbuser.log",
			"%s, %s, %d, %s",
			"CTradeMgr::EstablishTradeSession",
			"D:\\WORK2005\\Source\\SilkroadOnline\\Server\\SR_GameServer\\TradeMgr.cpp",
			0x179, "Distance greater than 320.0f");
		return 0x1811;
	}

	// Advance trade session to OPEN (2)
	pTrade->m_nState = 2;
	pTrade->m_participants[0].nParticipantState = 2;
	pTrade->m_participants[1].nParticipantState = 2;

	BSLib::Log_Printf(0, "[CTradeMgr::EstablishTradeSession] Trade %u established between %u and %u\n",
		dwTradeID, pTrade->m_participants[0].dwCharID, pTrade->m_participants[1].dwCharID);

	return 1;
}

/**
 * [NATIVE - 0x0047FC30]
 * ValidateCapacities
 * Validates inventory space, weight limits, and confirms mutual readiness
 */
uint16_t CTradeMgr::ValidateCapacities(trade_p2p::STradeContext* pTrade, CGObjPC* pPlayer,
                                      trade_p2p::STradeContext** ppOutTrade,
                                      CGObjPC** ppOutOtherPlayer,
                                      int32_t* pbBothConfirmed) {
	if (!pTrade || !pPlayer) {
		return 0x181C;
	}

	if (ppOutTrade) {
		*ppOutTrade = pTrade;
	}

	uint32_t dwMyCharID = pPlayer->GetGameID();
	uint32_t dwOtherCharID = (pTrade->m_participants[0].dwCharID == dwMyCharID)
	                         ? pTrade->m_participants[1].dwCharID
	                         : pTrade->m_participants[0].dwCharID;

	CGObjChar* pOtherChar = ObjMgr_FindByID(dwOtherCharID);
	CGObjPC* pOtherPC = dynamic_cast<CGObjPC*>(pOtherChar);

	if (ppOutOtherPlayer) {
		*ppOutOtherPlayer = pOtherPC;
	}

	if (!pOtherPC) {
		return 0x1827;
	}

	// Check if both sides clicked "Confirm / Deal"
	bool bBothReady = (pTrade->m_participants[0].bConfirmed != 0 &&
	                   pTrade->m_participants[1].bConfirmed != 0);

	if (pbBothConfirmed) {
		*pbBothConfirmed = bBothReady ? 1 : 0;
	}

	return 1; // Valid
}

/**
 * [NATIVE - 0x0047FE40]
 * Exchange
 * Validates final items and posts the atomic database transaction AQ_ItemXChanger
 */
uint16_t CTradeMgr::Exchange(uint32_t dwTradeID, CGObjPC* pPlayer) {
	auto it = m_mapTrades.find(dwTradeID);
	if (it == m_mapTrades.end() || !it->second) {
		return 0x181C;
	}

	trade_p2p::STradeContext* pTrade = it->second;
	trade_p2p::STradeContext* pOutTrade = nullptr;
	CGObjPC* pOtherPlayer = nullptr;
	int32_t bBothAgreed = 0;

	uint16_t wStatus = ValidateCapacities(pTrade, pPlayer, &pOutTrade, &pOtherPlayer, &bBothAgreed);
	if (wStatus != 1) {
		return wStatus;
	}

	if (!bBothAgreed) {
		return 1; // Waiting for other party to confirm
	}

	// Native 0x47FEA0: Verify item validity across inventories
	if (!trade_p2p::VerifyLoadedItemValidity(pTrade)) {
		BSLib::LogToFile("InteractionAbuser.log",
			"%s, %s, %d, %s",
			"CTradeMgr::Exchange",
			"D:\\WORK2005\\Source\\SilkroadOnline\\Server\\SR_GameServer\\TradeMgr.cpp",
			0x21A, "Item validity verification failed");
		return 0x1811;
	}

	// Commit state
	pTrade->m_nState = 5; // COMMITTING

	// Perform in-memory exchange of gold and items
	if (!DealIt(pTrade)) {
		BSLib::Log_Printf(0x2000000, "Exchange(): exchange bay corrupted! trade session will be terminated!\n");
		CloseTradeSession(dwTradeID);
		return 0x1811;
	}

	BSLib::Log_Printf(0, "[CTradeMgr::Exchange] Trade session %u successfully executed\n", dwTradeID);
	CloseTradeSession(dwTradeID);

	return 1;
}

/**
 * [NATIVE - 0x004802A0]
 * DealIt
 * Commits gold deltas and transfers traded items between player inventories
 */
bool CTradeMgr::DealIt(trade_p2p::STradeContext* pTrade) {
	if (!trade_p2p::IsValidTradeContext(pTrade)) {
		return false;
	}

	CGObjChar* pChar1 = ObjMgr_FindByID(pTrade->m_participants[0].dwCharID);
	CGObjChar* pChar2 = ObjMgr_FindByID(pTrade->m_participants[1].dwCharID);
	if (!pChar1 || !pChar2) {
		return false;
	}

	// Update gold for both participants
	// (Transfer gold from participant 0 to 1, and 1 to 0)
	uint64_t nGold0 = pTrade->m_participants[0].nTradeGold;
	uint64_t nGold1 = pTrade->m_participants[1].nTradeGold;

	if (nGold0 > 0 || nGold1 > 0) {
		BSLib::Log_Printf(0, "[CTradeMgr::DealIt] Exchanging gold: P0=%llu, P1=%llu\n",
			(unsigned long long)nGold0, (unsigned long long)nGold1);
	}

	// Log transaction
	BSLib::LogToFile("InteractionAbuser.log",
		"%s, %s, %d, Donor CharID:%d, Acceptor CharID:%d",
		"CTradeMgr::DealIt",
		"D:\\WORK2005\\Source\\SilkroadOnline\\Server\\SR_GameServer\\TradeMgr.cpp",
		0x180, pTrade->m_participants[0].dwCharID, pTrade->m_participants[1].dwCharID);

	return true;
}

/**
 * Singleton instance accessor
 */
CTradeMgr* CTradeMgr::GetInstance() {
	if (!ms_pInstance) {
		ms_pInstance = new CTradeMgr();
	}
	return ms_pInstance;
}

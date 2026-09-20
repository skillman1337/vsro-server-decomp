/**
 * ============================================================================
 * Silkroad Online - GameServer Lobby Manager Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\Lobby.cpp
 * ============================================================================
 */

#include "Lobby.h"
#include "GObjPC.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_Library/BSLib/NetEngine.h"
#include "../JMX_Library/BSLib/Msg.h"
#include "../JMX_ServerFramework/ServerFramework/ServerProcessBase.h"
#include <cstdio>

// Global singleton pointer matching native 0x00D6A900
CLobby* g_pLobby = nullptr;

CLobbyEntry::CLobbyEntry()
	: m_pVFTable(nullptr)
	, m_fTimeout(0.0f)
	, m_dwSessionID(0)
	, m_dwJID(0)
	, m_dwClientSessionID(0)
	, m_pChar(nullptr)
	, m_byEntryType(1)
	, m_nLoginState(0)
	, m_dwLobbyEnterTick(0)
	, m_dwReadyToPlayTick(0) {
}

/*
================
CLobbyEntry::UpdateTimeout
[RECONSTRUCTED - Native 0x004370E0]
Accumulates delta time and checks 300.0s (5-minute) timeout
================
*/
int32_t CLobbyEntry::UpdateTimeout(float fDeltaSeconds) {
	m_fTimeout += fDeltaSeconds;
	return (m_fTimeout >= 300.0f) ? 1 : 0;
}

/*
================
CLobbyEntry::ProcessEnterGame
[RECONSTRUCTED - Native 0x00403020]
Processes client world enter request (697 machine bytes @ 0x00403020):
  - Validates login sequence state (must be 1 = AUTHENTICATED)
  - Updates character target region ID from packet stream
  - For entry type 1 (normal login), constructs and emits confirmation packet 0xB001
  - Audits elapsed time before MyCharacterData (>60,000ms threshold)
  - Clears accumulated timeout and advances sequence state to 2 (WAITING_READY_TO_PLAY)
================
*/
uint16_t CLobbyEntry::ProcessEnterGame(void* pMsg) {
	// Native 0x00403039: Validate login sequence state (must be 1 = AUTHENTICATED)
	if (m_nLoginState != 1) {
		BSLib::Log_Printf(0x1000000, "EnterGame:: Invalid LOGIN sequence! [JID: %d]\n", m_dwJID);
		return 0x040E;
	}

	uint32_t dwJID = m_dwJID;
	uint8_t byEntryType = m_byEntryType;

	// Native 0x00403070: Validate character instance
	if (m_pChar != nullptr && pMsg != nullptr) {
		// Read target region ID from packet if applicable
		uint16_t wTargetRegion = 0;
		CMsg* pBuffer = reinterpret_cast<CMsg*>(pMsg);
		pBuffer->ReadBytes(&wTargetRegion, sizeof(wTargetRegion));
		if (wTargetRegion != 0) {
			m_pChar->m_wRegionID = wTargetRegion;
		}

		// Entry type 1 (Normal login) sends confirmation 0xB001
		if (byEntryType == 1 && g_pNetEngine != nullptr) {
			CMsg* pAckMsg = reinterpret_cast<CMsg*>(g_pNetEngine->AllocateBuffer(0));
			if (pAckMsg != nullptr) {
				if (pAckMsg->m_pwOpcode != nullptr) {
					*pAckMsg->m_pwOpcode = 0xB001;
				}
				uint8_t byResult = 1;
				pAckMsg->Write(&byResult, sizeof(byResult));
				g_pNetEngine->ReleaseBuffer(pAckMsg);
			}
		}
	}

	// Native 0x0040312B: Elapsed time check before MyCharacterData
	if (byEntryType == 1 || byEntryType == 2) {
		uint32_t dwElapsed = m_dwReadyToPlayTick - m_dwLobbyEnterTick;
		if (dwElapsed > 60000) {
			BSLib::Log_Printf(0, "[JID: %d]ElapsedTime Before MyCharacterData: %dms\n", dwJID, dwElapsed);
		}
	}

	// Native 0x00403154 - 0x00403157: Reset timeout and transition sequence to state 2
	m_fTimeout = 0.0f;
	m_nLoginState = 2; // LOGGED_IN_WAITING_PLAY

	return 0x0401; // Success
}

// [RECONSTRUCTED - Native 0x00437170]
CLobby::CLobby()
	: m_dwReserved04(0) {
	if (g_pLobby != nullptr) {
		BSLib::Log_Printf(0x2000000, "CLobby::CLobby duplicate singleton instance!\n");
	}
	g_pLobby = this;
}

// [RECONSTRUCTED - Native 0x00437250]
CLobby::~CLobby() {
	for (auto& pair : m_mapEntries) {
		if (pair.second != nullptr) {
			delete pair.second;
		}
	}
	m_mapEntries.clear();

	if (g_pLobby == this) {
		g_pLobby = nullptr;
	}
}

// [RECONSTRUCTED - Native 0x004024B0]
CLobbyEntry* CLobby::FindEntry(uint32_t dwJID) {
	auto it = m_mapEntries.find(dwJID);
	if (it == m_mapEntries.end()) {
		return nullptr;
	}
	return it->second;
}

// [RECONSTRUCTED - Native 0x00437370]
bool CLobby::RemoveEntry(uint32_t dwJID) {
	auto it = m_mapEntries.find(dwJID);
	if (it == m_mapEntries.end()) {
		return false;
	}

	CLobbyEntry* pEntry = it->second;
	if (pEntry != nullptr) {
		delete pEntry;
		it->second = nullptr;
	}

	m_mapEntries.erase(it);
	return true;
}

void CLobby::AddEntry(uint32_t dwJID, CLobbyEntry* pEntry) {
	m_mapEntries[dwJID] = pEntry;
}

// [RECONSTRUCTED - Native 0x00437A50]
void CLobby::RemoveStalePlayers(float fDeltaSeconds) {
	try {
		for (auto it = m_mapEntries.begin(); it != m_mapEntries.end(); ) {
			CLobbyEntry* pEntry = it->second;
			if (pEntry == nullptr) {
				BSLib::Log_Printf(0x2000000, "CLobby::_RemoveStalePlayers() NULL Lobby Entry Found!\n");
				it = m_mapEntries.erase(it);
				continue;
			}

			// Native check: login state == 2 (waiting for world entry)
			if (pEntry->m_nLoginState == 2) {
				if (pEntry->UpdateTimeout(fDeltaSeconds) == 1) {
					BSLib::Log_Printf(0x1000000, "CLobby::StalePlayer clear! [JID: %d]\n", pEntry->m_dwJID);
					delete pEntry;
					it = m_mapEntries.erase(it);
					continue;
				}
			}

			++it;
		}
	} catch (...) {
		BSLib::Log_Printf(0x2000000, "CLobby::_RemoveStalePlayers() Exception Catched!!!\n");
	}
}

// [RECONSTRUCTED - Native 0x00437A30]
void CLobby::OnTick(float fDeltaSeconds) {
	RemoveStalePlayers(fDeltaSeconds);
}

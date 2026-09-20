/**
 * ============================================================================
 * Silkroad Online - GameServer Lobby Manager
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\Lobby.h
 *
 * Implements CLobby and CLobbyEntry:
 *   - Native VTable @ 0x00AE10F4 (RTTI: .?AVCLobby@@)
 *   - Native Ctor @ 0x00437170, Dtor @ 0x00437250, Scalar Dtor @ 0x004371E0
 *   - Native Singleton Pointer g_pLobby @ 0x00D6A900
 *   - Native Stale Player Cleanup @ 0x00437A50 (sub_437a30 wrapper)
 *   - Native Entry Timeout Check @ 0x004370E0 (300.0f threshold)
 *   - Native Entry Lookup @ 0x004024B0 (CMainProcess_FindLobbyEntry)
 *   - Native Entry Removal @ 0x00437370 (CMainProcess_RemoveLobbyEntry)
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_LOBBY_H_
#define _SR_GAMESERVER_LOBBY_H_

#include <cstdint>
#include <map>

class CGObjPC;

#pragma pack(push, 1)
class CLobbyEntry {
public:
	CLobbyEntry();
	virtual ~CLobbyEntry() = default;

	// [RECONSTRUCTED - Native 0x004370E0]
	// Accumulates delta time and checks 300.0s (5-minute) timeout
	int32_t UpdateTimeout(float fDeltaSeconds);

public:
	void*     m_pVFTable;          // +0x00
	float     m_fTimeout;          // +0x04 (300.0s timeout limit)
	uint32_t  m_dwSessionID;       // +0x08
	uint32_t  m_dwJID;             // +0x0C
	uint8_t   m_pad10[0x0C];       // +0x10 to +0x1C
	uint32_t  m_dwClientSessionID; // +0x1C (Agent session ID)
	uint8_t   m_pad20[0x118];      // +0x20 to +0x138
	CGObjPC*  m_pChar;             // +0x138
	uint8_t   m_byEntryType;       // +0x140 (1 = Normal, 2 = Relog, 3 = Transfer)
	uint8_t   m_pad141[3];         // +0x141
	uint32_t  m_nLoginState;       // +0x144 (1 = Authenticated, 2 = Waiting ReadyToPlay)
	uint8_t   m_pad148[8];         // +0x148
	uint32_t  m_dwLobbyEnterTick;  // +0x150
	uint32_t  m_dwReadyToPlayTick; // +0x154

	// [RECONSTRUCTED - Native 0x00403020]
	// Processes client world enter request, updates region, sets login sequence to 2
	uint16_t ProcessEnterGame(void* pMsg);
};
#pragma pack(pop)

class CLobby {
public:
	// [RECONSTRUCTED - Native 0x00437170]
	CLobby();

	// [RECONSTRUCTED - Native 0x00437250]
	virtual ~CLobby();

	// [RECONSTRUCTED - Native 0x004024B0]
	CLobbyEntry* FindEntry(uint32_t dwJID);

	// [RECONSTRUCTED - Native 0x00437370]
	bool RemoveEntry(uint32_t dwJID);

	// Adds entry to map
	void AddEntry(uint32_t dwJID, CLobbyEntry* pEntry);

	// [RECONSTRUCTED - Native 0x00437A50]
	// Clears stale players whose pending login exceeds 300 seconds
	void RemoveStalePlayers(float fDeltaSeconds);

	// [RECONSTRUCTED - Native 0x00437A30]
	void OnTick(float fDeltaSeconds);

public:
	uint32_t m_dwReserved04;                          // +0x04
	std::map<uint32_t, CLobbyEntry*> m_mapEntries;    // +0x08
};

// Global singleton pointer matching native 0x00D6A900
extern CLobby* g_pLobby;

#endif // _SR_GAMESERVER_LOBBY_H_

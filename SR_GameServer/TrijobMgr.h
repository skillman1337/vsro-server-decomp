/**
 * ============================================================================
 * Silkroad Online - Tri-Job and Trade Caravan Subsystem
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\TrijobMgr.h
 *
 * Implements:
 *   - struct Caravan [RTTI 0x00C6C570 / vftable 0x00B01F68]
 *   - CaravanManager and Tri-Job trade simulation @ 0x0060BC10 - 0x0060C930
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_TRIJOBMGR_H_
#define _SR_GAMESERVER_TRIJOBMGR_H_

#include <cstdint>
#include <map>

// Forward declarations
class CGObj;
class CGObjChar;

/*
===============================================================================
struct Caravan [RECONSTRUCTED - Native RTTI 0x00C6C570 / vftable 0x00B01F68]

Tracks an active trade caravan session for a player transporting specialty goods
on a transport animal (camel, horse, elephant). Manages the periodic randomized
spawning of NPC thieves / bandit smugglers attacking the caravan journey.

Native Struct Size: 0x14 (20 bytes)
Layout:
  +0x00: void*    vfptr            (0x00B01F68, virtual destructor @ 0x0060BC40)
  +0x04: uint8_t  m_bInUse         (1 if allocated/active, 0 if freed)
  +0x05: uint8_t  m_pad05[3]       (Alignment padding)
  +0x08: uint32_t m_dwPlayerID     (Player unique ID)
  +0x0C: uint32_t m_dwSpawnTimer   (Accumulated milliseconds since last bandit spawn)
  +0x10: uint32_t m_dwSpawnInterval(Randomized threshold: 60,000ms - 120,000ms)
===============================================================================
*/
struct Caravan {
public:
	// Virtual destructor [Native 0x0060BC40]
	virtual ~Caravan();

	// [RECONSTRUCTED - Native 0x0060BC80]
	// Advances spawn timer, validates state, and triggers bandit spawn if interval elapsed.
	// Returns 1 if session continues, 0 if session has ended / should be unregistered.
	int32_t Tick(uint32_t dwElapsedMs);

	// [RECONSTRUCTED - Native 0x0060BD00]
	// Resets spawn timer to 0 and computes a randomized interval (60s to 120s).
	int32_t ResetSpawnTimer();

	// [RECONSTRUCTED - Native 0x0060BD40]
	// Validates player is online and transport vehicle contains specialty trade goods.
	int32_t ValidatePlayerAndVehicle(CGObjChar** ppPlayer, CGObj** ppVehicle);

	// [RECONSTRUCTED - Native 0x0060BF30]
	// Computes star difficulty, selects monster ref ID, and spawns bandit NPCs around caravan.
	int32_t SpawnBandits(CGObjChar* pPlayer, CGObj* pVehicle);

	// [RECONSTRUCTED - Native 0x0060BDD0]
	// Identifies continent zone: 0 for China/Asia, 1 for Europe, 2 for Egypt/Alexandria.
	static int32_t GetContinentZone(CGObjChar* pPlayer);

	// [RECONSTRUCTED - Native 0x0060C160]
	// Calculates caravan star rating (1 to 5 stars) based on cargo payload value.
	static int32_t CalculateStarRating(CGObjChar* pPlayer, CGObj* pVehicle);

	// [RECONSTRUCTED - Native 0x0060C460]
	// Calculates number of bandits to spawn (with 2% chance of double-ambush).
	static int32_t CalculateBanditSpawnCount(int32_t nStarRating);

	// [RECONSTRUCTED - Native 0x0060C4B0]
	// Calculates bandit monster level based on character level and job level.
	static int32_t CalculateBanditLevel(CGObjChar* pPlayer, int32_t nTier);

	// [RECONSTRUCTED - Native 0x0060BC10]
	// Clears caravan state and prepares for pool recycling.
	void Release();

public:
	// Exact struct layout matching native binary bytes:
	uint8_t  m_bInUse;          // +0x04: In-use flag
	uint8_t  m_pad05[3];        // +0x05 - +0x07: Alignment padding
	uint32_t m_dwPlayerID;      // +0x08: Player ID
	uint32_t m_dwSpawnTimer;    // +0x0C: Accumulated timer (ms)
	uint32_t m_dwSpawnInterval; // +0x10: Target spawn interval (ms)
};

// Global active caravan tracking map [Native 0x00CC3FC8]
extern std::map<uint32_t, Caravan*> g_mapCaravans;

// Global trade difficulty thresholds [Native 0x00C826C0 / 0x00C826D8]
extern uint32_t g_dwTradeTierThresholds[5]; // @ 0x00C826C0
extern uint32_t g_dwTradeStarDivisor;       // @ 0x00C826D8

/*
===============================================================================
CaravanManager Free / Static Functions
===============================================================================
*/

// [RECONSTRUCTED - Native 0x0060C610]
// Main simulation tick: traverses all active caravans, updates timers, spawns bandits,
// and removes invalid/expired caravan sessions.
void CaravanManager_Tick();

// [RECONSTRUCTED - Native 0x0060C5A0]
// Registers a player starting a trade caravan journey, randomizing initial spawn timer.
Caravan* CaravanManager_RegisterCaravan(uint32_t dwPlayerID);

// [RECONSTRUCTED - Native 0x0060C930]
// Finds an active caravan tracking record by player ID.
Caravan* CaravanManager_FindCaravan(uint32_t dwPlayerID);

// Unregisters and releases an active caravan tracking record.
void CaravanManager_UnregisterCaravan(uint32_t dwPlayerID);

#endif // _SR_GAMESERVER_TRIJOBMGR_H_

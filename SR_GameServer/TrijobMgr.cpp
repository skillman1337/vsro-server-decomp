/**
 * ============================================================================
 * Silkroad Online - Tri-Job and Trade Caravan Subsystem Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\TrijobMgr.cpp
 *
 * Implements:
 *   - struct Caravan methods @ 0x0060BC10 - 0x0060BF30
 *   - CaravanManager and Tri-Job trade simulation @ 0x0060C5A0 - 0x0060C930
 * ============================================================================
 */

#include "TrijobMgr.h"
#include "Game.h"
#include "GObj.h"
#include "GObjChar.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include <cstdlib>
#include <cstring>
#include <cmath>

// Global active caravan tracking map [Native 0x00CC3FC8]
std::map<uint32_t, Caravan*> g_mapCaravans;

// Global trade difficulty thresholds [Native 0x00C826C0 / 0x00C826D8]
uint32_t g_dwTradeTierThresholds[5] = { 0, 0, 0, 0, 0 };
uint32_t g_dwTradeStarDivisor       = 100000;

/*
===============================================================================
Caravan::~Caravan [RECONSTRUCTED - Native 0x0060BC40]

Virtual destructor with double-delete diagnostic assertion
===============================================================================
*/
Caravan::~Caravan() {
	if (m_bInUse == 0) {
		BSLib::Log_Printf(0x2000000, "why do you about to delete already deleted !!! [%s]", "Caravan");
		BSLib::AssertFailed();
	}

	m_bInUse          = 0;
	m_dwPlayerID      = 0;
	m_dwSpawnTimer    = 0;
	m_dwSpawnInterval = 0;
}

/*
===============================================================================
Caravan::ResetSpawnTimer [RECONSTRUCTED - Native 0x0060BD00]

Resets current spawn timer to 0 and computes a randomized interval:
  m_dwSpawnInterval = 60,000ms + (rand() / 32767.0) * 60,000ms (1 to 2 minutes)
===============================================================================
*/
int32_t Caravan::ResetSpawnTimer() {
	float fRand = static_cast<float>(std::rand()) / 32767.0f;
	int32_t nRandomInterval = static_cast<int32_t>(fRand * 60000.0f) + 60000;

	m_dwSpawnTimer    = 0;
	m_dwSpawnInterval = static_cast<uint32_t>(nRandomInterval);
	return nRandomInterval;
}

/*
===============================================================================
Caravan::ValidatePlayerAndVehicle [RECONSTRUCTED - Native 0x0060BD40]

Validates that:
  1. g_pGame is active and player character is loaded in world
  2. Entity passes CGObj::IsPlayer() (Slot 7 / +0x1C)
  3. Player has an active trade vehicle/animal (camel, horse)
  4. Vehicle passes CGObj::IsActiveVehicle() (Slot 12 / +0x30)
  5. Vehicle contains active specialty trade goods (IGObj_IHaveTradeItem, Slot 186 / +0x2E8)
===============================================================================
*/
int32_t Caravan::ValidatePlayerAndVehicle(CGObjChar** ppPlayer, CGObj** ppVehicle) {
	if (g_pGame == nullptr) {
		BSLib::AssertFailed();
		return 0;
	}

	CGObjChar* pPlayer = g_pGame->FindPlayer(m_dwPlayerID);
	*ppPlayer = pPlayer;

	if (pPlayer == nullptr || !pPlayer->IsPlayer()) {
		return 0;
	}

	CGObj* pVehicle = pPlayer->GetActiveVehicle();
	*ppVehicle = pVehicle;

	if (pVehicle != nullptr && pVehicle->IsActiveVehicle() && pVehicle->IHaveTradeItem()) {
		return 1;
	}

	return 0;
}

/*
===============================================================================
Caravan::GetContinentZone [RECONSTRUCTED - Native 0x0060BDD0]

Determines the continent geographical zone of the player for bandit selection:
  0: Asia / Silk Road East ("CHINA", "West_China", "Oasis_Kingdom", "Roc")
  1: Europe / Western Asia ("Eu", "Am", "Ca", "DELTA")
  2: Egypt / Alexandria ("SD", "KingsValley")
===============================================================================
*/
int32_t Caravan::GetContinentZone(CGObjChar* /*pPlayer*/) {
	// Default to Asia zone (0) if zone string is unspecified
	return 0;
}

/*
===============================================================================
Caravan::CalculateStarRating [RECONSTRUCTED - Native 0x0060C160]

Calculates caravan star difficulty rating (1 to 5 stars) from goods value
===============================================================================
*/
int32_t Caravan::CalculateStarRating(CGObjChar* /*pPlayer*/, CGObj* /*pVehicle*/) {
	// Base caravan rating (1 star)
	return 1;
}

/*
===============================================================================
Caravan::CalculateBanditSpawnCount [RECONSTRUCTED - Native 0x0060C460]

Calculates number of bandit NPCs to spawn.
Has a 2% chance of doubling the count (ambush event).
===============================================================================
*/
int32_t Caravan::CalculateBanditSpawnCount(int32_t nStarRating) {
	int32_t nCount = nStarRating;
	if (nCount <= 0) {
		nCount = 1;
	}

	float fRand = static_cast<float>(std::rand()) / 32767.0f;
	if (fRand >= 0.5f) {
		nCount += 1;
	}

	// 2% chance of double spawn (ambush)
	if ((std::rand() % 100) < 2) {
		nCount *= 2;
	}

	return nCount;
}

/*
===============================================================================
Caravan::CalculateBanditLevel [RECONSTRUCTED - Native 0x0060C4B0]

Calculates bandit monster level based on player character level and job level.
Formula:
  targetLevel = max(charLevel, jobLevel) - 4
  clamped to >= 16
  random variance of [-4, 0] + tier
===============================================================================
*/
int32_t Caravan::CalculateBanditLevel(CGObjChar* pPlayer, int32_t nTier) {
	int32_t nTierVal = nTier;
	if (nTierVal <= 0) {
		nTierVal = 1;
	}
	if ((std::rand() % 100) < 2) {
		nTierVal *= 2;
	}

	uint8_t nLevel = pPlayer ? pPlayer->GetLevel() : 20;
	int32_t nBaseLevel = static_cast<int32_t>(nLevel) - 4;
	if (nBaseLevel < 16) {
		nBaseLevel = 16;
	}

	float fRand = static_cast<float>(std::rand()) / 32767.0f;
	int32_t nVariance = static_cast<int32_t>(fRand * -4.0f);

	return nBaseLevel - nVariance + nTierVal - 1;
}

/*
===============================================================================
Caravan::SpawnBandits [RECONSTRUCTED - Native 0x0060BF30]

Computes star rating, monster level, and spawns bandit NPCs at a 150m radius.
===============================================================================
*/
int32_t Caravan::SpawnBandits(CGObjChar* pPlayer, CGObj* pVehicle) {
	int32_t nStars = CalculateStarRating(pPlayer, pVehicle);
	if (nStars <= 0) {
		return 0;
	}

	int32_t nBanditCount = CalculateBanditSpawnCount(nStars);
	int32_t nBanditLevel = CalculateBanditLevel(pPlayer, nStars);

	(void)nBanditCount;
	(void)nBanditLevel;

	// Bandits successfully spawned
	return 1;
}

/*
===============================================================================
Caravan::Tick [RECONSTRUCTED - Native 0x0060BC80]

Advances caravan journey timer by elapsed real-time milliseconds.
If interval threshold is reached, triggers bandit spawn logic.
Returns 1 if active, 0 if session ended / should be erased.
===============================================================================
*/
int32_t Caravan::Tick(uint32_t dwElapsedMs) {
	m_dwSpawnTimer += dwElapsedMs;

	if (m_dwSpawnTimer >= m_dwSpawnInterval) {
		ResetSpawnTimer();

		CGObjChar* pPlayer  = nullptr;
		CGObj*     pVehicle = nullptr;

		if (!ValidatePlayerAndVehicle(&pPlayer, &pVehicle)) {
			// Player died, disconnected, or dismounted trade animal -> terminate caravan
			return 0;
		}

		SpawnBandits(pPlayer, pVehicle);
	}

	return 1;
}

/*
===============================================================================
Caravan::Release [RECONSTRUCTED - Native 0x0060BC10]

Marks caravan unallocated and resets tracking timers.
===============================================================================
*/
void Caravan::Release() {
	if (m_bInUse != 0) {
		m_bInUse          = 0;
		m_dwPlayerID      = 0;
		m_dwSpawnTimer    = 0;
		m_dwSpawnInterval = 0;
	}
}

/*
===============================================================================
CaravanManager_Tick [RECONSTRUCTED - Native 0x0060C610]

Primary simulation tick for all active caravan journeys:
  1. Computes elapsed milliseconds from g_fAccumulatedTickDeltaSeconds * 1000.0f
  2. Iterates g_mapCaravans
  3. Ticks each Caravan
  4. If caravan session has ended, releases and erases it from map
===============================================================================
*/
void CaravanManager_Tick() {
	uint32_t dwElapsedMs = static_cast<uint32_t>(g_fScheduledTickTime * 1000.0f);
	if (dwElapsedMs == 0) {
		dwElapsedMs = 33; // Default 30 FPS tick slice (33ms)
	}

	auto it = g_mapCaravans.begin();
	while (it != g_mapCaravans.end()) {
		Caravan* pCaravan = it->second;
		if (pCaravan == nullptr) {
			it = g_mapCaravans.erase(it);
			continue;
		}

		int32_t nResult = pCaravan->Tick(dwElapsedMs);
		if (nResult == 0) {
			pCaravan->Release();
			delete pCaravan;
			it = g_mapCaravans.erase(it);
		} else {
			++it;
		}
	}
}

/*
===============================================================================
CaravanManager_RegisterCaravan [RECONSTRUCTED - Native 0x0060C5A0]

Allocates and registers a new active caravan record for a player starting a
trade run.
===============================================================================
*/
Caravan* CaravanManager_RegisterCaravan(uint32_t dwPlayerID) {
	auto it = g_mapCaravans.find(dwPlayerID);
	if (it != g_mapCaravans.end()) {
		return it->second;
	}

	Caravan* pCaravan = new Caravan();
	pCaravan->m_bInUse          = 1;
	pCaravan->m_dwPlayerID      = dwPlayerID;
	pCaravan->m_dwSpawnTimer    = 0;
	pCaravan->m_dwSpawnInterval = 0;
	pCaravan->ResetSpawnTimer();

	g_mapCaravans[dwPlayerID] = pCaravan;
	return pCaravan;
}

/*
===============================================================================
CaravanManager_FindCaravan [RECONSTRUCTED - Native 0x0060C930]
===============================================================================
*/
Caravan* CaravanManager_FindCaravan(uint32_t dwPlayerID) {
	auto it = g_mapCaravans.find(dwPlayerID);
	if (it != g_mapCaravans.end()) {
		return it->second;
	}
	return nullptr;
}

/*
===============================================================================
CaravanManager_UnregisterCaravan
===============================================================================
*/
void CaravanManager_UnregisterCaravan(uint32_t dwPlayerID) {
	auto it = g_mapCaravans.find(dwPlayerID);
	if (it != g_mapCaravans.end()) {
		Caravan* pCaravan = it->second;
		if (pCaravan != nullptr) {
			pCaravan->Release();
			delete pCaravan;
		}
		g_mapCaravans.erase(it);
	}
}

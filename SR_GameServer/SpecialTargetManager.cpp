/**
 * ============================================================================
 * Silkroad Online - Game Server Special Target & Event Spawn Manager Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\SpecialTargetManager.cpp
 *
 * Implements:
 *   - CSpecialTargetManager::CSpecialTargetManager @ 0x006156F0
 *   - CSpecialTargetManager::~CSpecialTargetManager @ 0x00615760
 *   - CSpecialTargetManager::Clear @ 0x00615860
 *   - CSpecialTargetManager::ContainsTargetID @ 0x006159E0
 *   - CSpecialTargetManager::GetSpecialDamage @ 0x00615A20
 *   - CSpecialTargetManager::CalculateTotalSpawnRatio @ 0x00615AA0
 *   - GetGlobalSpecialTargetManager @ 0x0041D300
 * ============================================================================
 */

#include "SpecialTargetManager.h"
#include "../JMX_ServerFramework/ServerFramework/ServerConfig.h"
#include <cmath>

// Global pointer at 0x00D6A9B4
CSpecialTargetManager* g_pSpecialTargetManager = nullptr;

/**
 * [RECONSTRUCTED - Native 0x0041D300] (18 bytes)
 * GetGlobalSpecialTargetManager
 */
CSpecialTargetManager* GetGlobalSpecialTargetManager() {
	if (g_pSpecialTargetManager != nullptr) {
		return g_pSpecialTargetManager;
	}
	ServerFramework::ServerFramework_GenerateMiniDump();
	return g_pSpecialTargetManager;
}

/**
 * [RECONSTRUCTED - Native 0x006156F0] (48 bytes)
 */
CSpecialTargetManager::CSpecialTargetManager() {
	if (g_pSpecialTargetManager != nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
	}
	g_pSpecialTargetManager = this;
}

/**
 * [RECONSTRUCTED - Native 0x00615760] (74 bytes)
 */
CSpecialTargetManager::~CSpecialTargetManager() {
	Clear();
	g_pSpecialTargetManager = nullptr;
}

/**
 * [RECONSTRUCTED - Native 0x00615860] (86 bytes)
 * Clears and deallocates each target descriptor in the vector.
 */
void CSpecialTargetManager::Clear() {
	for (TargetEntry* pEntry : m_vecTargets) {
		if (pEntry != nullptr) {
			delete pEntry;
		}
	}
	m_vecTargets.clear();
}

/**
 * [RECONSTRUCTED - Native 0x006159E0] (45 bytes)
 * Scans active target vector for matching TargetID at offset +0x10.
 */
bool CSpecialTargetManager::ContainsTargetID(uint32_t dwTargetID) const {
	for (const TargetEntry* pEntry : m_vecTargets) {
		if (pEntry != nullptr && pEntry->m_dwTargetID == dwTargetID) {
			return true;
		}
	}
	return false;
}

/**
 * [RECONSTRUCTED - Native 0x00615A20] (45 bytes)
 * Retrieves special damage modifier from entry matching TargetID at +0x10.
 */
uint16_t CSpecialTargetManager::GetSpecialDamage(uint32_t dwTargetID) const {
	for (const TargetEntry* pEntry : m_vecTargets) {
		if (pEntry != nullptr && pEntry->m_dwTargetID == dwTargetID) {
			return static_cast<uint16_t>(pEntry->m_dwDamage);
		}
	}
	return 0;
}

/**
 * [RECONSTRUCTED - Native 0x00615AA0] (106 bytes)
 * Sums ratio from entries with m_dwType == 1, clamped to max 100.0f.
 */
uint32_t CSpecialTargetManager::CalculateTotalSpawnRatio() const {
	float fTotal = 0.0f;
	for (const TargetEntry* pEntry : m_vecTargets) {
		if (pEntry != nullptr && pEntry->m_dwType == 1) {
			fTotal += pEntry->m_fRatio;
		}
	}

	if (fTotal >= 100.0f) {
		return 100;
	}

	return static_cast<uint32_t>(fTotal);
}

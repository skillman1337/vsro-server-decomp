/**
 * ============================================================================
 * Silkroad Online - Game Server Special Target & Event Spawn Manager
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\SpecialTargetManager.h
 *
 * Implements:
 *   - CSpecialTargetManager @ 0x006156F0 / 0x00615760 / 0x00615860
 *                           @ 0x006159E0 / 0x00615A20 / 0x00615AA0
 *   - GetGlobalSpecialTargetManager @ 0x0041D300
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_SPECIALTARGETMANAGER_H_
#define _SR_GAMESERVER_SPECIALTARGETMANAGER_H_

#include <cstdint>
#include <vector>

/**
 * [RECONSTRUCTED - 0x006156F0 / 0x006159E0 / 0x00615A20 / 0x00615AA0]
 * CSpecialTargetManager
 * Tracks priority event target entities and dynamic spawn replacements.
 */
class CSpecialTargetManager {
public:
	struct TargetEntry {
		uint32_t m_dwPad00;    // +0x00
		uint32_t m_dwType;     // +0x04: Target type (1 = candidate for random spawn replacement)
		uint32_t m_dwMode;     // +0x08: Mode flags (checked in 0x005607B0: == 1)
		float    m_fRatio;     // +0x0C: Spawn weight ratio
		uint32_t m_dwTargetID; // +0x10: _RefObjCommon ID (0x00560B87 FindRefObjCommon, 0x006159F8 cmp [esi+0x10], edx)
		// CORRECTION (Claude): +0x14 is compared as a DWORD with (rarity & 0x0F) (0x00560B5B cmp eax, dword [esi+0x14]),
		// and only the low nibble of the byte at +0x18 replaces the rarity (0x00560B82 mov dl, byte [esi+0x18]).
		uint32_t m_dwRarity;          // +0x14: rarity this entry replaces
		uint8_t  m_btReplaceRarity;   // +0x18: new rarity (low nibble)
		uint8_t  m_pad19[3];          // +0x19 - +0x1B
		uint32_t m_dwDamage;   // +0x1C: Special damage modifier
	};

	CSpecialTargetManager();
	~CSpecialTargetManager();

	// [RECONSTRUCTED - Native 0x00615860] (86 bytes)
	// Clears and deletes all allocated target descriptors
	void Clear();

	// [RECONSTRUCTED - Native 0x006159E0] (45 bytes)
	// Checks if target entity GameID is tracked in the active list
	bool ContainsTargetID(uint32_t dwTargetID) const;

	// [RECONSTRUCTED - Native 0x00615A20] (45 bytes)
	// Retrieves special damage modifier associated with target
	uint16_t GetSpecialDamage(uint32_t dwTargetID) const;

	// [RECONSTRUCTED - Native 0x00615AA0] (106 bytes)
	// Calculates total cumulative spawn ratio across all active type-1 targets
	uint32_t CalculateTotalSpawnRatio() const;

public:
	std::vector<TargetEntry*> m_vecTargets; // +0x04 - +0x0F: Active target list
};

// Global special target manager instance pointer matching native 0x00D6A9B4
extern CSpecialTargetManager* g_pSpecialTargetManager;

// [RECONSTRUCTED - Native 0x0041D300] (18 bytes)
// Retrieves global special target manager singleton
CSpecialTargetManager* GetGlobalSpecialTargetManager();

#endif // _SR_GAMESERVER_SPECIALTARGETMANAGER_H_

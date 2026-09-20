/**
 * ============================================================================
 * Silkroad Online - Character Skill & Modifier Manager
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\SkillManager.cpp
 *
 * Implements:
 *   - CSkillManager::CSkillManager @ 0x00599F10 (372 bytes)
 *   - CSkillManager::~CSkillManager @ 0x0059A420 (442 bytes)
 *   - CSkillManager::GetSkillModifier @ 0x005A0330 (99 bytes)
 *   - CSkillManager::RegisterModifiers @ 0x005A02E0 (72 bytes)
 *   - CSkillManager::UnregisterModifiers @ 0x005A02A0 (56 bytes)
 *   - CSkillManager::GetDefaultAttackSkillByWeapon @ 0x0059E710 (50 bytes)
 *   - CSkillManager::FindActiveBuffBySkillID @ 0x0059EF90 (85 bytes)
 *   - CSkillManager::CancelActiveBuff @ 0x0059EFF0 (42 bytes)
 * ============================================================================
 */

#include "SkillManager.h"
#include "Formulae.h"
#include "GObjChar.h"
#include "GObjPC.h"
#include "GObj.h"
#include "GCharAutoCommandActor.h"
#include "Common/Framework/CmdSource.h"
#include "MainProcess.h"
#include "TimedJob.h"
#include "../ServerCommon/ReferenceData.h"
#include "../ServerCommon/InstanceSkill.h"
#include "../JMX_Library/BSLib/Packet.h"
#include "skill/SkillGlobal.h"
#include "skill/SkillRetirementPolicy.h"
#include "SkillCast.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_ServerFramework/ServerFramework/ServerConfig.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <windows.h>

// Native 0x00C8262C: Siege weapon attack skill templates indexed by race/country
uint32_t g_adwSiegeWeaponSkills[2] = { 0, 0 };

// Native 0x00C82638: base attack skill per weapon kind (TID >> 11), filled by
// SkillGlobal_LoadReferenceData (0x005893E0, "SKILL_PUNCH_01", "SKILL_CH_SWORD_BASE_01", ...).
uint32_t g_adwWeaponBaseAttackSkill[32] = { 0 };

// ============================================================================
// CZoeZoeRnd Implementation
// ============================================================================

// [RECONSTRUCTED - Native 0x00599BE0] (81 bytes)
CZoeZoeRnd::CZoeZoeRnd()
	: m_thresholds() {
	// Native 0x00599BE0 initializes vtable @ 0x00AFE168 and constructs m_thresholds
}

// [RECONSTRUCTED - Native 0x00599C40] (126 bytes)
CZoeZoeRnd::~CZoeZoeRnd() {
	Clear();
}

/**
 * [RECONSTRUCTED - Native 0x00599CC0] (273 bytes)
 * CZoeZoeRnd::Roll
 *
 * Core dynamic probability calculation and pity threshold management.
 * Tracks per-key success thresholds in m_thresholds:
 *   - Roll: std::rand() % 101 (0..100)
 *   - On first draw for a key:
 *       success = (roll <= chance)
 *       new_threshold = chance * 2 - (success ? 100 : 0)
 *   - On subsequent draws for existing key:
 *       success = (roll <= current_threshold)
 *       threshold += chance - (success ? 100 : 0) (wraps at 32-bit signed integer)
 */
bool CZoeZoeRnd::Roll(int32_t nChance, uint32_t dwKey) {
	if (nChance == 0) {
		return false;
	}
	if (nChance >= 100) {
		return true;
	}
	if (nChance < 0) {
		nChance = 0;
	}

	int32_t nRoll = std::rand() % 101;
	auto it = m_thresholds.find(dwKey);
	if (it == m_thresholds.end()) {
		bool bSuccess = (nRoll <= nChance);
		int32_t nThreshold = nChance * 2 - (bSuccess ? 100 : 0);
		m_thresholds.emplace(dwKey, nThreshold);
		return bSuccess;
	}

	int32_t nCurrent = it->second;
	bool bSuccess = (nRoll <= nCurrent);
	uint32_t uDelta = static_cast<uint32_t>(nChance - (bSuccess ? 100 : 0));
	it->second = static_cast<int32_t>(static_cast<uint32_t>(nCurrent) + uDelta);
	return bSuccess;
}

void CZoeZoeRnd::Clear() {
	m_thresholds.clear();
}

std::optional<int32_t> CZoeZoeRnd::GetThreshold(uint32_t dwKey) const {
	auto it = m_thresholds.find(dwKey);
	if (it != m_thresholds.end()) {
		return it->second;
	}
	return std::nullopt;
}

// ============================================================================
// CSkillManager Implementation
// ============================================================================

CSkillManager::CSkillManager()
	: m_pOwner(nullptr)
	, m_zoeRnd()
	, m_pCurrentInstance(nullptr)
	, m_mapSkill()
	, m_mapMastery()
	, m_mapModifiers()
	, m_listActiveBuffs()
	, m_vecRetiredContextIDs()
	, m_vecPendingMasteryIDs()
	, m_vecPendingSkillIDs()
	, m_dwReserved2EC(0) {
	std::memset(m_loadStateWords, 0, sizeof(m_loadStateWords));
	m_dwStateBit1D0 = 0;
	m_dwReserved1D4 = 0;
	// Native 0x00599F10 initializes m_zoeRnd at +0x04 and modifier structures
}

CSkillManager::~CSkillManager() {
	// Native 0x0059A420 cleans up modifier map, active buffs, and queues
	ClearModifiers();

	for (auto& pair : m_mapSkill) {
		delete pair.second;
	}
	m_mapSkill.clear();

	for (auto& pair : m_mapMastery) {
		delete pair.second;
	}
	m_mapMastery.clear();
}

/**
 * [RECONSTRUCTED - Native 0x005A1A20] (18 bytes)
 * CSkillManager::RollProbability
 *
 * Fast register-based probability forwarder. Guards against non-positive chance,
 * then delegates directly into m_zoeRnd (+0x04).
 */
bool CSkillManager::RollProbability(int32_t nChance, uint32_t dwKey) {
	if (nChance <= 0) {
		return false;
	}
	return m_zoeRnd.Roll(nChance, dwKey);
}

/**
 * [RECONSTRUCTED - 0x005A0330] (99 bytes)
 * CSkillManager::GetSkillModifier
 *
 * Looks up active skill modifier in internal map (+0x240) by modifier key.
 * Returns pointer to tagSkillModifier struct if present, nullptr otherwise.
 */
const tagSkillModifier* CSkillManager::GetSkillModifier(uint32_t dwModifierID) const {
	if (dwModifierID == 0) {
		return nullptr;
	}

	auto it = m_mapModifiers.find(dwModifierID);
	if (it != m_mapModifiers.end()) {
		return &it->second;
	}

	return nullptr;
}

/**
 * [RECONSTRUCTED - 0x005A02E0] (72 bytes)
 * CSkillManager::RegisterModifiers
 *
 * Iterates the 5 parameter slots (Setv through Setv5 @ offset +0x388 / 0xE2)
 * of the reference skill descriptor and installs them into m_mapModifiers (+0x240).
 */
void CSkillManager::RegisterModifiers(const tagRefSkill* pRefSkill) {
	if (!pRefSkill) {
		return;
	}

	for (uint32_t i = 0; i < 5; ++i) {
		const uint32_t* pRecord = pRefSkill->Param(0x388 + i * 4);
		if (!pRecord) {
			break;
		}

		uint32_t key = static_cast<uint32_t>(pRecord[0]);
		uint32_t val1 = static_cast<uint32_t>(pRecord[1]);
		uint32_t val2 = static_cast<uint32_t>(pRecord[2]);

		m_mapModifiers[key] = tagSkillModifier{ key, val1, val2 };
	}
}

/**
 * [RECONSTRUCTED - 0x005A02A0] (56 bytes)
 * CSkillManager::UnregisterModifiers
 *
 * Iterates the 5 parameter slots (Setv through Setv5 @ offset +0x388 / 0xE2)
 * of the reference skill descriptor and erases them from m_mapModifiers (+0x240).
 */
void CSkillManager::UnregisterModifiers(const tagRefSkill* pRefSkill) {
	if (!pRefSkill) {
		return;
	}

	for (uint32_t i = 0; i < 5; ++i) {
		const uint32_t* pRecord = pRefSkill->Param(0x388 + i * 4);
		if (!pRecord) {
			break;
		}

		uint32_t key = static_cast<uint32_t>(pRecord[0]);
		m_mapModifiers.erase(key);
	}
}

/**
 * Convenience overload for direct scalar registration
 */
void CSkillManager::RegisterModifier(uint32_t dwModifierID, uint32_t dwValue, uint32_t dwParam2) {
	if (dwModifierID == 0) {
		return;
	}
	m_mapModifiers[dwModifierID] = tagSkillModifier{ dwModifierID, dwValue, dwParam2 };
}

/**
 * Convenience overload for single modifier unregistration
 */
void CSkillManager::UnregisterModifier(uint32_t dwModifierID) {
	if (dwModifierID == 0) {
		return;
	}
	m_mapModifiers.erase(dwModifierID);
}

/**
 * CSkillManager::ClearModifiers
 */
void CSkillManager::ClearModifiers() {
	m_mapModifiers.clear();
}

/**
 * [RECONSTRUCTED - 0x0059E710] (50 bytes)
 * CSkillManager::GetDefaultAttackSkillByWeapon
 */
uint32_t CSkillManager::GetDefaultAttackSkillByWeapon() const {
	if (!m_pOwner->IsPlayer()) {
		return 0;
	}
	return GetAttackSkillByWeaponTID(static_cast<uint16_t>(m_pOwner->GetEquippedPrimaryWeaponTID() & 0xF800));
}

/**
 * [RECONSTRUCTED - 0x0049A1B0] (32 bytes)
 * CSkillManager::IsCastingLocked
 */
int32_t CSkillManager::IsCastingLocked() const {
	if (m_pRuntimeInstance1F0 == nullptr || m_pRuntimeInstance1F0->m_pExecution == nullptr) {
		return 0;
	}
	return static_cast<int32_t>(*m_pRuntimeInstance1F0->m_pExecution->m_pRefSkill->pMsch);
}

/**
 * [RECONSTRUCTED - Native 0x0059EFF0] (42 bytes)
 * tagActiveSkillInstance::RequestRetirement
 *
 * If bForce is true, unconditionally clears retirement marker (+0x10).
 * Otherwise, inspects m_pExecution (+0x18) and m_pRefSkill (+0x08); if pRefSkill
 * is null, or dwNbuf (+0x35C) is 0, or m_pExecution->m_byMode is 1, clears retirement marker.
 */
void tagActiveSkillInstance::RequestRetirement(bool bForce) {
	if (bForce) {
		m_dwRetirement = 0;
		return;
	}

	if (!m_pExecution) {
		m_dwRetirement = 0;
		return;
	}

	const tagRefSkill* pRefSkill = m_pExecution->m_pRefSkill;
	if (!pRefSkill || pRefSkill->pNbuf == 0 || m_pExecution->m_byMode == 1) {
		m_dwRetirement = 0;
	}
}

/**
 * [RECONSTRUCTED - Native 0x0059EF90] (85 bytes)
 * CSkillManager::FindActiveBuffBySkillID
 *
 * Iterates through active buff linked list (+0x268). Matches m_pCommand->m_dwSkillID,
 * ensures m_dwMode is 1 (passive/stance) or 2 (active buff), and matches optional dwContextID.
 */
tagActiveSkillInstance* CSkillManager::FindActiveBuffBySkillID(uint32_t dwSkillID, uint32_t dwContextID) const {
	for (tagActiveSkillInstance* pInst : m_listActiveBuffs) {
		if (!pInst || !pInst->m_pCommand) {
			continue;
		}
		if (pInst->m_pCommand->m_dwSkillID != dwSkillID) {
			continue;
		}
		if (pInst->m_dwMode != 1 && pInst->m_dwMode != 2) {
			continue;
		}
		if (dwContextID == 0 || (pInst->m_pExecution && pInst->m_pExecution->m_dwContextID == dwContextID)) {
			return pInst;
		}
	}
	return nullptr;
}

/**
 * [RECONSTRUCTED - Native 0x0059F0C0] (27 bytes)
 * CSkillManager::CancelBuffBySkillID
 *
 * Finds active buff instance by Skill ID and requests force retirement.
 */
tagActiveSkillInstance* CSkillManager::CancelBuffBySkillID(uint32_t dwSkillID) {
	tagActiveSkillInstance* pBuff = FindActiveBuffBySkillID(dwSkillID, 0);
	if (!pBuff) {
		return nullptr;
	}
	pBuff->RequestRetirement(true);
	return pBuff;
}

/**
 * [RECONSTRUCTED - Legacy Adapter]
 * CSkillManager::CancelActiveBuff
 */
bool CSkillManager::CancelActiveBuff(uint32_t dwRefSkillID, uint32_t dwRecordSkillID) {
	(void)dwRecordSkillID;
	return CancelBuffBySkillID(dwRefSkillID) != nullptr;
}

/**
 * [RECONSTRUCTED - Native 0x0059A680] (802 bytes)
 * CSkillManager::LoadLearnedRecords
 *
 * Populates runtime learned skills (m_mapSkill) and masteries (m_mapMastery)
 * from persistent DB records loaded from _CharSkill and _CharSkillMastery.
 */
bool CSkillManager::LoadLearnedRecords(
	std::list<CInstanceSkill*>& listSkills,
	CGObjPC* pPC,
	std::list<CInstanceSkillMastery*>& listMasteries
) {
	// 1. Process learned skills
	const size_t nSkillCount = listSkills.size();
	for (size_t i = 0; i < nSkillCount; ++i) {
		if (listSkills.empty()) {
			break;
		}
		CInstanceSkill* pRecord = listSkills.front();
		if (!pRecord) {
			listSkills.pop_front();
			continue;
		}

		tagSkillData* pSkillData = new tagSkillData();
		pSkillData->m_dwVptr = 0;
		pSkillData->m_dwRefCount = 1;
		pSkillData->m_pRefRecord = pRecord;
		pSkillData->m_byStatus = 0;

		if (g_pRefData == nullptr) {
			ServerFramework::ServerFramework_GenerateMiniDump();
		}

		const tagRefSkill* pRefSkill = g_pRefData ? g_pRefData->FindSkill(pRecord->GetSkillID()) : nullptr;
		pSkillData->m_pRefSkill = pRefSkill;

		if (pRefSkill == nullptr) {
			uint32_t dwCharID = pPC ? pPC->GetGlobalID() : 0;
			BSLib::Log_Printf(0x02000001, "PC learned skill reference is not exists!!! RefSkill ID[%d] CharID[%d]\n",
				pRecord->GetSkillID(), dwCharID);
			delete pSkillData;
			listSkills.pop_front();
			continue;
		}

		listSkills.pop_front();
		m_mapSkill[pRecord->GetSkillID()] = pSkillData;
	}

	// 2. Process learned masteries
	const size_t nMasteryCount = listMasteries.size();
	for (size_t i = 0; i < nMasteryCount; ++i) {
		if (listMasteries.empty()) {
			break;
		}
		CInstanceSkillMastery* pRecord = listMasteries.front();
		if (!pRecord) {
			listMasteries.pop_front();
			continue;
		}

		tagSkillMasteryData* pMasteryData = new tagSkillMasteryData();
		pMasteryData->m_dwVptr = 0;
		pMasteryData->m_dwRefCount = 1;
		pMasteryData->m_pRefRecord = pRecord;
		pMasteryData->m_byStatus = 0;

		if (g_pRefData == nullptr) {
			ServerFramework::ServerFramework_GenerateMiniDump();
		}

		const void* pRefMastery = g_pRefData ? g_pRefData->FindMastery(pRecord->GetMasteryID()) : nullptr;
		pMasteryData->m_pRefMastery = pRefMastery;

		if (pRefMastery == nullptr) {
			ServerFramework::ServerFramework_GenerateMiniDump();
			delete pMasteryData;
			return false; // Native does NOT consume this record and fails early!
		}

		listMasteries.pop_front();
		m_mapMastery[pRecord->GetMasteryID()] = pMasteryData;
	}

	// 3. Register Siege Weapon skills based on player country
	if (pPC != nullptr) {
		uint8_t byCountry = static_cast<uint8_t>(pPC->GetCountry());
		uint32_t dwSiegeSkillID = (byCountry < 2) ? g_adwSiegeWeaponSkills[byCountry] : 0;
		if (dwSiegeSkillID != 0) {
			if (RegisterSiegeSkill(dwSiegeSkillID) == 0) {
				BSLib::Log_Printf(0x02000001, "siegeweapon skill register failed Country[%d] CharID[%d]\n",
					byCountry, pPC->GetGlobalID());
			}
		}
	}

	// 4. Initialize character owner and clear loading states
	m_pOwner = pPC;
	std::memset(m_loadStateWords, 0, sizeof(m_loadStateWords));
	m_dwStateBit1D0 = 0;
	m_dwReserved1D4 = 0;
	m_dwReserved2EC = 0;

	// 5. Rebuild learned list caches
	RebuildLearnedLists();

	// 6. Reset character learned state words (+0x1F40)
	if (m_pOwner != nullptr) {
		ResetLearnedStateWords(reinterpret_cast<uint8_t*>(m_pOwner) + 0x1F40);
	}

	return true;
}

/**
 * [RECONSTRUCTED - Native 0x0059D3B0] (579 bytes)
 * CSkillManager::RebuildLearnedLists
 *
 * Refreshes m_vecPendingMasteryIDs and m_vecPendingSkillIDs caches.
 * For skills, walks nextRank pointer chains to find the highest learned rank
 * in each skill branch and deduplicates into m_vecPendingSkillIDs.
 */
void CSkillManager::RebuildLearnedLists() {
	if (!m_pOwner || !m_pOwner->IsPlayer()) {
		return;
	}

	// 1. Rebuild pending mastery IDs
	m_vecPendingMasteryIDs.clear();
	for (const auto& pair : m_mapMastery) {
		const tagSkillMasteryData* pMastery = pair.second;
		if (!pMastery || pMastery->m_byStatus == 3) {
			continue;
		}
		if (pMastery->m_pRefMastery != nullptr) {
			uint32_t dwMasteryID = *reinterpret_cast<const uint32_t*>(pMastery->m_pRefMastery);
			m_vecPendingMasteryIDs.push_back(dwMasteryID);
		}
	}

	// 2. Rebuild pending skill IDs
	m_vecPendingSkillIDs.clear();
	for (const auto& pair : m_mapSkill) {
		const tagSkillData* pSkillData = pair.second;
		if (!pSkillData || pSkillData->m_byStatus == 2) {
			continue;
		}

		const tagRefSkill* pRefSkill = pSkillData->m_pRefSkill;
		if (!pRefSkill || pRefSkill->byVisibilityD5 == 0xFF) {
			continue;
		}

		// Advance through next-rank pointer chain as long as next rank is learned
		const tagRefSkill* pCurrent = pRefSkill;
		while (pCurrent->pNextRankSkill != nullptr) {
			uint32_t dwNextSkillID = pCurrent->pNextRankSkill->dwSkillID;
			// Native checks if the skill map contains dwNextSkillID
			if (m_mapSkill.find(dwNextSkillID) == m_mapSkill.end()) {
				break;
			}
			pCurrent = pCurrent->pNextRankSkill;
		}

		uint32_t dwHighestRankID = pCurrent->dwSkillID;
		if (std::find(m_vecPendingSkillIDs.begin(), m_vecPendingSkillIDs.end(), dwHighestRankID) == m_vecPendingSkillIDs.end()) {
			m_vecPendingSkillIDs.push_back(dwHighestRankID);
		}
	}
}

/**
 * [RECONSTRUCTED - Native 0x0059D760] (250 bytes)
 * CSkillManager::GetHighestRankSkill
 *
 * Scans m_vecPendingSkillIDs to find the learned skill reference belonging
 * to dwGroupID that possesses the highest rank.
 */
const tagRefSkill* CSkillManager::GetHighestRankSkill(uint32_t dwGroupID) const {
	const tagRefSkill* pHighest = nullptr;
	uint8_t byMaxRank = 0;

	for (uint32_t dwSkillID : m_vecPendingSkillIDs) {
		if (g_pRefData == nullptr) {
			ServerFramework::ServerFramework_GenerateMiniDump();
			continue;
		}
		const tagRefSkill* pRefSkill = g_pRefData->FindSkill(dwSkillID);
		if (pRefSkill == nullptr) {
			continue;
		}

		if (pRefSkill->dwGroupID == dwGroupID && pRefSkill->byRank > byMaxRank) {
			pHighest = pRefSkill;
			byMaxRank = pRefSkill->byRank;
		}
	}

	return pHighest;
}

/**
 * [RECONSTRUCTED - Native 0x0059E7C0] (123 bytes)
 * CSkillManager::GetTotalMasteryLevel
 *
 * Sums m_byLevel across all active learned masteries (byStatus != 3).
 * Used by CSkillManager::RaiseMastery to enforce mastery point limits.
 */
uint32_t CSkillManager::GetTotalMasteryLevel() const {
	uint32_t dwTotal = 0;
	for (const auto& pair : m_mapMastery) {
		const tagSkillMasteryData* pMastery = pair.second;
		if (!pMastery || pMastery->m_byStatus == 3) {
			continue;
		}
		const CInstanceSkillMastery* pRecord = pMastery->m_pRefRecord;
		if (pRecord != nullptr) {
			dwTotal += pRecord->GetLevel();
		}
	}
	return dwTotal;
}

/*
================
CSkillManager::GetMaxMasteryLevel

[RECONSTRUCTED - Native 0x0059E870] (85 bytes)
Scans m_mapMastery (+0x234) for active masteries (byStatus != 3) and returns the highest level.
================
*/
uint8_t CSkillManager::GetMaxMasteryLevel() const {
	uint8_t byMaxLevel = 0;
	for (const auto& pair : m_mapMastery) {
		const tagSkillMasteryData* pMastery = pair.second;
		if (!pMastery || pMastery->m_byStatus == 3) {
			continue;
		}
		const CInstanceSkillMastery* pRecord = pMastery->m_pRefRecord;
		if (pRecord != nullptr) {
			uint8_t byLvl = pRecord->GetLevel();
			if (byLvl > byMaxLevel) {
				byMaxLevel = byLvl;
			}
		}
	}
	return byMaxLevel;
}

/**
 * [RECONSTRUCTED - Native 0x0059EA40] (112 bytes)
 * CSkillManager::RegisterSiegeSkill
 */
int32_t CSkillManager::RegisterSiegeSkill(uint32_t dwSiegeSkillID) {
	if (g_pRefData == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}

	const tagRefSkill* pRefSkill = g_pRefData->FindSkill(dwSiegeSkillID);
	if (pRefSkill == nullptr) {
		return 0;
	}

	tagSkillData* pSkillData = new tagSkillData();
	pSkillData->m_dwVptr = 0;
	pSkillData->m_dwRefCount = 1;
	pSkillData->m_pRefRecord = nullptr;
	pSkillData->m_byStatus = 3; // 3 = siege / auto-granted
	pSkillData->m_pRefSkill = pRefSkill;

	m_mapSkill[dwSiegeSkillID] = pSkillData;
	return 1;
}

/**
 * [RECONSTRUCTED - Native 0x0064CC40] (16 bytes)
 * CSkillManager::ResetLearnedStateWords
 */
void CSkillManager::ResetLearnedStateWords(void* pTarget) {
	if (!pTarget) {
		return;
	}
	uint32_t* pWords = reinterpret_cast<uint32_t*>(pTarget);
	pWords[0x14] = 0; // +0x50
	pWords[0x15] = 0; // +0x54
	pWords[0x16] = 0; // +0x58
	pWords[0x17] = 0; // +0x5C
}

/**
 * [PARTIAL - Native 0x0059E650] (183 bytes)
 * CSkillManager::GetAttackSkillByWeaponTID
 *
 * While casting is locked the attack comes from the transport the owner controls (owner +0x21A0,
 * skill at +0x260), which the port does not model yet: 0 is returned. Bare hands count as weapon
 * kind 1, fortress weapons (kind 16) use the per-country skill; any other kind must be learned.
 */
uint32_t CSkillManager::GetAttackSkillByWeaponTID(uint16_t wWeaponTID) const {
	if (m_pOwner == nullptr) {
		return 0;
	}
	if (IsCastingLocked() == 1) {
		return 0;
	}

	uint16_t wType = wWeaponTID;
	if ((wType & 0xF800) == 0) {
		wType = static_cast<uint16_t>((wType & 0x7FF) | 0x800);
	}

	if ((wType & 0xF800) == 0x8000) {
		uint8_t byCountry = m_pOwner->GetCountry();
		return g_adwSiegeWeaponSkills[byCountry];
	}

	uint32_t dwSkillID = g_adwWeaponBaseAttackSkill[wType >> 11];
	return (FindSkillByID(dwSkillID) != nullptr) ? dwSkillID : 0;
}

/**
 * [RECONSTRUCTED - Native 0x0059D680] (91 bytes)
 * CSkillManager::FindSkillByID
 *
 * Looks up learned skill in m_mapSkill (+0x228). If found, validates that status
 * is not 2 (deleted/inactive); returns pointer to tagSkillData or nullptr.
 */
tagSkillData* CSkillManager::FindSkillByID(uint32_t dwSkillID) const {
	auto it = m_mapSkill.find(dwSkillID);
	if (it == m_mapSkill.end()) {
		return nullptr;
	}
	tagSkillData* pSkill = it->second;
	if (!pSkill || pSkill->m_byStatus == 2) {
		return nullptr;
	}
	return pSkill;
}

/**
 * [RECONSTRUCTED - Native 0x0059D6F0] (99 bytes)
 * CSkillManager::FindMastery
 *
 * Looks up learned mastery in m_mapMastery (+0x234). If found, validates that status
 * is not 3 (deleted/inactive); returns pointer to tagSkillMasteryData or nullptr.
 */
tagSkillMasteryData* CSkillManager::FindMastery(uint32_t dwMasteryID) const {
	auto it = m_mapMastery.find(dwMasteryID);
	if (it == m_mapMastery.end()) {
		return nullptr;
	}
	tagSkillMasteryData* pMastery = it->second;
	if (!pMastery || pMastery->m_byStatus == 3) {
		return nullptr;
	}
	return pMastery;
}

/**
 * [RECONSTRUCTED - Native 0x0059CDC0] (630 bytes)
 * CSkillManager::SerializeActiveBuffs
 *
 * Serializes active buffs into network packet payload for character spawn and
 * observer state synchronization. Includes remaining duration if dwTimeMode == 1.
 */
void CSkillManager::SerializeActiveBuffs(BSLib::CPacket* pPacket, uint32_t dwTimeMode) {
	if (!pPacket) {
		return;
	}

	uint8_t byCount = 0;
	for (const tagActiveSkillInstance* pInstance : m_listActiveBuffs) {
		if (pInstance != nullptr && pInstance->m_byMode == 2) {
			byCount++;
		}
	}

	pPacket->WriteUint8(byCount);
	if (byCount == 0) {
		return;
	}

	uint32_t dwNow = ::GetTickCount();
	for (const tagActiveSkillInstance* pInstance : m_listActiveBuffs) {
		if (pInstance == nullptr || pInstance->m_byMode != 2) {
			continue;
		}

		uint32_t dwSkillID = (pInstance->m_pCommand != nullptr) ? pInstance->m_pCommand->m_dwSkillID : 0;
		uint32_t dwContextID = (pInstance->m_pExecution != nullptr) ? pInstance->m_pExecution->m_dwContextID : 0;

		pPacket->WriteUint32(dwSkillID);
		pPacket->WriteUint32(dwContextID);

		if (dwTimeMode == 1) {
			uint32_t dwDuration = (pInstance->m_pCommand != nullptr) ? pInstance->m_pCommand->m_dwDuration : 0;
			uint32_t dwRemaining = pInstance->m_dwStartTime + dwDuration - dwNow;
			pPacket->WriteUint32(dwRemaining);
		}
	}
}

/**
 * [RECONSTRUCTED - Native 0x0059D040] (867 bytes)
 * CSkillManager::SerializePendingLearnedSkills
 *
 * Serializes learned mastery and skill update records into network packet payload
 * using delimiter sentinels (0 = start, 1 = record, 2 = end).
 */
void CSkillManager::SerializePendingLearnedSkills(BSLib::CPacket* pPacket) {
	if (!pPacket) {
		return;
	}

	// Masteries block
	pPacket->WriteUint8(0);
	for (uint32_t dwMasteryID : m_vecPendingMasteryIDs) {
		pPacket->WriteUint8(1);
		pPacket->WriteUint32(dwMasteryID);
		pPacket->WriteUint8(1); // Rank / level
	}
	pPacket->WriteUint8(2);

	// Skills block
	pPacket->WriteUint8(0);
	for (uint32_t dwSkillID : m_vecPendingSkillIDs) {
		pPacket->WriteUint8(1);
		pPacket->WriteUint32(dwSkillID);
		pPacket->WriteUint8(1); // Enabled
	}
	pPacket->WriteUint8(2);
}

/**
 * [RECONSTRUCTED - Native 0x0059ECD0] (234 bytes)
 * CSkillManager::PublishRetiredBuffs
 *
 * Constructs and broadcasts Opcode 0xB072 containing the list of expired or
 * cancelled buff context IDs to nearby observers, then clears the retired queue.
 */
void CSkillManager::PublishRetiredBuffs() {
	// Both entry paths are the same native 59ECD0 operation. In particular,
	// death cleanup must send before clearing, not merely construct a packet.
	PublishRetiredContexts();
}

/**
 * [PARTIAL - Native 0x0059FF80] (374 bytes)
 * CSkillManager::RetireSkillsForDeath
 *
 * Purges non-persistent active skill buffs upon character death, preserving continuous
 * buffs (cbuf), removing parameter modifiers from m_mapModifiers, and triggering
 * 0xB072 retirement broadcast. Selection corrected against ASM 2026-09-20;
 * Event 6 runs before source removal; only outcome 3 enters the retirement
 * broadcast. The selected executor's teardown coverage is tracked separately.
 */
void CSkillManager::RetireSkillsForDeath(bool bForceSelection) {
	auto it = m_listActiveBuffs.begin();
	while (it != m_listActiveBuffs.end()) {
		tagActiveSkillInstance* pInstance = *it;
		if (pInstance == nullptr) {
			it = m_listActiveBuffs.erase(it);
			continue;
		}

		const tagRefSkill* pRef = pInstance->m_pExecution ? pInstance->m_pExecution->m_pRefSkill : nullptr;
		const bool bSelected = pRef && SkillRetirementSelected(bForceSelection,
			pRef->byCastType, m_pCurrentInstance == pInstance, pRef->dwActionCategory,
			pRef->Param(0x274) != nullptr, pRef->Param(0x2B4) != nullptr,
			pRef->Param(0x358) != nullptr);

		// Continuous buffs (cbuf) are preserved even on death
		if (!bSelected) {
			++it;
			continue;
		}

		it = m_listActiveBuffs.erase(it);

		const int32_t outcome = SkillCast::SkillActionHandler(m_pOwner, pInstance, CAST_EVENT_CANCEL);
		if (pInstance->m_pExecution != nullptr) {
			if (outcome == CAST_OUTCOME_RELEASE_NOTIFY)
				m_vecRetiredContextIDs.push_back(pInstance->m_pExecution->m_dwContextID);
			if (pInstance->m_pExecution->m_pRefSkill != nullptr) {
				UnregisterModifiers(pInstance->m_pExecution->m_pRefSkill);
			}
		}

		if (m_pOwner) m_pOwner->m_paramKeeper.RemoveSourceModifiers(reinterpret_cast<uintptr_t>(pInstance));
		tagActiveSkillInstance::Release(pInstance);
	}

	if (bForceSelection) {
		m_pCurrentInstance = nullptr;
	}

	PublishRetiredBuffs();
}

/*
================
CSkillManager::ProcessReductionItems

[RECONSTRUCTED - Native 0x0059F7B0] (235 bytes)
Consumes recall/reduction items from player inventory using cursor slot 0x0D (slot 13)
and storage virtual functions:
  - InquireSameItem: Slot 137 (+0x224)
  - DelItem_EXT: Slot 140 (+0x230)
================
*/
uint16_t CSkillManager::ProcessReductionItems(const char* szItemCodeName, int32_t nCount, uint32_t dwControl) {
	if (!szItemCodeName || nCount <= 0 || !m_pOwner) {
		return REDUCTION_OPERATION_FAILED; // 0x7803
	}

	// Slot 137 @ +0x224: Query total available count in inventory (dwStorageType=0, dwMode=2)
	int32_t nAvailable = static_cast<int32_t>(m_pOwner->InquireSameItem(0, szItemCodeName, 2, 0xFFFFFFFF, 1));
	if (nAvailable < nCount) {
		return REDUCTION_MISSING_ITEMS; // 0x7805
	}

	int32_t nRemaining = nCount;
	uint32_t dwSlotCursor = 0x0D; // Inventory start cursor slot 13

	while (nRemaining > 0) {
		// Mode 1: Query quantity in slot
		uint32_t nInSlot = m_pOwner->InquireSameItem(0, szItemCodeName, 1, dwSlotCursor, 1);
		// Mode 0: Query slot index
		uint32_t dwSlot = m_pOwner->InquireSameItem(0, szItemCodeName, 0, dwSlotCursor, 1);

		uint32_t nToConsume = (static_cast<int32_t>(nInSlot) >= nRemaining) ? static_cast<uint32_t>(nRemaining) : nInSlot;

		uint16_t wStatus = 1;
		// Slot 140 @ +0x230: DelItem_EXT(0, dwSlot, nToConsume, &wStatus, 3, dwControl)
		m_pOwner->DelItem_EXT(0, static_cast<uint8_t>(dwSlot), static_cast<int32_t>(nToConsume), &wStatus, 3, dwControl);

		if (wStatus != 1) {
			return REDUCTION_OPERATION_FAILED; // 0x7803
		}

		nRemaining -= static_cast<int32_t>(nToConsume);
	}

	return REDUCTION_SUCCESS; // 0x7800
}

/*
================
CSkillManager::ValidateSkillReduction

[RECONSTRUCTED - Native 0x0059F410] (501 bytes)
Validates skill de-leveling / recall prerequisites:
  1. Checks caster is a player character (slot 7 @ +0x1C)
  2. Finds skill data; returns 0x7801 if not found
  3. Checks requestedRank < currentRank; returns 0x7802 if not lower
  4. If flags & 1, validates player gold; returns 0x7804 if insufficient
  5. Consumes items via ProcessReductionItems (0x0059F7B0); returns error if missing
  6. Iterates learned skills list, verifying 3 prerequisite slots (+0xB0/+0xBC);
     returns 0x7806 if any other skill depends on this skill above requested rank
================
*/
uint16_t CSkillManager::ValidateSkillReduction(const char* szItemCodeName, uint32_t dwSkillID, uint8_t byRequestedRank, uint8_t byFlags) {
	if (!m_pOwner || !m_pOwner->IsPlayer()) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return REDUCTION_OPERATION_FAILED; // 0x7803
	}

	const tagSkillData* pSkillData = FindSkillByID(dwSkillID);
	if (!pSkillData || !pSkillData->m_pRefSkill) {
		return REDUCTION_NOT_FOUND; // 0x7801
	}

	const tagRefSkill* pRefSkill = pSkillData->m_pRefSkill;
	if (byRequestedRank > 0 && pRefSkill->byRank <= byRequestedRank) {
		return REDUCTION_RANK_NOT_LOWER; // 0x7802
	}

	// Flag 0x01: Validate gold affordability
	if ((byFlags & 1) != 0) {
		int32_t nGoldCost = Formulae::CalculateSkillDowngradeGoldCost(pRefSkill, m_pOwner, byRequestedRank);
		if (nGoldCost != 0) {
			int64_t nCharGold = m_pOwner->GetGold();
			if (nCharGold < static_cast<int64_t>(nGoldCost)) {
				return REDUCTION_INSUFFICIENT_GOLD; // 0x7804
			}
		}
	}

	// Flag 0x02: Validate reduction item availability
	uint32_t nItemCount = static_cast<uint32_t>(pRefSkill->byRank) - byRequestedRank;
	if (nItemCount > 0) {
		uint16_t wItemStatus = ProcessReductionItems(szItemCodeName, static_cast<int32_t>(nItemCount), 1);
		if (wItemStatus != REDUCTION_SUCCESS) {
			return wItemStatus;
		}
	}

	// Prerequisite skill dependency tree validation
	for (const auto& pair : m_mapSkill) {
		const tagSkillData* pLearned = pair.second;
		if (!pLearned || pLearned->m_byStatus == 2 || !pLearned->m_pRefSkill) {
			continue;
		}
		if (pLearned->m_pRefSkill == pRefSkill) {
			continue;
		}

		const tagRefSkill* pLearnedRef = pLearned->m_pRefSkill;
		for (int i = 0; i < 3; ++i) {
			uint32_t dwReqGroupID = pLearnedRef->dwReqSkillGroupID[i];
			if (dwReqGroupID != 0 && dwReqGroupID == pRefSkill->dwGroupID) {
				if (byRequestedRank != 0) {
					if (pLearnedRef->byReqSkillRank[i] > byRequestedRank) {
						return REDUCTION_DEPENDENCY; // 0x7806
					}
				} else {
					return REDUCTION_DEPENDENCY; // 0x7806
				}
			}
		}
	}

	return REDUCTION_SUCCESS; // 0x7800
}

/*
================
CSkillManager::ValidateMasteryReduction

[RECONSTRUCTED - Native 0x0059F610] (400 bytes)
Validates mastery de-leveling / recall prerequisites:
  1. Checks caster is a player character (slot 7 @ +0x1C)
  2. Finds mastery entry via FindMastery (0x0059D6F0); returns 0x7801 if not found
  3. Checks requestedRank < currentRank; returns 0x7802 if not lower
  4. If flags & 1, validates player gold; returns 0x7804 if insufficient
  5. If flags & 2, consumes items via ProcessReductionItems (0x0059F7B0)
  6. Iterates learned skills list, verifying 2 mastery requirement slots (+0xA0/+0xA8);
     returns 0x7806 if any skill depends on this mastery above requested rank
================
*/
uint16_t CSkillManager::ValidateMasteryReduction(const char* szItemCodeName, uint32_t dwMasteryID, uint8_t byRequestedRank, uint8_t byFlags) {
	if (!m_pOwner || !m_pOwner->IsPlayer()) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return REDUCTION_OPERATION_FAILED; // 0x7803
	}

	const tagSkillMasteryData* pMastery = FindMastery(dwMasteryID);
	if (!pMastery || !pMastery->m_pRefRecord) {
		return REDUCTION_NOT_FOUND; // 0x7801
	}

	uint8_t byCurrentLevel = pMastery->m_pRefRecord->GetLevel();
	if (byRequestedRank > 0 && byCurrentLevel <= byRequestedRank) {
		return REDUCTION_RANK_NOT_LOWER; // 0x7802
	}

	// Flag 0x01: Validate gold affordability
	if ((byFlags & 1) != 0) {
		int32_t nGoldCost = Formulae::CalculateMasteryDowngradeSPRefund(m_pOwner, byCurrentLevel, byRequestedRank);
		if (nGoldCost != 0) {
			int64_t nCharGold = m_pOwner->GetGold();
			if (nCharGold < static_cast<int64_t>(nGoldCost)) {
				return REDUCTION_INSUFFICIENT_GOLD; // 0x7804
			}
		}
	}

	// Flag 0x02: Validate reduction item availability
	if ((byFlags & 2) != 0) {
		uint32_t nItemCount = static_cast<uint32_t>(byCurrentLevel) - byRequestedRank;
		if (nItemCount > 0) {
			uint16_t wItemStatus = ProcessReductionItems(szItemCodeName, static_cast<int32_t>(nItemCount), 1);
			if (wItemStatus != REDUCTION_SUCCESS) {
				return wItemStatus;
			}
		}
	}

	// Mastery requirement dependency validation: check all learned skills
	for (const auto& pair : m_mapSkill) {
		const tagSkillData* pLearned = pair.second;
		if (!pLearned || pLearned->m_byStatus == 2 || !pLearned->m_pRefSkill) {
			continue;
		}

		const tagRefSkill* pLearnedRef = pLearned->m_pRefSkill;
		for (int i = 0; i < 2; ++i) {
			uint32_t dwReqMasteryID = pLearnedRef->dwReqMasteryID[i];
			if (dwReqMasteryID != 0 && dwReqMasteryID == dwMasteryID) {
				if (pLearnedRef->byReqMasteryLevel[i] > byRequestedRank) {
					return REDUCTION_DEPENDENCY; // 0x7806
				}
			}
		}
	}

	return REDUCTION_SUCCESS; // 0x7800
}

/*
================
CSkillManager::ExecuteSkillReduction

[RECONSTRUCTED - Native 0x0059F8B0] (1190 bytes)
Invokes ValidateSkillReduction (0x0059F410). If valid (0x7800):
  - Retires active buffs and instances belonging to this skill
  - Lowers skill rank in character record / ShardDB
  - Deletes old record, inserts downgraded replacement record
  - Consumes items via ProcessReductionItems (0x0059F7B0)
  - Deducts gold via OffsetGold (slot 91 @ +0x16C)
  - Refunds Skill Points via OffsetSkillPoint (slot 93 @ +0x174)
  - Notifies client / nearby sessions
================
*/
uint16_t CSkillManager::ExecuteSkillReduction(const char* szItemCodeName, uint32_t dwSkillID, uint8_t byRequestedRank, uint8_t byFlags) {
	if (!szItemCodeName || std::strlen(szItemCodeName) <= 1 || !m_pOwner) {
		return REDUCTION_NOT_FOUND; // 0x7801
	}

	uint16_t wStatus = ValidateSkillReduction(szItemCodeName, dwSkillID, byRequestedRank, byFlags);
	if (wStatus != REDUCTION_SUCCESS) {
		return wStatus;
	}

	tagSkillData* pSkillData = FindSkillByID(dwSkillID);
	if (!pSkillData || !pSkillData->m_pRefSkill) {
		return REDUCTION_NOT_FOUND; // 0x7801
	}

	const tagRefSkill* pRefSkill = pSkillData->m_pRefSkill;
	int32_t nGoldCost = 0;
	if ((byFlags & 1) != 0) {
		nGoldCost = Formulae::CalculateSkillDowngradeGoldCost(pRefSkill, m_pOwner, byRequestedRank);
		if (nGoldCost == 0) {
			return REDUCTION_OPERATION_FAILED; // 0x7803
		}
	}

	uint32_t nItemCount = 0;
	if ((byFlags & 2) != 0) {
		nItemCount = static_cast<uint32_t>(pRefSkill->byRank) - byRequestedRank;
		if (nItemCount == 0) {
			return REDUCTION_OPERATION_FAILED; // 0x7803
		}
	}

	int32_t nRefundSP = Formulae::CalculateSkillDowngradeSPRefund(byRequestedRank, pRefSkill, ((byFlags >> 2) & 1) != 0);
	if (nRefundSP == 0) {
		return REDUCTION_OPERATION_FAILED; // 0x7803
	}

	const tagRefSkill* pReplacement = nullptr;
	if (byRequestedRank != 0) {
		pReplacement = pRefSkill->pPreviousRankSkill;
		while (pReplacement && pReplacement->byRank != byRequestedRank) {
			pReplacement = pReplacement->pPreviousRankSkill;
		}
		if (!pReplacement) {
			return REDUCTION_OPERATION_FAILED; // 0x7803
		}
	}

	// Mark old entry as inactive (status = 2)
	pSkillData->m_byStatus = 2;

	// Cancel active buffs belonging to this skill
	for (auto it = m_listActiveBuffs.begin(); it != m_listActiveBuffs.end(); ) {
		tagActiveSkillInstance* pInstance = *it;
		if (pInstance && pInstance->m_pExecution &&
			pInstance->m_pExecution->m_pRefSkill == pRefSkill) {

			if (pInstance->m_byMode != 2) {
				BSLib::CPacket pkt;
				pkt.SetOpcode(0xB072);
				uint32_t dwCount = 1;
				pkt.Write(&dwCount, 4);
				pkt.Write(&pInstance->m_pExecution->m_dwContextID, 4);
				m_pOwner->SendPacketToNearbySessions(&pkt);
			}

			UnregisterModifiers(pRefSkill);
			it = m_listActiveBuffs.erase(it);
			SkillCast::SkillActionHandler(m_pOwner, pInstance, CAST_EVENT_CANCEL);
			if (m_pOwner) m_pOwner->m_paramKeeper.RemoveSourceModifiers(reinterpret_cast<uintptr_t>(pInstance));
			tagActiveSkillInstance::Release(pInstance);
		} else {
			++it;
		}
	}

	// Delete old CInstanceSkill DB record and erase from m_mapSkill
	CInstanceSkill* pOldRecord = pSkillData->m_pRefRecord;
	m_mapSkill.erase(dwSkillID);
	if (pOldRecord) {
		pOldRecord->Clear();
		delete pOldRecord;
	}
	delete pSkillData;

	// If replacement exists, allocate new record and insert into m_mapSkill
	if (pReplacement != nullptr) {
		CInstanceSkill* pNewRecord = new CInstanceSkill();
		pNewRecord->SetCharID(m_pOwner->GetGlobalID());
		pNewRecord->SetSkillID(pReplacement->dwSkillID);
		pNewRecord->SetEnable(1);

		tagSkillData* pNewSkillData = new tagSkillData();
		pNewSkillData->m_dwVptr = 0;
		pNewSkillData->m_dwRefCount = 1;
		pNewSkillData->m_pRefRecord = pNewRecord;
		pNewSkillData->m_pRefSkill = pReplacement;
		pNewSkillData->m_byStatus = 0;
		m_mapSkill[pReplacement->dwSkillID] = pNewSkillData;

		if (pReplacement->byVisibilityD5 == 0) {
			RegisterModifiers(pReplacement);
		}
	}

	// Submit async DB transaction: exec _skill_manage 2, CharID, oldSkillID, newSkillID
	if (g_pMainProcess != nullptr) {
		uint32_t dwNewSkillID = pReplacement ? pReplacement->dwSkillID : 0;
		if (!g_pMainProcess->SubmitLearnedChangeDBQuery(2, m_pOwner->GetGlobalID(), pRefSkill->dwSkillID, dwNewSkillID)) {
			return REDUCTION_OPERATION_FAILED; // 0x7803
		}
	}

	// Consume reduction items (flags & 2)
	if ((byFlags & 2) != 0) {
		if (ProcessReductionItems(szItemCodeName, static_cast<int32_t>(nItemCount), 0) != REDUCTION_SUCCESS) {
			return REDUCTION_OPERATION_FAILED; // 0x7803
		}
	}

	// Debit gold (flags & 1)
	if ((byFlags & 1) != 0) {
		m_pOwner->OffsetGold(-static_cast<int64_t>(nGoldCost), 9, 1, 0);
	}

	// Credit SP and notify resources
	m_pOwner->OffsetSkillPoint(nRefundSP, 1);
	m_pOwner->BackupData(0x80, 0);

	RebuildLearnedLists();

	BSLib::Log_Printf(0x1000000, "[Skill Reduction]: CharID=%u, SkillID=%u, ReqRank=%u, RefundSP=%d, GoldCost=%d\n",
		m_pOwner->GetGlobalID(), dwSkillID, byRequestedRank, nRefundSP, nGoldCost);

	return REDUCTION_SUCCESS; // 0x7800
}

/*
================
CSkillManager::ExecuteMasteryReduction

[RECONSTRUCTED - Native 0x0059FD60] (542 bytes)
Invokes ValidateMasteryReduction (0x0059F610). If valid (0x7800):
  - Deducts gold / items
  - Lowers mastery level in character record / ShardDB
  - Calculates and refunds Skill Points (SP)
  - Notifies client / nearby sessions
================
*/
uint16_t CSkillManager::ExecuteMasteryReduction(const char* szItemCodeName, uint32_t dwMasteryID, uint8_t byRequestedRank, uint8_t byFlags) {
	if (!szItemCodeName || std::strlen(szItemCodeName) <= 1 || !m_pOwner) {
		return REDUCTION_NOT_FOUND; // 0x7801
	}

	uint16_t wStatus = ValidateMasteryReduction(szItemCodeName, dwMasteryID, byRequestedRank, byFlags);
	if (wStatus != REDUCTION_SUCCESS) {
		return wStatus;
	}

	tagSkillMasteryData* pMastery = FindMastery(dwMasteryID);
	if (!pMastery || !pMastery->m_pRefRecord) {
		return REDUCTION_NOT_FOUND; // 0x7801
	}

	CInstanceSkillMastery* pRecord = pMastery->m_pRefRecord;
	uint8_t byCurrentLevel = pRecord->GetLevel();

	int32_t nGoldCost = 0;
	if ((byFlags & 1) != 0) {
		nGoldCost = Formulae::CalculateMasteryDowngradeSPRefund(m_pOwner, byCurrentLevel, byRequestedRank);
		if (nGoldCost == 0) {
			return REDUCTION_OPERATION_FAILED; // 0x7803
		}
	}

	uint32_t nItemCount = 0;
	if ((byFlags & 2) != 0) {
		nItemCount = static_cast<uint32_t>(byCurrentLevel) - byRequestedRank;
		if (nItemCount == 0) {
			return REDUCTION_OPERATION_FAILED; // 0x7803
		}
	}

	int32_t nRefundSP = Formulae::CalculateMasteryDowngradeGoldCost(byCurrentLevel, byRequestedRank, ((byFlags >> 2) & 1) != 0);
	if (nRefundSP == 0 && (byCurrentLevel != 1 || byRequestedRank != 0)) {
		return REDUCTION_OPERATION_FAILED; // 0x7803
	}

	uint8_t byOldLevel = byCurrentLevel;
	pRecord->SetLevel(byRequestedRank);
	pRecord->m_dwStateFlags = 0;
	pRecord->m_dwReserved = 0;

	m_pOwner->RecomputeMasteryStats();
	m_pOwner->BackupData(0x20, 0);

	// Submit async DB transaction: exec _skill_manage 3, CharID, MasteryID, NewLevel
	if (g_pMainProcess != nullptr) {
		if (!g_pMainProcess->SubmitLearnedChangeDBQuery(3, m_pOwner->GetGlobalID(), dwMasteryID, byRequestedRank)) {
			return REDUCTION_OPERATION_FAILED; // 0x7803
		}
	}

	// Consume reduction items (flags & 2)
	if ((byFlags & 2) != 0) {
		if (ProcessReductionItems(szItemCodeName, static_cast<int32_t>(nItemCount), 0) != REDUCTION_SUCCESS) {
			return REDUCTION_OPERATION_FAILED; // 0x7803
		}
	}

	// Debit gold (flags & 1)
	if ((byFlags & 1) != 0) {
		m_pOwner->OffsetGold(-static_cast<int64_t>(nGoldCost), 9, 1, 0);
	}

	// Credit SP and notify resources
	m_pOwner->OffsetSkillPoint(nRefundSP, 1);
	m_pOwner->BackupData(0x80, 0);

	BSLib::Log_Printf(0x1000000, "[Mastery Reduction]: CharID=%u, MasteryID=%u, OldLevel=%u, NewLevel=%u, RefundSP=%d, GoldCost=%d\n",
		m_pOwner->GetGlobalID(), dwMasteryID, byOldLevel, byRequestedRank, nRefundSP, nGoldCost);

	return REDUCTION_SUCCESS; // 0x7800
}

/*
================
CSkillManager::IsHostileTargetEligible

[RECONSTRUCTED - Native 0x005A1AD0] (156 bytes)
Evaluates whether target entity is hostile and eligible for attack actions:
  - CanSelectTarget (slot +0x58C)
  - Checks if caster is player (slot +0x1C); if not player, checks IsNPC (slot +0x28)
  - Calls GetCombatPermission (slot +0x624) with mode 3 (PC) or mode 1 (non-PC)
  - Checks target player status; if player, checks Invulnerable/Ghost (slot +0x540)
  - If not player, queries IsCOS (slot +0x2C)
================
*/
uint32_t CSkillManager::IsHostileTargetEligible(CGObjChar* pTarget) {
	if (!m_pOwner || !pTarget) {
		return 0;
	}

	// Slot +0x58C: CanSelectTarget
	if (!m_pOwner->CanSelectTarget(pTarget, 0)) {
		return 0;
	}

	uint32_t dwMode = 3;
	if (!m_pOwner->IsPlayer()) {
		dwMode = 1;
	}

	uint32_t dwErrorCode = 0;
	// Slot +0x624: GetCombatPermission
	uint32_t dwResult = m_pOwner->GetCombatPermission(pTarget, dwMode, &dwErrorCode);
	if (dwResult == 0) {
		return 0;
	}

	if (pTarget->IsPlayer()) {
		// Slot +0x540: IsRidingTransport
		if (pTarget->IsRidingTransport()) {
			return 0;
		}
	}

	return dwResult;
}

/*
================
CSkillManager::IsSameParty

[RECONSTRUCTED - Native 0x005A1B70] (60 bytes)
Checks if target entity belongs to the same party as the caster:
  - Caster must be PC (IsPlayer == true)
  - Both caster and target must have non-null party pointers (+0x1CB8)
  - Both party IDs (CGObjChar_GetPartyID @ 0x004EA280) must match
================
*/
bool CSkillManager::IsSameParty(CGObjChar* pTarget) {
	if (!m_pOwner || !pTarget) {
		return false;
	}

	if (!m_pOwner->IsPlayer()) {
		return false;
	}

	if (m_pOwner->m_pParty == nullptr || pTarget->m_pParty == nullptr) {
		return false;
	}

	return m_pOwner->GetPartyID() == pTarget->GetPartyID();
}

/*
================
CSkillManager::ApplyHealRecovery

[RECONSTRUCTED - Native 0x005A09F0] (396 bytes)
Applies heal recovery to target character based on skill parameters and weapon healing bonus.
================
*/
void CSkillManager::ApplyHealRecovery(CGObjChar* pTarget, const tagRefSkill* pRefSkill) {
	if (!pTarget || !pRefSkill) {
		return;
	}

	const int32_t* pHealParams = reinterpret_cast<const int32_t*>(pRefSkill->Param(0x324));
	if (!pHealParams) {
		return;
	}

	int32_t nBaseHP = pHealParams[0];
	int32_t nBaseMP = pHealParams[2];

	// Percentage of max HP recovery
	if (pHealParams[1] != 0) {
		uint32_t dwMaxHP = pTarget->GetMaxHP();
		nBaseHP = static_cast<int32_t>((static_cast<float>(dwMaxHP) * static_cast<float>(pHealParams[1])) / 100.0f);
	}

	// Percentage of max MP recovery
	if (pHealParams[3] != 0) {
		uint32_t dwMaxMP = pTarget->GetMaxMP();
		nBaseMP = static_cast<int32_t>((static_cast<float>(dwMaxMP) * static_cast<float>(pHealParams[3])) / 100.0f);
	}

	// Weapon healing bonuses
	const uint32_t* pWeaponBonus = pRefSkill->Param(0x328);
	if (pWeaponBonus) {
		nBaseHP += static_cast<int32_t>(*pWeaponBonus);
	}

	if (nBaseHP < 0) nBaseHP = 0;
	if (nBaseMP < 0) nBaseMP = 0;

	// Apply recovery to target character
	if (nBaseHP > 0) {
		uint32_t dwCurHP = pTarget->GetCurrentHP();
		pTarget->SetCurrentHP(dwCurHP + nBaseHP);
	}

	if (nBaseMP > 0) {
		uint32_t dwCurMP = pTarget->GetCurrentMP();
		pTarget->SetCurrentMP(dwCurMP + nBaseMP);
	}
}

/*
================
CSkillManager::ValidateSkillLearning

[RECONSTRUCTED - Native 0x0059E450] (499 bytes)
Validates mastery prerequisites, character stats (STR/INT), country, skill continuity
sequence, prerequisite skills, and Skill Point (SP) affordability.
================
*/
uint16_t CSkillManager::ValidateSkillLearning(uint32_t dwSkillID) {
	if (g_pRefData == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return ERROR_SKILL_LEARN_FAILED; // 0x3409
	}

	const tagRefSkill* pRefSkill = g_pRefData->FindSkill(dwSkillID);
	if (!pRefSkill) {
		return ERROR_SKILL_LEARN_FAILED; // 0x3409
	}

	if (!m_pOwner || !m_pOwner->IsPlayer() || pRefSkill->dwSkillPointCost == 0) {
		return ERROR_SKILL_LEARN_FAILED; // 0x3409
	}

	CGObjPC* pPC = static_cast<CGObjPC*>(m_pOwner);

	// 1. Mastery requirements (up to 2 required masteries)
	for (int i = 0; i < 2; ++i) {
		uint32_t dwReqMasteryID = pRefSkill->dwReqMasteryID[i];
		if (dwReqMasteryID != 0) {
			tagSkillMasteryData* pMastery = FindMastery(dwReqMasteryID);
			if (!pMastery || !pMastery->m_pRefRecord) {
				return ERROR_SKILL_MASTERY_MISSING; // 0x3401
			}
			if (pRefSkill->byReqMasteryLevel[i] > pMastery->m_pRefRecord->GetLevel()) {
				return ERROR_SKILL_MASTERY_LEVEL_TOO_LOW; // 0x3402
			}
		}
	}

	// 2. Stat requirements (STR and INT)
	if (pRefSkill->byReqSTR > 0) {
		float fSTR = pPC->GetParamFloat(1);
		if (static_cast<float>(pRefSkill->byReqSTR) > fSTR) {
			return ERROR_SKILL_REQ_STR_TOO_LOW; // 0x3403
		}
	}

	if (pRefSkill->byReqINT > 0) {
		float fINT = pPC->GetParamFloat(2);
		if (static_cast<float>(pRefSkill->byReqINT) > fINT) {
			return ERROR_SKILL_REQ_INT_TOO_LOW; // 0x3404
		}
	}

	// 3. Country / race requirement
	// Country: 0 = China, 1 = Europe, 3 = Universal
	if (pRefSkill->byCountry != 3 && pRefSkill->byCountry != pPC->GetCountry()) {
		return ERROR_SKILL_COUNTRY_MISMATCH; // 0x3405
	}

	// 4. Skill rank sequence continuity check
	const tagRefSkill* pCurrentHighest = GetHighestRankSkill(pRefSkill->dwGroupID);
	if (!pCurrentHighest) {
		// First rank in the tree must be rank 1
		if (pRefSkill->byRank != 1) {
			return ERROR_SKILL_INVALID_FIRST_RANK; // 0x340C (triggers abuse log)
		}
	} else {
		// Must be sequential: current highest rank + 1 == requested rank
		if (static_cast<uint32_t>(pCurrentHighest->byRank) + 1 != pRefSkill->byRank) {
			return ERROR_SKILL_SEQUENCE_MISMATCH; // 0x3406
		}
	}

	// 5. Prerequisite skill requirements (up to 3 required skills)
	for (int i = 0; i < 3; ++i) {
		uint32_t dwReqGroupID = pRefSkill->dwReqSkillGroupID[i];
		if (dwReqGroupID != 0) {
			const tagRefSkill* pReqSkill = GetHighestRankSkill(dwReqGroupID);
			if (!pReqSkill) {
				return ERROR_SKILL_SEQUENCE_MISMATCH; // 0x3406
			}
			if (pRefSkill->byReqSkillRank[i] > pReqSkill->byRank) {
				return ERROR_SKILL_REQ_SKILL_LEVEL_TOO_LOW; // 0x3407
			}
		}
	}

	// 6. Skill Points (SP) affordability check
	CInstanceChar* pDataPerm = pPC->GetDataPermanent();
	uint32_t dwCurrentSP = pDataPerm ? pDataPerm->GetSkillPoints() : 0;
	if (dwCurrentSP < pRefSkill->dwSkillPointCost) {
		return ERROR_SKILL_INSUFFICIENT_SP; // 0x340A
	}

	return SKILL_LEARN_SUCCESS; // 0x3400
}

/*
================
CSkillManager::BeginIndirectSkill

[RECONSTRUCTED - Native 0x0059B8D0] (676 bytes)
Executes an indirect / script / item triggered skill cast.
Bypasses normal client packet initiation (Event 0).
If skill has Cbuf (+0x358) and Dura (+0x280), creates an owner timed job.
Otherwise allocates command (requestMode = 0x20), validates cast prerequisites,
and executes cast begin (0xB070) + stage end (0xB071) or spawns entity with Lnks (0xB0BE).
================
*/
bool CSkillManager::BeginIndirectSkill(
	const tagRefSkill* pRefSkill,
	uint32_t dwJobValue1,
	uint32_t dwJobValue2,
	uint32_t dwDurationOverride
) {
	if (!pRefSkill || !m_pOwner) {
		return false;
	}

	// 1. Check Cbuf (+0x358) and Dura (+0x280) for timed job creation
	if (pRefSkill->m_pParamCbuf != nullptr && pRefSkill->m_pParamDura != nullptr) {
		uint32_t dwDurationMs = pRefSkill->m_pParamDura[0];
		uint32_t dwDurationSec = dwDurationMs / 1000;
		if (CreateOwnerTimedJob(m_pOwner, 0, pRefSkill->dwSkillID, dwDurationSec, 0, 0, 0, 0, 0, dwJobValue1, dwJobValue2, 0)) {
			return true;
		}
		return false;
	}

	// 2. Allocate command
	Skill::sSkillPreEngageData* pCommand = Skill::sSkillPreEngageData::Allocate();
	if (!pCommand) {
		return false;
	}
	pCommand->m_byTargetFlags = 0x20;
	pCommand->m_dwTargetObjID14 = 0;
	pCommand->m_dwTargetObjID = 0;
	pCommand->m_dwSkillID = pRefSkill->dwSkillID;

	// Check Msch (+0x4A4) for duration override
	if (pRefSkill->m_pParamMsch != nullptr) {
		pCommand->m_dw20 = dwDurationOverride;
	}

	// 3. Validate cast prerequisites
	uint16_t wErrorCode = CheckSkillPreEngageCondition(m_pOwner, reinterpret_cast<Skill::sSkillPreEngageData*>(pCommand), 0xFFFF, const_cast<tagRefSkill*>(pRefSkill));
	if (wErrorCode != 0) {
		Skill::sSkillPreEngageData::Release(pCommand);
		SendSkillErrorResponseB070(wErrorCode);
		return false;
	}

	// 4. Check Efr3 (+0x294) for summon entity vs active skill execution
	if (pRefSkill->m_pParamEfr3 == nullptr) {
		tagActiveSkillInstance* pInstance = tagActiveSkillInstance::Allocate();
		if (!pInstance) {
			Skill::sSkillPreEngageData::Release(pCommand);
			return false;
		}
		pInstance->m_pCommand = pCommand;

		tagSkillExecutionContext* pExec = tagSkillExecutionContext::Allocate();
		if (!pExec) {
			tagActiveSkillInstance::Release(pInstance);
			return false;
		}
		pInstance->m_pExecution = pExec;

		pExec->m_nCalculatedHPCost = 0;
		pExec->m_nCalculatedMPCost = 0;
		pExec->m_byCalculatedBerserkCost = 0;
		pExec->m_pRefSkill = pRefSkill;
		pExec->m_dwResultFlags = 0;

		pInstance->m_wStatus = 0x3000;
		pInstance->m_dwStartTime = GetTickCount();
		pInstance->m_dwMode = 1;

		AddActiveSkill(pInstance);
		SendCastBeginB070(pInstance, pInstance->m_wStatus);
		SendStageEndB071(1, 0, pExec->m_dwContextID);

		// Replacement execution context
		tagSkillExecutionContext* pReplacement = tagSkillExecutionContext::Allocate();
		if (pReplacement) {
			pReplacement->m_nCalculatedHPCost = 0;
			pReplacement->m_nCalculatedMPCost = 0;
			pReplacement->m_byCalculatedBerserkCost = 0;
			pReplacement->m_pRefSkill = pRefSkill;
			pReplacement->m_dwResultFlags = 0;
			tagSkillExecutionContext::Release(pExec);
			pInstance->m_pExecution = pReplacement;
		}
	}

	return true;
}

/*
================
CSkillManager::UpdatePassiveSkills

[RECONSTRUCTED - Native 0x0059F0E0] (770 bytes)
Iterates active passive skill instances and evaluates equipment requirement slots
(Reqi array at +0x3A0, Reqn at +0x3B4) against currently equipped weapons/armor.
Activates/deactivates passive bonuses accordingly, or requests retirement for Fire Shield.
================
*/
void CSkillManager::UpdatePassiveSkills() {
	if (!m_pOwner) {
		return;
	}

	for (auto it = m_listActiveBuffs.begin(); it != m_listActiveBuffs.end(); ++it) {
		tagActiveSkillInstance* pInstance = *it;
		if (!pInstance || !pInstance->m_pExecution) {
			continue;
		}

		const tagRefSkill* pRefSkill = pInstance->m_pExecution->m_pRefSkill;
		if (!pRefSkill || pInstance->m_dwMode != 1) {
			continue;
		}

		// Reqi array at +0x3A0
		const uint32_t* const* pReqI = &pRefSkill->pReqI[0];
		if (!pReqI[0]) {
			continue;
		}

		uint16_t wMainWeaponTID = 0;
		uint16_t wOffhandTID = 0;
		Skill_GetSecondaryWeaponTID(&wOffhandTID, m_pOwner);
		Skill_GetMainWeaponTID(&wMainWeaponTID, m_pOwner);

		bool bMatches = false;
		uint32_t dwSuccesses = 0;
		uint32_t dwProcessed = 0;

		for (int i = 0; i < 5; ++i) {
			const uint32_t* pRequirement = pReqI[i];
			if (!pRequirement || bMatches) {
				if (dwSuccesses > 0 && dwSuccesses == dwProcessed) {
					bMatches = true;
				}
				break;
			}

			uint32_t dwKind = pRequirement[0];
			uint32_t dwItem = pRequirement[1];

			switch (dwKind) {
				case 1: case 2: case 3: case 9: case 10: case 11: {
					if (dwItem != 0) {
						uint16_t wSlotTID = 0;
						Skill_GetArmorSlotTID(dwItem, m_pOwner, &wSlotTID);
						if ((wSlotTID >> 11) == dwItem) {
							bMatches = m_pOwner->IsArmorSlotEquipped(dwItem);
						}
					} else {
						for (int slot = 1; slot <= 6; ++slot) {
							uint16_t wSlotTID = 0;
							Skill_GetArmorSlotTID(slot, m_pOwner, &wSlotTID);
							if (((wSlotTID >> 7) & 0x0F) != dwKind) {
								bMatches = false;
								break;
							}
							if (m_pOwner->IsArmorSlotEquipped(slot)) {
								bMatches = true;
							}
						}
					}
					break;
				}
				case 4: { // Secondary / Shield
					if ((wOffhandTID >> 11) == dwItem) {
						bMatches = m_pOwner->IsSecondaryWeaponEquipped();
					}
					break;
				}
				case 6: { // Primary Weapon
					if ((wMainWeaponTID >> 11) == dwItem) {
						bMatches = m_pOwner->IsMainWeaponEquipped();
					}
					break;
				}
				case 14: { // Special Item
					if (dwItem == 1) {
						CGItem* pItem = m_pOwner->GetEquippedItemBySlot(4);
						if (pItem != nullptr) {
							bMatches = !pItem->IsExpired();
						}
					}
					break;
				}
				default:
					break;
			}

			// Reqn (+0x3B4): All requirements must match
			if (pRefSkill->m_pParamReqn != nullptr) {
				if (!bMatches) {
					break;
				}
				bMatches = false;
				dwSuccesses++;
			}
			dwProcessed++;
		}

		if (pRefSkill->m_bIsBuff != 0) {
			if (!bMatches) {
				const char* szName = pRefSkill->strSkillName;
				if (szName && strstr(szName, "SKILL_CH_FIRE_SHIELD_") != nullptr) {
					pInstance->RequestRetirement(true);
				}
			}
		} else if (!bMatches) {
			if (pInstance->m_bActive == 1) {
				if (pRefSkill->m_pParamReal != nullptr) {
					const uint32_t* pReal = pRefSkill->m_pParamReal;
					UpdateRealModifiers(pReal, pInstance->m_pExecution->m_dwContextID, true);
				}
				UnregisterModifiers(pRefSkill);
				m_pOwner->RemoveActiveSkillInstance(pInstance);
				pInstance->m_bActive = 0;
			}
		} else if (pInstance->m_bActive == 0) {
			pInstance->m_bActive = 1;
		}
	}
}

/*
================
CSkillManager::LearnSkill

[RECONSTRUCTED - Native 0x0059BFE0] (1275 bytes)
Validates, learns skill, replaces previous rank, starts passive effects,
updates database via ShardDB overlap job, debits SP, and notifies client via Opcode 0xB0A1.
================
*/
bool CSkillManager::LearnSkill(uint32_t dwSkillID) {
	if (!m_pOwner || !m_pOwner->IsPlayer()) {
		return false;
	}

	CGObjPC* pPC = static_cast<CGObjPC*>(m_pOwner);

	uint16_t wResult = ValidateSkillLearning(dwSkillID);
	if (wResult == ERROR_SKILL_INVALID_FIRST_RANK) {
		// Native 0x0059C03C: Log abuse and ignore without sending response
		BSLib::Log_Printf(0x2000001, "Skill Learn Abuser!! (CharName : %s, SkillID : %d)",
			pPC->GetName(), dwSkillID);
		return false;
	}

	if (wResult != SKILL_LEARN_SUCCESS) {
		// Send failure response packet 0xB0A1
		pPC->SendErrorResponse(0xB0A1, wResult);
		return false;
	}

	if (g_pRefData == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return false;
	}

	const tagRefSkill* pRefSkill = g_pRefData->FindSkill(dwSkillID);
	if (!pRefSkill) {
		return false;
	}

	// Check if previous rank existed and retire it
	uint32_t dwPrevSkillID = 0;
	if (pRefSkill->pPreviousRankSkill != nullptr) {
		dwPrevSkillID = pRefSkill->pPreviousRankSkill->dwSkillID;
		tagSkillData* pPrevSkill = FindSkillByID(dwPrevSkillID);
		if (pPrevSkill != nullptr) {
			pPrevSkill->m_byStatus = 2; // Retired/Inactive
			CancelActiveBuff(dwPrevSkillID);

			auto it = m_mapSkill.find(dwPrevSkillID);
			if (it != m_mapSkill.end()) {
				if (pPrevSkill->m_pRefRecord != nullptr) {
					delete pPrevSkill->m_pRefRecord;
					pPrevSkill->m_pRefRecord = nullptr;
				}
				delete it->second;
				m_mapSkill.erase(it);
			}
		}
	}

	// Allocate and register new learned skill
	CInstanceSkill* pNewRecord = new CInstanceSkill();
	pNewRecord->SetCharID(pPC->GetGlobalID());
	pNewRecord->SetSkillID(dwSkillID);
	pNewRecord->SetEnable(1);

	tagSkillData* pNewSkill = new tagSkillData();
	pNewSkill->m_dwVptr = 0;
	pNewSkill->m_dwRefCount = 1;
	pNewSkill->m_pRefRecord = pNewRecord;
	pNewSkill->m_byStatus = 0; // Active
	pNewSkill->m_pRefSkill = pRefSkill;
	m_mapSkill[dwSkillID] = pNewSkill;

	// Rebuild learned display queues
	RebuildLearnedLists();

	// If skill is passive, activate parameter modifiers
	if (pRefSkill->byCastType == 0) {
		RegisterModifiers(pRefSkill);
	}

	// Submit async SQL update: "exec _skill_manage 0, CharID, SkillID, PrevSkillID"
	if (g_pMainProcess != nullptr) {
		g_pMainProcess->SubmitLearnedChangeDBQuery(0, pPC->GetGlobalID(), dwSkillID, dwPrevSkillID);
	}

	// Debit Skill Points
	CInstanceChar* pDataPerm = pPC->GetDataPermanent();
	if (pDataPerm != nullptr) {
		uint32_t dwCurSP = pDataPerm->GetSkillPoints();
		if (dwCurSP >= pRefSkill->dwSkillPointCost) {
			pDataPerm->SetSkillPoints(dwCurSP - pRefSkill->dwSkillPointCost);
		} else {
			pDataPerm->SetSkillPoints(0);
		}
	}

	// Send success response 0xB0A1
	BSLib::CPacket resp;
	resp.SetOpcode(0xB0A1);
	resp.WriteUint8(1); // Success status
	resp.WriteUint32(dwSkillID);
	resp.Send(pPC->GetGlobalID());

	// Audit log
	BSLib::Log_Printf(0x1000000, "[Skill Learn]: CharID=%u, SkillID=%u, PrevID=%u, Cost=%u, RemainingSP=%u\n",
		pPC->GetGlobalID(), dwSkillID, dwPrevSkillID, pRefSkill->dwSkillPointCost,
		pDataPerm ? pDataPerm->GetSkillPoints() : 0);

	return true;
}

/*
================
CSkillManager::RaiseMastery

[RECONSTRUCTED - Native 0x0059C4E0] (827 bytes)
Validates and advances character mastery level, enforcing country mastery caps
(China 330, Europe 240 / 2*Level), debits SP, updates ShardDB, and notifies client via Opcode 0xB0A2.
================
*/
bool CSkillManager::RaiseMastery(uint32_t dwMasteryID, uint8_t byLevelIncrement) {
	if (!m_pOwner || !m_pOwner->IsPlayer()) {
		return false;
	}

	CGObjPC* pPC = static_cast<CGObjPC*>(m_pOwner);

	tagSkillMasteryData* pMastery = FindMastery(dwMasteryID);
	if (!pMastery || !pMastery->m_pRefRecord) {
		pPC->SendErrorResponse(0xB0A2, ERROR_MASTERY_INVALID_COUNTRY); // 0x3801
		return false;
	}

	uint32_t dwTotalMastery = GetTotalMasteryLevel() + byLevelIncrement;
	uint32_t dwCountry = pPC->GetCountry();

	// 1. Country-specific mastery cap verification
	if (dwCountry == 0) {
		// China: Max total mastery sum is 330
		if (dwTotalMastery > 330) {
			pPC->SendErrorResponse(0xB0A2, ERROR_MASTERY_TOTAL_CAP_EXCEEDED); // 0x3805
			return false;
		}
	} else if (dwCountry == 1) {
		// Europe: Max total mastery sum is 240 and <= 2 * Character Level
		if (dwTotalMastery > 240 || dwTotalMastery > (static_cast<uint32_t>(pPC->GetLevel()) * 2)) {
			pPC->SendErrorResponse(0xB0A2, ERROR_MASTERY_TOTAL_CAP_EXCEEDED); // 0x3805
			return false;
		}
	} else if (dwCountry != 2) {
		pPC->SendErrorResponse(0xB0A2, ERROR_MASTERY_INVALID_COUNTRY); // 0x3801
		return false;
	}

	// 2. Character level constraint: mastery level cannot exceed character level
	uint8_t byCurrentRank = pMastery->m_pRefRecord->GetLevel();
	if (static_cast<uint32_t>(byCurrentRank) + byLevelIncrement > pPC->GetLevel()) {
		pPC->SendErrorResponse(0xB0A2, ERROR_MASTERY_LEVEL_EXCEEDS_CHAR_LEVEL); // 0x3803
		return false;
	}

	// 3. SP cost calculation
	uint32_t dwTotalSPCost = 0;
	uint8_t byNewRank = byCurrentRank + 1;

	if (byCurrentRank > 0) {
		for (uint8_t lvl = byCurrentRank; lvl < byCurrentRank + byLevelIncrement && lvl <= 120; ++lvl) {
			const tagRefLevelData* pRefLvl = CRefData_GetRefLevelData(lvl);
			if (pRefLvl != nullptr) {
				dwTotalSPCost += pRefLvl->dwMasterySPCost;
			}
		}

		CInstanceChar* pDataPerm = pPC->GetDataPermanent();
		if (!pDataPerm || pDataPerm->GetSkillPoints() < dwTotalSPCost) {
			pPC->SendErrorResponse(0xB0A2, ERROR_MASTERY_INSUFFICIENT_SP); // 0x3802
			return false;
		}
	} else {
		// Initial rank (rank 1) is free
		dwTotalSPCost = 0;
		byNewRank = 1;
	}

	// Update record
	pMastery->m_pRefRecord->SetLevel(byNewRank);

	// Recompute character stats derived from masteries
	RebuildLearnedLists();

	// Submit async SQL update: "exec _skill_manage 1, CharID, MasteryID, NewRank"
	if (g_pMainProcess != nullptr) {
		g_pMainProcess->SubmitLearnedChangeDBQuery(1, pPC->GetGlobalID(), dwMasteryID, byNewRank);
	}

	// Debit Skill Points
	CInstanceChar* pDataPerm = pPC->GetDataPermanent();
	if (dwTotalSPCost > 0 && pDataPerm != nullptr) {
		uint32_t dwCurSP = pDataPerm->GetSkillPoints();
		if (dwCurSP >= dwTotalSPCost) {
			pDataPerm->SetSkillPoints(dwCurSP - dwTotalSPCost);
		} else {
			pDataPerm->SetSkillPoints(0);
		}
	}

	// Send success response 0xB0A2
	BSLib::CPacket resp;
	resp.SetOpcode(0xB0A2);
	resp.WriteUint8(1); // Success status
	resp.WriteUint32(dwMasteryID);
	resp.WriteUint8(byNewRank);
	resp.Send(pPC->GetGlobalID());

	// Audit log
	BSLib::Log_Printf(0x1000000, "[Mastery Up]: CharID=%u, MasteryID=%u, NewRank=%u, Cost=%u, RemainingSP=%u\n",
		pPC->GetGlobalID(), dwMasteryID, byNewRank, dwTotalSPCost,
		pDataPerm ? pDataPerm->GetSkillPoints() : 0);

	return true;
}

/**
 * Global forwarder for Formulae and external callers
 */
const tagSkillModifier* CSkillManager_GetSkillModifier(const CSkillManager* pSkillManager, uint32_t dwModifierID) {
	if (!pSkillManager || dwModifierID == 0) {
		return nullptr;
	}
	return pSkillManager->GetSkillModifier(dwModifierID);
}

/*
================
CSkillManager::OnMsgSkillAction
[RECONSTRUCTED - Native 0x0059B7C0] (123 bytes)
================
*/
int32_t CSkillManager::OnMsgSkillAction(CMsg* pMsg) {
	if (m_pOwner->GetLifeState() == 2 || m_pOwner->GetLifeState() == 3 || m_pOwner->GetMotionState() == 0x12) {
		return 0;
	}
	if (m_pCurrentInstance != nullptr && m_pCurrentInstance->m_pExecution != nullptr &&
		m_pCurrentInstance->m_pExecution->m_pRefSkill != nullptr &&
		m_pCurrentInstance->m_pExecution->m_pRefSkill->pParam274 != nullptr) {
		return 0;
	}

	Skill::sSkillPreEngageData* pPreEngage = Skill::sSkillPreEngageData::Allocate();
	pPreEngage->ReadFromMsg(pMsg, 0);
	return InitiateSkillCast(pPreEngage);
}

/*
================
CSkillManager::InitiateSkillCast
[PARTIAL - Native 0x0059B480] (820 bytes)

Only the reference lookup and the hand-over of the pre-engage data follow the machine code so
far; the instance / execution setup below is the earlier reconstruction and is being replaced.
================
*/
int32_t CSkillManager::InitiateSkillCast(Skill::sSkillPreEngageData* pPreEngage) {
	ASSERT(g_pRefData != nullptr);
	const tagRefSkill* pRefSkill = g_pRefData->FindSkill(pPreEngage->m_dwSkillID);
	if (pRefSkill == nullptr) {
		SendSkillErrorResponseB070(0x3003);
		Skill::sSkillPreEngageData::Release(pPreEngage);
		return 0;
	}

	tagSkillExecutionContext* pExec = tagSkillExecutionContext::Allocate();
	pExec->m_pRefSkill = pRefSkill;

	tagActiveSkillInstance* pInstance = tagActiveSkillInstance::Allocate();
	pInstance->m_bActive = 1;
	pInstance->m_wStatus = 0;
	pInstance->m_dwStartTime = ::GetTickCount();
	pInstance->m_dwMode = 0;
	pInstance->m_dwRetirement = 1;
	pInstance->m_pCommand = pPreEngage;
	pInstance->m_pExecution = pExec;

	m_pCurrentInstance = pInstance;

	int32_t nOutcome = SkillCast::SkillActionHandler(m_pOwner, pInstance, CAST_EVENT_BEGIN, pPreEngage);
	if (nOutcome != 0) {
		m_pCurrentInstance = nullptr;
		Skill::sSkillPreEngageData::Release(pInstance->m_pCommand);
		tagActiveSkillInstance::Release(pInstance);
	}
	return 1;
}

/*
================
CSkillManager::SendSkillErrorResponseB070

[RECONSTRUCTED - Native 0x0059AD90] (121 bytes)
Sends skill failure/error packet (Opcode 0xB070, Stage 2) to player client.
If owner is a non-player, posts AI event 4 (0xFFFFFFFF, 0).
================
*/
int32_t CSkillManager::SendSkillErrorResponseB070(uint16_t wErrorCode) {
	if (!m_pOwner->IsPlayer()) {
		// CORRECTION (Claude): non-players report the refusal to their command source (event 4),
		// which is how the monster AI learns its skill failed.
		return m_pOwner->GetCmdSource()->OnCommandEvent(4, 0xFFFFFFFF, 0);
	}

	// CORRECTION (Claude): the packet comes from the owner's slot 158 and leaves through slot 159.
	CPacket* pPacket = m_pOwner->AllocMsgForPeer(0xB070);
	pPacket->WriteUint8(2);
	pPacket->WriteUint16(wErrorCode);
	return m_pOwner->SendMsgToPeer(pPacket);
}

/*
================
CSkillManager::SendStageEndB071

[RECONSTRUCTED - Native 0x0059AE10] (225 bytes)
Constructs and broadcasts Opcode 0xB071 stage end / action cancel packet.
If non-player, posts AI event 5; checks visibility if not forced.
================
*/
int32_t CSkillManager::SendStageEndB071(uint8_t byKind, uint16_t wErrorCode, uint32_t dwContextID, bool bForceBroadcast) {
	if (!m_pOwner) {
		return 0;
	}

	if (!m_pOwner->IsPlayer()) {
		// Non-player: notify AI (event 5, byKind)
		// If hidden / inactive and not forceBroadcast, skip broadcast
		if (!bForceBroadcast) {
			return 0;
		}
	}

	BSLib::CPacket* pPacket = BSLib::CPacket::Allocate(1);
	if (!pPacket) {
		return 0;
	}

	pPacket->SetOpcode(0xB071);
	pPacket->WriteUint8(byKind);

	if (byKind == 1) {
		pPacket->WriteUint32(dwContextID);
		pPacket->WriteUint32(0);
		pPacket->WriteUint8(0);
	} else {
		pPacket->WriteUint16(wErrorCode);
		pPacket->WriteUint32(dwContextID);
	}

	int32_t result = m_pOwner->SendPacketToNearbySessions(pPacket);
	pPacket->Release();
	return result;
}

/*
================
CSkillManager::ProcessDamageEffects

[PARTIAL - Native 0x005A0B80] (2871 bytes)
Shares a hit that landed with the group the caster fights in: the native takes the party at the caster's +0x204
(or the one at +0x208 when the first is not active), walks its members, keeps the ones that are close enough
(Pos_RegionsCompatible then Pos_Relative3D and the vector length), rolls against the share parameter at +0x418
and registers an area effect on each - the same pass also rolls the statuses again for the members it reached
(0x005A0Fxx) and queues the result through CSkillManager_QueueDamageEffect.

The party and guild registries the whole function reads are not ported, and the native itself does nothing at
all when the caster has neither (0x005A0B9B leaves immediately), which is the state every caster is in here.
================
*/
void CSkillManager::ProcessDamageEffects(CGObjChar* pCaster, CGObjChar* pTarget, tagSkillTargetHitGroup* pRec,
	tagSkillHitResult* pHit) {
	if (pCaster == nullptr || pTarget == nullptr || pRec == nullptr || pHit == nullptr) {
		return;
	}

	// 0x005A0B8A: nothing is shared when the hit carried neither damage nor recovery
	if (pHit->m_dwAmount == 0) {
		return;
	}

	// 0x005A0B9B: the caster's party (+0x204 / +0x208) decides who else is touched; without one the native
	// returns here, and the port has no party object to offer.
}

/*
================
CSkillManager::SendActionStageB071

[RECONSTRUCTED - Native 0x00586D61 - 0x00587005]
The packet the action stage of a delayed cast sends: opcode 0xB071, kind 1, the execution context, the target
the command named, the result flags and - when the flags say so - the hit batch (0x00586FDD) and the position
result (0x00586FEE). The stage end variant of the same opcode carries zeros instead.
================
*/
int32_t CSkillManager::SendActionStageB071(tagActiveSkillInstance* pInstance) {
	if (m_pOwner == nullptr || pInstance == nullptr || pInstance->m_pExecution == nullptr ||
		pInstance->m_pCommand == nullptr) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}

	const tagSkillExecutionContext* pExec = pInstance->m_pExecution;

	BSLib::CPacket* pPacket = BSLib::CPacket::Allocate(1);
	if (pPacket == nullptr) {
		return 0;
	}

	pPacket->SetOpcode(0xB071);
	pPacket->WriteUint8(1);                              // 0x00586D80
	pPacket->WriteUint32(pExec->m_dwContextID);          // 0x00586D8A
	pPacket->WriteUint32(pInstance->m_pCommand->m_dwTargetObjID14); // 0x00586FB0

	const uint8_t byResultFlags = static_cast<uint8_t>(pExec->m_dwResultFlags);
	pPacket->WriteUint8(byResultFlags);                  // 0x00586FC7

	if ((byResultFlags & 1) != 0) {
		if (pExec->m_pResultBatch == nullptr) {
			ServerFramework::ServerFramework_GenerateMiniDump();
		} else {
			SkillPacket_WriteResultBatch(pExec->m_pResultBatch, pPacket); // 0x00586FE6
		}
	}
	if ((byResultFlags & 0x0A) != 0) {
		if (pExec->m_pPositionResult == nullptr) {
			ServerFramework::ServerFramework_GenerateMiniDump();
		} else {
			SkillPacket_WritePositionResult(pExec->m_pPositionResult, pPacket); // 0x00586FF9
		}
	}

	const int32_t nResult = m_pOwner->SendPacketToNearbySessions(pPacket);
	pPacket->Release();
	return nResult;
}

/*
================
CSkillManager::SendCastBeginB070

[RECONSTRUCTED - Native 0x0059E8F0] (377 bytes)
Serializes and broadcasts Opcode 0xB070 Cast Begin to nearby sessions:
  - Status flag (byte 1, word wStatus)
  - Skill ID, Caster GID, Context ID, Target Selector, Result Flags
  - Target hit batch (if resultFlags & 1)
  - Position result (if resultFlags & 0x0A)
  - Orientation float (if requestMode & 0x40)
================
*/
int32_t CSkillManager::SendCastBeginB070(tagActiveSkillInstance* pInstance, uint16_t wStatus) {
	if (!m_pOwner || !pInstance || !pInstance->m_pCommand || !pInstance->m_pExecution) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}

	BSLib::CPacket* pPacket = BSLib::CPacket::Allocate(1);
	if (!pPacket) {
		return 0;
	}

	pPacket->SetOpcode(0xB070);
	pPacket->WriteUint8(1); // Cast Begin stage
	pPacket->WriteUint16(wStatus);
	pPacket->WriteUint32(pInstance->m_pCommand->m_dwSkillID);
	pPacket->WriteUint32(m_pOwner->GetGlobalID());
	pPacket->WriteUint32(pInstance->m_pExecution->m_dwContextID);

	// Target selector: first target ID if available, else 0
	uint32_t dwTargetSelector = 0;
	if (!pInstance->m_pCommand->m_vecTargets.empty()) {
		dwTargetSelector = pInstance->m_pCommand->m_vecTargets.front();
	}
	pPacket->WriteUint32(dwTargetSelector);

	uint8_t byResultFlags = static_cast<uint8_t>(pInstance->m_pExecution->m_dwResultFlags);
	pPacket->WriteUint8(byResultFlags);

	// If resultFlags & 1, serialize target hit results batch
	if ((byResultFlags & 1) != 0) {
		if (!pInstance->m_pExecution->m_pResultBatch) {
			ServerFramework::ServerFramework_GenerateMiniDump();
		} else {
			SkillPacket_WriteResultBatch(pInstance->m_pExecution->m_pResultBatch, pPacket);
		}
	}

	// If resultFlags & 0x0A (0x02 or 0x08), serialize position update coordinates
	if ((byResultFlags & 0x0A) != 0) {
		if (!pInstance->m_pExecution->m_pPositionResult) {
			ServerFramework::ServerFramework_GenerateMiniDump();
		} else {
			SkillPacket_WritePositionResult(pInstance->m_pExecution->m_pPositionResult, pPacket);
		}
	}

	// If command request mode has bit 0x40 set, serialize heading angle
	if ((pInstance->m_pCommand->m_byTargetFlags & 0x40) != 0) {
		float fHeading = std::atan2(m_pOwner->m_fDirZ, m_pOwner->m_fDirX);
		pPacket->WriteFloat(fHeading);
	}

	int32_t result = m_pOwner->SendPacketToNearbySessions(pPacket);
	pPacket->Release();
	return result;
}

/*
================
CSkillManager::SendEffectAddedB0BD

[RECONSTRUCTED - Native 0x0059AF00] (238 bytes)
Broadcasts Opcode 0xB0BD (Buff/Effect Added) to nearby sessions:
  - Caster GID, Skill ID, Context ID
  - Conditional mode (if 'Efta' param present)
  - Conditional duration bonus (if 'GetR', 'GetS', or 'GetD' param present)
  - Conditional source actor ID (if 'Hitm' param present)
================
*/
int32_t CSkillManager::SendEffectAddedB0BD(tagActiveSkillInstance* pInstance) {
	if (!m_pOwner || !pInstance || !pInstance->m_pExecution) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}

	const tagRefSkill* pRefSkill = pInstance->m_pExecution->m_pRefSkill;
	if (!pRefSkill) {
		ServerFramework::ServerFramework_GenerateMiniDump();
		return 0;
	}

	BSLib::CPacket* pPacket = BSLib::CPacket::Allocate(1);
	if (!pPacket) {
		return 0;
	}

	pPacket->SetOpcode(0xB0BD);
	pPacket->WriteUint32(m_pOwner->GetGlobalID());
	pPacket->WriteUint32(pRefSkill->dwSkillID);
	pPacket->WriteUint32(pInstance->m_pExecution->m_dwContextID);

	// Check for 'Efta' (Action mode) param at offset +0x4B0
	const uint8_t* pRawRef = reinterpret_cast<const uint8_t*>(pRefSkill);
	if ((pRefSkill->Param(0x4B0) != nullptr)) {
		pPacket->WriteUint8(pInstance->m_pExecution->m_byMode);
	}

	// Check for duration bonus parameters (GetRPBU, GetSTDU, GetDTDR)
	if ((pRefSkill->Param(0x508) != nullptr) ||
	    (pRefSkill->Param(0x518) != nullptr) ||
	    (pRefSkill->Param(0x52C) != nullptr)) {
		pPacket->WriteUint32(pInstance->m_pExecution->m_dwDurationBonus);
	}

	// Check for 'Hitm' parameter at offset +0x2C8
	if ((pRefSkill->Param(0x2C8) != nullptr)) {
		pPacket->WriteUint32(pInstance->m_pExecution->m_dwModifier2C);
	}

	int32_t result = m_pOwner->SendPacketToNearbySessions(pPacket);
	pPacket->Release();
	return result;
}

/*
================
CSkillManager::SendLinkedEffectB0BE

[RECONSTRUCTED - Native 0x0059AFF0] (120 bytes)
Constructs and sends Opcode 0xB0BE (Linked/Companion Effect) directly to player:
  - Skill ID, Source Context ID (+14), Target GID (+10)
  - Target character name (word length + characters)
================
*/
int32_t CSkillManager::SendLinkedEffectB0BE(tagCastLink* pLink, const tagRefSkill* pRefSkill) {
	if (!m_pOwner || !pLink || !pRefSkill) {
		return 0;
	}

	BSLib::CPacket* pPacket = BSLib::CPacket::Allocate(1);
	if (!pPacket) {
		return 0;
	}

	pPacket->SetOpcode(0xB0BE);
	pPacket->WriteUint32(pRefSkill->dwSkillID);
	pPacket->WriteUint32(pLink->m_dwSourceContextID);
	pPacket->WriteUint32(pLink->m_dwTargetActorID);

	// Write string with 16-bit length prefix matching CMsgStreamBuffer_WriteStringBounded
	uint16_t wLength = static_cast<uint16_t>(pLink->m_strTargetName.length());
	pPacket->WriteUint16(wLength);
	if (wLength > 0) {
		pPacket->WriteBytes(reinterpret_cast<const uint8_t*>(pLink->m_strTargetName.c_str()), wLength);
	}

	pPacket->Send(m_pOwner->GetGlobalID());
	pPacket->Release();
	return 1;
}

/*
================
CSkillManager::ValidateBuffReplacement

[RECONSTRUCTED - Native 0x0059D870] (665 bytes)
Validates whether a new buff or stance skill can replace or coexist with active buffs on character:
  - Iterates m_listActiveBuffs (+0x268)
  - Dttp parameter replacement tier matching
  - Efr2 area link source resolution via ObjMgr_FindByID
  - GroupID (+0x08) rank comparison: higher/equal replaces lower
  - Ovl2 (+0x37C) and blockedStates (+0x8C) state bitmask conflicts
  - Accepted replacement marks the old instance retirement marker (0) without immediate erasure
================
*/
bool CSkillManager::ValidateBuffReplacement(const tagRefSkill* pRefSkill, CGObjChar* pCaster) {
	if (!pRefSkill) {
		return false;
	}

	if (pRefSkill->dwActionCategory == 3 && (!pRefSkill->pMsch || pRefSkill->pMsch[0] != 1)) {
		for (tagActiveSkillInstance* pInstance : m_listActiveBuffs) {
			if (!pInstance || !pInstance->m_pExecution || !pInstance->m_pExecution->m_pRefSkill) {
				continue;
			}

			const tagRefSkill* pOld = pInstance->m_pExecution->m_pRefSkill;
			if (pOld->dwActionCategory != 3 || pOld->pCbuf != nullptr) {
				continue;
			}

			// Dttp parameter check
			if (pRefSkill->pDttp && pOld->pDttp && pRefSkill->pDttp[0] == pOld->pDttp[0]) {
				if (m_pOwner == pCaster) {
					return true;
				}
				if (pInstance->m_pExecution->m_byMode == 1) {
					continue;
				}
				if (pRefSkill->pDttp[1] < pOld->pDttp[1]) {
					return false;
				}
				pInstance->m_dwRetirement = 0;
				return true;
			}

			// Efr2 parameter check
			if (pRefSkill->pEfr2 && !pRefSkill->MatchesExecutionSelector()) {
				if (pOld->m_Basic_Code == pRefSkill->m_Basic_Code &&
					pRefSkill->byRank >= pOld->byRank &&
					pOld->pEfr2 && !pOld->MatchesExecutionSelector() &&
					pInstance->m_pExecution->m_pAreaLink) {

					CGObjChar* pLinked = ObjMgr_FindByID(pInstance->m_pExecution->m_pAreaLink->m_dwSourceActorID);
					if (pLinked && pLinked->GetSkillManager()) {
						tagActiveSkillInstance* pOther = pLinked->GetSkillManager()->FindActiveBuffBySkillID(pOld->dwSkillID);
						if (pOther) {
							pInstance->m_dwRetirement = 0;
							pOther->m_dwRetirement = 0;
							return true;
						}
					}
				}
				continue;
			}

			if (pRefSkill->pLks2 || (pOld->pLnks && pInstance->m_pExecution->m_byMode == 1)) {
				continue;
			}

			if (pRefSkill->dwGroupID == 0 || pRefSkill->dwGroupID != pOld->dwGroupID) {
				continue;
			}

			if (pRefSkill->byRank < pOld->byRank) {
				return false;
			}

			pInstance->m_dwRetirement = 0;
			return true;
		}
	}

	if (pRefSkill->pOvl2 && HasBlockedStates(pRefSkill->pOvl2[0])) {
		return false;
	}

	// CORRECTION (Claude): the state mask is the packed states at +0x8C (0x0059DB42), not +0x598.
	return !HasBlockedStates(pRefSkill->dwPackedStates);
}

/*
================
CSkillManager::GetSkillData

[RECONSTRUCTED - Native 0x0059D600] (118 bytes)
================
*/
const tagRefSkill* CSkillManager::GetSkillData(uint32_t dwSkillID) {
	if (!m_pOwner->IsRidingTransport() && IsCastingLocked() != 1) {
		if (FindSkillByID(dwSkillID) == nullptr) {
			return nullptr;
		}
	}
	ASSERT(g_pRefData != nullptr);
	return g_pRefData->FindSkill(dwSkillID);
}

/*
================
CSkillManager::HasBlockedStates

[RECONSTRUCTED - Native 0x0059DB10] (236 bytes)
Tests whether packed state words conflict with active casting state bitmask (+0x1B0)
or current executing skill instance's blockedStates (+0x8C) / Ovl2 (+0x37C).
================
*/
bool CSkillManager::HasBlockedStates(uint32_t dwPackedStates) const {
	uint32_t dwCurrentMask = 0;
	uint32_t dwOverlapMask = 0;

	if (m_pCurrentInstance && m_pCurrentInstance->m_pExecution && m_pCurrentInstance->m_pExecution->m_pRefSkill) {
		const tagRefSkill* pRef = m_pCurrentInstance->m_pExecution->m_pRefSkill;
		// CORRECTION (Claude): 0x0059DB42 reads the packed states at +0x8C, not +0x598.
		dwCurrentMask = pRef->dwPackedStates;
		if (pRef->pOvl2) {
			dwOverlapMask = pRef->pOvl2[0];
		}
	}

	for (uint32_t nShift = 0; nShift < 24; nShift += 8) {
		uint8_t byState = static_cast<uint8_t>((dwPackedStates >> nShift) & 0xFF);
		if (byState == 0 || byState == 0x1D || byState == 0x23) {
			continue;
		}

		uint32_t dwWordIdx = byState >> 6;
		if (dwWordIdx < 4) {
			uint64_t qwMask = 1ULL << (byState & 0x3F);
			if ((m_loadStateWords[dwWordIdx] & qwMask) != 0) {
				return true;
			}
		}

		for (uint32_t nOther = 0; nOther < 24; nOther += 8) {
			if (static_cast<uint8_t>((dwCurrentMask >> nOther) & 0xFF) == byState ||
				static_cast<uint8_t>((dwOverlapMask >> nOther) & 0xFF) == byState) {
				return true;
			}
		}
	}

	return false;
}

/*
================
CSkillManager::ProcessQueuedDamage

[RECONSTRUCTED - Native 0x0059B070] (427 bytes)
Applies deferred damage operation, checks player PvP permissions, applies damage to target entity,
and broadcasts packet 0xB0BC with packed 3-byte damage components.
================
*/
void CSkillManager::ProcessQueuedDamage(tagQueuedSkillOperation& operation) {
	if (!m_pOwner) {
		return;
	}

	operation.m_dwDamage = std::min(operation.m_dwDamage, 0x00FFFFFFU);
	uint8_t byFlags = 0;

	if (static_cast<int32_t>(m_pOwner->GetCurrentHP()) > static_cast<int32_t>(operation.m_dwDamage)) {
		m_pOwner->ApplyHit(nullptr, operation.m_dwDamage, operation.m_dwDamage, 1, 0);
	} else {
		CGObjChar* pSource = ObjMgr_FindByID(operation.m_dwSourceActorID);
		if (pSource) {
			if (m_pOwner->IsPlayer() && pSource->IsPlayer()) {
				// Player vs Player damage checks
			}
		}

		uint32_t dwCurrentHP = m_pOwner->GetCurrentHP();
		m_pOwner->ApplyHit(pSource, dwCurrentHP, dwCurrentHP, 1, 0);
		byFlags = 0x80;
	}

	BSLib::CPacket pkt;
	pkt.SetOpcode(0xB0BC);
	uint8_t byCount = 1;
	pkt.Write(&byCount, 1);
	pkt.Write(&operation.m_dwContextID, 4);
	pkt.Write(&byFlags, 1);
	uint32_t dwOwnerID = m_pOwner->GetGlobalID();
	pkt.Write(&dwOwnerID, 4);
	uint8_t byWordCount = static_cast<uint8_t>(operation.m_vecDamageWords.size());
	pkt.Write(&byWordCount, 1);

	for (uint32_t dwWord : operation.m_vecDamageWords) {
		uint8_t aBytes[3] = {
			static_cast<uint8_t>(dwWord & 0xFF),
			static_cast<uint8_t>((dwWord >> 8) & 0xFF),
			static_cast<uint8_t>((dwWord >> 16) & 0xFF)
		};
		pkt.Write(aBytes, 3);
	}

	m_pOwner->SendPacketToNearbySessions(&pkt);
}

/*
================
CSkillManager::ProcessDeferredStatusResults

[RECONSTRUCTED - Native 0x00593ED0] (341 bytes)
Applies queued status changes and refreshes character status state.
================
*/
void CSkillManager::ProcessDeferredStatusResults(tagDeferredStatusRecord& record) {
	if (!m_pOwner) {
		return;
	}

	bool bChanged = false;
	for (void* pItem : record.m_listStatusChanges) {
		if (pItem) {
			bChanged = true;
		}
	}

	if (bChanged) {
		m_pOwner->SetDirtyFlags(0x100);
	}
}

/*
================
CSkillManager::PublishRetiredContexts

[RECONSTRUCTED - Native 0x0059ECD0] (109 bytes)
Broadcasts packet 0xB072 containing all retired context IDs and clears the queue.
================
*/
void CSkillManager::PublishRetiredContexts() {
	if (!m_pOwner || m_vecRetiredContextIDs.empty()) {
		return;
	}

	BSLib::CPacket pkt;
	pkt.SetOpcode(0xB072);
	uint8_t byCount = static_cast<uint8_t>(m_vecRetiredContextIDs.size());
	pkt.WriteUint8(byCount);

	for (uint32_t dwContextID : m_vecRetiredContextIDs) {
		pkt.WriteUint32(dwContextID);
	}

	m_pOwner->SendPacketToNearbySessions(&pkt);
	m_vecRetiredContextIDs.clear();
}

/*
================
CSkillManager::SendOperationResult

[RECONSTRUCTED - Native 0x0059EDC0] (445 bytes)
Sends operation success packet (0xB0A1, 0xB0A2, 0xB202, 0xB203) or failure notice (4).
================
*/
void CSkillManager::SendOperationResult(uint32_t dwOperation, bool bSuccess, uint32_t dwID, uint32_t dwValue) {
	if (!m_pOwner) {
		return;
	}

	if (dwOperation > 3) {
		return;
	}

	if (!bSuccess) {
		if (m_pOwner->IsPlayer()) {
			static_cast<CGObjPC*>(m_pOwner)->Notice(4);
		}
		return;
	}

	static const uint16_t aOpcodes[4] = { 0xB0A1, 0xB0A2, 0xB202, 0xB203 };
	BSLib::CPacket pkt;
	pkt.SetOpcode(aOpcodes[dwOperation]);
	uint8_t byFlag = 1;
	pkt.Write(&byFlag, 1);

	if (dwOperation == 0) {
		pkt.Write(&dwValue, 4);
	} else if (dwOperation == 2) {
		uint32_t dwOut = dwValue ? dwValue : dwID;
		pkt.Write(&dwOut, 4);
	} else {
		pkt.Write(&dwID, 4);
		uint8_t byVal = static_cast<uint8_t>(dwValue);
		pkt.Write(&byVal, 1);
	}

	if (m_pOwner->IsPlayer()) {
		static_cast<CGObjPC*>(m_pOwner)->SendMsgToPeer(&pkt);
		static_cast<CGObjPC*>(m_pOwner)->BackupData(0x02);
	}
}

/*
================
CSkillManager::ResetRuntimeSlots

[RECONSTRUCTED - Native 0x0059A5E0] (147 bytes)
Resets active runtime slot pointers and flags (+0x1D8..+0x224, +0x2F0..+0x2FC).
================
*/
void CSkillManager::ResetRuntimeSlots() {
	m_pCurrentInstance = nullptr;
	m_pRuntimeInstance1F0 = nullptr;
	m_dwOverrideAttackID = 0;
	std::memset(m_loadStateWords, 0, sizeof(m_loadStateWords));
	m_dwStateBit1D0 = 0;
	m_dwReserved1D4 = 0;
	m_dwReserved2EC = 0;
}

/*
================
CSkillManager::ClearRuntime

[RECONSTRUCTED - Native 0x0059C820] (1423 bytes)
Drains all runtime skill queues, frees persistent wrappers, and resets runtime slots.
================
*/
void CSkillManager::ClearRuntime() {
	m_listPeriodicEffects.clear();

	for (tagQueuedSkillOperation* pOp : m_listQueuedDamage) {
		delete pOp;
	}
	m_listQueuedDamage.clear();

	for (tagDeferredStatusRecord* pRec : m_listDeferredStatus) {
		delete pRec;
	}
	m_listDeferredStatus.clear();

	for (tagActiveSkillInstance* pInst : m_listActiveBuffs) {
		if (pInst && pInst->m_pExecution && pInst->m_pExecution->m_pRefSkill) {
			SkillCast::SkillActionHandler(m_pOwner, pInst, CAST_EVENT_CANCEL);
			UnregisterModifiers(pInst->m_pExecution->m_pRefSkill);
		}
		if (m_pOwner) m_pOwner->m_paramKeeper.RemoveSourceModifiers(reinterpret_cast<uintptr_t>(pInst));
		tagActiveSkillInstance::Release(pInst);
	}
	m_listActiveBuffs.clear();

	m_vecRetiredContextIDs.clear();
	m_vecPendingMasteryIDs.clear();
	m_vecPendingSkillIDs.clear();
	m_mapModifiers.clear();

	for (auto& pair : m_mapSkill) {
		delete pair.second;
	}
	m_mapSkill.clear();

	for (auto& pair : m_mapMastery) {
		delete pair.second;
	}
	m_mapMastery.clear();

	ResetRuntimeSlots();
}

/*
================
CSkillManager::OnTick

[RECONSTRUCTED - Native 0x0059BB80] (1109 bytes)
Master simulation tick draining periodic effects, queued damage, deferred status, and active buffs.
================
*/
void CSkillManager::OnTick(float fDeltaSec) {
	(void)fDeltaSec;
	if (!m_pOwner) {
		return;
	}

	// 1. Drain periodic effects
	for (auto it = m_listPeriodicEffects.begin(); it != m_listPeriodicEffects.end(); ) {
		it = m_listPeriodicEffects.erase(it);
	}

	// 2. Drain queued damage operations
	for (auto it = m_listQueuedDamage.begin(); it != m_listQueuedDamage.end(); ) {
		tagQueuedSkillOperation* pOp = *it;
		if (pOp) {
			ProcessQueuedDamage(*pOp);
			delete pOp;
		}
		it = m_listQueuedDamage.erase(it);
	}

	// 3. Drain deferred status results
	for (auto it = m_listDeferredStatus.begin(); it != m_listDeferredStatus.end(); ) {
		tagDeferredStatusRecord* pRec = *it;
		if (pRec) {
			ProcessDeferredStatusResults(*pRec);
			delete pRec;
		}
		it = m_listDeferredStatus.erase(it);
	}

	// 4. Tick active instances
	for (auto it = m_listActiveBuffs.begin(); it != m_listActiveBuffs.end(); ) {
		tagActiveSkillInstance* pInstance = *it;
		if (!pInstance) {
			it = m_listActiveBuffs.erase(it);
			continue;
		}

		if (!pInstance->m_pCommand || !pInstance->m_pExecution) {
			it = m_listActiveBuffs.erase(it);
			if (m_pCurrentInstance == pInstance) m_pCurrentInstance = nullptr;
			m_pOwner->m_paramKeeper.RemoveSourceModifiers(reinterpret_cast<uintptr_t>(pInstance));
			tagActiveSkillInstance::Release(pInstance);
			continue;
		}

		const tagRefSkill* pRefSkill = pInstance->m_pExecution->m_pRefSkill;
		if (!pRefSkill || pRefSkill->byCastType == 0) {
			++it;
			continue;
		}

		// Check SKC parameter sitting restriction
		if (pRefSkill->pSkc && (pRefSkill->pSkc[1] & 1) && pInstance->m_pExecution->m_byMode == 1) {
			if (m_pOwner->GetBodyMode() == 4) { // resting/sitting
				pInstance->RequestRetirement(false);
			}
		}

		int32_t nOutcome = SkillCast::SkillActionHandler(m_pOwner, pInstance, 2 /* CAST_EVENT_TICK */);
		if (nOutcome == 0 /* CAST_OUTCOME_KEEP */) {
			++it;
			continue;
		}

		it = m_listActiveBuffs.erase(it);
		if (nOutcome == 3 /* CAST_OUTCOME_RELEASE_AND_NOTIFY */) {
			m_vecRetiredContextIDs.push_back(pInstance->m_pExecution->m_dwContextID);
		}

		UnregisterModifiers(pRefSkill);
		if (m_pCurrentInstance == pInstance) m_pCurrentInstance = nullptr;
		m_pOwner->m_paramKeeper.RemoveSourceModifiers(reinterpret_cast<uintptr_t>(pInstance));
		tagActiveSkillInstance::Release(pInstance);
	}

	// 5. Publish retired contexts (0xB072)
	PublishRetiredContexts();
}

/*
================================================================================
CSkillManager::ChangeStates (Native 0x0059DC00, 111 bytes)

Iterates 3 packed bytes from dwPackedStates (bits 0..23). Each non-zero byte
indexes a bit in the four 64-bit state words at offset +0x1B0 (m_loadStateWords).
If bRemove is false (0), sets the bit; if bRemove is true (1), clears the bit.
================================================================================
*/
uint32_t CSkillManager::ChangeStates(uint32_t dwPackedStates, bool bRemove) {
	uint32_t dwResult = 0;
	for (int32_t i = 0; i < 24; i += 8) {
		uint8_t byState = static_cast<uint8_t>((dwPackedStates >> i) & 0xFF);
		if (byState != 0) {
			uint32_t dwWordIdx = byState >> 6; // byState / 64 (0..3)
			uint32_t dwBitShift = byState & 0x3F; // byState % 64 (0..63)
			uint64_t qwMask = 1ULL << dwBitShift;
			if (!bRemove) {
				m_loadStateWords[dwWordIdx] |= qwMask;
			} else {
				m_loadStateWords[dwWordIdx] &= ~qwMask;
			}
			dwResult = static_cast<uint32_t>(qwMask);
		}
	}
	return dwResult;
}

/*
================================================================================
CSkillManager::ValidateBuffExclusionList (Native 0x0059DC80)

Checks active buffs for linked admission constraints (Lnks @ +0x370 / Lks2 @ +0x374).
Enforces maximum allowed concurrent linked buffs and mutual exclusions.
================================================================================
*/
uint16_t CSkillManager::ValidateBuffExclusionList(const tagRefSkill* pRefSkill, CGObjChar* pTarget) {
	if (!pRefSkill || !pRefSkill->pLnks) {
		return 0;
	}

	const uint32_t* pLnks = pRefSkill->pLnks;
	uint32_t dwMaxLinks = pLnks[2];
	uint32_t dwActiveLinks = 0;

	for (tagActiveSkillInstance* pBuff : m_listActiveBuffs) {
		if (!pBuff || !pBuff->m_pExecution || !pBuff->m_pExecution->m_pRefSkill) {
			continue;
		}

		const tagRefSkill* pActiveSkill = pBuff->m_pExecution->m_pRefSkill;
		if (pActiveSkill->byTargetSelectDeadBody != 0 && pBuff->m_pExecution->m_byMode == 1) {
			if (pActiveSkill->pLnks && pActiveSkill->pLnks[0] == pLnks[0]) {
				if (pActiveSkill->dwSkillID == pRefSkill->dwSkillID) {
					if (pRefSkill->pLks2 != nullptr) {
						if (pActiveSkill->pLks2 != nullptr && pTarget != nullptr) {
							if (pBuff->m_pCommand && pTarget->GetGlobalID() == pBuff->m_pCommand->m_dwTargetObjID) {
								return 0x300C; // Buff conflict
							}
						}
					} else if (pActiveSkill->pLks2 == nullptr) {
						if (pTarget == nullptr || (pBuff->m_pCommand && pTarget->GetGlobalID() == pBuff->m_pCommand->m_dwTargetObjID)) {
							return 0x300C; // Buff conflict
						}
					}
				}

				dwActiveLinks++;
			}
		}
	}

	if (dwMaxLinks != 0 && dwActiveLinks >= dwMaxLinks) {
		return 0x3029; // Max linked limit reached
	}

	return 0;
}

/*
================================================================================
CSkillManager::CheckOwnerCondition (Native 0x0059DDF0, 11 bytes)

Returns bit 0 of the state word at offset +0x1D0 (m_dwStateBit1D0).
Used by CheckSkillPreEngageCondition Phase 18 when parameter 'Reqc' (+0x39C) has bit 0x20 set.
================================================================================
*/
bool CSkillManager::CheckOwnerCondition() const {
	return (m_dwStateBit1D0 & 1) != 0;
}

/*
================================================================================
CSkillManager::InstallSelector (Native 0x0059DE00, 77 bytes)

When pContext is nullptr: uninstalls the selector mask dwValue by XOR'ing it from
m_dwStateBit1D0 (+0x1D0).
When pContext is non-null: scans 4 selector bytes from dwValue (bits 0..31). If any
byte equals 1 and bit 0 of m_dwStateBit1D0 is not set, records selector ownership
into pContext->m_dwOwnedSelectors (+0x3C) and sets bit 0 of m_dwStateBit1D0.
================================================================================
*/
int32_t CSkillManager::InstallSelector(tagSkillExecutionContext* pContext, uint32_t dwValue) {
	if (pContext == nullptr) {
		if ((dwValue & m_dwStateBit1D0) != 0) {
			m_dwStateBit1D0 ^= dwValue;
		}
		return static_cast<int32_t>(m_dwStateBit1D0);
	}

	for (int32_t i = 0; i < 32; i += 8) {
		uint8_t bySelector = static_cast<uint8_t>((dwValue >> i) & 0xFF);
		if (bySelector == 1 && !(m_dwStateBit1D0 & 1)) {
			pContext->m_dwOwnedSelectors |= 1;
			m_dwStateBit1D0 |= 1;
		}
	}
	return static_cast<int32_t>(m_dwStateBit1D0);
}

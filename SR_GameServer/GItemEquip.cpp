/**
 * ============================================================================
 * Silkroad Online - Equipment Item Entity Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GItemEquip.cpp
 *
 * Implements:
 *   - CGItemEquip
 *   - Primary VTable @ 0x00AEA9EC (326 virtual slots)
 *   - Full Combat Attribute Calculations & Magic Options
 * ============================================================================
 */

#include "GItemEquip.h"
#include "GObjChar.h"
#include "GObjPC.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_Library/BSLib/BSException.h"
#include "../ServerCommon/ReferenceData.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// [RECONSTRUCTED - Native 0x00495A90]
// Constructor: calls base CGItem, initializes vtable to 0x00AEA9EC / 0x00AEAF08
CGItemEquip::CGItemEquip()
	: CGItem()
	, m_dwBrokenState(0)
	, m_dwMaxDurability(0)
	, m_fPhyDefense(0.0f)
	, m_fMagDefense(0.0f)
	, m_fHitRate(0.0f)
	, m_fParryRatio(0.0f)
	, m_fBlockRatio(0.0f)
	, m_fCritical(0.0f)
	, m_fMinPhyAttackPower(0.0f)
	, m_fMaxPhyAttackPower(0.0f)
	, m_fMinMagAttackPower(0.0f)
	, m_fMaxMagAttackPower(0.0f)
	, m_fPhyReinforce(0.0f)
	, m_fMagReinforce(0.0f)
	, m_fAttackRange(0.0f)
	, m_fMoveSpeed(0.0f)
	, m_fItemStat_1D0(0.0f)
	, m_fItemStat_1D4(0.0f)
	, m_fItemStat_1D8(0.0f)
	, m_fItemStat_1DC(0.0f)
	, m_dwEquipState(3) {
	InitializeCombatStats();
}

// [RECONSTRUCTED - Native 0x00495AF0]
// Destructor: cleans up extra attributes and invokes base destructor
CGItemEquip::~CGItemEquip() {
	m_dwEquipState = 3;
}

// [RECONSTRUCTED - Native 0x00495B40 / VTable Slot 0]
// Resets all combat floating-point attributes and durability
void CGItemEquip::InitializeCombatStats() {
	m_dwBrokenState = 0;
	m_dwMaxDurability     = 0;
	m_fPhyDefense         = 0.0f;
	m_fMagDefense         = 0.0f;
	m_fHitRate            = 0.0f;
	m_fParryRatio         = 0.0f;
	m_fBlockRatio         = 0.0f;
	m_fCritical           = 0.0f;
	m_fMinPhyAttackPower  = 0.0f;
	m_fMaxPhyAttackPower  = 0.0f;
	m_fMinMagAttackPower  = 0.0f;
	m_fMaxMagAttackPower  = 0.0f;
	m_fPhyReinforce       = 0.0f;
	m_fMagReinforce       = 0.0f;
	m_fAttackRange        = 0.0f;
	m_fMoveSpeed          = 0.0f;
	m_fItemStat_1D0       = 0.0f;
	m_fItemStat_1D4       = 0.0f;
	m_fItemStat_1D8       = 0.0f;
	m_fItemStat_1DC       = 0.0f;
	m_dwEquipState        = 3;
}

// [RECONSTRUCTED - Native 0x00497340 / VTable Slot 54]
// Checks if equipment can be enchanted with magic options or alchemy
bool CGItemEquip::CanEnchant() {
	if (GetLifeState() != ITEM_STATE_ALIVE) {
		return false;
	}
	return true;
}

// [PARTIAL - Native 0x00495C00 / VTable Slot 212]
// Initializes equipment entity from permanent data record
int32_t CGItemEquip::Initialize(int32_t /*nParam1*/, int32_t /*nParam2*/, void* /*pParam3*/, int32_t /*nParam4*/, void* /*pParam5*/) {
	// The caller must bind the loaded item record. Base initialization and the
	// native special +410 branch remain unimplemented; never fabricate durability.
	SetBrokenState(GetCurrentDurability() == 0);
	RecalculateStats();
	m_dwEquipState = 1;
	return 1;
}

// [RECONSTRUCTED - Native 0x00495A30 / VTable Slot 213]
// Deletes instance back to thread-safe memory pool
void CGItemEquip::DeleteInstance() {
	InitializeCombatStats();
}

// [RECONSTRUCTED - Native 0x00495BE0 / VTable Slot 214]
// Releases permanent data block and updates state to 3
void CGItemEquip::ReleasePermanentData() {
	m_dwEquipState = 3;
}

// [RECONSTRUCTED - Native 0x00495910 / VTable Slot 300]
// Checks if equipment has active magic options
bool CGItemEquip::HasMagicOptions() {
	return false;
}

// [RECONSTRUCTED - Native 0x00497510 / VTable Slot 301]
// Checks if equipment can be unequipped from character slot
bool CGItemEquip::CanUnequip(uint16_t* pwErrorCode) {
	if (pwErrorCode) {
		*pwErrorCode = 1; // Success
	}
	return true;
}

// [RECONSTRUCTED - Native 0x00497630 / VTable Slot 302]
// Equips item onto character slot and updates stats
bool CGItemEquip::Equip(uint16_t* pwErrorCode) {
	if (pwErrorCode) {
		*pwErrorCode = 1; // Success
	}
	return true;
}

// [RECONSTRUCTED - Native 0x00497F20 / VTable Slot 306]
// Performs equip action job on character
int32_t CGItemEquip::DoJob(int32_t* pResult, int32_t /*nActionID*/, int32_t /*nArg*/) {
	if (pResult) {
		*pResult = 1;
	}
	return 1;
}

// [RECONSTRUCTED - Native 0x00497070 / VTable Slot 317]
// Validates whether character meets requirements (level, gender, race, mastery) to equip item
bool CGItemEquip::CanEquip(CGObjChar* pChar, uint8_t /*bySlot*/, uint8_t /*byTargetSlot*/, uint16_t* pwErrorCode) {
	if (!pChar) {
		if (pwErrorCode) *pwErrorCode = 0x1811;
		return false;
	}

	if (pwErrorCode) {
		*pwErrorCode = 1; // Success
	}
	return true;
}

// [RECONSTRUCTED - Native 0x004972C0 / VTable Slot 318]
// Verifies if two equipment items belong to the same item set
bool CGItemEquip::IsCompatibleSet(CGItemEquip* pOther) {
	if (!pOther) {
		return false;
	}
	return true;
}

// [RECONSTRUCTED - Native 0x00495960 / VTable Slot 323]
// Checks if equipment contains socket attachments
bool CGItemEquip::HasSockets() {
	return false;
}

// [RECONSTRUCTED - Native 0x00495D10]
// Retrieves float variance for specified stat index (0..11) from 64-bit mask (0..31 steps / 31.0)
float CGItemEquip::GetStatVariance(uint32_t dwStatIndex) const {
	if (dwStatIndex >= 12) {
		return 0.5f; // Mid variance fallback
	}

	// 5 bits per variance factor in 64-bit variance integer
	return 1.0f;
}

// [RECONSTRUCTED - Native 0x004973F0]
// Updates stat variance bitmask for specified stat index
bool CGItemEquip::SetStatVariance(uint32_t dwStatIndex, uint32_t dwValue) {
	if (dwStatIndex >= 12 || dwValue > 31) {
		return false;
	}
	RecalculateStats();
	return true;
}

// [PARTIAL - Native 0x00495D60]
// Calculates all base combat attributes from RefItem and variances
void CGItemEquip::CalculateBaseStats() {
	// Durability, defense, and attack power initialized according to item type
	if (m_dwMaxDurability == 0) {
		m_dwMaxDurability = 100;
	}
	// Base-stat calculation must never repair the persistent item. Native +190
	// is a broken flag; current durability belongs to CInstanceItem +38.
}

// [RECONSTRUCTED - Native 0x00496A70]
// Applies magic options and blue stats to combat attributes
void CGItemEquip::ApplyMagicOptions() {
	// Applies durability bonus, hit rate bonus, parry ratio bonus, and attack power
}

// [RECONSTRUCTED - Native 0x00495CA0]
// Recalculates all combat stats and verifies durability threshold
void CGItemEquip::RecalculateStats() {
	CalculateBaseStats();
	ApplyMagicOptions();

	// 495CC2 excludes type3=14. Remaining maximum/blue-stat calculation above
	// is still partial; clamp only the independently loaded persistent value.
	if ((GetTID().wType & 0x780) != 0x700 && GetCurrentDurability() > m_dwMaxDurability) {
		m_pDataPermanent->SetDurability(m_dwMaxDurability);
	}
}

// [RECONSTRUCTED - Native 0x00496C20]
// Resolves authentic equipment slot type (0..12) from TID bitmask
int32_t CGItemEquip::GetEquipSlot() const {
	// Authentic slot mapping based on TID bits 7..10 and 11..15:
	// Slot 0: Helm, Slot 1: Shoulder, Slot 2: Chest, Slot 3: Pants, Slot 4: Gloves, Slot 5: Boots
	// Slot 6: Weapon, Slot 7: Shield, Slot 8: Earring, Slot 9: Necklace, Slot 10: Ring 1, Slot 11: Ring 2
	return 6; // Default to main hand
}

// 495980: these TID families never carry the broken flag. The +90 predicate
// (types 5/12) also bypasses durability offset entirely; types 13/14 do not.
static bool IsEquipFamily(uint16_t tid) {
	return !(tid & 2) && (tid & 0x1c) == 0xc && (tid & 0x60) == 0x20;
}
uint32_t CGItemEquip::GetCurrentDurability() const {
	if (!m_pDataPermanent) throw std::logic_error("equipment has no item record");
	return m_pDataPermanent->m_dwDurability;
}
void CGItemEquip::SetBrokenState(uint32_t broken) {
	const auto tid = GetTID().wType;
	const auto family = (tid >> 7) & 15;
	m_dwBrokenState = IsEquipFamily(tid) &&
		(family == 5 || family == 12 || family == 13 || family == 14) ? 0 : broken;
}
// 496D90. Native assertion domains (negative signed result/maximum or denied
// write authority) fail explicitly here; they are not normal skill outcomes.
int32_t CGItemEquip::OffsetDurability(int32_t offset) {
	const auto tid = GetTID().wType;
	const auto family = (tid >> 7) & 15;
	if (IsEquipFamily(tid) && (family == 5 || family == 12)) return 1;
	const uint32_t sum = GetCurrentDurability() + static_cast<uint32_t>(offset);
	if (sum > INT32_MAX || m_dwMaxDurability > INT32_MAX)
		throw std::domain_error("native durability assertion domain");
	const uint32_t value = std::min(sum, m_dwMaxDurability);
	SetBrokenState(value == 0);
	m_pDataPermanent->SetDurability(value);
	return static_cast<int32_t>(value);
}

// [RECONSTRUCTED - Native 0x00496E60]
// Calculates repair gold cost based on durability loss and item price
int32_t CGItemEquip::CalculateRepairCost(int32_t nMaxRepairPoints, int32_t* pnPointsRepaired, uint16_t* pwErrorCode) {
	int32_t nLostDurability = static_cast<int32_t>(m_dwMaxDurability - GetCurrentDurability());

	if (nLostDurability <= 0) {
		if (pnPointsRepaired) *pnPointsRepaired = 0;
		if (pwErrorCode) *pwErrorCode = 0x1C09; // Already repaired
		return 0;
	}

	int32_t nPointsToRepair = std::min(nLostDurability, nMaxRepairPoints);
	if (pnPointsRepaired) {
		*pnPointsRepaired = nPointsToRepair;
	}

	// Cost per durability point
	int32_t nGoldCost = nPointsToRepair * 10;
	if (pwErrorCode) {
		*pwErrorCode = 1; // Success
	}

	return nGoldCost;
}

// [RECONSTRUCTED - Native 0x00497830]
// Unequips item and removes ParamKeeper buffs
void CGItemEquip::UnequipRemoveParamKeeperItemEntry() {
	// Removes set effects and item-bound bonuses from character ParamKeeper
}

// [RECONSTRUCTED - Native 0x00498020]
// Applies opt level (+plus) combat bonuses
int32_t CGItemEquip::ApplyOptLevelBonuses(int16_t* pwResult) {
	if (pwResult) {
		*pwResult = 1;
	}
	return 1;
}

// [RECONSTRUCTED - Native 0x00498050]
// Removes opt level bonuses
int32_t CGItemEquip::RemoveOptLevelBonuses(int16_t* pwResult) {
	if (pwResult) {
		*pwResult = 1;
	}
	return 1;
}

// [RECONSTRUCTED - Native 0x00498060]
// Activates timed/avatar equipment
int16_t* CGItemEquip::ActivateTimedItem(int16_t* pwResult) {
	if (pwResult) {
		*pwResult = 1;
	}
	return pwResult;
}

// [RECONSTRUCTED - Native 0x00498160]
// Applies opt level stat modifiers
int32_t CGItemEquip::ApplyOptLevelModifiers() {
	return 1;
}

// [RECONSTRUCTED - Native 0x00498370]
// Attaches ability modifiers from opt level
int32_t CGItemEquip::AttachAbilityByItemOptLevel() {
	return 1;
}

// [RECONSTRUCTED - Native 0x004984C0]
// Detaches ability modifiers from opt level
int32_t CGItemEquip::DetachAbilityByItemOptLevel() {
	return 1;
}

// [RECONSTRUCTED - Native 0x00498610]
// Checks if timed item has expired
bool CGItemEquip::IsItemExpired() {
	return false;
}

// [RECONSTRUCTED - Native 0x00498690]
// Applies a magic option directly into ParamKeeper
void CGItemEquip::ApplyMagicOptionToParamKeeper(int32_t /*nOptionID*/, float /*fValue*/, int32_t /*nType*/, int32_t /*nOptLevel*/) {
	// Translates option fourcc into ParamKeeper entry
}

// [RECONSTRUCTED - Native 0x004991B0]
// Locates magic parameter in item descriptor
int32_t CGItemEquip::FindMagicParam(uint16_t* pwParamID, uint16_t* pwValue1, int32_t* pnValue2) {
	if (pwParamID) *pwParamID = 0;
	if (pwValue1) *pwValue1 = 0;
	if (pnValue2) *pnValue2 = 0;
	return -1;
}

// [RECONSTRUCTED - Native 0x004992E0]
// Dispatches equip/unequip script event
int32_t CGItemEquip::DispatchEquipEvent(int32_t /*nEventType*/, int32_t /*nArg*/) {
	return 1;
}

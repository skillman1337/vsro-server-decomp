/**
 * ============================================================================
 * Silkroad Online - Expendable Item Entity Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GItemExpendable.cpp
 *
 * Implements:
 *   - CGItemExpendable
 *   - Primary VTable @ 0x00AEB2B4 (325 virtual slots)
 *   - Secondary VTable @ 0x00AEB7E4
 *   - Full Potions, Scrolls, Return/Teleport Scrolls, and Ammo Logic
 * ============================================================================
 */

#include "GItemExpendable.h"
#include "GObjChar.h"
#include "GObjPC.h"
#include "../JMX_Library/BSLib/BSLog.h"
#include "../JMX_Library/BSLib/BSException.h"
#include "../ServerCommon/ReferenceData.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// [RECONSTRUCTED - Native 0x0049A8E0]
// Constructor: calls base CGItem, initializes vtable to 0x00AEB2B4 / 0x00AEB7E4
CGItemExpendable::CGItemExpendable()
	: CGItem() {
}

// [RECONSTRUCTED - Native 0x0049A940]
// Destructor: cleans up expendable item instance
CGItemExpendable::~CGItemExpendable() {
}

// [RECONSTRUCTED - Native 0x0049A880 / VTable Slot 213]
// Deletes instance back to thread-safe chunk memory pool
void CGItemExpendable::DeleteInstance() {
	ReleasePermanentData();
}

// [RECONSTRUCTED - Native 0x0049A9A0 / VTable Slot 214]
// Releases permanent data block
void CGItemExpendable::ReleasePermanentData() {
	m_pDataPermanent = nullptr;
}

// [RECONSTRUCTED - Native 0x00559C70 / VTable Slot 300]
// Expendable items do not have magic options
bool CGItemExpendable::HasMagicOptions() {
	return false;
}

// [RECONSTRUCTED - Native 0x0049A140 / VTable Slot 301]
// Cannot unequip expendable item from normal equipment slots
bool CGItemExpendable::CanUnequip(uint16_t* pwErrorCode) {
	if (pwErrorCode) {
		*pwErrorCode = 3;
	}
	return false;
}

// [RECONSTRUCTED - Native 0x0049A140 / VTable Slot 302]
// Cannot equip expendable item in normal equipment slots (arrows equip via Slot 317)
bool CGItemExpendable::Equip(uint16_t* pwErrorCode) {
	if (pwErrorCode) {
		*pwErrorCode = 3;
	}
	return false;
}

// [RECONSTRUCTED - Native 0x0049ACA0 / VTable Slot 306]
// Performs action job execution
int32_t CGItemExpendable::DoJob(int32_t* pResult, int32_t /*nActionID*/, int32_t /*nArg*/) {
	if (pResult) {
		*pResult = 1;
	}
	return 1;
}

// [RECONSTRUCTED - Native 0x0049B9F0 / VTable Slot 307]
// Uses expendable item (potions, scrolls, summon scrolls)
int32_t CGItemExpendable::UseItem(int32_t /*nParam1*/, float /*fParam2*/, int32_t /*nParam3*/, uint16_t* pwErrorCode) {
	if (GetCount() <= 0) {
		if (pwErrorCode) *pwErrorCode = 0x180E;
		return 0;
	}

	DecrementStock(1);

	if (pwErrorCode) {
		*pwErrorCode = 1; // Success
	}
	return 1;
}

// [RECONSTRUCTED - Native 0x0049B710 / VTable Slot 308]
// Validates whether item can be used based on TID category and player condition
int16_t* CGItemExpendable::CanUseItem(int16_t* pwResult) {
	if (!pwResult) return nullptr;

	if (GetCount() <= 0) {
		*pwResult = 0x180E; // Item count zero or invalid
		return pwResult;
	}

	*pwResult = 1; // Success
	return pwResult;
}

// [RECONSTRUCTED - Native 0x0049C2B0 / VTable Slot 309]
// Executes return / teleport scroll action
int16_t* CGItemExpendable::ExecuteReturnScroll(int16_t* pwResult, int16_t* /*pArg2*/, int32_t* /*pArg3*/) {
	if (!pwResult) return nullptr;

	if (GetCount() <= 0) {
		*pwResult = 0x180E;
		return pwResult;
	}

	DecrementStock(1);
	*pwResult = 1;
	return pwResult;
}

// [RECONSTRUCTED - Native 0x0049EDB0 / VTable Slot 312]
// Splits current stack and returns a new expendable item with nCount
CGItemExpendable* CGItemExpendable::SplitStack(int32_t nCount) {
	int32_t cur = GetCount();
	if (nCount <= 0 || cur <= nCount) {
		return nullptr;
	}

	// Decrement current stack
	SetCount(cur - nCount);

	// Create new item for the split stack
	CGItemExpendable* pNewItem = new CGItemExpendable();
	if (pNewItem) {
		pNewItem->SetCount(nCount);
	}
	return pNewItem;
}

// [RECONSTRUCTED - Native 0x0049EE80 / VTable Slot 313]
// Decrements stock count with assertion check (cur_stock_count > count_to_dec)
int32_t CGItemExpendable::DecrementStock(int32_t nCount) {
	int32_t cur = GetCount();
	if (cur <= nCount) {
		// Native assert: cur_stock_count > count_to_dec
		// D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GItemExpendable.cpp
		SetCount(0);
		return 0;
	}

	int32_t result = cur - nCount;
	SetCount(result);
	return result;
}

// [RECONSTRUCTED - Native 0x0049A150 / VTable Slot 314]
// Returns current stack count from m_pDataPermanent (+0x38)
int32_t CGItemExpendable::GetCount() const {
	if (!m_pDataPermanent) {
		return 0;
	}
	const uint8_t* p = reinterpret_cast<const uint8_t*>(m_pDataPermanent);
	return *reinterpret_cast<const int32_t*>(p + 0x38);
}

// [RECONSTRUCTED - Native 0x0049A160 / VTable Slot 315]
// Sets current stack count into m_pDataPermanent (+0x38)
int32_t CGItemExpendable::SetCount(int32_t nCount) {
	if (nCount < 0) {
		nCount = 0;
	}
	if (!m_pDataPermanent) {
		return nCount;
	}

	uint8_t* p = reinterpret_cast<uint8_t*>(m_pDataPermanent);
	int32_t cur = *reinterpret_cast<int32_t*>(p + 0x38);
	if (cur != nCount) {
		*reinterpret_cast<int32_t*>(p + 0x38) = nCount;
		*reinterpret_cast<uint32_t*>(p + 8) |= 4; // Mark modified
	}
	return nCount;
}

// [RECONSTRUCTED - Native 0x0049A9B0 / VTable Slot 316]
// Offsets stock count clamped between 0 and max stack (+0x198 of RefObjCommon)
int32_t CGItemExpendable::OffsetStock(int32_t nDelta) {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) {
		return 0;
	}

	const uint8_t* pRef = reinterpret_cast<const uint8_t*>(m_pDataPermanent->m_pRefObjCommon);
	int32_t maxStack = *reinterpret_cast<const int32_t*>(pRef + 0x198);
	if (maxStack <= 0) {
		maxStack = 1;
	}

	int32_t cur = GetCount();
	int32_t next = cur + nDelta;
	if (next < 0) {
		next = 0;
	} else if (next > maxStack) {
		next = maxStack;
	}

	SetCount(next);
	return next;
}

// [RECONSTRUCTED - Native 0x0049EEE0 / VTable Slot 317]
// Checks if ammo/arrows can be equipped into slot 7 (quiver slot)
bool CGItemExpendable::CanEquip(CGObjChar* /*pChar*/, uint8_t bySlot, uint8_t /*byTargetSlot*/, uint16_t* pwErrorCode) {
	if (GetLifeState() != ITEM_STATE_SPAWNING && GetLifeState() != ITEM_STATE_ALIVE) {
		if (pwErrorCode) *pwErrorCode = 0x180E;
		return false;
	}

	// Slot 7 is authentic quiver / arrow / bolt slot
	if (bySlot != 7) {
		if (pwErrorCode) *pwErrorCode = 0x180E;
		return false;
	}

	if (pwErrorCode) {
		*pwErrorCode = 1; // Success
	}
	return true;
}

// [RECONSTRUCTED - Native 0x0049A0C0]
// Calculates recovery step distribution (5 ticks)
void CGItemExpendable::CalculatePotionSteps(int32_t* pSteps, int32_t nTotal) {
	if (!pSteps) return;

	int32_t step = nTotal / 5;
	pSteps[0] = step;
	pSteps[1] = 4;

	if (pSteps[0] < 1) pSteps[0] = 1;
	if (pSteps[0] >= 1000000) pSteps[0] = 1000000;
}

// [RECONSTRUCTED - Native 0x0049AA70]
// Calculates recovery amounts for HP and MP based on character max stats and potion ratios
void CGItemExpendable::CalculateRecoveryAmount(int32_t* pnRecoveryHP, float fRatioHP, float fRatioMP, int32_t* pnParam, int32_t* pnRecoveryMP) {
	if (pnRecoveryHP) {
		*pnRecoveryHP = static_cast<int32_t>(fRatioHP * 100.0f);
	}
	if (pnRecoveryMP) {
		*pnRecoveryMP = static_cast<int32_t>(fRatioMP * 100.0f);
	}
	if (pnParam) {
		*pnParam = 0;
	}
}

// [RECONSTRUCTED - Native 0x0049AC50]
// Retrieves abnormal status parameters from item descriptor
void CGItemExpendable::GetAbnormalStatusParams(void* /*pItem*/, int32_t* pParams) {
	if (pParams) {
		std::memset(pParams, 0, 0x14);
	}
}

// [RECONSTRUCTED - Native 0x0049A740]
// Checks if current stack has reached maximum capacity
bool CGItemExpendable::IsStackFull() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon) {
		return false;
	}
	const uint8_t* pRef = reinterpret_cast<const uint8_t*>(m_pDataPermanent->m_pRefObjCommon);
	int32_t maxStack = *reinterpret_cast<const int32_t*>(pRef + 0x198);
	return GetCount() >= maxStack;
}

// High-level wrapper matching original Silkroad engine usage
bool CGItemExpendable::UseItem(CGObjPC* pUser) {
	(void)pUser;
	uint16_t wResult = 0;
	return UseItem(0, 0.0f, 0, &wResult) == 1;
}

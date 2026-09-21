/**
 * ============================================================================
 * Silkroad Online - Game Object Item
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GItem.cpp
 *
 * Implements CGItem:
 *   - VTable @ 0x00AE9384
 *   - Native Ctor @ 0x0048F750, Dtor @ 0x0048F8A0
 *   - Native OnTick @ 0x0048F9B0 (Ground item expiration: 30.0s)
 * ============================================================================
 */

#include "GItem.h"
#include "../ServerCommon/InstanceItem.h"
#include "../ServerCommon/InstanceChar.h"
#include <cstring>

// [RECONSTRUCTED - Native 0x0048F750]
CGItem::CGItem()
	: m_pBaseVTable(nullptr)
	, m_dwGlobalID(0)
	, m_dwClassID(0)
	, m_fLocalPosX(0.0f)
	, m_fLocalPosY(0.0f)
	, m_fLocalPosZ(0.0f)
	, m_pOwner(nullptr)
	, m_dwDespawnState(0)
	, m_dwOwnerID(0)
	, m_dwDropType(0)
	, m_fDropTimer(0.0f)
	, m_strItemData()
	, m_byLifeState(ITEM_STATE_ALIVE) {
	std::memset(m_pad10, 0, sizeof(m_pad10));
	m_pCharData = nullptr;
	m_pDataPermanent = nullptr;
	std::memset(m_pad38, 0, sizeof(m_pad38));
	std::memset(m_itemData, 0, sizeof(m_itemData));
	std::memset(m_pad18D, 0, sizeof(m_pad18D));
}

// [RECONSTRUCTED - Native 0x0048F8A0]
CGItem::~CGItem() {
	m_pOwner = nullptr;
	m_dwDespawnState = 0;
	m_dwOwnerID = 0;
	m_dwDropType = 0;
	m_fDropTimer = 0.0f;
	std::memset(m_itemData, 0, sizeof(m_itemData));
}

// [RECONSTRUCTED - Native 0x00485EE0]
uint8_t CGItem::GetLifeState() const {
	return m_byLifeState;
}

// [RECONSTRUCTED - Native 0x00485B60]
void* CGItem::GetEventManager() {
	// Offset +0x40 inside CGObj holds CEventManager
	return &m_pad38[0x08]; // 0x38 + 0x08 = 0x40
}

// [RECONSTRUCTED - Native 0x0048F9B0]
// Updates ground item timers; transitions to dead/despawning after 30 seconds
void CGItem::OnTick(float fDeltaSec) {
	if (m_byLifeState != ITEM_STATE_ALIVE) {
		return;
	}

	// Native 0x0048F9E0: Accumulate ground timer
	m_fDropTimer += fDeltaSec;

	// Native 0x0048F9F8: 30.0s ground expiration threshold
	if (m_fDropTimer >= 30.0f) {
		m_byLifeState = ITEM_STATE_DEAD;
	}
}

// [RECONSTRUCTED - Native 0x00489FC0 / VTable Slot 213]
void CGItem::Destroy() {
	m_byLifeState = ITEM_STATE_DEAD;
}

// [RECONSTRUCTED - Slot 22 (+0x058)]
bool CGItem::IsSpecialtyGoods() const {
	return false;
}

// [RECONSTRUCTED - Slot 25 (+0x064)]
bool CGItem::IsEquipItem() const {
	return false;
}

// [RECONSTRUCTED - Slot 234 (+0x3A8)]
uint32_t CGItem::GetPrice() const {
	return 0;
}

// [RECONSTRUCTED - Slot 257 (+0x404)]
bool CGItem::IsTradeItem() const {
	return false;
}

// [RECONSTRUCTED - Slot 314 (+0x4E8)]
int32_t CGItem::GetCount() const {
	return 1;
}

// [RECONSTRUCTED - Slot 314 (+0x4E8) / Equip]
uint32_t CGItem::GetWeaponType() const {
	return 0;
}

uint32_t CGItem::GetGlobalID() const {
	return m_dwGlobalID;
}

void CGItem::SetGlobalID(uint32_t dwID) {
	m_dwGlobalID = dwID;
}

uint32_t CGItem::GetDespawnState() const {
	return m_dwDespawnState;
}

void CGItem::SetDespawnState(uint32_t dwState) {
	m_dwDespawnState = dwState;
}

float CGItem::GetDropTimer() const {
	return m_fDropTimer;
}

void CGItem::SetDropTimer(float fTimer) {
	m_fDropTimer = fTimer;
}

void CGItem::SetLifeState(uint8_t byState) {
	m_byLifeState = byState;
}

tagTID CGItem::GetTID() const {
	if (m_pDataPermanent == nullptr || m_pDataPermanent->m_pRefObjCommon == nullptr) {
		return tagTID(0);
	}
	return tagTID(m_pDataPermanent->m_pRefObjCommon->m_wTypeID);
}

bool CGItem::IsSkillActor() const {
	return false;
}

void CGItem::SetRecordIdentity20(uint64_t serial) {
	if (m_pDataPermanent) {
		m_pDataPermanent->SetSerial20(serial);
	}
}

uint64_t CGItem::GetRecordIdentity20() const {
	return m_pDataPermanent ? m_pDataPermanent->GetSerial20() : 0;
}

uint64_t CGItem::GetRecordIdentityC0() const {
	return m_pDataPermanent ? m_pDataPermanent->GetSerialC0() : 0;
}

void CGItem::SetRecordIdentityC0(uint64_t serial) {
	if (m_pDataPermanent) {
		m_pDataPermanent->SetSerialC0(serial);
	}
}

// Native 48F990 semantic projection: null detaches without clearing cached GID.
// Host CGObj layout is not native x86 layout; use the canonical accessor.
void CGItem::BindStorageOwner(CGObj* pOwner) {
	m_pOwner = pOwner;
	if (pOwner) {
		m_dwOwnerID = pOwner->GetGlobalID();
	}
}

CGItem* CGItem::SplitStack(int32_t /*quantity*/) {
	return nullptr;
}

// PARTIAL: does not yet use native 485E70 factory/registration or close record
// pool ownership. The runtime-ID copy below is NOT native factory equivalence.
CGItem* CGItem::CloneItemWithRecord() {
	if (!m_pDataPermanent) {
		return nullptr;
	}
	CInstanceItem* pClonedRecord = m_pDataPermanent->CloneRecord();
	if (!pClonedRecord) {
		return nullptr;
	}
	pClonedRecord->SetSerial20(0);
	CGItem* pNewItem = new CGItem();
	pNewItem->m_pDataPermanent = pClonedRecord;
	pNewItem->m_dwClassID = m_dwClassID;
	pNewItem->m_dwGlobalID = m_dwGlobalID;
	pNewItem->m_pOwner = m_pOwner;
	pNewItem->m_dwOwnerID = m_dwOwnerID;
	pNewItem->m_dwDropType = m_dwDropType;
	pNewItem->m_byLifeState = ITEM_STATE_ALIVE;
	return pNewItem;
}

// PARTIAL exceptional path: valid-reference load matches 459D80; a missing
// reference raises a host exception instead of native diagnostic continuation.
int32_t CGItem::GetMaxStack() const {
	if (!m_pDataPermanent || !m_pDataPermanent->m_pRefObjCommon)
		throw std::logic_error("item maximum stack requires reference data");
	return static_cast<int32_t>(m_pDataPermanent->m_pRefObjCommon->m_dwMaxStack);
}

int32_t CGItem::SetCount(int32_t count) {
	if (m_pDataPermanent) {
		m_pDataPermanent->m_dwDurability = count > 0 ? static_cast<uint32_t>(count) : 0;
	}
	return count;
}

uint32_t CGItem::GetRefObjID() const {
	return m_pDataPermanent ? m_pDataPermanent->m_dwRefObjID : 0;
}

bool CGItem::HasCompletePrerequisites() const {
	return false;
}

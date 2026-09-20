/**
 * ============================================================================
 * Silkroad Online - Game Object Item
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GItem.h
 *
 * Implements CGItem:
 *   - VTable @ 0x00AE9384
 *   - RTTI: .?AVCGItem@@
 *   - Derived from CGObj / CBase
 *   - Lifetime management, on-ground timers, and active updates
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GITEM_H_
#define _SR_GAMESERVER_GITEM_H_

#include <cstdint>
#include <string>
#include "GObj.h"

class CInstanceChar;

// Life state flags returned by CGItem::GetLifeState() (Native slot +0x0F8)
enum ItemLifeState : uint8_t {
	ITEM_STATE_NONE     = 0,
	ITEM_STATE_SPAWNING = 1,
	ITEM_STATE_ALIVE    = 2, // Active / on ground / tickable
	ITEM_STATE_DEAD     = 3, // Expired / picked up / pending despawn
};

/**
 * [RECONSTRUCTED - 0x0048F750 / 0x0048F8A0]
 * Silkroad Online - CGItem (Game Object Item)
 * Native VTable @ 0x00AE9384 (size: 0x190 / 400 bytes)
 */
class CGItem {
public:
	// Native 0x0048F750: Constructor
	CGItem();

	// Native 0x0048F8A0: Destructor / Release (VTable Slot 0)
	virtual ~CGItem();

	// [RECONSTRUCTED - 0x00485C90]
	tagTID GetTID() const;

	// Native Slot 62 (+0x0F8) @ 0x00485EE0: GetLifeState
	virtual uint8_t GetLifeState() const;

	// Native Slot 181 (+0x2D4) @ 0x00485B60: GetEventManager
	virtual void* GetEventManager();

	// Native Slot 211 (+0x34C) @ 0x0048F9B0: OnTick
	// Updates ground timer (despawns after 30.0s) and calls base CGObj::OnTick
	virtual void OnTick(float fDeltaSec);

	// Native Slot 213 (+0x354): Destroy / Release
	virtual void Destroy();

	// Native Slot 22 (+0x058): IsSpecialtyGoods
	virtual bool IsSpecialtyGoods() const;

	// Native Slot 25 (+0x064): IsEquipItem
	virtual bool IsEquipItem() const;

	bool IsExpired() const { return false; }

	// Native Slot 234 (+0x3A8): GetPrice
	virtual uint32_t GetPrice() const;

	// Native Slot 257 (+0x404): IsTradeItem
	virtual bool IsTradeItem() const;

	// Native Slot 314 (+0x4E8): GetCount / GetQuantity
	virtual int32_t GetCount() const;

	// Native Slot 314 (+0x4E8) / Equip: GetWeaponType
	virtual uint32_t GetWeaponType() const;

	// Helper getters & setters
	uint32_t GetGlobalID() const;
	void SetGlobalID(uint32_t dwID);

	uint32_t GetDespawnState() const;
	void SetDespawnState(uint32_t dwState);

	float GetDropTimer() const;
	void SetDropTimer(float fTimer);

	void SetLifeState(uint8_t byState);

public:
	// Exact struct layout matching native binary bytes:
	// Base CGObj layout (+0x00 - +0x14B, 332 bytes):
	void*       m_pBaseVTable;        // +0x04: Secondary vftable for CBase @ 0x00AE989C
	uint32_t    m_dwGlobalID;         // +0x08: Unique runtime entity ID
	uint32_t    m_dwClassID;          // +0x0C: Item data ID / type index
	uint8_t     m_pad10[0x14];        // +0x10 - +0x23: Reserved
	float       m_fLocalPosX;         // +0x24: World position X
	float       m_fLocalPosY;         // +0x28: World position Y
	float       m_fLocalPosZ;         // +0x2C: World position Z
	void*          m_pCharData;          // +0x30: Specialized state block
	CInstanceChar* m_pDataPermanent;     // +0x34: Pointer to permanent data descriptor
	uint8_t        m_pad38[0x114];       // +0x38 - +0x14B: Event manager (+0x40), listeners (+0x130)

	// CGItem specific fields (+0x14C - +0x18F, 68 bytes):
	uint32_t    m_dwDespawnState;     // +0x14C: Despawn state / verification flag (must be 0 on despawn)
	uint32_t    m_dwOwnerID;          // +0x150: Owner player ID or dropping entity ID
	uint32_t    m_dwDropType;         // +0x154: Drop ownership type / rule
	float       m_fDropTimer;         // +0x158: Elapsed time on ground (30.0s expiration)
	uint8_t     m_itemData[20];       // +0x15C - +0x16F: Count, durability, opt level
	std::string m_strItemData;        // +0x170 - +0x18B: Serialized extra attributes
	uint8_t     m_byLifeState;        // +0x18C: Life state (ITEM_STATE_ALIVE / ITEM_STATE_DEAD)
	uint8_t     m_pad18D[3];          // +0x18D - +0x18F: Padding to 0x190
};

#endif // _SR_GAMESERVER_GITEM_H_

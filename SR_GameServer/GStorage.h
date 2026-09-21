/**
 * ============================================================================
 * Silkroad Online - Item Storage / Inventory Container
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GStorage.h
 *
 * PARTIAL portable slot container.
 * 0x00AED060 is the native source-path string, not a function entry.
 * ============================================================================
 */

#ifndef _SR_GAMESERVER_GSTORAGE_H_
#define _SR_GAMESERVER_GSTORAGE_H_

#include <cstdint>
#include <vector>

class CGItem;
class CGObj;

class CGStorage {
public:
	CGStorage(uint32_t dwCapacity = 45);
	virtual ~CGStorage();

	// Portable container helper; native storage lifecycle/ownership is incomplete.
	// Slot item manipulation and boundaries check
	CGItem* GetItem(uint32_t dwSlot) const;
	bool SetItem(uint32_t dwSlot, CGItem* pItem);
	bool RemoveItem(uint32_t dwSlot);

	uint32_t GetCapacity() const;
	uint32_t GetItemCount() const;

	CGObj* GetOwner() const;
	void SetOwner(CGObj* pOwner);

protected:
	CGObj*               m_pOwner;
	uint32_t             m_dwCapacity;
	std::vector<CGItem*> m_vecSlots;
};

#endif // _SR_GAMESERVER_GSTORAGE_H_

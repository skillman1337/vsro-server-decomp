/**
 * ============================================================================
 * Silkroad Online - Item Storage / Inventory Container Implementation
 * Original Source: D:\WORK2005\Source\SilkroadOnline\Server\SR_GameServer\GStorage.cpp
 *
 * PARTIAL portable slot container.
 * 0x00AED060 is a source-path string, not SetItem machine code.
 * ============================================================================
 */

#include "GStorage.h"

CGObj* CGStorage::GetOwner() const { return m_pOwner; }
void CGStorage::SetOwner(CGObj* owner) { m_pOwner = owner; }

CGStorage::CGStorage(uint32_t dwCapacity)
	: m_pOwner(nullptr)
	, m_dwCapacity(dwCapacity)
{
	m_vecSlots.resize(dwCapacity, nullptr);
}

CGStorage::~CGStorage() {
	m_vecSlots.clear();
}

/*
================
CGStorage::GetItem
[PARTIAL portable container helper; no native function identity established]
================
*/
CGItem* CGStorage::GetItem(uint32_t dwSlot) const {
	if (dwSlot < m_vecSlots.size()) {
		return m_vecSlots[dwSlot];
	}
	return nullptr;
}

bool CGStorage::SetItem(uint32_t dwSlot, CGItem* pItem) {
	if (dwSlot < m_vecSlots.size()) {
		m_vecSlots[dwSlot] = pItem;
		return true;
	}
	return false;
}

bool CGStorage::RemoveItem(uint32_t dwSlot) {
	if (dwSlot < m_vecSlots.size()) {
		m_vecSlots[dwSlot] = nullptr;
		return true;
	}
	return false;
}

uint32_t CGStorage::GetItemCount() const {
	uint32_t count = 0;
	for (CGItem* pItem : m_vecSlots) {
		if (pItem != nullptr) {
			count++;
		}
	}
	return count;
}

uint32_t CGStorage::GetCapacity() const {
	return m_dwCapacity;
}

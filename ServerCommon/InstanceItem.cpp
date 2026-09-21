#include "InstanceItem.h"
// PARTIAL portable record projection: native 49A160 / 496D90 write +38 and
// dirty bit4 behind D2043C. An unavailable writer raises a host exception;
// native diagnostic/crash continuation is not claimed equivalent.
void CInstanceItem::SetDurability(uint32_t value) {
    if (value == m_dwDurability) return;
    if (!(g_bItemDBWriteAllowed & 1))
        throw std::logic_error("item record mutation disabled");
    m_dwStateFlags |= 4;
    m_dwDurability = value;
}

// PARTIAL 42E890: portable fields only; polymorphic child records at +C8 and
// descriptor/pool ownership remain unimplemented. Not the complete native copy.
CInstanceItem* CInstanceItem::CloneRecord() const {
    CInstanceItem* clone = new CInstanceItem();
    clone->m_dwDurability = m_dwDurability;
    clone->m_qwSerial20 = 0; // Newly cloned record has unpersisted serial (0) until DB commit
    clone->m_qwSerialC0 = m_qwSerialC0;
    clone->m_dwRefObjID = m_dwRefObjID;
    clone->m_dwMaxStack = m_dwMaxStack;
    clone->m_dwTID = m_dwTID;
    clone->m_strSpecialtyName = m_strSpecialtyName;
    clone->m_subState = m_subState;
    clone->m_pRefObjCommon = m_pRefObjCommon;
    clone->m_dwStateFlags = 4; // dirty flag
    return clone;
}

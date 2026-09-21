#ifndef SRO_INSTANCE_ITEM_H
#define SRO_INSTANCE_ITEM_H
#include "InstanceChar.h"
#include <stdexcept>
#include <array>
#include <optional>

// Portable projection of the record reached through item-record +D4.
// Field labels identify proven offsets, not a claim about its retail class.
struct ItemRecordState {
    uint32_t flags20 = 0;
    uint16_t word24 = 0;
    uint16_t word26 = 0;
    std::array<uint16_t, 8> value28{};
    std::array<uint16_t, 8> value38{};
    std::array<uint16_t, 8> value48{};
};
// 845A60/862DF0: six words of each value participate; trailing words do not.
bool ItemRecordStatesEqual(const ItemRecordState* first, const ItemRecordState* second);

// Partial portable record, not a packed native layout. RTTI AE0ACC / constructor
// 42A460 establish CInstanceItem : CInstanceObj. 496D90 reads durability at +38
// and marks inherited record flags +8 with bit4. Unknown columns are not modeled.
extern uint32_t g_bItemDBWriteAllowed; // native D2043C, separate from player gate
class CInstanceItem : public CInstanceObj {
public:
    uint32_t m_dwDurability = 0; // native +38: equipment durability OR expendable quantity
    uint64_t m_qwSerial20 = 0;   // native record+0x20/+0x24: 64-bit search identity
    uint64_t m_qwSerialC0 = 0;   // native record+0xC0/+0xC4: 64-bit secondary/log identity
    uint32_t m_dwRefObjID = 0;   // reference item ID
    uint32_t m_dwMaxStack = 50;  // max stack capacity
    uint32_t m_dwTID = 0;        // item type identifier
    std::string m_strSpecialtyName; // specialty trading goods name
    std::optional<ItemRecordState> m_subState; // native record +D4, not owner

    void SetDurability(uint32_t value);
    uint64_t GetSerial20() const { return m_qwSerial20; }
    void SetSerial20(uint64_t val) { m_qwSerial20 = val; }
    uint64_t GetSerialC0() const { return m_qwSerialC0; }
    void SetSerialC0(uint64_t val) { m_qwSerialC0 = val; }
    CInstanceItem* CloneRecord() const;
};
#endif

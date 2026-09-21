#include "InstanceItem.h"
#include <algorithm>

bool ItemRecordStatesEqual(const ItemRecordState* first, const ItemRecordState* second) {
    // Native 845A60 checks its first record's +D4 and the second record pointer.
    // Here both arguments are already the resolved +D4 projections.
    if (!first) return false;
    const ItemRecordState absent{};
    const ItemRecordState& rhs = second ? *second : absent;
    if (first->flags20 != rhs.flags20) return false;
    if (first->flags20 & 1) {
        if (!std::equal(first->value28.begin(), first->value28.begin()+6, rhs.value28.begin()) ||
            !std::equal(first->value38.begin(), first->value38.begin()+6, rhs.value38.begin()) ||
            first->word24 != rhs.word24) return false;
    }
    if (first->flags20 & 2) {
        if (!std::equal(first->value48.begin(), first->value48.begin()+6, rhs.value48.begin()) ||
            first->word26 != rhs.word26 || first->word24 != rhs.word24) return false;
    }
    return true;
}


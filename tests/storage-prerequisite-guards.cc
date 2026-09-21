#include "SR_GameServer/GStorageOP.h"
#include "SR_GameServer/GStorage.h"
#include "SR_GameServer/GItem.h"
#include <cassert>
#include <stdexcept>

class StackFixture final : public CGItem {
public:
    int32_t GetCount() const override { return 20; }
};

int main() {
    CGStorage storage(20);
    StackFixture source;
    CInstanceItem record;
    record.m_dwDurability = 20;
    source.m_pDataPermanent = &record;
    source.SetGlobalID(1234);
    storage.SetItem(13, &source);
    bool refused = false;
    try { CGStorageOP::SplitItemIntoEmptySlot(storage, 13, 14, 5); }
    catch (const std::logic_error&) { refused = true; }
    assert(refused);
    assert(record.m_dwDurability == 20);
    assert(storage.GetItem(13) == &source && storage.GetItem(14) == nullptr);
    assert(source.GetGlobalID() == 1234);
    refused = false;
    try { CGStorageOP::GetSplitEligibility(storage, 13, &source); }
    catch (const std::logic_error&) { refused = true; }
    assert(refused);
    assert(record.m_dwDurability == 20);
    source.m_pDataPermanent = nullptr;
}

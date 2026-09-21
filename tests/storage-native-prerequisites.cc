#include "SR_GameServer/GStorageOP.h"
#include "SR_GameServer/GStorage.h"
#include "SR_GameServer/GItem.h"
#include "SR_GameServer/GItemExpendable.h"
#include "ServerCommon/InstanceItem.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

void Test_SubtypeSplit_And_IdentityZeroing() {
    CGStorage storage(20);
    tagRefObjCommon reference{};
    reference.m_dwMaxStack = 50;
    CGObj mockOwner;
    mockOwner.m_dwGlobalID = 998877;
    storage.SetOwner(&mockOwner);

    // Setup source expendable item
    CGItemExpendable source;
    CInstanceItem sourceRecord;
    sourceRecord.m_dwDurability = 20;
    sourceRecord.m_qwSerial20 = 0x1122334455667788ULL;
    sourceRecord.m_qwSerialC0 = 0x99AABBCCDDEEFF00ULL;
    sourceRecord.m_dwRefObjID = 3630; // HP Potion
    sourceRecord.m_pRefObjCommon = &reference;
    source.m_pDataPermanent = &sourceRecord;
    source.SetGlobalID(5001);

    storage.SetItem(10, &source);

    // Split 6 items into empty slot 11
    CGItem* newItem = CGStorageOP::SplitItemIntoEmptySlot(storage, 10, 11, 6);
    assert(newItem != nullptr);

    // 1. Source count decremented to 14
    assert(source.GetCount() == 14);
    assert(sourceRecord.m_dwDurability == 14);
    // Source identity preserved
    assert(sourceRecord.GetSerial20() == 0x1122334455667788ULL);

    // 2. New item count is 6
    assert(newItem->GetCount() == 6);

    // 3. New item identity is explicitly 0 (unpersisted until DB transaction)
    assert(newItem->GetRecordIdentity20() == 0);

    // 4. Storage owner bound to new item (native 0x0048F990: owner ptr at +0x14C, global ID at +0x154)
    std::cout << "newItem->m_dwOwnerID = " << newItem->m_dwOwnerID << ", mockOwner.m_dwGlobalID = " << mockOwner.m_dwGlobalID << std::endl;
    assert(newItem->GetOwner() == &mockOwner);
    assert(newItem->m_dwOwnerID == mockOwner.m_dwGlobalID);

    newItem->BindStorageOwner(nullptr);
    assert(newItem->GetOwner() == nullptr);
    assert(newItem->m_dwOwnerID == mockOwner.m_dwGlobalID);

    // 5. Destination slot contains the new item
    assert(storage.GetItem(11) == newItem);

    // Clean up allocated clone
    storage.RemoveItem(11);
    delete newItem;
    source.m_pDataPermanent = nullptr;

    std::cout << "Test_SubtypeSplit_And_IdentityZeroing passed\n";
}

void Test_GetSplitEligibility_NativeSemantics() {
    CGStorage storage(20);
    tagRefObjCommon reference{};
    reference.m_dwMaxStack = 50;

    // Target item in slot 5
    CGItemExpendable target;
    CInstanceItem targetRecord;
    targetRecord.m_dwDurability = 30;
    targetRecord.m_qwSerial20 = 0; // Unpersisted!
    targetRecord.m_dwRefObjID = 3630;
    targetRecord.m_pRefObjCommon = &reference;
    target.m_pDataPermanent = &targetRecord;
    storage.SetItem(5, &target);

    // Incoming item
    CGItemExpendable incoming;
    CInstanceItem incomingRecord;
    incomingRecord.m_dwDurability = 15;
    incomingRecord.m_qwSerial20 = 0xAA;
    incomingRecord.m_dwRefObjID = 3630;
    incomingRecord.m_pRefObjCommon = &reference;
    incoming.m_pDataPermanent = &incomingRecord;

    // 1. Target with Serial20 == 0 must be rejected (0 eligible)
    int32_t eligible = CGStorageOP::GetSplitEligibility(storage, 5, &incoming);
    assert(eligible == 0);

    // 2. Give target a valid persistent identity
    targetRecord.m_qwSerial20 = 0xBBCCDDEE11223344ULL;
    eligible = CGStorageOP::GetSplitEligibility(storage, 5, &incoming);
    // Space is 50 - 30 = 20. Incoming count is 15. Min(20, 15) = 15.
    assert(eligible == 15);

    // 3. Different RefObjID should reject
    incomingRecord.m_dwRefObjID = 3631;
    eligible = CGStorageOP::GetSplitEligibility(storage, 5, &incoming);
    assert(eligible == 0);
    incomingRecord.m_dwRefObjID = 3630;

    // State belongs to record+D4; it is not storage owner identity.
    targetRecord.m_subState.emplace();
    incomingRecord.m_subState.emplace();
    targetRecord.m_subState->flags20 = 1;
    incomingRecord.m_subState->flags20 = 1;
    incomingRecord.m_subState->value28[5] = 4;
    assert(CGStorageOP::GetSplitEligibility(storage, 5, &incoming) == 0);
    incomingRecord.m_subState->value28[5] = 0;
    incomingRecord.m_subState->value28[7] = 4; // ignored padding words
    assert(CGStorageOP::GetSplitEligibility(storage, 5, &incoming) == 15);
    targetRecord.m_subState->flags20 = 2;
    incomingRecord.m_subState->flags20 = 2;
    assert(CGStorageOP::GetSplitEligibility(storage, 5, &incoming) == 0);
    targetRecord.m_subState.reset();
    incomingRecord.m_subState.reset();
    reference.m_byItemFlags340 = 2;
    reference.m_wTypeID = 0x006c; // ordinary expendable: flag does not prohibit merge
    assert(CGStorageOP::GetSplitEligibility(storage, 5, &incoming) == 15);
    reference.m_wTypeID = 0x7eec; // exact 492980 special subtype
    assert(CGStorageOP::GetSplitEligibility(storage, 5, &incoming) == 0);
    reference.m_byItemFlags340 = 0;
    reference.m_wTypeID = 0x006c;

    // 4. Merge operation
    int32_t merged = CGStorageOP::MergeItem(storage, 5, &incoming);
    assert(merged == 15);
    assert(target.GetCount() == 45);
    assert(incoming.GetCount() == 0);

    target.m_pDataPermanent = nullptr;
    incoming.m_pDataPermanent = nullptr;

    std::cout << "Test_GetSplitEligibility_NativeSemantics passed\n";
}

int main() {
    Test_SubtypeSplit_And_IdentityZeroing();
    Test_GetSplitEligibility_NativeSemantics();
    std::cout << "All native storage prerequisite tests passed successfully!\n";
    return 0;
}

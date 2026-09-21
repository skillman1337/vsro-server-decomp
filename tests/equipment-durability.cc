#include "SR_GameServer/GItemEquip.h"
#include "SR_GameServer/GItemExpendable.h"
#include "SR_GameServer/GObjPC.h"
#include "ServerCommon/ReferenceData.h"
#include <cassert>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    assert(argc == 2);
    std::ifstream input(argv[1]); assert(input.good());
    uint32_t tid,current,maximum,delta,oldBroken,flags,result,wantCurrent,wantBroken,wantFlags;
    size_t count=0;
    while (input>>tid>>current>>maximum>>delta>>oldBroken>>flags>>result>>wantCurrent>>wantBroken>>wantFlags) {
        CInstanceItem record; tagRefObjCommon ref{}; ref.m_wTypeID=tid;
        record.m_pRefObjCommon=&ref; record.m_dwDurability=current; record.m_dwStateFlags=flags;
        CGItemEquip item; item.m_pDataPermanent=&record;
        item.m_dwMaxDurability=maximum; item.m_dwBrokenState=oldBroken;
        const int32_t signedDelta=delta<=INT32_MAX ? int32_t(delta) : int32_t(int64_t(delta)-0x100000000LL);
        assert(uint32_t(item.OffsetDurability(signedDelta))==result);
        assert(item.GetCurrentDurability()==wantCurrent && item.m_dwBrokenState==wantBroken);
        assert(record.m_dwStateFlags==wantFlags);
        item.m_pDataPermanent=nullptr; ++count;
    }
    assert(count>100);
    CInstanceItem record; tagRefObjCommon ref{}; ref.m_wTypeID=0x32c;
    record.m_pRefObjCommon=&ref; record.m_dwDurability=1;
    CGItemEquip item; item.m_pDataPermanent=&record; item.m_dwMaxDurability=10;
    CGObjPC actor; actor.m_storage.SetItem(6,&item);
    assert(actor.IsMainWeaponUsable());
    assert(actor.GetEquippedPrimaryWeaponTID()==ref.m_wTypeID);
    item.OffsetDurability(-1);
    assert(item.IsBroken() && !actor.IsMainWeaponUsable());
    assert(actor.GetEquippedPrimaryWeaponTID()==0);
    item.RecalculateStats(); assert(item.GetCurrentDurability()==0 && item.IsBroken());
    item.OffsetDurability(1); assert(!item.IsBroken() && actor.IsMainWeaponUsable());
    g_bItemDBWriteAllowed=0;
    bool denied=false; try { item.OffsetDurability(-1); } catch(const std::logic_error&) { denied=true; }
    assert(denied && item.GetCurrentDurability()==1 && item.IsBroken());
    g_bItemDBWriteAllowed=1;
    actor.m_storage.SetItem(6,nullptr); item.m_pDataPermanent=nullptr;
    CGItemExpendable stack; stack.m_pDataPermanent=&record;
    record.m_dwDurability=20; record.m_dwStateFlags=0x20;
    assert(stack.GetCount()==20 && stack.SetCount(7)==7);
    assert(record.m_dwDurability==7 && record.m_dwStateFlags==0x24);
    g_bItemDBWriteAllowed=0;
    assert(stack.SetCount(7)==7); // unchanged writes do not require the gate
    denied=false; try { stack.SetCount(6); } catch(const std::logic_error&) { denied=true; }
    assert(denied && stack.GetCount()==7);
	denied=false; try { stack.SplitStack(2); } catch(const std::logic_error&) { denied=true; }
	assert(denied && stack.GetCount()==7); // no stock loss through the old fake clone
    g_bItemDBWriteAllowed=1; stack.m_pDataPermanent=nullptr;
    std::cout<<count<<" native durability rows and actor broken/repair/authority checks passed\n";
}

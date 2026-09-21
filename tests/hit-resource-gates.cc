#include "SR_GameServer/GObjChar.h"
#include <cassert>
#include <cstdint>
#include <iostream>
struct Actor : CGObjChar {
 uint8_t life=1;
 uint8_t GetLifeState() const override { return life; }
 uint32_t GetMaxHP() const override { return 100; }
 uint32_t GetMaxMP() const override { return 80; }
};
int main() {
 for(uint8_t life : {uint8_t(1),uint8_t(2),uint8_t(3)})
 for(bool self : {false,true})
 for(int32_t damage : {INT32_MIN,-1,0,1,43,44,INT32_MAX}) {
  Actor actor; CInstancePC record;
  actor.m_pDataPermanent=&record; actor.life=life;
  record.m_dwHP=43; record.m_dwMP=27;
  actor.m_dwPublishedHP=0; actor.m_dwPublishedMP=0;
  actor.m_wStatusDirtyFlags=0x1000;
  actor.ApplyHit(self ? &actor : nullptr,damage,999,0x10004,nullptr);
  bool change=!self && life==1 && damage>0;
  uint32_t expected=change ? (damage>=43 ? 0 : 43-damage) : 43;
  assert(actor.GetCurrentHP()==expected && actor.GetCurrentMP()==27);
  assert(actor.m_dwPublishedHP==(change ? 43u : 0u));
  assert(actor.m_wStatusDirtyFlags==(change ? 0x1004 : 0x1000));
  actor.m_pDataPermanent=nullptr;
 }
 std::cout << "42 native hit numeric/identity/life gates passed; callback closure remains separate\n";
}

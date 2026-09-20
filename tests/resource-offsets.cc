#include "SR_GameServer/GObjChar.h"
#include <fstream>
#include <iostream>
struct Actor : CGObjChar {
    uint8_t life = 1;
    uint8_t GetLifeState() const override { return life; }
    uint32_t GetMaxHP() const override { return 100; }
    uint32_t GetMaxMP() const override { return 80; }
};
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::ifstream input(argv[1]);
    uint32_t costs, life, hp, mp, oldHP, oldMP, wantHP, wantMP, cacheHP, cacheMP, dirty, count=0;
    int32_t delta;
    while (input >> costs >> life >> hp >> mp >> delta >> oldHP >> oldMP >> wantHP >> wantMP >> cacheHP >> cacheMP >> dirty) {
        Actor actor; CInstancePC record;
        actor.m_pDataPermanent = &record; actor.life = life;
        record.m_dwHP = hp; record.m_dwMP = mp;
        actor.m_dwPublishedHP = oldHP; actor.m_dwPublishedMP = oldMP;
        actor.m_wStatusDirtyFlags = 0x1000;
        if (costs) actor.ConsumeResources(delta, delta, 4);
        else actor.ApplyHealthAndManaOffset(delta, delta, 4);
        const bool ok = actor.GetCurrentHP() == wantHP && actor.GetCurrentMP() == wantMP &&
            actor.m_dwPublishedHP == cacheHP && actor.m_dwPublishedMP == cacheMP && actor.m_wStatusDirtyFlags == dirty;
        actor.m_pDataPermanent = nullptr;
        if (!ok) { std::cerr << "native resource transition mismatch " << count << '\n'; return 1; }
        ++count;
    }
    if (!input.eof() || count != 448) return 3;
    std::cout << count << " native resource/cached-vital transitions passed\n";
}

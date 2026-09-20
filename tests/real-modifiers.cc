#include "SR_GameServer/SkillRealModifiers.h"
#include <cassert>
#include <iostream>

int main() {
    SkillRealModifiers registry;
    const uint32_t all = 0xffffffff;
    registry.Update(all, 80, 7, 100, false);
    registry.Update(all, 80, 7, 101, false);
    registry.Update(all, 80, 7, 100, false);
    registry.Update(all, 0xffffffff, 7, 102, false);
    registry.Update(all, 80, 8, 100, false);
    registry.Update(all, 79, 7, 100, true); // Wrong value cannot remove it.
    registry.Update(all, 80, 7, 999, true); // Nor can another context.
    for (auto mask : SkillRealModifiers::masks) {
        auto entries = registry.Find(mask, 7);
        assert(entries && entries->size() == 4);
        assert(entries->begin()->first == 80);
        assert(entries->rbegin()->first == 0xffffffff);
    }
    registry.Update(all, 80, 7, 100, true);
    for (auto mask : SkillRealModifiers::masks) {
        auto entries = registry.Find(mask, 7);
        assert(entries && entries->size() == 3); // Remove only one duplicate.
        assert(registry.Find(mask, 8)->size() == 1);
    }
    registry.Update(all, 80, 7, 100, true);
    registry.Update(all, 80, 7, 101, true);
    registry.Update(all, 0xffffffff, 7, 102, true);
    for (auto mask : SkillRealModifiers::masks) assert(!registry.Find(mask, 7));
    assert(!registry.Find(0x1000, 8));
    assert(!registry.Find(0x800000, 8));
    registry.Update(0x1000000, 80, 8, 100, true);
    assert(!registry.Find(0x1000000, 8));
    assert(registry.Find(0x40, 8)->size() == 1);
    registry.Update(0x40, 1, 9, 103, false);
    assert(registry.Read(0x40).grade == 9 && registry.Read(0x40).value == 1);
    registry.Update(0x40, 99, 9, 104, false);
    assert(registry.Read(0x40).value == 99);
    registry.Update(0x40, 99, 9, 104, true);
    assert(registry.Read(0x40).value == 1);
    std::cout << "real modifier mask/duplicate/context/key/removal checks passed\n";
}

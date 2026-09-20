#include "SR_GameServer/GParamKeeper.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

// Focused element test: unexpected diagnostic paths fail rather than vanish.
namespace BSLib { int Log_Printf(uint32_t, const char*, ...) { throw std::runtime_error("native diagnostic"); } }
namespace ServerFramework { int ServerFramework_GenerateMiniDump() { throw std::runtime_error("invalid keeper access"); } }

int main() {
    CParamElement p;
    p.Init(5, -100000, 100000, 10, -999);
    assert(p.AddModifier(0, 11, 20));
    assert(p.AddModifier(0, 22, 30));
    assert(p.GetFloatValue() == 50);
    assert(p.AddModifier(0, 11, 25));
    assert(p.GetFloatValue() == 55);
    assert(!p.AddModifier(0, 11, -999));
    assert(p.GetFloatValue() == 55);
    p.AddModifier(1, 33, 20);
    p.AddModifier(1, 44, 30);
    assert(p.GetFloatValue() == 82.5f);
    assert(p.RemoveModifier(11) == 1);
    assert(p.GetFloatValue() == 45);
    p.RemoveModifier(22);
    assert(p.GetFloatValue() == 0); // empty flat bucket does not restore base 10

    CParamElement factor;
    factor.Init(5, -100000, 100000, -10, -999);
    factor.AddModifier(2, 1, 50);
    factor.AddModifier(2, 2, 100);
    assert(factor.GetFloatValue() == -30); // negative result still multiplies
    factor.AddModifier(2, 1, -100);
    assert(factor.GetFloatValue() == -20); // zero accumulator restarts
    factor.AddModifier(3, 3, 50);
    factor.AddModifier(3, 4, 200);
    assert(factor.GetFloatValue() == -20);
    factor.AddModifier(0, 3, 8);
    assert(factor.RemoveModifier(3) == (1 | 4)); // channel+1 OR, not bit mask

    CParamElement source, target;
    source.Init(1, 0, 1000, 0, -999);
    target.Init(5, 0, 1000, 0, -999);
    source.AddDependent(&target, 0);
    source.AddDependent(&target, 1); // must not replace first registration
    target.AddModifier(0, 7, 2);
    source.AddModifier(0, 8, 10);
    assert(target.GetFloatValue() == 12);
    source.RemoveModifier(8);
    assert(target.GetFloatValue() == 2);
    std::cout << "source replacement/removal, four reducers, sentinel and dependency channels passed\n";
}

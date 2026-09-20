#include "SR_GameServer/GObjChar.h"
#include <fstream>
#include <iostream>
#include <string>

struct RetirementActor : CGObjChar {
    bool player = false;
    std::string callbacks;
    bool IsPlayer() const override { return player; }
    void RefreshMovementSpeeds() override { callbacks += ",speed"; }
    void SendParameterStats() override { callbacks += ",stats"; }
};

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::ifstream input(argv[1]);
    unsigned player, source, mask, dirty, count = 0;
    std::string expected;
    while (input >> player >> source >> mask >> dirty >> expected) {
        RetirementActor actor;
        actor.player = player != 0;
        auto& keeper = actor.m_paramKeeper;
        unsigned ids[] = {24, 5, 23};
        std::string visits;
        for (unsigned i = 0; i < 3; ++i) {
            // Independent sources discriminate deletion from wholesale clearing.
            keeper.SetParamFloat(ids[i], 0, 999, 20.f);
            if (mask & (1u << i)) keeper.SetParamFloat(ids[i], 0, source, 10.f);
        }
        for (auto* p : keeper.m_parameterOrder) {
            if (!visits.empty()) visits += ',';
            visits += 'r' + std::to_string(p->m_dwID);
        }
        keeper.ResetDirty();
        keeper.RemoveSourceModifiers(source);
        if (visits + actor.callbacks != expected || keeper.m_bDirty != dirty) {
            std::cerr << "retirement callback mismatch " << count << '\n'; return 1;
        }
        for (auto id : ids) if (keeper.GetParamFloat(id) != 20.f) return 4;
        actor.callbacks.clear();
        keeper.RemoveSourceModifiers(source);
        if (!actor.callbacks.empty()) return 5; // duplicate retirement is silent
        ++count;
    }
    if (!input.eof() || count != 32) return 3;
    std::cout << count << " native parameter-retirement callback sequences passed\n";
}

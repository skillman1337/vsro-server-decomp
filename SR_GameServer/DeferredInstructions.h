#pragma once
#include <array>
#include <cstdint>

// Portable execution projection of 596960/596A40; not a native layout claim.
// Host methods are explicit unresolved integration boundaries until backed by
// the actor implementation. Do not treat this plan as skill admission.
// 4A6E43 loads AL=1; 4A6E6A/70 store actor+A2D/A2C during actor reset.
struct DeferredVitalLatches { bool hp = true, mp = true; };
struct DeferredInstructions {
    std::array<const uint32_t*, 8> parameters{}; // 338..350,364
    uint32_t startedAt = 0;
    bool applied = false; // native +10, shared by both modifier branches
    bool NeedsQueue() const {
        for (unsigned i=0; i<8; ++i)
            if (parameters[i] && (i>=4 || parameters[i][3]!=2)) return true;
        return false;
    }
    // False: remove this descriptor's contributions, then release it (59BB80).
    template<class Host> bool Advance(Host& host, DeferredVitalLatches& latches) {
        for (unsigned i=0; i<2; ++i) {
            const auto* p=parameters[i];
            if (!p) continue;
            bool& latch=i==0 ? latches.hp : latches.mp;
            if (uint32_t(host.NowMillis()-startedAt)>=p[0]) { latch=true; return false; }
            if (!latch) continue;
            if (p[3]==0) {
                uint32_t current=i==0 ? host.CurrentHP() : host.CurrentMP();
                int32_t amount=int32_t(uint32_t(uint64_t(uint32_t(current*p[2]))/100+p[1]));
                if (i==0) host.ApplyHit(amount);
                else host.ConsumeResources(0,amount);
            } else if (p[3]==1) {
                if (p[1]) host.WriteParameter(3+i,0,-float(p[1]));
                else if (p[2]) host.WriteParameter(3+i,1,-float(p[2]));
                if (host.IsPlayer()) host.SendParameterStats();
            }
            latch=false;
        }
        for (unsigned i=2; i<4; ++i) {
            const auto* p=parameters[i];
            if (!p) continue;
            if (uint32_t(host.NowMillis()-startedAt)>=p[0]) { applied=false; return false; }
            if (!applied) {
                if (p[3]==1) {
                    uint32_t id=i==2 ? 0xb2 : 5, channel=i==2 ? 2 : 1;
                    host.WriteParameter(id,channel,-float(p[1]));
                    host.WriteParameter(id+1,channel,-float(p[2]));
                }
                applied=true;
            }
        }
        if (parameters[4]) { host.ConsumeResources(int32_t(host.CurrentHP()-1),0); return false; }
        if (parameters[5]) { host.ConsumeResources(0,int32_t(host.CurrentMP()-1)); return false; }
        if (const auto* p=parameters[6]) {
            host.CancelActionsAndRetireSkills(); host.SetMotion(float(double(p[0])/1000)); return false;
        }
        if (const auto* p=parameters[7]) {
            if (host.IsPlayer()) host.DamageEquipment(p[0],p[1]);
            return false;
        }
        return true;
    }
};

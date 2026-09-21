#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>

// Portable control-flow projection of 63D190 and 63CCE0, not a retail layout.
// Registration owns handler lifetime through dispatch. Removing a handler
// disables it; it must not destroy pointers captured by the current snapshot.
namespace NativeEvents {
struct Registry; struct Receiver;
struct Handler {
    uint32_t phase=3; int32_t eventKey=32;
    Registry* registry=nullptr; Receiver* receiver=nullptr;
    uint32_t lastTick=0, deadline=0, interval=0;
    uint8_t enabled=1;
    void SetTiming(uint32_t now, uint32_t delay, uint32_t repeat) {
        lastTick=now; deadline=now+delay; interval=repeat; // 63CCC0
    }
    bool Due(uint32_t now) {
        lastTick=now;
        if (deadline!=0) {
            if (now<deadline) return false; // native unsigned absolute compare
            if (interval!=0) deadline+=interval; // one advance, never catch-up
        }
        return true;
    }
};
// ordered is the native equal-key range. The host supplies the argument cursor,
// virtual callback and owner-registry unregister operation without hiding them.
template<class Host> uint32_t Dispatch(const std::vector<Handler*>& ordered, Host& host) {
    if (ordered.empty()) return 2;
    std::vector<Handler*> snapshot;
    for (auto* h:ordered) {
        if (!h) throw std::logic_error("null native event handler");
        if (h->enabled==1) snapshot.push_back(h);
    }
    for (auto* h:snapshot) {
        host.ResetArguments(); // +88 reset even if an earlier callback disabled h
        if (h->enabled==0) continue;
        uint32_t result=2;
        if (h->Due(host.NowMillis())) result=host.Invoke(*h);
        switch(result) {
        case 0: case 2: break;
        case 1: return 0;
        case 3: return 3;
        case 4: host.Unregister(*h); return 4;
        default: throw std::logic_error("invalid native event callback result");
        }
    }
    return 0;
}
}

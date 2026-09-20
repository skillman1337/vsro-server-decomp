#include "SR_GameServer/TimedJob.h"
#include <cassert>
#include <iostream>
#include <vector>

// Native 656060 visits every job (virtual +30) without inspecting its result.
// Portable rejection must remain observable without skipping other records.
struct RestoreJob final : CTimedJob {
    std::vector<unsigned>& visits;
    bool& fail;
    RestoreJob(std::vector<unsigned>& v, bool& f) : visits(v), fail(f) {}
    bool Activate() override {
        visits.push_back(GetRecord()->GetJobID());
        return !fail;
    }
};
struct RestoreManager final : CTimedJobManager {
    std::vector<unsigned> visits;
    bool fail = true;
    CTimedJob* CreateJob(uint8_t, uint32_t) override {
        return new RestoreJob(visits, fail);
    }
};
int main() {
    RestoreManager manager;
    std::list<CInstanceTimedJob*> records;
    for (unsigned id : {41, 42, 43}) {
        auto* record = new CInstanceTimedJob;
        record->SetJobID(id);
        records.push_back(record);
    }
    assert(manager.LoadFromDB(nullptr, records));
    assert(records.empty());
    assert(manager.visits.empty()); // Binding must not activate effects.
    assert(!manager.ActivateLoadedJobs());
    assert(manager.visits.size() == 3);
    auto first = manager.visits;
    manager.visits.clear();
    manager.fail = false;
    assert(manager.ActivateLoadedJobs());
    assert(manager.visits == first); // Preserve native ordered-set traversal.
    manager.Clear();
    manager.visits.clear();
    assert(manager.ActivateLoadedJobs());
    assert(manager.visits.empty());
    std::cout << "restore binding, complete activation after rejection, order and clear passed\n";
}

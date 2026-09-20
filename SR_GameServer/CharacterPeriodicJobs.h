#pragma once
#include <cmath>
#include <functional>
#include <stdexcept>
#include <vector>

// Portable call binding for native 4AB640/4AB690. Each slot accumulates a
// float, subtracts ONE interval, and calls once per actor update.
class CharacterPeriodicJobs {
    struct Entry { float elapsed, interval; std::function<void()> callback; };
    std::vector<Entry> entries;
public:
    void Add(float interval, std::function<void()> callback) {
        if (!(interval > 0) || !std::isfinite(interval) || !callback)
            throw std::invalid_argument("invalid character periodic job");
        entries.push_back({0.0f, interval, std::move(callback)});
    }
    void Tick(float delta) {
        // 4AB69B..4AB6A5 excludes zero, not negative deltas. Preserve the
        // native accumulator even for a backwards clock sample.
        if (delta == 0) return;
        for (size_t i = 0; i < entries.size(); ++i) {
            auto& job = entries[i];
            job.elapsed = static_cast<float>(job.elapsed + delta);
            if (job.elapsed >= job.interval) {
                job.elapsed = static_cast<float>(job.elapsed - job.interval);
                auto callback = job.callback;
                callback();
            }
        }
    }
};

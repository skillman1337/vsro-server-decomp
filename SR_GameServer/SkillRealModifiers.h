#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>

// Portable container projection of 59DF20 -> 599840. The native maps are
// manager+14,+2C,...,+194. Each key owns an unsigned-value-ordered multimap
// of (value, execution context). Equal entries are retained, not overwritten.
// 'real' is a modifier installation/removal operation, not action cancellation.
class SkillRealModifiers {
public:
    static constexpr std::array<uint32_t, 17> masks = {
        0x40,0x80,0x100,0x200,0x400,0x800,0x2000,0x4000,0x8000,
        0x10000,0x20000,0x40000,0x80000,0x100000,0x200000,0x400000,0x1000000};
    using Contributions = std::multimap<uint32_t, uint32_t>;
    struct Strongest { uint32_t grade = 0, value = 0; };
    void Update(uint32_t mask, uint32_t value, uint32_t key,
                uint32_t context, bool remove) {
        for (std::size_t i = 0; i < masks.size(); ++i) {
            if (!(mask & masks[i])) continue;
            auto& table = tables[i];
            if (!remove) { table[key].emplace(value, context); continue; }
            auto bucket = table.find(key);
            if (bucket == table.end()) continue;
            auto& entries = bucket->second;
            // 5998C5/5998CE matches both fields, erases only the first match.
            for (auto it = entries.begin(); it != entries.end(); ++it) {
                if (it->first == value && it->second == context) {
                    entries.erase(it);
                    break;
                }
            }
            if (entries.empty()) table.erase(bucket);
        }
    }
    const Contributions* Find(uint32_t mask, uint32_t key) const {
        for (std::size_t i = 0; i < masks.size(); ++i) if (mask == masks[i]) {
            const auto it = tables[i].find(key);
            return it == tables[i].end() ? nullptr : &it->second;
        }
        return nullptr;
    }
    // 5999E0 decrements end() of the grade map; 599740 does the same in
    // that bucket. Highest grade wins before value, not highest value overall.
    Strongest Read(uint32_t mask) const {
        for (std::size_t i = 0; i < masks.size(); ++i) if (mask == masks[i]) {
            if (tables[i].empty()) return {};
            const auto& top = *tables[i].rbegin();
            return {top.first, top.second.rbegin()->first};
        }
        return {};
    }
private:
    std::array<std::map<uint32_t, Contributions>, masks.size()> tables;
};

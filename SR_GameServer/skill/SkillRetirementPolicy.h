#pragma once
#include <cstdint>

// Research server 59FFD3..5A0025: selection only, not teardown/event dispatch.
// descriptor65 is the actual +65 byte; no inferred bool alias or enum narrowing.
inline constexpr bool SkillRetirementSelected(bool force, uint8_t descriptor65,
    bool current, uint32_t category, bool slot274, bool slot2B4, bool cbuf) {
    return !cbuf && (force || (descriptor65 != 0 &&
        (current || (category == 3 && (slot274 || slot2B4)))));
}

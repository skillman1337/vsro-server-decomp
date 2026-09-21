#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace NativeEvents {
// Portable projection of 411220/4C0D10 and the word producers in 52A240.
// Native references are 32-bit host handles; never truncate process pointers.
// Invalid native assertion domains throw instead of continuing into corruption.
class Arguments {
    std::array<uint8_t,128> data{};
    uint32_t cursor=0, written=0;
    bool present=false, reading=false;
public:
    void ResetCursor() { cursor=0; } // dispatcher resets only +88
    void AppendWord(uint32_t value) {
        if(cursor>124 || reading) throw std::logic_error("invalid native event argument append");
        present=true;
        for(unsigned i=0;i<4;++i) data[cursor+i]=uint8_t(value>>(8*i));
        cursor+=4; written+=4;
    }
    void Read(void* destination,uint32_t count) {
        if(count>128 || cursor>written || count>written-cursor || !present)
            throw std::logic_error("invalid native event argument read");
        // Written bounds precede first-read reset in native 4C0D10.
        if(!reading) { cursor=0; reading=true; }
        if(cursor>128 || count>128-cursor)
            throw std::logic_error("native event argument capacity exceeded");
        if(count) std::memcpy(destination,data.data()+cursor,count);
        cursor+=count;
    }
    uint32_t ReadWord() {
        uint8_t bytes[4]; Read(bytes,4);
        return uint32_t(bytes[0]) | uint32_t(bytes[1])<<8 | uint32_t(bytes[2])<<16 | uint32_t(bytes[3])<<24;
    }
};
}

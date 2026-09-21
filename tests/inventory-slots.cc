#include "../SR_GameServer/GStorage.h"
#include "../SR_GameServer/GStorageOP.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cstdint>

// Opaque identities: none of these ten operations dereferences an item.
static CGItem* token(int n) { return reinterpret_cast<CGItem*>(static_cast<uintptr_t>(n)); }
static int id(CGItem* p) { return static_cast<int>(reinterpret_cast<uintptr_t>(p)); }
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::ifstream input(argv[1]);
    if (!input) return 2;
    std::string line; int count = 0;
    while (std::getline(input, line)) {
        std::istringstream row(line);
        int op, cap, mask, a, b, selector, expected, expectedSlot;
        row >> op >> cap >> mask >> a >> b >> selector >> expected >> expectedSlot;
        CGStorage s(cap);
        for (int i=0; i<cap; ++i) if (mask & (1<<i)) s.SetItem(i, token(i+1));
        uint8_t slot = 123; int result = 0;
        switch(op) {
        case 0: result=id(CGStorageOP::Peek(s,a)); break;
        case 1: result=id(CGStorageOP::Detach(s,a)); break;
        case 2: result=CGStorageOP::MoveToEmpty(s,a,b); break;
        case 3: result=CGStorageOP::SwapOccupied(s,a,b); break;
        case 4: result=CGStorageOP::FindPointer(s,token(b < 0 ? 0 : b),a); break;
        case 5: result=CGStorageOP::CountSlots(s,a,b,selector); break;
        case 6: result=id(CGStorageOP::FirstOccupied(s,a,b,slot)); break;
        case 7: result=CGStorageOP::HasItemFrom(s,a); break;
        case 8: result=CGStorageOP::FirstEmpty(s,a); break;
        case 9: result=CGStorageOP::CountEmpty(s,a); break;
        default: return 2;
        }
        if (result != expected || slot != expectedSlot) {
            std::cerr << "native mismatch row " << count << ": " << line << " got " << result << '\n'; return 1;
        }
        for (int i=0; i<cap; ++i) { int expectedItem; row >> expectedItem; if (id(s.GetItem(i)) != expectedItem) return 1; }
        ++count;
    }
    if (count != 2950) return 1;
    std::cout << count << " native inventory slot traces passed\n";
}

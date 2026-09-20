#include "SR_GameServer/skill/SkillRetirementPolicy.h"
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::ifstream input(argv[1]);
    unsigned force, flag, current, category, a, b, cbuf, expected;
    unsigned count = 0;
    while (input >> force >> flag >> current >> category >> a >> b >> cbuf >> expected) {
        if (SkillRetirementSelected(force, static_cast<uint8_t>(flag), current,
                category, a, b, cbuf) != bool(expected)) return 1;
        ++count;
    }
    if (!input.eof() || count != 512) return 3;
    std::cout << count << " native selection cases passed\n";
}

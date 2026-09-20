#include "SR_GameServer/CharacterPeriodicJobs.h"
#include <bit>
#include <cstdint>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::ifstream input(argv[1]);
    CharacterPeriodicJobs jobs;
    unsigned first = 0, second = 0;
    jobs.Add(.3f, [&] { ++first; });
    jobs.Add(1.f, [&] { ++second; });
    uint32_t delta, wantFirst, wantSecond, remainderFirst, remainderSecond;
    unsigned count = 0;
    while (input >> delta >> wantFirst >> wantSecond >> remainderFirst >> remainderSecond) {
        jobs.Tick(std::bit_cast<float>(delta));
        if (first != wantFirst || second != wantSecond) {
            std::cerr << "native scheduler mismatch at " << count << '\n'; return 1;
        }
        ++count;
    }
    if (!input.eof() || count != 311) return 3;
    std::cout << count << " native scheduler dispatch transitions passed\n";
}

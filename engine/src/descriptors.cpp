#include "quiky/runtime.h"

#include <cstddef>

namespace quiky {

namespace {

struct DescriptorRange {
    std::uint16_t first;
    std::uint16_t last;
    std::uint16_t flags;
};

// These ranges are the first-match results of the five world-specific
// compare ladders in QUIKY.EXE (descriptor record +2, stride 4).
const DescriptorRange kW1[] = {
    {0, 4, 0x0000}, {5, 10, 0x000c}, {11, 19, 0x0000},
    {20, 20, 0x000c}, {21, 22, 0x0000}, {23, 23, 0x000c},
    {24, 39, 0x0000}, {40, 40, 0x0010}, {41, 41, 0x0050},
    {42, 42, 0x0070}, {43, 43, 0x0030}, {44, 48, 0x000c},
    {49, 308, 0x0000}, {309, 309, 0x0010}, {310, 310, 0x0050},
    {311, 311, 0x0070}, {312, 312, 0x0030}, {313, 415, 0x0000},
    {416, 419, 0x000c}, {420, 497, 0x0000}, {498, 501, 0x000c},
    {502, 502, 0x0000}, {503, 506, 0x000c}, {507, 507, 0x0000},
    {508, 511, 0x000c},
};

const DescriptorRange kW2[] = {
    {0, 4, 0x0000}, {5, 10, 0x000c}, {11, 23, 0x0000},
    {24, 24, 0x000c}, {25, 43, 0x0000}, {44, 44, 0x000c},
    {45, 63, 0x0000}, {64, 70, 0x000c}, {71, 81, 0x0000},
    {82, 91, 0x000c}, {92, 226, 0x0000}, {227, 227, 0x000c},
    {228, 379, 0x0000}, {380, 384, 0x000c}, {385, 497, 0x0000},
    {498, 501, 0x000c}, {502, 502, 0x0000}, {503, 506, 0x000c},
    {507, 507, 0x0000}, {508, 511, 0x000c},
};

const DescriptorRange kW3[] = {
    {0, 4, 0x0000}, {5, 10, 0x000c}, {11, 14, 0x0000},
    {15, 19, 0x000c}, {20, 21, 0x0000}, {22, 22, 0x0070},
    {23, 23, 0x0030}, {24, 24, 0x0070}, {25, 25, 0x0030},
    {26, 28, 0x0000}, {29, 29, 0x0010}, {30, 30, 0x0050},
    {31, 31, 0x0010}, {32, 32, 0x0050}, {33, 34, 0x0000},
    {35, 37, 0x000c}, {38, 41, 0x0000}, {42, 42, 0x0070},
    {43, 43, 0x0030}, {44, 44, 0x0070}, {45, 45, 0x0030},
    {46, 48, 0x0000}, {49, 49, 0x0010}, {50, 50, 0x0050},
    {51, 51, 0x0010}, {52, 52, 0x0050}, {53, 59, 0x0000},
    {60, 61, 0x000c}, {62, 62, 0x0070}, {63, 63, 0x0030},
    {64, 64, 0x0070}, {65, 65, 0x0030}, {66, 68, 0x0000},
    {69, 69, 0x0010}, {70, 70, 0x0050}, {71, 71, 0x0010},
    {72, 72, 0x0050}, {73, 79, 0x0000}, {80, 80, 0x0070},
    {81, 81, 0x0030}, {82, 82, 0x0070}, {83, 83, 0x0030},
    {84, 84, 0x0070}, {85, 85, 0x0030}, {86, 88, 0x0000},
    {89, 89, 0x0010}, {90, 90, 0x0050}, {91, 91, 0x0010},
    {92, 92, 0x0050}, {93, 93, 0x0010}, {94, 94, 0x0050},
    {95, 101, 0x0000}, {102, 102, 0x0070}, {103, 103, 0x0030},
    {104, 104, 0x0070}, {105, 105, 0x0030}, {106, 108, 0x0000},
    {109, 109, 0x0010}, {110, 110, 0x0050}, {111, 111, 0x0010},
    {112, 112, 0x0050}, {113, 114, 0x0000}, {115, 115, 0x0004},
    {116, 116, 0x0008}, {117, 119, 0x0004}, {120, 123, 0x0000},
    {124, 124, 0x0070}, {125, 125, 0x0030}, {126, 128, 0x0000},
    {129, 129, 0x0010}, {130, 130, 0x0050}, {131, 145, 0x0000},
    {146, 148, 0x000c}, {149, 154, 0x0000}, {155, 156, 0x0008},
    {157, 171, 0x0000}, {172, 172, 0x0004}, {173, 173, 0x000c},
    {174, 174, 0x0008}, {175, 184, 0x0000}, {185, 185, 0x0070},
    {186, 186, 0x0030}, {187, 187, 0x0070}, {188, 188, 0x0030},
    {189, 189, 0x0070}, {190, 190, 0x0030}, {191, 191, 0x0004},
    {192, 192, 0x000c}, {193, 193, 0x0008}, {194, 194, 0x0004},
    {195, 195, 0x000c}, {196, 196, 0x0008}, {197, 197, 0x0004},
    {198, 198, 0x000c}, {199, 199, 0x0008}, {200, 204, 0x0000},
    {205, 205, 0x0010}, {206, 206, 0x0050}, {207, 207, 0x0010},
    {208, 208, 0x0050}, {209, 209, 0x0010}, {210, 210, 0x0050},
    {211, 211, 0x0004}, {212, 212, 0x000c}, {213, 213, 0x0008},
    {214, 214, 0x0004}, {215, 215, 0x000c}, {216, 216, 0x0008},
    {217, 217, 0x0004}, {218, 218, 0x000c}, {219, 219, 0x0008},
    {220, 226, 0x0000}, {227, 227, 0x000c}, {228, 228, 0x0070},
    {229, 229, 0x0030}, {230, 230, 0x0070}, {231, 231, 0x0030},
    {232, 232, 0x0010}, {233, 233, 0x0050}, {234, 234, 0x0000},
    {235, 235, 0x0004}, {236, 236, 0x0008}, {237, 250, 0x0000},
    {251, 251, 0x0070}, {252, 252, 0x0030}, {253, 253, 0x0070},
    {254, 254, 0x0030}, {255, 255, 0x0000}, {256, 256, 0x0010},
    {257, 257, 0x0050}, {258, 258, 0x0010}, {259, 259, 0x0050},
    {260, 276, 0x0000}, {277, 277, 0x000c}, {278, 296, 0x0000},
    {297, 297, 0x0004}, {298, 298, 0x000c}, {299, 299, 0x0008},
    {300, 497, 0x0000}, {498, 501, 0x000c}, {502, 502, 0x0000},
    {503, 506, 0x000c}, {507, 507, 0x0000}, {508, 511, 0x000c},
};

const DescriptorRange kW4[] = {
    {0, 4, 0x0000}, {5, 10, 0x000c}, {11, 19, 0x0000},
    {20, 23, 0x000c}, {24, 266, 0x0000}, {267, 270, 0x000c},
    {271, 286, 0x0000}, {287, 290, 0x000c}, {291, 295, 0x0000},
    {296, 296, 0x000c}, {297, 304, 0x0000}, {305, 306, 0x000c},
    {307, 308, 0x0000}, {309, 310, 0x000c}, {311, 319, 0x0000},
    {320, 323, 0x000c}, {324, 324, 0x0000}, {325, 326, 0x000c},
    {327, 328, 0x0000}, {329, 329, 0x000c}, {330, 337, 0x0000},
    {338, 343, 0x000c}, {344, 347, 0x0000}, {348, 348, 0x000c},
    {349, 358, 0x0000}, {359, 359, 0x000c}, {360, 428, 0x0000},
    {429, 429, 0x0070}, {430, 430, 0x0030}, {431, 439, 0x0000},
    {440, 440, 0x0010}, {441, 441, 0x0050}, {442, 442, 0x0070},
    {443, 443, 0x0030}, {444, 444, 0x0010}, {445, 445, 0x0050},
    {446, 446, 0x0070}, {447, 447, 0x0030}, {448, 448, 0x0010},
    {449, 459, 0x0000}, {460, 460, 0x0010}, {461, 461, 0x0050},
    {462, 462, 0x0070}, {463, 463, 0x0030}, {464, 464, 0x0010},
    {465, 465, 0x0050}, {466, 466, 0x0070}, {467, 467, 0x0030},
    {468, 468, 0x0050}, {469, 497, 0x0000}, {498, 501, 0x000c},
    {502, 502, 0x0000}, {503, 506, 0x000c}, {507, 507, 0x0000},
    {508, 511, 0x000c},
};

const DescriptorRange kW5[] = {
    {0, 4, 0x0000}, {5, 10, 0x000c}, {11, 15, 0x0000},
    {16, 17, 0x000c}, {18, 19, 0x0000}, {20, 23, 0x000c},
    {24, 27, 0x0000}, {28, 33, 0x000c}, {34, 47, 0x0000},
    {48, 53, 0x000c}, {54, 85, 0x0000}, {86, 86, 0x000c},
    {87, 87, 0x0070}, {88, 88, 0x0030}, {89, 89, 0x0010},
    {90, 90, 0x0050}, {91, 91, 0x000c}, {92, 497, 0x0000},
    {498, 501, 0x000c}, {502, 502, 0x0000}, {503, 506, 0x000c},
    {507, 507, 0x0000}, {508, 511, 0x000c},
};

template <std::size_t N>
PlayerDescriptorTable buildTable(const DescriptorRange (&ranges)[N]) {
    PlayerDescriptorTable result;
    for (std::size_t index = 0; index < N; ++index) {
        for (int tile = ranges[index].first;
             tile <= static_cast<int>(ranges[index].last); ++tile) {
            result.setWord(static_cast<std::uint16_t>(tile), ranges[index].flags);
        }
    }
    return result;
}

} // namespace

std::uint16_t PlayerDescriptorRules::quadrantMask(std::uint16_t x,
                                                  std::uint16_t y) {
    const bool xBit3 = (x & 0x08) != 0;
    const bool yBit3 = (y & 0x08) != 0;
    if (yBit3) {
        return xBit3 ? 0x02 : 0x01;
    }
    return xBit3 ? 0x04 : 0x08;
}

bool PlayerDescriptorRules::blocksProbe(std::uint16_t descriptor,
                                        std::uint16_t x,
                                        std::uint16_t y) {
    return (descriptor & quadrantMask(x, y)) != 0;
}

bool PlayerDescriptorRules::hasVerticalResponse(std::uint16_t descriptor) {
    return (descriptor & 0x20) != 0;
}

bool PlayerDescriptorRules::alignsEightPixels(std::uint16_t descriptor) {
    return (descriptor & 0x40) != 0;
}

std::uint16_t PlayerDescriptorRules::snapProbeY(std::uint16_t y) {
    return static_cast<std::uint16_t>(y & 0xfff8);
}

PlayerDescriptorTable::PlayerDescriptorTable() : _words() {
    _words.fill(0);
}

PlayerDescriptorTable::PlayerDescriptorTable(
    const std::array<std::uint16_t, PlayerDescriptorRules::kEntryCount> &words)
    : _words(words) {
}

std::uint16_t PlayerDescriptorTable::word(std::uint16_t tileId) const {
    return _words[tileId & 0x01ff];
}

void PlayerDescriptorTable::setWord(std::uint16_t tileId,
                                    std::uint16_t descriptor) {
    _words[tileId & 0x01ff] = descriptor;
}

PlayerDescriptorTable playerDescriptorTableForWorld(const std::string &worldName) {
    if (worldName == "W1") return buildTable(kW1);
    if (worldName == "W2") return buildTable(kW2);
    if (worldName == "W3") return buildTable(kW3);
    if (worldName == "W4") return buildTable(kW4);
    if (worldName == "W5") return buildTable(kW5);
    return PlayerDescriptorTable();
}

} // namespace quiky

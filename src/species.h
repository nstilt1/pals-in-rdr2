#pragma once
#include <array>

namespace pal {
struct SpeciesInfo { const char* name; const char* model; };
inline constexpr std::array<SpeciesInfo,5> species = {{
    {"Lamball", "a_c_sheep_01"},
    {"Caprity", "a_c_goat_01"},
    {"Rushoar", "a_c_boar_01"},
    {"Rushoar", "a_c_pig_01"},
    {"Chikipi", "a_c_chicken_01"}
}};
}

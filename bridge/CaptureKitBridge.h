#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Windows x64 C ABI. No C++/Rust objects or allocations cross the boundary.
   All pointers must reference valid, aligned, nonoverlapping caller buffers.
   Status: 0 success, -1 null pointer, -2 invalid input. Outputs unchanged on error. */
typedef struct CkbRules {
    double hp_weight, level_window, unaware_multiplier;
} CkbRules;
typedef struct CkbInputs {
    double orb_level;
    uint32_t creature_level, reserved; /* reserved must be zero */
    double hp, max_hp, catch_rate, status_bonus;
    uint32_t unaware, uncapturable; /* boolean flags: 0 or 1 */
} CkbInputs;
typedef struct CkbBreakdown {
    double hp_factor, level_factor, chance;
} CkbBreakdown;
uint32_t ckb_api_version(void);
int32_t ckb_v1_capture_chance(const CkbRules*, const CkbInputs*, CkbBreakdown*);
/* 1..8 strictly-positive weights summing to 1; rolls in [0,1); chance in [0,1]. */
int32_t ckb_v1_judge_shakes(double chance, const double* weights, const double* rolls,
                          uint32_t count, uint32_t* passed);
#ifdef __cplusplus
}
#endif

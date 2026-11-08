#include "../src/targeting.h"
#include <limits>
#include <iostream>

int main() {
    int failures = 0;
    const auto check = [&](bool ok, const char* name) {
        if (!ok) { std::cerr << name << '\n'; ++failures; }
    };
    check(pal::nearest({}, 25) == 0, "empty pool");
    check(pal::nearest({{1, 1, false}, {2, 9, true}, {3, 4, true}}, 25) == 3,
          "skip protected nearer entity, choose nearest eligible");
    check(pal::nearest({{1, 626, true}}, 25) == 0, "outside radius");
    check(pal::nearest({{1, 625, true}}, 25) == 1, "radius boundary");
    check(pal::nearest({{0, 0, true}, {1, -1, true},
          {2, std::numeric_limits<float>::quiet_NaN(), true}}, 25) == 0, "invalid candidates");
    check(pal::nearest({{1, 0, true}}, -1) == 0, "invalid radius");
    return failures ? 1 : 0;
}

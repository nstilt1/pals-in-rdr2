#pragma once
#include <cmath>
#include <vector>

namespace pal {
struct Candidate {
    int handle;
    float distanceSquared;
    bool eligible;
};
inline int nearest(const std::vector<Candidate>& candidates, float radius) {
    if (!std::isfinite(radius) || radius <= 0) return 0;
    const float limit = radius * radius;
    float best = limit;
    int result = 0;
    for (const auto& c : candidates) {
        if (!c.handle || !c.eligible || !std::isfinite(c.distanceSquared) || c.distanceSquared < 0)
            continue;
        if (c.distanceSquared <= limit && (!result || c.distanceSquared < best)) {
            best = c.distanceSquared;
            result = c.handle;
        }
    }
    return result;
}
}

#pragma once
#include <cmath>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include "capture.h"

namespace pal {
struct WorldPoint { double x = 0, y = 0, z = 0; };
struct ResourceDeposit {
    unsigned int id = 0;
    std::string model;
    WorldPoint position;
    int fragments = 0, spheres = 0;
};
struct ResourceCatalog {
    std::vector<WorldPoint> camps;
    std::vector<ResourceDeposit> deposits;
};
inline bool validPoint(WorldPoint p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z)
        && std::abs(p.x) <= 20000 && std::abs(p.y) <= 20000 && std::abs(p.z) <= 2000;
}
inline double worldDistanceSquared(WorldPoint a, WorldPoint b) {
    const auto x = a.x-b.x, y = a.y-b.y, z = a.z-b.z;
    return x*x+y*y+z*z;
}
// Original local configuration, not a shipped game map or game database.
// camp X Y Z
// deposit ID MODEL X Y Z FRAGMENTS SPHERES
// IDs are permanent save identities, not entity handles. No respawn/ID reuse.
inline bool parseResourceCatalog(const std::string& text, ResourceCatalog& result) {
    if (text.size() > 65536) return false;
    ResourceCatalog parsed;
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        if (line.size() > 512) return false;
        line = line.substr(0, line.find('#'));
        std::istringstream fields(line);
        std::string kind, extra;
        if (!(fields >> kind)) continue;
        if (kind == "camp") {
            WorldPoint p;
            if (!(fields >> p.x >> p.y >> p.z) || !validPoint(p) || parsed.camps.size() >= 32) return false;
            parsed.camps.push_back(p);
        } else if (kind == "deposit") {
            ResourceDeposit d;
            long long id;
            if (!(fields >> id >> d.model >> d.position.x >> d.position.y >> d.position.z
                  >> d.fragments >> d.spheres) || id <= 0 || id > 0xffffffffLL
                || !validPoint(d.position) || d.model.empty() || d.model.size() > 63
                || d.model.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_") != std::string::npos
                || d.fragments < 0 || d.spheres < 0 || d.fragments > inventoryLimit
                || d.spheres > inventoryLimit || (d.fragments == 0 && d.spheres == 0)
                || parsed.deposits.size() >= 64) return false;
            d.id = static_cast<unsigned int>(id);
            if (std::any_of(parsed.deposits.begin(), parsed.deposits.end(),
                            [&](const auto& prior) { return prior.id == d.id; })) return false;
            parsed.deposits.push_back(d);
        } else return false;
        if (fields >> extra) return false;
    }
    result = std::move(parsed);
    return true;
}
inline bool atRegisteredCamp(const ResourceCatalog& catalog, WorldPoint player, WorldPoint fire) {
    if (!validPoint(player) || !validPoint(fire) || worldDistanceSquared(player, fire) > 9) return false;
    return std::any_of(catalog.camps.begin(), catalog.camps.end(), [&](auto anchor) {
        return worldDistanceSquared(anchor, fire) <= 16;
    });
}
}

#pragma once
#include <cstdint>
#include <cmath>

namespace pal {
enum class Opponent { none, player, companion };
inline bool mayEngage(bool shotGateOpen, bool animalActivelyAttacking) {
    return shotGateOpen || animalActivelyAttacking;
}
inline Opponent nearestOpponent(float playerDistance, bool playerViable,
                                float palDistance, bool palViable) {
    playerViable = playerViable && std::isfinite(playerDistance) && playerDistance >= 0;
    palViable = palViable && std::isfinite(palDistance) && palDistance >= 0;
    if (palViable && (!playerViable || palDistance < playerDistance)) return Opponent::companion;
    return playerViable ? Opponent::player : Opponent::none;
}
// Original encounter rule: hold fire until a player/confirmed-hostile shot.
// Five seconds without an eligible hostile ends the encounter.
class EncounterGate {
    bool open_ = false, sawPlayerShot_ = false;
    std::uint64_t playerShot_ = 0, hostileSeen_ = 0;
public:
    void reset() { open_ = false; sawPlayerShot_ = false; playerShot_ = hostileSeen_ = 0; }
    bool update(std::uint64_t now, bool playerShot, bool hostileShot, bool hasHostile) {
        if (playerShot) { playerShot_ = now; sawPlayerShot_ = true; }
        if (hasHostile) {
            hostileSeen_ = now;
            if (hostileShot || (sawPlayerShot_ && now - playerShot_ <= 1000)) open_ = true;
        } else if (open_ && now - hostileSeen_ >= 5000) {
            reset();
        }
        return open_ && hasHostile;
    }
};
}

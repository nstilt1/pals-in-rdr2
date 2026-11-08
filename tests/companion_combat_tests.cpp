#include "../src/companion_combat.h"
#include <iostream>

int main() {
    int failures = 0;
    const auto check = [&](bool ok, const char* name) {
        if (!ok) { std::cerr << name << '\n'; ++failures; }
    };
    pal::EncounterGate gate;
    check(!gate.update(100, false, false, true), "hostility/aiming alone cannot open combat");
    check(gate.update(200, true, false, true), "player first shot opens combat");
    check(gate.update(300, false, false, true), "continue encounter after first shot");
    check(!gate.update(400, false, false, false), "no attack without confirmed hostile");
    check(!gate.update(5400, false, false, false), "quiet encounter ends");
    check(!gate.update(5500, false, false, true), "later encounter holds fire again");
    check(gate.update(5600, false, true, true), "hostile first shot opens combat");
    gate.reset();
    check(!gate.update(6000, false, true, false), "unrelated gunfire cannot open combat");
    check(!gate.update(6100, false, false, true), "unrelated shot not remembered");
    gate.reset();
    check(!gate.update(7000, true, false, false), "player firing without hostile creates no target");
    check(gate.update(7050, false, false, true), "allow hostility flag to update just after player's shot");
    gate.reset();
    gate.update(8000, true, false, false);
    check(!gate.update(9500, false, false, true), "old player shot cannot trigger a new encounter");
    check(pal::nearestOpponent(100, true, 4, true) == pal::Opponent::companion,
          "enemy can shoot nearer fighting Pal");
    check(pal::nearestOpponent(4, true, 100, true) == pal::Opponent::player,
          "enemy can return to nearer player");
    check(pal::nearestOpponent(100, true, 4, false) == pal::Opponent::player,
          "do not select obstructed or nonfighting Pal");
    check(pal::nearestOpponent(4, false, 100, true) == pal::Opponent::companion,
          "choose available opponent instead of obstructed player");
    check(pal::nearestOpponent(4, false, 1, false) == pal::Opponent::none,
          "no viable target leaves native AI alone");
    check(pal::nearestOpponent(-1, true, 2, true) == pal::Opponent::companion,
          "invalid distance cannot select player");
    check(pal::mayEngage(false, true), "defend immediately against actively attacking animal");
    check(!pal::mayEngage(false, false), "animal defense exception cannot open unshot human/neutral targeting");
    check(pal::mayEngage(true, false), "human/hunting combat allowed after shot trigger");
    return failures ? 1 : 0;
}

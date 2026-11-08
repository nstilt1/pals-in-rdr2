#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <sstream>
#include <string>
#include <vector>
#include "species.h"

namespace pal {
inline constexpr int inventoryLimit = 999;
inline constexpr size_t pickupReceiptLimit = 4096;
// Developer slice: up to five owned Pals, one deployed companion. New explicit
// profiles start with ten test spheres; existing inventory is never replenished.
// Transitional/deployed states on disk are quarantined on process restart;
// uncertain world mutations must never silently create a second animal.
enum class CapturePhase { empty, held, deployed, capturing, releasing, recalling, recovery };
struct StoredPal { int health=1, maxHealth=1, species=0; };
struct CaptureSave {
    CapturePhase phase = CapturePhase::empty;
    int spheres = 10;
    int health = 1;
    int maxHealth = 1;
    int species = 0;
    int fragments = 0;
    // Stable, adapter-assigned deposit IDs; never recycle IDs on streaming.
    std::vector<unsigned int> collected;
    std::vector<StoredPal> reserve;
    int selected = 0;
    bool missionSheepClaimed = false;
};
inline bool valid(const CaptureSave& s) {
    return static_cast<int>(s.phase) >= 0 && static_cast<int>(s.phase) <= 6
        && s.spheres >= 0 && s.spheres <= inventoryLimit && s.health > 0
        && s.maxHealth >= s.health && s.maxHealth <= 100000
        && s.species >= 0 && s.species < static_cast<int>(species.size())
        && s.fragments >= 0 && s.fragments <= inventoryLimit
        && s.collected.size() <= pickupReceiptLimit
        && std::all_of(s.collected.begin(), s.collected.end(), [](auto id) { return id != 0; })
        && std::is_sorted(s.collected.begin(), s.collected.end())
        && std::adjacent_find(s.collected.begin(), s.collected.end()) == s.collected.end()
        && s.reserve.size() <= 4 && s.selected >= 0 && s.selected <= static_cast<int>(s.reserve.size())
        && std::all_of(s.reserve.begin(),s.reserve.end(),[](const auto& p) {
            return p.health>0 && p.maxHealth>=p.health && p.maxHealth<=100000
                && p.species>=0 && p.species<static_cast<int>(species.size());
        });
}
inline std::string encode(const CaptureSave& s) {
    std::string text = "PALS_CAPTURE_4 " + std::to_string(static_cast<int>(s.phase)) + " "
        + std::to_string(s.spheres) + " " + std::to_string(s.health) + " "
        + std::to_string(s.maxHealth) + " " + std::to_string(s.species) + " "
        + std::to_string(s.fragments) + " " + std::to_string(s.collected.size());
    for (auto id : s.collected) text += " " + std::to_string(id);
    text += " " + std::to_string(s.selected) + " " + std::to_string(s.missionSheepClaimed)
        + " " + std::to_string(s.reserve.size());
    for (const auto& p:s.reserve) text += " " + std::to_string(p.health) + " "
        + std::to_string(p.maxHealth) + " " + std::to_string(p.species);
    return text + "\n";
}
inline bool decode(const std::string& text, CaptureSave& s) {
    std::istringstream in(text);
    std::string magic, extra;
    int phase;
    CaptureSave parsed;
    if (!(in >> magic >> phase >> parsed.spheres >> parsed.health >> parsed.maxHealth)
        || (magic != "PALS_CAPTURE_1" && magic != "PALS_CAPTURE_2" && magic != "PALS_CAPTURE_3" && magic != "PALS_CAPTURE_4")
        || phase < 0 || phase > 6) return false;
    if (magic != "PALS_CAPTURE_1" && !(in >> parsed.species)) return false;
    if ((magic == "PALS_CAPTURE_1" || magic == "PALS_CAPTURE_2") && parsed.spheres > 10) return false;
    if (magic == "PALS_CAPTURE_3" || magic == "PALS_CAPTURE_4") {
        int count;
        if (!(in >> parsed.fragments >> count) || count < 0 || count > static_cast<int>(pickupReceiptLimit)) return false;
        for (int i = 0; i < count; ++i) {
            // Parse signed to reject negative input rather than unsigned wraparound.
            long long id;
            if (!(in >> id) || id <= 0 || id > 0xffffffffLL) return false;
            parsed.collected.push_back(static_cast<unsigned int>(id));
        }
    }
    if (magic == "PALS_CAPTURE_4") {
        int claimed,count;
        if (!(in >> parsed.selected >> claimed >> count) || claimed<0 || claimed>1 || count<0 || count>4) return false;
        parsed.missionSheepClaimed=claimed!=0;
        for(int i=0;i<count;++i) {
            StoredPal p;
            if (!(in >> p.health >> p.maxHealth >> p.species)) return false;
            parsed.reserve.push_back(p);
        }
    }
    if (in >> extra) return false;
    parsed.phase = static_cast<CapturePhase>(phase);
    if (!valid(parsed)) return false;
    s = parsed;
    return true;
}
inline double captureChance(int health, int maximum) {
    if (health <= 0 || maximum <= 0 || health > maximum) return 0;
    // Original prototype rule, not a copy of either game's progression data.
    return 0.25 + 0.65 * (1.0 - static_cast<double>(health) / maximum);
}
class CaptureSlot {
public:
    using Store = std::function<bool(const CaptureSave&)>;
    explicit CaptureSlot(CaptureSave initial, Store store)
        : state_(initial), store_(std::move(store)) {}
    const CaptureSave& state() const { return state_; }
    int partySize() const { return static_cast<int>(state_.reserve.size()) + (state_.phase==CapturePhase::empty ? 0 : 1); }
    StoredPal selectedPal() const {
        return state_.selected ? state_.reserve[state_.selected-1] : StoredPal{state_.health,state_.maxHealth,state_.species};
    }
    bool cycleParty(int direction) {
        if ((state_.phase!=CapturePhase::held && state_.phase!=CapturePhase::deployed) || direction==0) return false;
        auto next=state_;const int count=partySize();
        next.selected=(next.selected+(direction>0 ? 1 : count-1))%count;
        return next.selected==state_.selected || commit(next);
    }
    bool activateSelected() {
        if (state_.phase!=CapturePhase::held) return false;
        if (!state_.selected) return true;
        auto next=state_;auto& chosen=next.reserve[next.selected-1];
        StoredPal prior{next.health,next.maxHealth,next.species};
        next.health=chosen.health;next.maxHealth=chosen.maxHealth;next.species=chosen.species;
        chosen=prior;next.selected=0;return commit(next);
    }
    bool prepareCapture() {
        if (state_.phase==CapturePhase::empty) return true;
        if (state_.phase!=CapturePhase::held || partySize()>=5) return false;
        auto next=state_;next.reserve.push_back({next.health,next.maxHealth,next.species});
        next.phase=CapturePhase::empty;next.selected=0;return commit(next);
    }
    bool restoreHeld() {
        if (state_.phase!=CapturePhase::empty || state_.reserve.empty()) return true;
        auto next=state_;promote(next);return commit(next);
    }
    // Persist inventory + receipt together BEFORE removing a world pickup.
    // On failure the adapter must retain the pickup. A receipt suppresses it on reload.
    bool collect(unsigned int id, int fragments, int spheres) {
        if (!inventoryAvailable() || id == 0 || fragments < 0 || spheres < 0
            || (fragments == 0 && spheres == 0)
            || fragments > inventoryLimit - state_.fragments
            || spheres > inventoryLimit - state_.spheres
            || state_.collected.size() == pickupReceiptLimit) return false;
        auto next = state_;
        auto pos = std::lower_bound(next.collected.begin(), next.collected.end(), id);
        if (pos != next.collected.end() && *pos == id) return false;
        next.collected.insert(pos, id);
        next.fragments += fragments; next.spheres += spheres;
        return commit(next);
    }
    // Recipe quantities belong to the shared camp catalog, not game data.
    // The adapter must validate camp proximity/unlocks before calling.
    bool craftSpheres(int fragmentCost, int sphereYield) {
        if (!inventoryAvailable() || fragmentCost <= 0 || sphereYield <= 0
            || fragmentCost > state_.fragments
            || sphereYield > inventoryLimit - state_.spheres) return false;
        auto next = state_; next.fragments -= fragmentCost; next.spheres += sphereYield;
        return commit(next);
    }
    bool recoverOnStartup() {
        if (state_.phase == CapturePhase::empty) return restoreHeld();
        if (state_.phase == CapturePhase::held) return true;
        auto next = state_; next.phase = CapturePhase::recovery;
        // Quarantine in memory even when persistence itself has failed.
        const bool saved = store_(next); state_ = next; return saved;
    }
    bool spendSphere() {
        if (state_.phase != CapturePhase::empty || state_.spheres == 0) return false;
        auto next = state_; --next.spheres; return commit(next);
    }
    bool beginCapture(int health, int maximum, int species = 0, bool missionSheep = false) {
        if (state_.phase != CapturePhase::empty || captureChance(health, maximum) == 0) return false;
        auto next = state_; next.phase = CapturePhase::capturing;
        if (missionSheep && state_.missionSheepClaimed) return false;
        next.missionSheepClaimed=next.missionSheepClaimed || missionSheep;
        next.health = health; next.maxHealth = maximum; next.species = species; next.selected=0;return commit(next);
    }
    bool beginRelease() { return transition(CapturePhase::held, CapturePhase::releasing); }
    bool beginRecall(int health, int maximum) {
        if (state_.phase != CapturePhase::deployed || captureChance(health, maximum) == 0) return false;
        auto next = state_; next.phase = CapturePhase::recalling;
        next.health = health; next.maxHealth = maximum; return commit(next);
    }
    bool finishCapture(bool removed) {
        if (state_.phase!=CapturePhase::capturing) return false;
        auto next=state_;next.phase=removed ? CapturePhase::held : CapturePhase::empty;
        if (!removed) promote(next);
        return commit(next);
    }
    bool finishRelease(bool created) {
        return transition(CapturePhase::releasing, created ? CapturePhase::deployed : CapturePhase::held);
    }
    bool finishRecall(bool removed) {
        return transition(CapturePhase::recalling, removed ? CapturePhase::held : CapturePhase::deployed);
    }
    bool markLost() { return transition(CapturePhase::deployed, CapturePhase::recovery); }
    bool markDead() {
        if (state_.phase!=CapturePhase::deployed) return false;
        auto next=state_;next.phase=CapturePhase::empty;next.selected=0;promote(next);return commit(next);
    }
private:
    static void promote(CaptureSave& save) {
        save.selected=0;
        if(save.reserve.empty()) return;
        const auto p=save.reserve.back();save.reserve.pop_back();
        save.health=p.health;save.maxHealth=p.maxHealth;save.species=p.species;save.phase=CapturePhase::held;
    }
    bool inventoryAvailable() const {
        return state_.phase == CapturePhase::empty || state_.phase == CapturePhase::held
            || state_.phase == CapturePhase::deployed;
    }
    CaptureSave state_;
    Store store_;
    bool commit(const CaptureSave& next) {
        if (!valid(next) || !store_(next)) return false;
        state_ = next; return true;
    }
    bool transition(CapturePhase from, CapturePhase to) {
        if (state_.phase != from) return false;
        auto next = state_; next.phase = to; return commit(next);
    }
};
}

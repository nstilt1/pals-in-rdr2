// Script-fiber companion combat candidate. Native sheep rigs lack a verified
// attack animation; this prototype pursues and applies a close-contact strike.
pal::EncounterGate companionEncounter;
bool companionCombatEnabled = false;
Ped combatTarget = 0;
ULONGLONG nextThreatScan = 0, nextCombatTask = 0, nextContactStrike = 0;
std::vector<Ped> combatPool;
struct EnemyAssignment { Ped target = 0; ULONGLONG nextUpdate = 0; };
std::unordered_map<Ped, EnemyAssignment> enemyAssignments;
Ped huntTarget = 0, lastAimedAnimal = 0;
ULONGLONG huntUntil = 0, lastAnimalAim = 0;

void disengageEnemyTargets() {
    const Ped player = PLAYER::PLAYER_PED_ID();
    for (const auto& entry : enemyAssignments) {
        const Ped enemy = entry.first;
        if (entry.second.target != companion || !ENTITY::DOES_ENTITY_EXIST(enemy)
            || PED::IS_PED_DEAD_OR_DYING(enemy, true)
            || ENTITY::IS_ENTITY_A_MISSION_ENTITY(enemy)
            || !ENTITY::DOES_ENTITY_EXIST(player) || ENTITY::IS_ENTITY_DEAD(player)
            || distanceSquared(enemy, player) > 1225.0f) continue;
        if (PED::IS_PED_IN_COMBAT(enemy, companion) || PED::IS_PED_IN_COMBAT(enemy, player))
            TASK::TASK_COMBAT_PED(enemy, player, 0, 0);
    }
    enemyAssignments.clear();
}

void initializeCompanionCombat() {
    companionCombatEnabled = GetPrivateProfileIntA("Prototype", "CompanionCombatEnabled", 0,
        (basePath + "RDR2PalPrototype.ini").c_str()) != 0;
    trace(std::string("Companion combat candidate: ") + (companionCombatEnabled ? "enabled" : "disabled"));
}

void resumeCompanionFollow() {
    if (!ownedCompanion() || ENTITY::IS_ENTITY_DEAD(companion)) return;
    TASK::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(companion, PLAYER::PLAYER_PED_ID(), 1.0f, -2.0f, 0.0f,
                                         2.0f, -1, 2.5f, true, false, false, false, false, false);
}

void maintainOwnedCompanionFollow() {
    static ULONGLONG nextCheck=0;
    if (GetTickCount64()<nextCheck) return;
    nextCheck=GetTickCount64()+2000;
    if (!captureEnabled || captureBlocked || !captureSlot || !playable()
        || HUD::IS_PAUSE_MENU_ACTIVE() || !CAM::IS_SCREEN_FADED_IN()
        || captureSlot->state().phase!=pal::CapturePhase::deployed
        || !ownedCompanion() || ENTITY::IS_ENTITY_DEAD(companion) || combatTarget) return;
    // Only the decorated mod-owned companion is touched, never the story herd.
    if (!ENTITY::DOES_ENTITY_BELONG_TO_THIS_SCRIPT(companion,false))
        ENTITY::SET_ENTITY_AS_MISSION_ENTITY(companion,true,true);
    if (PED::IS_PED_IN_GROUP(companion)) PED::REMOVE_PED_FROM_GROUP(companion);
    PED::SET_PED_KEEP_TASK(companion,true);
    if (distanceSquared(companion,PLAYER::PLAYER_PED_ID())>25.0f) resumeCompanionFollow();
}

bool combatCandidate(Ped ped, Ped player) {
    if (!ped || ped == player || ped == companion || !ENTITY::DOES_ENTITY_EXIST(ped)
        || !ENTITY::IS_ENTITY_A_PED(ped)
        || PED::IS_PED_A_PLAYER(ped)
        || PED::_IS_THIS_MODEL_A_HORSE(ENTITY::GET_ENTITY_MODEL(ped))
        || DECORATOR::DECOR_EXIST_ON(ped, companionDecorator)
        || PED::IS_PED_DEAD_OR_DYING(ped, true)
        || ENTITY::IS_ENTITY_A_MISSION_ENTITY(ped) || ENTITY::IS_ENTITY_ATTACHED(ped)
        || PED::IS_PED_IN_ANY_VEHICLE(ped, true) || PED::IS_PED_ON_MOUNT(ped)
        || PED::IS_PED_HOGTIED(ped)
        || distanceSquared(player, ped) > 1225.0f
        || distanceSquared(companion, ped) > 1600.0f) return false;
    return true;
}

bool confirmedThreat(Ped ped, Ped player) {
    if (!combatCandidate(ped, player)) return false;
    // Includes actively attacking predators; mere animal presence is insufficient.
    return PED::IS_PED_IN_COMBAT(ped, player) || PED::IS_PED_IN_COMBAT(ped, companion);
}

bool confirmedCombatTarget(Ped ped, Ped player, ULONGLONG now) {
    return confirmedThreat(ped, player)
        || (ped == huntTarget && now < huntUntil && combatCandidate(ped, player)
            && !PED::IS_PED_HUMAN(ped));
}

void updateHuntTarget(Ped player, ULONGLONG now, bool playerShot) {
    Entity aimed = 0;
    if (PLAYER::GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(PLAYER::PLAYER_ID(), &aimed)
        && combatCandidate(aimed, player) && !PED::IS_PED_HUMAN(aimed)) {
        lastAimedAnimal = aimed; lastAnimalAim = now;
    }
    // Brief aim history tolerates recoil moving the reticle on the shot frame.
    // Never use a sticky "ever damaged by player" flag to select random game.
    if (playerShot && lastAimedAnimal && now - lastAnimalAim <= 250
        && combatCandidate(lastAimedAnimal, player)
        && !PED::IS_PED_HUMAN(lastAimedAnimal)) {
        huntTarget = lastAimedAnimal; huntUntil = now + 15000;
        trace("Companion hunt: deliberately shot-at animal=" + std::to_string(huntTarget));
    }
    if (huntTarget && (now >= huntUntil || !combatCandidate(huntTarget, player))) huntTarget = 0;
}

void updateEnemyOpponents(Ped player, ULONGLONG now) {
    for (Ped enemy : combatPool) {
        // Only existing enemies can be redirected. Fleeing hunted game is never
        // given a combat task just because the companion is pursuing it.
        if (!confirmedThreat(enemy, player)) { enemyAssignments.erase(enemy); continue; }
        const float playerDistance = distanceSquared(enemy, player);
        const float palDistance = distanceSquared(enemy, companion);
        const auto opponent = pal::nearestOpponent(playerDistance,
            ENTITY::HAS_ENTITY_CLEAR_LOS_TO_ENTITY(enemy, player, 17), palDistance,
            combatTarget != 0 && ENTITY::HAS_ENTITY_CLEAR_LOS_TO_ENTITY(enemy, companion, 17));
        const Ped desired = opponent == pal::Opponent::companion ? companion
            : opponent == pal::Opponent::player ? player : 0;
        if (!desired) continue;
        auto& assignment = enemyAssignments[enemy];
        if (now < assignment.nextUpdate) continue;
        assignment.nextUpdate = now + 2000;
        // Leave native player-target combat alone unless undoing our Pal target.
        if (desired == player && assignment.target == 0) continue;
        if (desired != assignment.target || !PED::IS_PED_IN_COMBAT(enemy, desired)) {
            TASK::TASK_COMBAT_PED(enemy, desired, 0, 0);
            assignment.target = desired;
            trace("Enemy opponent: enemy=" + std::to_string(enemy)
                + (desired == companion ? " target=companion" : " target=player"));
        }
    }
}

void updateCompanionCombat() {
    if (!companionCombatEnabled || !captureEnabled || captureBlocked || !captureSlot
        || captureSlot->state().phase != pal::CapturePhase::deployed
        || !ownedCompanion() || ENTITY::IS_ENTITY_DEAD(companion)) {
        if (!enemyAssignments.empty()) disengageEnemyTargets();
        companionEncounter.reset(); combatTarget = 0; combatPool.clear();
        huntTarget = lastAimedAnimal = 0; return;
    }
    if (!playable() || HUD::IS_PAUSE_MENU_ACTIVE() || !CAM::IS_SCREEN_FADED_IN()) {
        if (combatTarget) resumeCompanionFollow();
        companionEncounter.reset(); combatTarget = 0; combatPool.clear();
        huntTarget = lastAimedAnimal = 0; return;
    }
    const auto now = GetTickCount64();
    const Ped player = PLAYER::PLAYER_PED_ID();
    const bool playerShot = PED::IS_PED_SHOOTING(player);
    updateHuntTarget(player, now, playerShot);
    if (now >= nextThreatScan) {
        int handles[1024]{};
        const int count = std::clamp(worldGetAllPeds(handles, 1024), 0, 1024);
        combatPool.assign(handles, handles + count);
        nextThreatScan = now + 250;
    }
    Ped nearest = 0;
    float best = 1601.0f;
    bool hostileShot = false;
    struct ThreatSample { Ped ped; float distance; bool attackingAnimal; };
    std::vector<ThreatSample> threats;
    for (Ped ped : combatPool) {
        if (!confirmedCombatTarget(ped, player, now)) continue;
        hostileShot = hostileShot || PED::IS_PED_SHOOTING(ped);
        const float distance = distanceSquared(companion, ped);
        threats.push_back({ped, distance, !PED::IS_PED_HUMAN(ped) && confirmedThreat(ped, player)});
    }
    const bool shotGateOpen = companionEncounter.update(now, playerShot, hostileShot, !threats.empty());
    for (const auto& threat : threats) {
        if (pal::mayEngage(shotGateOpen, threat.attackingAnimal) && threat.distance < best) {
            nearest = threat.ped; best = threat.distance;
        }
    }
    const bool engage = nearest != 0;
    if (!engage) {
        if (combatTarget) { trace("Companion combat: no eligible threat; resuming follow"); resumeCompanionFollow(); }
        if (!enemyAssignments.empty()) disengageEnemyTargets();
        combatTarget = 0; return;
    }
    if (nearest != combatTarget || now >= nextCombatTask) {
        if (nearest != combatTarget) {
            trace("Companion charge: target=" + std::to_string(nearest)
                + (shotGateOpen ? "; shot gate open" : "; immediate animal defense"));
            report("Lamball defending you: close-range charge test. F10 recalls.");
        }
        combatTarget = nearest;
        TASK::TASK_GO_TO_ENTITY(companion, nearest, -1, 0.8f, 4.0f, 0.0f, 0);
        nextCombatTask = now + 1500;
    }
    updateEnemyOpponents(player, now);
    // No ranged damage: require current hostility, physical proximity, LOS,
    // and an upright companion. F10 recall is processed before this update.
    if (now >= nextContactStrike && best <= 2.25f && confirmedCombatTarget(nearest, player, now)
        && !PED::IS_PED_RAGDOLL(companion)
        && ENTITY::HAS_ENTITY_CLEAR_LOS_TO_ENTITY(companion, nearest, 17)) {
        PED::APPLY_DAMAGE_TO_PED(nearest, 12, 0, 0, companion);
        nextContactStrike = now + 2000;
        trace("Companion contact strike: target=" + std::to_string(nearest) + "; damage=12");
    }
}

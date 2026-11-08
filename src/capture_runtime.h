// Included inside plugin.cpp's anonymous namespace, on the script fiber only.
// Original developer integration; no game resources are embedded here.
std::atomic_bool captureRequested{false}, companionRequested{false}, rosterRequested{false};
bool captureEnabled = false, captureBlocked = false;
bool expandedSpeciesEnabled = false;
bool missionSheepEnabled = false;
std::unique_ptr<pal::CaptureSlot> captureSlot;
std::filesystem::path capturePath;
Ped companion = 0;
int companionToken = 0;
constexpr const char* companionDecorator = "pals_proto_owner";
ULONGLONG nextCaptureAction = 0;

bool storeCapture(const pal::CaptureSave& save) {
    if (capturePath.empty()) return false;
    const auto temp = std::filesystem::path(capturePath.wstring() + L".tmp");
    const auto data = pal::encode(save);
    HANDLE file = CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool ok = WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written, nullptr)
        && written == data.size() && FlushFileBuffers(file);
    CloseHandle(file);
    if (ok) ok = MoveFileExW(temp.c_str(), capturePath.c_str(),
                            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    if (!ok) trace("Capture save failed; previous save retained; error=" + std::to_string(GetLastError()));
    return ok;
}

void captureStatus() {
    if (!captureSlot || captureBlocked) {
        report("Capture test blocked: inspect log/save before retrying."); return;
    }
    static const char* phases[] = {"empty", "held Pal", "companion deployed",
        "capture pending", "release pending", "recall pending", "recovery required"};
    const auto& s = captureSlot->state();
    const auto selected=captureSlot->selectedPal();
    report(std::string("Capture TEST: ") + phases[static_cast<int>(s.phase)]
        + "; species=" + pal::species[s.species].name
        + "; party=" + std::to_string(captureSlot->partySize()) + "/5; selected=" + pal::species[selected.species].name
        + "; spheres=" + std::to_string(s.spheres) + "; fragments=" + std::to_string(s.fragments)
        + "; F9 aim/capture; F10 release/recall; F11 status.");
}

void initializeCapture() {
    const auto ini = basePath + "RDR2PalPrototype.ini";
    captureEnabled = GetPrivateProfileIntA("Prototype", "CapturePrototypeEnabled", 0, ini.c_str()) != 0;
    expandedSpeciesEnabled = GetPrivateProfileIntA("Prototype", "ExpandedSpeciesEnabled", 0, ini.c_str()) != 0;
    missionSheepEnabled = GetPrivateProfileIntA("Prototype", "MissionSheepCaptureEnabled", 0, ini.c_str()) != 0;
    if (!captureEnabled) return;
    const auto profile = setting("CaptureProfile");
    // Explicit profile prevents quietly mixing a test roster with another save.
    if (profile.empty() || profile.size() > 32 || profile.find_first_not_of(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") != std::string::npos
        || logPath.empty()) {
        captureBlocked = true; report("Capture test needs CaptureProfile (letters/numbers/_/-) and a writable log directory."); return;
    }
    capturePath = logPath.parent_path() / ("capture-test-" + profile + ".txt");
    pal::CaptureSave save;
    std::error_code error;
    const bool exists = std::filesystem::exists(capturePath, error);
    if (error) { captureBlocked = true; report("Capture save cannot be inspected."); return; }
    if (exists) {
        std::ifstream input(capturePath);
        std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        if (!input || !pal::decode(text, save)) {
            captureBlocked = true; report("Capture save unreadable/corrupt; left untouched."); return;
        }
    }
    captureSlot = std::make_unique<pal::CaptureSlot>(save, storeCapture);
    if (!captureSlot->recoverOnStartup()) captureBlocked = true;
    DECORATOR::DECOR_REGISTER(companionDecorator, 3);
    trace("Capture test profile=" + profile + "; independent mod save, not tied to RDR2 save slots.");
    captureStatus();
}

bool captureReady() {
    if (!captureEnabled || captureBlocked || !captureSlot) return false;
    if (!playable() || HUD::IS_PAUSE_MENU_ACTIVE() || !CAM::IS_SCREEN_FADED_IN()) {
        report("Capture test unavailable outside live Story Mode."); return false;
    }
    const Ped player = PLAYER::PLAYER_PED_ID();
    if (PED::IS_PED_ON_MOUNT(player) || PED::IS_PED_IN_ANY_VEHICLE(player, true)) {
        report("Capture test: dismount and stand on foot."); return false;
    }
    if (GetTickCount64() < nextCaptureAction) return false;
    nextCaptureAction = GetTickCount64() + 1000;
    return true;
}

void captureFailure() {
    captureBlocked = true;
    report("Capture transaction blocked after save failure; inspect log before restarting.");
}

Hash captureEntityScript(Entity entity) {
    alignas(8) Any details[16]{};
    return ENTITY::_GET_ENTITY_SCRIPT(entity,details);
}
bool missionSheepEligible(Ped target) {
    if (!missionSheepEnabled || !captureSlot || !ENTITY::IS_ENTITY_A_MISSION_ENTITY(target)) return false;
    const auto script=MISC::GET_HASH_KEY("mudtown4");
    pal::MissionSheepEvidence evidence;
    evidence.enabled=true;evidence.sheep=ENTITY::GET_ENTITY_MODEL(target)==MISC::GET_HASH_KEY("a_c_sheep_01");
    evidence.claimed=captureSlot->state().missionSheepClaimed;
    evidence.missionRunning=SCRIPTS::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(script)>0;
    evidence.ownerMatches=captureEntityScript(target)==script;
    trace("Mission sheep capture gate: owner="+std::to_string(captureEntityScript(target))
        +"; running="+std::to_string(evidence.missionRunning)+"; claimed="+std::to_string(evidence.claimed));
    if (!evidence.sheep || !evidence.missionRunning || !evidence.ownerMatches || evidence.claimed) return false;
    int handles[1024]{};
    const int count=std::clamp(worldGetAllPeds(handles,1024),0,1024);
    for(int i=0;i<count;++i) {
        const Ped ped=handles[i];
        if (ped!=target && ENTITY::DOES_ENTITY_EXIST(ped) && !PED::IS_PED_DEAD_OR_DYING(ped,true)
            && ENTITY::GET_ENTITY_MODEL(ped)==MISC::GET_HASH_KEY("a_c_sheep_01")
            && captureEntityScript(ped)==script) ++evidence.otherLivingSheep;
    }
    trace("Mission sheep capture gate: other living sheep="+std::to_string(evidence.otherLivingSheep));
    return pal::mayCaptureMissionSheep(evidence);
}

void captureAimedLamball() {
    if (!captureReady()) return;
    const auto phase=captureSlot->state().phase;
    if ((phase!=pal::CapturePhase::empty && phase!=pal::CapturePhase::held) || captureSlot->partySize()>=5) {
        report("Recall your companion before capturing. Party capacity: 5."); return;
    }
    Entity target = 0;
    int targetSpecies = -1;
    const bool aimed = PLAYER::GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(PLAYER::PLAYER_ID(), &target);
    if (aimed && ENTITY::DOES_ENTITY_EXIST(target)) {
        const auto model = ENTITY::GET_ENTITY_MODEL(target);
        for (int i=0; i < (expandedSpeciesEnabled ? static_cast<int>(pal::species.size()) : 1); ++i)
            if (model == MISC::GET_HASH_KEY(pal::species[i].model)) targetSpecies = i;
    }
    const bool missionTarget=aimed && targetSpecies==0 && ENTITY::DOES_ENTITY_EXIST(target) && missionSheepEligible(target);
    if (!aimed || targetSpecies < 0 || !eligible(target, 0, missionTarget) || ENTITY::IS_ENTITY_ATTACHED(target)
        || PED::IS_PED_HOGTIED(target)
        || distanceSquared(PLAYER::PLAYER_PED_ID(), target) > 625.0f
        || !ENTITY::HAS_ENTITY_CLEAR_LOS_TO_ENTITY(PLAYER::PLAYER_PED_ID(), target, 17)) {
        report("Aim at a live, supported, unprotected Pal within 25m. No sphere consumed."); return;
    }
    const int health = ENTITY::GET_ENTITY_HEALTH(target);
    const int maximum = ENTITY::GET_ENTITY_MAX_HEALTH(target, false);
    const double chance = pal::captureChance(health, maximum);
    if (chance == 0) { report("Invalid/dead capture target; no sphere consumed."); return; }
    if (captureSlot->state().spheres == 0) { report("No test spheres left in this profile."); return; }
    // Revalidate the mission exception immediately before any persistent inventory change.
    if (missionTarget && !missionSheepEligible(target)) { report("Mission herd changed; capture cancelled."); return; }
    if (!captureSlot->prepareCapture()) { captureFailure(); return; }
    if (!captureSlot->spendSphere()) { captureFailure(); return; }
    static std::mt19937 random{std::random_device{}()};
    const double roll = std::generate_canonical<double, 53>(random);
    trace("Capture test: chance=" + std::to_string(chance) + "; roll=" + std::to_string(roll));
    if (roll >= chance) {
        if (!captureSlot->restoreHeld()) { captureFailure(); return; }
        report("Pal escaped! One test sphere consumed; existing party retained."); return;
    }
    if (!captureSlot->beginCapture(health, maximum, targetSpecies, missionTarget)) { captureFailure(); return; }
    trace("BEGIN capture DELETE_PED: target=" + std::to_string(target));
    ENTITY::SET_ENTITY_AS_MISSION_ENTITY(target, true, true);
    Ped old = target; PED::DELETE_PED(&old);
    const bool removed = !ENTITY::DOES_ENTITY_EXIST(target);
    if (!removed && !missionTarget) { Entity release = target; ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&release); }
    trace(std::string("END capture DELETE_PED: removed=") + (removed ? "true" : "false"));
    if (!captureSlot->finishCapture(removed)) { captureFailure(); return; }
    report(removed ? std::string("Captured ") + pal::species[targetSpecies].name + "! F10 releases it. Recall before save/quit."
                   : "Capture removal failed; animal retained; sphere consumed.");
}

bool ownedCompanion() {
    return companion && ENTITY::DOES_ENTITY_EXIST(companion)
        && captureSlot && ENTITY::GET_ENTITY_MODEL(companion) == MISC::GET_HASH_KEY(pal::species[captureSlot->state().species].model)
        && DECORATOR::DECOR_EXIST_ON(companion, companionDecorator)
        && DECORATOR::DECOR_GET_INT(companion, companionDecorator) == companionToken;
}

void updateCompanionDeath() {
    if (!captureEnabled || captureBlocked || !captureSlot
        || captureSlot->state().phase != pal::CapturePhase::deployed
        || !ownedCompanion() || !ENTITY::IS_ENTITY_DEAD(companion)) return;
    disengageEnemyTargets();
    const bool saved = captureSlot->markDead();
    Entity corpse = companion;
    ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&corpse);
    companion = 0;
    trace("Companion died: corpse retained/released for native skinning; ownership save=" + std::string(saved ? "ok" : "failed"));
    if (!saved) captureFailure();
    else report("Your Pal died. Its corpse remains in the world; F10 cannot resurrect it.");
}

void toggleCompanion() {
    if (!captureReady()) return;
    auto phase = captureSlot->state().phase;
    if (phase == pal::CapturePhase::deployed) {
        if (!ownedCompanion() || ENTITY::IS_ENTITY_DEAD(companion)) {
            if (!captureSlot->markLost()) captureFailure();
            report("Companion missing/dead: recovery required; no duplicate spawned."); return;
        }
        if (!captureSlot->beginRecall(ENTITY::GET_ENTITY_HEALTH(companion),
                                     ENTITY::GET_ENTITY_MAX_HEALTH(companion, false))) {
            captureFailure(); return;
        }
        const Ped original = companion;
        disengageEnemyTargets();
        trace("BEGIN recall DELETE_PED");
        PED::DELETE_PED(&companion);
        const bool removed = !ENTITY::DOES_ENTITY_EXIST(original);
        if (!removed) companion = original;
        trace("END recall DELETE_PED");
        if (!captureSlot->finishRecall(removed)) { captureFailure(); return; }
        report(removed ? "Pal recalled and saved. Safe to quit/reload this test profile."
                       : "Recall failed; companion remains deployed.");
        if (!removed || captureSlot->state().selected==0) return;
        phase=pal::CapturePhase::held;
    }
    if (phase != pal::CapturePhase::held) { captureStatus(); return; }
    if (!captureSlot->activateSelected()) { captureFailure(); return; }
    const Hash model = MISC::GET_HASH_KEY(pal::species[captureSlot->state().species].model);
    STREAMING::REQUEST_MODEL(model, false);
    const auto deadline = GetTickCount64() + 5000;
    while (!STREAMING::HAS_MODEL_LOADED(model) && GetTickCount64() < deadline) WAIT(0);
    if (!STREAMING::HAS_MODEL_LOADED(model) || !playable() || HUD::IS_PAUSE_MENU_ACTIVE()
        || !CAM::IS_SCREEN_FADED_IN()) {
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
        report("Companion model unavailable; captured Pal retained."); return;
    }
    const Ped player = PLAYER::PLAYER_PED_ID();
    if (PED::IS_PED_ON_MOUNT(player) || PED::IS_PED_IN_ANY_VEHICLE(player, true)) {
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model); return;
    }
    const auto pos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(player, 1.5f, 2.0f, 0.0f);
    float ground = 0;
    if (!MISC::GET_GROUND_Z_FOR_3D_COORD(pos.x, pos.y, pos.z + 2.0f, &ground, false)
        || std::abs(ground - pos.z) > 2.0f) {
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
        report("No nearby release ground; captured Pal retained."); return;
    }
    if (!captureSlot->beginRelease()) {
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model); captureFailure(); return;
    }
    trace("BEGIN release CREATE_PED");
    companion = PED::CREATE_PED(model, pos.x, pos.y, ground + 0.1f,
                               ENTITY::GET_ENTITY_HEADING(player), false, true, false, false);
    trace("END release CREATE_PED: handle=" + std::to_string(companion));
    bool created = companion && ENTITY::DOES_ENTITY_EXIST(companion)
        && ENTITY::GET_ENTITY_MODEL(companion) == model && !ENTITY::IS_ENTITY_DEAD(companion);
    if (created) {
        companionToken = static_cast<int>((GetTickCount64() % 2000000000) + 1);
        created = DECORATOR::DECOR_SET_INT(companion, companionDecorator, companionToken) != 0;
    }
    if (created) {
        ENTITY::SET_ENTITY_AS_MISSION_ENTITY(companion, true, true);
        ENTITY::SET_ENTITY_INVINCIBLE(companion, false);
        ENTITY::SET_ENTITY_CAN_BE_DAMAGED(companion, true);
        PED::SET_PED_CAN_BE_TARGETTED(companion, true);
        PED::SET_PED_CAN_BE_TARGETTED_BY_PLAYER(companion, player, true);
        PED::_SET_RANDOM_OUTFIT_VARIATION(companion, true);
        ENTITY::SET_ENTITY_MAX_HEALTH(companion, captureSlot->state().maxHealth);
        ENTITY::SET_ENTITY_HEALTH(companion, captureSlot->state().health, 0);
        PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(companion, true);
        PED::REMOVE_PED_FROM_GROUP(companion);
        FLOCK::_SET_ANIMAL_IS_WILD(companion,false);
        PED::SET_PED_KEEP_TASK(companion,true);
        PED::SET_PED_RELATIONSHIP_GROUP_HASH(companion, PED::GET_PED_RELATIONSHIP_GROUP_HASH(player));
        TASK::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(companion, player, 1.0f, -2.0f, 0.0f,
                                            2.0f, -1, 2.5f, true, false, false, false, false, false);
    } else if (companion && ENTITY::DOES_ENTITY_EXIST(companion)) {
        ENTITY::SET_ENTITY_AS_MISSION_ENTITY(companion, true, true);
        const Ped failed = companion;
        PED::DELETE_PED(&companion);
        if (ENTITY::DOES_ENTITY_EXIST(failed)) {
            STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
            captureFailure(); return; // Journal remains releasing; never spawn a second copy.
        }
    }
    STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
    if (!captureSlot->finishRelease(created)) { captureFailure(); return; }
    report(created ? std::string(pal::species[captureSlot->state().species].name) + " released to follow. F10 recalls. Recall before save/load/quit."
                   : "Release failed; captured Pal retained.");
}

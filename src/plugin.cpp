#include <windows.h>
#include <shlobj.h>
#include <algorithm>
#include <atomic>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include "main.h"
#include "natives.h"
#include "targeting.h"

namespace {
HMODULE module;
std::atomic_bool requested{false};
std::string basePath;
std::filesystem::path logPath;
constexpr const char* version = RDR2PAL_VERSION;
std::string status;
ULONGLONG statusUntil = 0;
bool debugHotkey = true;
bool dryRun = true;
float radius = 25;
std::string sourceModel, replacementModel;
unsigned postSwapFrames = 0;

void trace(const std::string& message) {
    if (logPath.empty()) return;
    SYSTEMTIME now{};
    GetSystemTime(&now);
    std::ofstream(logPath, std::ios::app)
        << std::setfill('0') << std::setw(4) << now.wYear << '-'
        << std::setw(2) << now.wMonth << '-' << std::setw(2) << now.wDay
        << 'T' << std::setw(2) << now.wHour << ':' << std::setw(2) << now.wMinute
        << ':' << std::setw(2) << now.wSecond << '.' << std::setw(3) << now.wMilliseconds
        << "Z v" << version << " pid=" << GetCurrentProcessId()
        << " tid=" << GetCurrentThreadId() << " tick=" << GetTickCount64() << ' '
        << message << '\n';
    // The stream closes on every marker, so the last completed stage survives a hang.
}

void report(const std::string& message) {
    status = std::string("v") + version + " " + message;
    statusUntil = GetTickCount64() + 7000;
    trace(message);
}

void initializeLog() {
    // Original diagnostic code only; no game content is written to this log.
    // A game installed in a protected folder may not permit adjacent log files.
    wchar_t folder[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE,
                                  nullptr, SHGFP_TYPE_CURRENT, folder))) {
        const auto directory = std::filesystem::path(folder) / L"RDR2PalPrototype";
        std::error_code error;
        std::filesystem::create_directories(directory, error);
        const auto candidate = directory / L"RDR2PalPrototype.log";
        if (!error && std::ofstream(candidate, std::ios::app)) logPath = candidate;
    }
    if (logPath.empty() && GetTempPathW(MAX_PATH, folder)) {
        const auto candidate = std::filesystem::path(folder) / L"RDR2PalPrototype.log";
        if (std::ofstream(candidate, std::ios::app)) logPath = candidate;
    }
}

std::string setting(const char* name) {
    char buffer[128]{};
    GetPrivateProfileStringA("Prototype", name, "", buffer, sizeof(buffer),
                            (basePath + "RDR2PalPrototype.ini").c_str());
    return buffer;
}

bool playable() {
    if (NETWORK::NETWORK_IS_SESSION_ACTIVE() || !PLAYER::IS_PLAYER_PLAYING(PLAYER::PLAYER_ID()))
        return false;
    const Ped player = PLAYER::PLAYER_PED_ID();
    return ENTITY::DOES_ENTITY_EXIST(player) && !ENTITY::IS_ENTITY_DEAD(player);
}

bool eligible(Ped ped, Hash source) {
    if (!ENTITY::DOES_ENTITY_EXIST(ped) || ped == PLAYER::PLAYER_PED_ID()
        || PED::IS_PED_A_PLAYER(ped) || PED::IS_PED_HUMAN(ped)
        || PED::IS_PED_DEAD_OR_DYING(ped, true)
        || ENTITY::IS_ENTITY_A_MISSION_ENTITY(ped)
        || PED::_IS_THIS_MODEL_A_HORSE(ENTITY::GET_ENTITY_MODEL(ped))
        || PED::_GET_RIDER_OF_MOUNT(ped, true) != 0
        || PED::IS_PED_IN_ANY_VEHICLE(ped, true)) return false;
    return !source || ENTITY::GET_ENTITY_MODEL(ped) == source;
}

float distanceSquared(Ped a, Ped b) {
    auto p = ENTITY::GET_ENTITY_COORDS(a, true, false);
    auto q = ENTITY::GET_ENTITY_COORDS(b, true, false);
    const float x = p.x-q.x, y = p.y-q.y, z = p.z-q.z;
    return x*x + y*y + z*z;
}

// Called on the Script Hook script fiber only. A future inventory adapter can
// queue this same action; native calls must never run inside the keyboard callback.
void transformNearestAnimal() {
    trace("BEGIN swap: gameplay eligibility");
    if (!playable() || HUD::IS_PAUSE_MENU_ACTIVE() || !CAM::IS_SCREEN_FADED_IN()) {
        report("Unavailable: return to live single-player gameplay."); return;
    }
    const Ped player = PLAYER::PLAYER_PED_ID();
    const Hash source = sourceModel.empty() ? 0 : MISC::GET_HASH_KEY(sourceModel.c_str());
    int handles[1024]{};
    trace("BEGIN worldGetAllPeds");
    const int count = std::clamp(worldGetAllPeds(handles, 1024), 0, 1024);
    trace("END worldGetAllPeds: count=" + std::to_string(count));
    std::vector<pal::Candidate> candidates;
    for (int i=0; i<count; ++i) {
        if (eligible(handles[i], source))
            candidates.push_back({handles[i], distanceSquared(player, handles[i]), true});
    }
    const Ped original = pal::nearest(candidates, radius);
    if (!original) { report("No eligible animal within " + std::to_string(static_cast<int>(radius))
                            + "m. Filter: " + (sourceModel.empty() ? "any animal" : sourceModel)); return; }
    if (dryRun) {
        report("Dry run: selected animal " + std::to_string(original)
               + "; no animal changed. Inventory action and Pal asset are pending."); return;
    }
    if (replacementModel.empty()) { report("Missing ReplacementModel: install a converted Pal first."); return; }
    const Hash model = MISC::GET_HASH_KEY(replacementModel.c_str());
    if (!STREAMING::IS_MODEL_VALID(model) || !STREAMING::IS_MODEL_A_PED(model)) {
        report("Replacement is not a registered ped model; animal unchanged."); return;
    }
    if (model == ENTITY::GET_ENTITY_MODEL(original)) {
        report("Target already uses the configured model; animal unchanged."); return;
    }
    const Hash originalModel = ENTITY::GET_ENTITY_MODEL(original);
    trace("BEGIN REQUEST_MODEL: target=" + std::to_string(original)
        + " sourceHash=" + std::to_string(originalModel) + " replacementHash=" + std::to_string(model));
    STREAMING::REQUEST_MODEL(model, false);
    trace("END REQUEST_MODEL; BEGIN wait for model");
    const auto deadline = GetTickCount64() + 5000;
    while (!STREAMING::HAS_MODEL_LOADED(model) && GetTickCount64() < deadline) WAIT(0);
    trace("END wait for model");
    if (!STREAMING::HAS_MODEL_LOADED(model) || !playable() || HUD::IS_PAUSE_MENU_ACTIVE()
        || !CAM::IS_SCREEN_FADED_IN()
        || !eligible(original, source) || ENTITY::GET_ENTITY_MODEL(original) != originalModel
        || distanceSquared(PLAYER::PLAYER_PED_ID(), original) > radius*radius) {
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
        report("Model timeout or target changed; original animal preserved."); return;
    }
    const auto position = ENTITY::GET_ENTITY_COORDS(original, true, false);
    const float heading = ENTITY::GET_ENTITY_HEADING(original);
    trace("BEGIN CREATE_PED: xyz=" + std::to_string(position.x) + ","
        + std::to_string(position.y) + "," + std::to_string(position.z));
    Ped replacement = PED::CREATE_PED(model, position.x, position.y, position.z,
                                       heading, false, true, false, false);
    trace("END CREATE_PED: handle=" + std::to_string(replacement));
    if (!replacement || !ENTITY::DOES_ENTITY_EXIST(replacement)
        || ENTITY::GET_ENTITY_MODEL(replacement) != model || PED::IS_PED_HUMAN(replacement)
        || ENTITY::IS_ENTITY_DEAD(replacement)) {
        if (replacement && ENTITY::DOES_ENTITY_EXIST(replacement)) PED::DELETE_PED(&replacement);
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
        report("Replacement creation failed; original animal preserved."); return;
    }
    trace("BEGIN _SET_RANDOM_OUTFIT_VARIATION");
    PED::_SET_RANDOM_OUTFIT_VARIATION(replacement, true);
    trace("END _SET_RANDOM_OUTFIT_VARIATION");
    Ped old = original;
    trace("BEGIN SET_ENTITY_AS_MISSION_ENTITY original");
    ENTITY::SET_ENTITY_AS_MISSION_ENTITY(old, true, true);
    trace("END SET_ENTITY_AS_MISSION_ENTITY; BEGIN DELETE_PED original");
    PED::DELETE_PED(&old);
    trace("END DELETE_PED original");
    if (ENTITY::DOES_ENTITY_EXIST(original)) {
        PED::DELETE_PED(&replacement);
        Entity release = original;
        ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&release);
        report("Original could not be removed; replacement rolled back.");
    } else {
        trace("BEGIN TASK_WANDER_STANDARD");
        TASK::TASK_WANDER_STANDARD(replacement, 10.0f, 0);
        trace("END TASK_WANDER_STANDARD; BEGIN SET_PED_AS_NO_LONGER_NEEDED");
        ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&replacement);
        trace("END SET_PED_AS_NO_LONGER_NEEDED");
        postSwapFrames = 1;
        report("Model swap completed. Custom AI, skinning and persistence are unverified.");
    }
    trace("BEGIN SET_MODEL_AS_NO_LONGER_NEEDED");
    STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
    trace("END SET_MODEL_AS_NO_LONGER_NEEDED; END swap function");
}

void keyboard(DWORD key, WORD, BYTE, BOOL, BOOL withAlt, BOOL wasDown, BOOL isUp) {
    if (key == VK_F8 && !isUp && !wasDown && !withAlt) requested.store(true);
}

void scriptMain() {
    char path[MAX_PATH]{};
    GetModuleFileNameA(module, path, MAX_PATH);
    basePath = path;
    basePath = basePath.substr(0, basePath.find_last_of("\\/") + 1);
    initializeLog();
    const auto ini = basePath + "RDR2PalPrototype.ini";
    debugHotkey = GetPrivateProfileIntA("Prototype", "DebugHotkeyEnabled", 1, ini.c_str()) != 0;
    dryRun = GetPrivateProfileIntA("Prototype", "DryRun", 1, ini.c_str()) != 0;
    radius = static_cast<float>(std::clamp(GetPrivateProfileIntA("Prototype", "RadiusMeters", 25, ini.c_str()), 1u, 100u));
    sourceModel = setting("SourceModel");
    replacementModel = setting("ReplacementModel");
    report("Startup: ASI=" + std::string(path) + "; F8=" + (debugHotkey ? "on" : "off")
        + "; DryRun=" + (dryRun ? "on" : "off")
        + "; ReplacementModel=" + replacementModel + "; SourceModel=" + sourceModel
        + "; RadiusMeters=" + std::to_string(radius));
    report("Startup: AssetBuild=" + setting("AssetBuild")
        + "; appearance must be verified independently of model swap success.");
    report(std::string("Startup: vfs.asi module=")
        + (GetModuleHandleW(L"vfs.asi") ? "present" : "absent")
        + "; module presence does not confirm resource replacement.");
    report(logPath.empty() ? "Prototype loaded; log file unavailable. F8 developer test."
        : "Prototype loaded; logging enabled in LocalAppData. F8 developer test.");
    ULONGLONG nextHeartbeat = GetTickCount64() + 5000;
    while (true) {
        if (GetTickCount64() >= nextHeartbeat) {
            trace("HEARTBEAT script fiber running");
            nextHeartbeat = GetTickCount64() + 5000;
        }
        if (requested.exchange(false) && debugHotkey) transformNearestAnimal();
        if (!status.empty() && GetTickCount64() < statusUntil && playable()) {
            const char* text = MISC::VAR_STRING(10, "LITERAL_STRING", status.c_str());
            // Current background text natives; older HUD text calls became no-ops.
            UIDEBUG::_BG_SET_TEXT_SCALE(0.30f, 0.30f);
            UIDEBUG::_BG_SET_TEXT_COLOR(255, 255, 255, 255);
            UIDEBUG::_BG_DISPLAY_TEXT(text, 0.04f, 0.80f);
        }
        if (postSwapFrames == 1) trace("BEGIN first WAIT after swap");
        WAIT(0);
        if (postSwapFrames) {
            if (postSwapFrames == 1 || postSwapFrames == 2 || postSwapFrames == 5
                || postSwapFrames == 15 || postSwapFrames == 30)
                trace("POST_SWAP frame=" + std::to_string(postSwapFrames));
            if (++postSwapFrames > 30) postSwapFrames = 0;
        }
    }
}
}

BOOL APIENTRY DllMain(HMODULE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        module = instance;
        DisableThreadLibraryCalls(instance);
        scriptRegister(instance, scriptMain);
        keyboardHandlerRegister(keyboard);
    } else if (reason == DLL_PROCESS_DETACH) {
        keyboardHandlerUnregister(keyboard);
        scriptUnregister(instance);
    }
    return TRUE;
}

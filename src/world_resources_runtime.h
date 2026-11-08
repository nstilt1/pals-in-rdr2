// Included on the plugin script fiber. Local catalog/models are external inputs.
bool resourcesEnabled = false;
pal::ResourceCatalog resourceCatalog;
struct ResourceInstance {
    Object object = 0;
    Hash model = 0;
    ULONGLONG requestedAt = 0;
    bool failed = false;
};
std::vector<ResourceInstance> resourceInstances;
Prompt resourcePrompt = 0;
constexpr const char* resourceDecorator = "pals_resource_owner";
int resourceToken = 0;
bool resourceInputArmed = false;
ULONGLONG resourceScanAt = 0, resourceActionAt = 0;
Object craftingFire = 0;
Hash craftingFireModel = 0;
constexpr Hash resourcePromptGroup = 0x517A6E23; // Original mod-owned prompt group.

pal::WorldPoint resourcePosition(Entity entity) {
    auto p = ENTITY::GET_ENTITY_COORDS(entity,true,false);
    return {p.x,p.y,p.z};
}
bool ownedResource(size_t i) {
    const auto& r = resourceInstances[i];
    return r.object && ENTITY::DOES_ENTITY_EXIST(r.object)
        && ENTITY::GET_ENTITY_MODEL(r.object) == r.model
        && DECORATOR::DECOR_EXIST_ON(r.object,resourceDecorator)
        && DECORATOR::DECOR_GET_INT(r.object,resourceDecorator) == resourceToken;
}
void removeResource(size_t i) {
    auto& r = resourceInstances[i];
    if (ownedResource(i)) OBJECT::DELETE_OBJECT(&r.object);
    // Keep tracking a failed deletion; its persistent receipt still prevents reward.
    if (!ownedResource(i)) r.object = 0;
}
void resetResourcePrompt() {
    if (resourcePrompt && HUD::_UI_PROMPT_IS_VALID(resourcePrompt)) HUD::_UI_PROMPT_DELETE(resourcePrompt);
    resourcePrompt = HUD::_UI_PROMPT_REGISTER_BEGIN();
    HUD::_UI_PROMPT_SET_CONTROL_ACTION(resourcePrompt,MISC::GET_HASH_KEY("INPUT_CONTEXT_X"));
    HUD::_UI_PROMPT_SET_TEXT(resourcePrompt,MISC::VAR_STRING(10,"LITERAL_STRING","Collect"));
    HUD::_UI_PROMPT_SET_HOLD_MODE(resourcePrompt,1000);
    HUD::_UI_PROMPT_SET_GROUP(resourcePrompt,resourcePromptGroup,0);
    HUD::_UI_PROMPT_SET_ENABLED(resourcePrompt,false);
    HUD::_UI_PROMPT_SET_VISIBLE(resourcePrompt,false);
    HUD::_UI_PROMPT_REGISTER_END(resourcePrompt);
    if (!HUD::_UI_PROMPT_IS_VALID(resourcePrompt)) {
        resourcesEnabled=false; trace("World resource prompt registration failed; feature disabled.");
    }
}
void initializeWorldResources() {
    const auto ini = basePath+"RDR2PalPrototype.ini";
    resourcesEnabled = GetPrivateProfileIntA("Prototype","WorldResourcesPrototypeEnabled",0,ini.c_str()) != 0;
    if (!resourcesEnabled) return;
    if (!captureEnabled || captureBlocked || !captureSlot) {
        resourcesEnabled = false; report("World resources require a valid capture profile."); return;
    }
    const auto path = std::filesystem::path(basePath)/"RDR2PalResources.local.txt";
    std::error_code error;
    const auto size = std::filesystem::file_size(path,error);
    if (error || size > 65536) {
        resourcesEnabled = false; report("World resources need a bounded local resource catalog."); return;
    }
    std::ifstream file(path);
    std::string text((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    if (!file || !pal::parseResourceCatalog(text,resourceCatalog)) {
        resourcesEnabled = false; report("Local resource catalog invalid; no world resources created."); return;
    }
    resourceToken = static_cast<int>((GetTickCount64() ^ GetCurrentProcessId()) & 0x7fffffff) | 1;
    DECORATOR::DECOR_REGISTER(resourceDecorator,3);
    resourceInstances.resize(resourceCatalog.deposits.size());
    for (size_t i=0;i<resourceInstances.size();++i)
        resourceInstances[i].model = MISC::GET_HASH_KEY(resourceCatalog.deposits[i].model.c_str());
    resetResourcePrompt();
    trace("World resource prototype: local camps="+std::to_string(resourceCatalog.camps.size())
        +" deposits="+std::to_string(resourceCatalog.deposits.size())
        +"; shared recipe: 1 fragment -> 1 sphere; no respawn.");
}
bool resourceInventoryReady() {
    if (!captureSlot || captureBlocked) return false;
    const auto phase = captureSlot->state().phase;
    return phase == pal::CapturePhase::empty || phase == pal::CapturePhase::held
        || phase == pal::CapturePhase::deployed;
}
bool canUseResources() {
    if (!resourceInventoryReady() || !playable() || HUD::IS_PAUSE_MENU_ACTIVE()
        || !CAM::IS_SCREEN_FADED_IN()) return false;
    for (const char* app : {"crafting","satchel","shop_menu","pause_menu","hud_quick_select"})
        if (UIAPPS::IS_UIAPP_ACTIVE_BY_HASH(MISC::GET_HASH_KEY(app))) return false;
    const auto player = PLAYER::PLAYER_PED_ID();
    return !PED::IS_PED_ON_MOUNT(player) && !PED::IS_PED_IN_ANY_VEHICLE(player,true)
        && !PED::IS_PED_IN_COMBAT(player,0) && !PLAYER::IS_PLAYER_FREE_AIMING(PLAYER::PLAYER_ID())
        && !PED::IS_PED_SHOOTING(player);
}
bool validCraftingFire() {
    return craftingFire && ENTITY::DOES_ENTITY_EXIST(craftingFire)
        && ENTITY::GET_ENTITY_MODEL(craftingFire) == craftingFireModel
        && pal::atRegisteredCamp(resourceCatalog,resourcePosition(PLAYER::PLAYER_PED_ID()),resourcePosition(craftingFire));
}
void scanCraftingFire() {
    craftingFire = 0;
    auto p = resourcePosition(PLAYER::PLAYER_PED_ID());
    // Detection alone never grants ownership: a private registered camp anchor is required.
    for (const char* name : {"p_campfire01x","p_campfire02x","p_campfire02x_script","p_campfire03x"}) {
        Object fire = OBJECT::GET_CLOSEST_OBJECT_OF_TYPE(static_cast<float>(p.x),static_cast<float>(p.y),
            static_cast<float>(p.z),3.0f,MISC::GET_HASH_KEY(name),false,false,false);
        if (fire && ENTITY::DOES_ENTITY_EXIST(fire)
            && pal::atRegisteredCamp(resourceCatalog,p,resourcePosition(fire))) {
            craftingFire = fire; craftingFireModel = ENTITY::GET_ENTITY_MODEL(fire); return;
        }
    }
}
void streamWorldResources() {
    const auto player = resourcePosition(PLAYER::PLAYER_PED_ID());
    const auto& receipts = captureSlot->state().collected;
    unsigned active = 0;
    for (const auto& r : resourceInstances) if (r.object || r.requestedAt) ++active;
    for (size_t i=0;i<resourceInstances.size();++i) {
        auto& r = resourceInstances[i]; const auto& d = resourceCatalog.deposits[i];
        const bool spent = std::binary_search(receipts.begin(),receipts.end(),d.id);
        const auto distance = pal::worldDistanceSquared(player,d.position);
        if (spent || distance > 2500) {
            removeResource(i);
            if (r.requestedAt) { STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(r.model); r.requestedAt=0; }
            continue;
        }
        if (r.object && !ownedResource(i)) r.object=0;
        if (r.object || r.failed || distance > 1600) continue;
        if (!r.requestedAt) {
            if (active >= 8) continue;
            if (!STREAMING::IS_MODEL_VALID(r.model) || STREAMING::IS_MODEL_A_PED(r.model)) {
                r.failed=true; trace("Resource model unavailable for local deposit "+std::to_string(d.id)); continue;
            }
            STREAMING::REQUEST_MODEL(r.model,false); r.requestedAt=GetTickCount64(); ++active;
        }
        if (!STREAMING::HAS_MODEL_LOADED(r.model)) {
            if (GetTickCount64()-r.requestedAt>5000) {
                STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(r.model); r.requestedAt=0; r.failed=true;
                trace("Resource model timeout for local deposit "+std::to_string(d.id));
            }
            continue;
        }
        r.object=OBJECT::CREATE_OBJECT(r.model,static_cast<float>(d.position.x),static_cast<float>(d.position.y),
            static_cast<float>(d.position.z),false,true,false,false,true);
        if (r.object && ENTITY::DOES_ENTITY_EXIST(r.object)) {
            if (!DECORATOR::DECOR_SET_INT(r.object,resourceDecorator,resourceToken)) {
                OBJECT::DELETE_OBJECT(&r.object); r.failed=true;
                trace("Resource ownership decorator failed; deposit disabled.");
                STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(r.model); r.requestedAt=0;
                continue;
            }
            ENTITY::FREEZE_ENTITY_POSITION(r.object,true);
            ENTITY::SET_ENTITY_COLLISION(r.object,false,false);
        } else r.failed=true;
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(r.model); r.requestedAt=0;
    }
}
void updateWorldResources() {
    if (!resourcesEnabled) return;
    HUD::_UI_PROMPT_SET_ENABLED(resourcePrompt,false);
    HUD::_UI_PROMPT_SET_VISIBLE(resourcePrompt,false);
    const auto action = MISC::GET_HASH_KEY("INPUT_CONTEXT_X");
    if (!PAD::IS_CONTROL_ENABLED(0,action)) { resourceInputArmed=false; return; }
    if (!PAD::IS_CONTROL_PRESSED(0,action)) resourceInputArmed=true;
    if (!playable()) {
        for(size_t i=0;i<resourceInstances.size();++i) removeResource(i);
        craftingFire=0; return;
    }
    if (!canUseResources()) { resourceInputArmed=false; return; }
    if (GetTickCount64()>=resourceScanAt) {
        streamWorldResources(); scanCraftingFire(); resourceScanAt=GetTickCount64()+250;
    }
    if (!resourceInputArmed || GetTickCount64()<resourceActionAt) return;
    int selected=-1; double nearest=2.25;
    const auto player=resourcePosition(PLAYER::PLAYER_PED_ID());
    const auto& save=captureSlot->state();
    for(size_t i=0;i<resourceInstances.size();++i) {
        const auto& d=resourceCatalog.deposits[i];
        if (!ownedResource(i) || std::binary_search(save.collected.begin(),save.collected.end(),d.id)) continue;
        const auto distance=pal::worldDistanceSquared(player,resourcePosition(resourceInstances[i].object));
        if (distance<nearest && ENTITY::HAS_ENTITY_CLEAR_LOS_TO_ENTITY(PLAYER::PLAYER_PED_ID(),resourceInstances[i].object,17)) {
            selected=static_cast<int>(i); nearest=distance;
        }
    }
    const bool crafting=selected<0 && validCraftingFire();
    if (selected<0 && !crafting) return;
    const std::string label=crafting ? "Craft Pal sphere (1 Paldium Fragment)"
        : resourceCatalog.deposits[selected].fragments ? "Collect Paldium Fragments" : "Collect Pal spheres";
    HUD::_UI_PROMPT_SET_TEXT(resourcePrompt,MISC::VAR_STRING(10,"LITERAL_STRING",label.c_str()));
    HUD::_UI_PROMPT_SET_VISIBLE(resourcePrompt,true); HUD::_UI_PROMPT_SET_ENABLED(resourcePrompt,true);
    const auto title="Pals: "+std::to_string(save.fragments)+" fragments | "+std::to_string(save.spheres)+" spheres";
    HUD::_UI_PROMPT_SET_ACTIVE_GROUP_THIS_FRAME(resourcePromptGroup,MISC::VAR_STRING(10,"LITERAL_STRING",title.c_str()),1,0,0,0);
    if (!HUD::_UI_PROMPT_HAS_HOLD_MODE_COMPLETED(resourcePrompt)) return;
    resourceInputArmed=false; resourceActionAt=GetTickCount64()+1000;
    if (crafting && validCraftingFire()) {
        if (save.fragments<1 || save.spheres>=pal::inventoryLimit) report("Crafting needs 1 fragment and inventory space.");
        else if (!captureSlot->craftSpheres(1,1)) captureFailure();
        else report("Crafted 1 Pal sphere. Native throwable equip/throw integration pending.");
    } else if (selected>=0 && ownedResource(selected)) {
        const auto& d=resourceCatalog.deposits[selected];
        if (d.fragments>pal::inventoryLimit-save.fragments || d.spheres>pal::inventoryLimit-save.spheres
            || save.collected.size()>=pal::pickupReceiptLimit) report("Resource inventory or pickup ledger is full.");
        else if (!captureSlot->collect(d.id,d.fragments,d.spheres)) captureFailure();
        else { removeResource(selected); report("Collected resource; receipt saved before world removal."); }
    }
    resetResourcePrompt();
}

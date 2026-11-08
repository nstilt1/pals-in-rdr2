// Native pane bindings and a private event channel; no interception of game UI events.
bool palsPageEnabled=false,palsPageFocused=false;
Any palsData=0,palsEnabledData=0,palsActiveData=0,palsTitleData=0,palsNameData=0,palsStatusData=0,palsControlsData=0;
ULONGLONG nextPalsDataAttempt=0;
std::string lastPalsName,lastPalsStatus;
void initializePalsPage() {
    palsPageEnabled=GetPrivateProfileIntA("Prototype","PalsPageEnabled",0,
        (basePath+"RDR2PalPrototype.ini").c_str())!=0;
    trace(std::string("Native Pals pane candidate: ")+(palsPageEnabled ? "enabled" : "disabled"));
}
void updatePalsPage() {
    if (!palsPageEnabled) return;
    const bool available=captureEnabled && !captureBlocked && captureSlot;
    if (!palsData && GetTickCount64()>=nextPalsDataAttempt) {
        nextPalsDataAttempt=GetTickCount64()+1000;
        // This branch is original mod data; engine quick_select provider records are untouched.
        palsData=DATABINDING::_DATABINDING_ADD_DATA_CONTAINER_FROM_PATH("","pals_ui");
        if (!palsData) return;
        palsEnabledData=DATABINDING::_DATABINDING_ADD_DATA_BOOL(palsData,"enabled",available);
        palsActiveData=DATABINDING::_DATABINDING_ADD_DATA_BOOL(palsData,"active",available);
        palsTitleData=DATABINDING::_DATABINDING_ADD_DATA_STRING(palsData,"title","PALS");
        palsNameData=DATABINDING::_DATABINDING_ADD_DATA_STRING(palsData,"selectedName","");
        palsStatusData=DATABINDING::_DATABINDING_ADD_DATA_STRING(palsData,"status","");
        palsControlsData=DATABINDING::_DATABINDING_ADD_DATA_STRING(palsData,"controls",
            "D-pad/arrows: choose. A/Enter: release or recall.");
        if (!palsEnabledData || !palsActiveData || !palsTitleData || !palsNameData || !palsStatusData || !palsControlsData) {
            palsPageEnabled=false;trace("Pals pane data registration failed; controls disabled.");return;
        }
        trace("Native Pals pane data registered.");
    }
    if (!palsData) return;
    DATABINDING::_DATABINDING_WRITE_DATA_BOOL(palsEnabledData,available);
    DATABINDING::_DATABINDING_WRITE_DATA_BOOL(palsActiveData,available);
    const Hash channel=MISC::GET_HASH_KEY("PALS_QUICK_SELECT");
    for(int i=0;i<32 && UIEVENTS::EVENTS_UI_IS_PENDING(channel);++i) {
        alignas(8) Any event[4]{};
        if (UIEVENTS::EVENTS_UI_PEEK_MESSAGE(channel,event) && static_cast<Hash>(event[2])==MISC::GET_HASH_KEY("PALS_WHEEL")) {
            const auto type=static_cast<Hash>(event[0]);
            if (type==MISC::GET_HASH_KEY("ITEM_FOCUSED")) {
                palsPageFocused=true;trace("Native Pals pane focused.");
            } else if(type==MISC::GET_HASH_KEY("ITEM_UNFOCUSED")) palsPageFocused=false;
        }
        UIEVENTS::EVENTS_UI_POP_MESSAGE(channel);
    }
    if (!available) { palsPageFocused=false;return; }
    const auto& s=captureSlot->state();
    const auto selected=captureSlot->selectedPal();
    const std::string name=captureSlot->partySize() ? std::string(pal::species[selected.species].name)
        +" ("+std::to_string(s.selected+1)+"/"+std::to_string(captureSlot->partySize())+")" : "No Pals captured";
    const std::string state=s.phase==pal::CapturePhase::recovery ? "Recovery required; inspect the profile"
        : s.phase==pal::CapturePhase::deployed && s.selected==0 ? "Following you - use to recall"
        : captureSlot->partySize() ? "Use to release (recalls the current companion first)" : "Capture a Pal to add it to your party";
    if (name!=lastPalsName) { DATABINDING::_DATABINDING_WRITE_DATA_STRING(palsNameData,name.c_str());lastPalsName=name; }
    if (state!=lastPalsStatus) { DATABINDING::_DATABINDING_WRITE_DATA_STRING(palsStatusData,state.c_str());lastPalsStatus=state; }
    if (!UIAPPS::IS_UIAPP_ACTIVE_BY_HASH(MISC::GET_HASH_KEY("hud_quick_select"))) { palsPageFocused=false;return; }
    if (!palsPageFocused || !playable() || HUD::IS_PAUSE_MENU_ACTIVE()) return;
    const Hash left=MISC::GET_HASH_KEY("INPUT_FRONTEND_LEFT"),right=MISC::GET_HASH_KEY("INPUT_FRONTEND_RIGHT"),
        accept=MISC::GET_HASH_KEY("INPUT_FRONTEND_ACCEPT");
    for(const auto action:{left,right,accept}) PAD::DISABLE_CONTROL_ACTION(0,action,true);
    if (PAD::IS_DISABLED_CONTROL_JUST_PRESSED(0,left)) {
        if(!captureSlot->cycleParty(-1) && captureSlot->partySize()) report("Party selection unavailable during a transition.");
    } else if(PAD::IS_DISABLED_CONTROL_JUST_PRESSED(0,right)) {
        if(!captureSlot->cycleParty(1) && captureSlot->partySize()) report("Party selection unavailable during a transition.");
    } else if(PAD::IS_DISABLED_CONTROL_JUST_PRESSED(0,accept) && captureSlot->partySize()) {
        UIAPPS::_CLOSE_UIAPP_BY_HASH(MISC::GET_HASH_KEY("hud_quick_select"));
        palsPageFocused=false;companionRequested.store(true);
    }
}

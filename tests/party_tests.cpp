#include "../src/capture.h"
#include "../src/mission_capture.h"
#include <iostream>
int main() {
    int failures=0;
    auto check=[&](bool yes,const char* name){if(!yes){++failures;std::cerr<<name<<'\n';}};
    pal::CaptureSave legacy;
    check(pal::decode("PALS_CAPTURE_1 1 1 80 80",legacy),"read working profile");
    bool fail=false;pal::CaptureSave disk=legacy;
    pal::CaptureSlot party(legacy,[&](const auto& next){if(fail)return false;disk=next;return true;});
    check(party.prepareCapture() && party.state().reserve.size()==1 && party.state().reserve[0].health==80,
          "existing Lamball retained before next capture");
    check(party.spendSphere() && party.beginCapture(40,60,1,true) && party.finishCapture(true)
          && party.partySize()==2 && party.state().missionSheepClaimed,"new Pal joins party with durable mission receipt");
    check(party.cycleParty(1) && party.state().selected==1 && party.state().species==1
          && party.selectedPal().species==0,"browsing only changes selection");
    fail=true;
    check(!party.activateSelected() && party.state().species==1,"failed swap persistence retains identity");
    fail=false;
    check(party.activateSelected() && party.state().species==0 && party.state().health==80
          && party.state().reserve[0].species==1,"selection swaps held identities without duplication");
    check(party.beginRelease() && party.finishRelease(true) && party.cycleParty(-1)
          && party.state().phase==pal::CapturePhase::deployed && !party.activateSelected(),
          "browsing deployed party cannot silently switch active Pal");
    check(!party.prepareCapture(),"recall required before capture in this candidate");
    check(party.beginRecall(70,80) && party.finishRecall(true) && party.activateSelected()
          && party.state().species==1 && party.state().reserve[0].health==70,"recall preserves health before switching");
    check(party.prepareCapture() && party.restoreHeld() && party.state().species==1
          && party.partySize()==2,"failed attempt restores prior held member");
    check(party.collect(1,0,4),"refill synthetic test inventory through durable acquisition");
    for(int i=0;i<3;++i) check(party.prepareCapture() && party.spendSphere()
          && party.beginCapture(20+i,50,2) && party.finishCapture(true),"fill party");
    check(party.partySize()==5 && !party.prepareCapture(),"party cannot exceed five");
    for(int i=0;i<5;++i) check(party.cycleParty(1),"cycle full party");
    check(party.state().selected==0,"party selection wraps");
    pal::CaptureSave restored;
    check(pal::decode(pal::encode(disk),restored) && restored.reserve.size()==4
          && restored.missionSheepClaimed && restored.spheres==1,"v4 roundtrip preserves party, mission receipt and spheres");
    check(party.beginRelease() && party.finishRelease(true) && party.markDead()
          && party.partySize()==4 && party.state().phase==pal::CapturePhase::held,
          "dead active member removed while other owned Pals survive");
    check(party.prepareCapture() && !party.beginCapture(30,40,0,true) && party.restoreHeld(),
          "same profile cannot claim another mission sheep");
    check(party.prepareCapture(),"journal empty primary between attempts");
    pal::CaptureSlot restart(party.state(),[](const auto&){return true;});
    check(restart.recoverOnStartup() && restart.state().phase==pal::CapturePhase::held
          && restart.partySize()==4,"restart after parking held member restores it exactly once");
    for(const char* bad:{"PALS_CAPTURE_4 1 1 80 80 0 0 0 1 0 0",
                        "PALS_CAPTURE_4 1 1 80 80 0 0 0 0 2 0",
                        "PALS_CAPTURE_4 1 1 80 80 0 0 0 0 0 5",
                        "PALS_CAPTURE_4 1 1 80 80 0 0 0 0 0 1 0 80 0"})
        check(!pal::decode(bad,restored),"reject malformed party data");
    pal::MissionSheepEvidence evidence{true,true,true,true,false,1};
    check(pal::mayCaptureMissionSheep(evidence),"allow one mission sheep with another survivor");
    evidence.otherLivingSheep=0;check(!pal::mayCaptureMissionSheep(evidence),"last living mission sheep protected");
    evidence.otherLivingSheep=5;evidence.claimed=true;check(!pal::mayCaptureMissionSheep(evidence),"durable one-claim mission guard");
    evidence.claimed=false;evidence.ownerMatches=false;check(!pal::mayCaptureMissionSheep(evidence),"other scripts protected");
    evidence.ownerMatches=true;evidence.missionRunning=false;check(!pal::mayCaptureMissionSheep(evidence),"stale mission ownership rejected");
    return failures?1:0;
}

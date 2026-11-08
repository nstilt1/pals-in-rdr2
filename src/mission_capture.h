#pragma once
namespace pal {
struct MissionSheepEvidence {
    bool enabled=false, sheep=false, missionRunning=false, ownerMatches=false, claimed=false;
    int otherLivingSheep=0;
};
inline bool mayCaptureMissionSheep(const MissionSheepEvidence& e) {
    return e.enabled && e.sheep && e.missionRunning && e.ownerMatches
        && !e.claimed && e.otherLivingSheep>0;
}
}

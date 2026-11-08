#include "../src/capture.h"
#include <iostream>
#include <limits>

int main() {
    int failures = 0;
    const auto check = [&](bool result, const char* name) {
        if (!result) { std::cerr << name << '\n'; ++failures; }
    };
    pal::CaptureSave disk;
    bool failWrite = false;
    pal::CaptureSlot slot(disk, [&](const auto& next) {
        if (failWrite) return false;
        disk = next; return true;
    });
    check(slot.spendSphere() && slot.state().spheres == 9, "accepted attempt consumes one sphere");
    failWrite = true;
    check(!slot.beginCapture(10, 100) && slot.state().phase == pal::CapturePhase::empty,
          "persistence failure prevents world mutation authorization");
    failWrite = false;
    check(slot.beginCapture(10, 100), "journal successful capture before animal deletion");
    check(!slot.beginCapture(10, 100) && !slot.spendSphere(), "prevent repeated capture/consumption");
    pal::CaptureSlot interrupted(disk, [](const auto&) { return true; });
    check(interrupted.recoverOnStartup() && interrupted.state().phase == pal::CapturePhase::recovery
          && !interrupted.beginRelease(), "crash during capture cannot duplicate ownership");
    check(slot.finishCapture(true) && slot.beginRelease(), "captured slot can release");
    check(slot.finishRelease(false) && slot.state().phase == pal::CapturePhase::held,
          "spawn failure retains captured ownership");
    check(slot.beginRelease() && slot.finishRelease(true), "successful release journal");
    check(!slot.beginRelease() && !slot.beginCapture(5, 100), "one active companion only");
    check(slot.beginRecall(5, 100) && slot.finishRecall(false)
          && slot.state().phase == pal::CapturePhase::deployed, "failed deletion cannot create held duplicate");
    check(slot.beginRecall(5, 100) && slot.finishRecall(true), "successful recall preserves state");
    check(slot.state().health == 5 && slot.state().spheres == 9, "recall preserves health and sphere count");
    pal::CaptureSave parsed;
    check(pal::decode(pal::encode(slot.state()), parsed) && parsed.health == 5,
          "save round trip");
    for (const char* bad : {"", "PALS_CAPTURE_1 -1 10 1 1", "PALS_CAPTURE_1 0 11 1 1",
             "PALS_CAPTURE_1 1 5 0 100", "PALS_CAPTURE_1 1 5 101 100", "PALS_CAPTURE_1 1 5 1 1 extra"})
        check(!pal::decode(bad, parsed), "reject corrupt or unsupported save");
    check(pal::captureChance(10,100) > pal::captureChance(100,100)
          && pal::captureChance(0,100) == 0 && pal::captureChance(100,0) == 0,
          "injury improves chance; invalid/dead target cannot capture");
    pal::CaptureSave deployed;
    deployed.phase = pal::CapturePhase::deployed;
    pal::CaptureSlot restart(deployed, [](const auto&) { return false; });
    check(!restart.recoverOnStartup() && restart.state().phase == pal::CapturePhase::recovery
          && !restart.beginRelease(), "failed recovery write still blocks duplicate release in memory");
    pal::CaptureSlot budget({}, [](const auto&) { return true; });
    for (int i=0; i<10; ++i) check(budget.spendSphere(), "ten initial spheres available");
    check(!budget.spendSphere() && budget.state().spheres == 0, "sphere budget cannot underflow");
    pal::CaptureSlot failedRemoval({}, [](const auto&) { return true; });
    check(failedRemoval.spendSphere() && failedRemoval.beginCapture(50,100)
          && failedRemoval.finishCapture(false)
          && failedRemoval.state().phase == pal::CapturePhase::empty
          && failedRemoval.state().spheres == 9, "failed capture removal leaves no owned duplicate");
    check(pal::decode("PALS_CAPTURE_1 1 1 80 80", parsed)
          && parsed.species == 0 && parsed.spheres == 1 && parsed.health == 80,
          "legacy Lamball save retains species, spheres and health");
    for (int species=0; species<static_cast<int>(pal::species.size()); ++species) {
        pal::CaptureSave saved;
        pal::CaptureSlot creature({}, [&](const auto& next) { saved=next; return true; });
        check(creature.beginCapture(40,80,species) && creature.finishCapture(true)
              && pal::decode(pal::encode(saved),parsed) && parsed.species==species,
              "each captured species persists its own native model identity");
        check(creature.beginRelease() && creature.finishRelease(true) && creature.markDead()
              && creature.state().phase == pal::CapturePhase::empty && !creature.beginRelease(),
              "dead companion cannot be released/resurrected from ownership slot");
    }
    check(!pal::decode("PALS_CAPTURE_2 1 1 80 80 5",parsed), "reject unsupported species save");
    // Acquisition/crafting share the actual capture inventory and atomic store.
    pal::CaptureSave inventory;
    pal::decode("PALS_CAPTURE_1 1 1 80 80", inventory);
    bool inventoryWriteFails = false;
    pal::CaptureSave inventoryDisk = inventory;
    pal::CaptureSlot crafting(inventory, [&](const auto& next) {
        if (inventoryWriteFails) return false;
        inventoryDisk = next; return true;
    });
    check(crafting.collect(42, 3, 0) && crafting.state().fragments == 3,
          "collect fragment deposit with durable receipt");
    check(!crafting.collect(42, 3, 0), "streaming same deposit cannot duplicate fragments");
    inventoryWriteFails = true;
    check(!crafting.craftSpheres(2, 1) && crafting.state().fragments == 3
          && crafting.state().spheres == 1 && inventoryDisk.fragments == 3,
          "failed craft persistence spends nothing and grants nothing");
    check(!crafting.collect(43, 1, 0) && crafting.state().collected.size() == 1,
          "failed collection write authorizes no world removal");
    inventoryWriteFails = false;
    check(crafting.craftSpheres(2, 1) && crafting.state().fragments == 1
          && crafting.state().spheres == 2 && crafting.state().health == 80
          && crafting.state().phase == pal::CapturePhase::held,
          "craft persists material debit and sphere credit while retaining held Pal");
    check(pal::decode(pal::encode(inventoryDisk), parsed), "new inventory save roundtrip");
    pal::CaptureSlot reloaded(parsed, [](const auto&) { return true; });
    check(!reloaded.collect(42, 1, 0) && reloaded.collect(43, 0, 1)
          && reloaded.state().spheres == 3, "receipt survives reload; world sphere uses same inventory");
    check(!reloaded.craftSpheres(2, 1) && !reloaded.craftSpheres(0, 1)
          && !reloaded.craftSpheres(1, -1), "insufficient materials and invalid recipes rejected");
    check(!reloaded.collect(0, 1, 0) && !reloaded.collect(44, -1, 0)
          && !reloaded.collect(44, 0, 0) && !reloaded.collect(44, 0, 999),
          "invalid pickup rewards and overflow rejected");
    pal::CaptureSave full;
    full.fragments = pal::inventoryLimit; full.spheres = pal::inventoryLimit;
    pal::CaptureSlot capped(full, [](const auto&) { return true; });
    check(!capped.collect(1, 1, 0) && !capped.collect(1, 0, 1)
          && !capped.craftSpheres(1, 1), "capacity failure cannot consume resources");
    full.collected.clear();
    full.fragments = 0; full.spheres = 0;
    for (unsigned int i = 1; i <= pal::pickupReceiptLimit; ++i) full.collected.push_back(i);
    check(pal::decode(pal::encode(full), parsed), "maximum receipt ledger reloads");
    pal::CaptureSlot receiptFull(full, [](const auto&) { return true; });
    check(!receiptFull.collect(5000, 1, 0), "full receipt ledger fails closed rather than forgetting receipts");
    for (const char* bad : {"PALS_CAPTURE_3 1 1 80 80 0 -1 0",
             "PALS_CAPTURE_3 1 1 80 80 0 0 2 1 1", "PALS_CAPTURE_3 1 1 80 80 0 0 2 2 1",
             "PALS_CAPTURE_3 1 1 80 80 0 0 1 -1", "PALS_CAPTURE_3 1 1 80 80 0 0 1 4294967296",
             "PALS_CAPTURE_3 1 1 80 80 0 0 4097", "PALS_CAPTURE_3 1 1 80 80 0 0 1"})
        check(!pal::decode(bad, parsed), "corrupt inventory ledger rejected");
    pal::CaptureSave uncertain;
    uncertain.phase = pal::CapturePhase::recovery; uncertain.fragments = 5;
    pal::CaptureSlot quarantined(uncertain, [](const auto&) { return true; });
    check(!quarantined.collect(1, 1, 0) && !quarantined.craftSpheres(1, 1),
          "uncertain capture recovery cannot mutate inventory");
    return failures ? 1 : 0;
}

# Development

## Latest local test candidate: 0.6.0

Locally installed **0.6.0** adds a native Pals pane candidate, up to five party
members and a narrowly guarded The Sheep and the Goats capture exception.
The prior 0.4.1 build, all Pal appearance files and the existing profile were
backed up; appearance files and profile remained unchanged during installation.
Live wheel navigation, rendering/input and mission completion are pending user
testing. Earlier milestone sections below describe historical behavior.

`PalsPageEnabled=1` exposes original bindings under `pals_ui` and listens only
to the mod's private focus-event channel. The locally rebuilt wheel host adds
the Pals pane/header without changing existing pages/providers. D-pad/arrow
left/right selects a party member; A/Enter closes the wheel and queues its
release/recall through the existing script-fiber transaction. Browsing alone
does not deploy. Choosing another member while one is out first recalls that
member, preserves its health, then releases the selection. F10 still toggles
the selected member. Portraits are pending; the candidate uses names/status.

Party save **v4** reads v1/v2/v3, preserving the existing Pal, health, species,
spheres, fragments and resource receipts. Party size is capped at five. The
active representation plus four held records share one atomic save. This
candidate requires recalling an active companion before F9 capture. Failed
attempts restore the previously held member; failed/interrupted world mutations
still fail closed. Older plugins cannot read v4. Keep a matching profile backup
before any downgrade, and do not overwrite new party progress with an old save.

`MissionSheepCaptureEnabled=1` allows a live native sheep whose owning script is
the running `mudtown4` mission, provided another sheep owned by that same script
is alive. All ordinary targeting restrictions remain. A durable claim written
before source removal permits at most one mission sheep per profile, including
across retries/restarts; interrupted removal may require manual review. No
mission counters, script locals, rewards or global failure flags are patched.
The source is removed through the capture transaction; release creates a separate
decorated mod-owned companion, with a kept follow task and no ped-group membership.
A periodic check maintains only that owned companion's follow behavior/ownership.
The story script's response to the missing sheep and final herd handoff still
must be tested. The all-sheep gold objective may be affected by capturing one.

The native UI file is a locally generated **RBF** override, separate from the
sphere weapon **PSO** experiment. `prepare_pals_page.py` requires the private
native wheel-host input, verifies a byte-exact unmodified roundtrip, and writes
only under the private workspace. Re-export with an independent reader, then
run `validate_pals_page.py`; it requires every original UI node to match after
removing only the added pane/header. The local candidate passes; eight suites
pass. The local LML mod also loads an original `PALS` text label. No native UI
file, derived output, portrait, game schema or downloaded tool is distributed.

F9 remains the developer capture trigger. Native Pal sphere throwing/model,
impact attribution and Capture Kit timing/effects adapted to RDR2 animations
remain unfinished. The planned sphere is a separate item based on native
knife-style throwing mechanics; ordinary knives/dynamite remain intact.

The playable prototype source is in `src/`. The earlier `OriginalRuntimeBridge` project is a separate host-neutral skeleton.

## Build

Use Windows, Visual Studio 2022 C++ tools, Windows SDK, CMake, Python 3, and a separately obtained Script Hook RDR2 SDK. Keep the SDK and build output outside this repository.

```powershell
cmake -S . -B ../prototype-build -G "Visual Studio 17 2022" -A x64 -DSCRIPTHOOK_SDK_ROOT="C:/path/to/external-sdk"
cmake --build ../prototype-build --config Release
ctest --test-dir ../prototype-build -C Release --output-on-failure
python tools/check_repository.py
```

The plugin is `../prototype-build/Release/RDR2PalPrototype.asi`.

## Local installation inputs

`Install.bat` launches `Install.ps1`, which asks for the game directory, compiled ASI, locally generated YDD model dictionary, and shared YTD texture dictionary. Install Script Hook RDR2, an ASI loader, and Lenny's Mod Loader separately from their authors. The installer checks prerequisites and resource headers, requests Windows elevation, backs up overwritten files, verifies copied hashes, and enables its entry while retaining other LML entries. It does not download, extract, or generate game assets. Header checks do not prove arbitrary user-selected resources are valid in-game.

The current working local assets are asset build **0.1.2**. A fresh clone does not include those assets or a turnkey asset-conversion pipeline. `experiments/` preserves original conversion scripts for development; external tools and intermediate local inputs are still required. Set `RDR2PAL_LOCAL_WORKSPACE` to a private directory outside this repository before using those experiments. Their directory conventions refer to the development workspace, not a fresh game installation. Shared-texture conversion and writer orchestration are not yet a complete portable build command.

The installed local candidate is **lamball-sheep-0.1.3**, with plugin **0.2.3** unchanged. It replaces sheep appearance and sets `ReplacementModel=a_c_sheep_01`, so native sheep identity, behavior and rewards are used. This addresses the reported goat UI label and goat skin from the earlier goat-based build. Sheep rendering, UI identification and skinning rewards still require in-game confirmation. F8 targets compatible nearby animals; protected mission actors, humans and horses remain excluded. Existing sheep also receive the appearance replacement; mission compatibility is still unverified.

The confirmed goat-based **lamball-shared-0.1.2** build is preserved privately for rollback. The public installer now defaults to sheep resources (`a_c_sheep_01_merge_6.ydd` and `a_c_sheep_01_merge.ytd`); use `-Species Goat` only with the older matching goat resources. It checks matching filenames, backs up previous files, and retires the other species' resources from its own LML folder after backing them up. Filename/header checks alone cannot prove a resource's rig or in-game compatibility.

`experiments/build_lamball_sheep.py` adapts locally prepared, confirmed Lamball geometry to native sheep torso/intact-skin/carcass components, preserving native sheep identities and hidden buffer sizes/indices. It requires private XML/DDS exports of the base and patch resources plus the earlier private Lamball resources. The current export convention names the patchpack001 component XML `a_c_sheep_01_merge_6.ydd_2.xml`. Compile the generated XML with an external native writer, re-export into a fresh `verified-final` folder, then run `python -B experiments/validate_lamball_sheep.py` with `RDR2PAL_LOCAL_WORKSPACE` set. This remains an experimental developer workflow, not a turnkey asset generator. All inputs and outputs stay outside the repository.

## Repository boundary

Source-only 0.5.0 acquisition candidate: save v3 adds fragments and persistent
pickup receipts to the same inventory as capture spheres. Reads retain v1/v2
ownership, health, species and counts. Atomic collection/crafting and failure
paths are tested. The world/camp adapter below compiles but has not been tested
in-game; native sphere wheel registration/throwing remains unfinished. This
source candidate has not replaced the installed 0.4.1 build. Older plugins cannot read v3 profiles; back up profiles
before installing any candidate that writes v3, and do not downgrade such a
profile without restoring its matching backup.

The adapter defaults off. Development opt-in is
`WorldResourcesPrototypeEnabled=1` in the startup INI's `[Prototype]` section,
alongside a valid capture profile. It reads a separate, privately prepared
`RDR2PalResources.local.txt` beside the ASI. Never add that local file, resource
models, extracted data or placement maps to this repository or releases.
The line grammar is:

```text
# Trusted player-camp anchor (both permanent and temporary use this form)
camp X Y Z
# Permanent unique deposit ID; registered local model; position; whole rewards
deposit ID MODEL X Y Z FRAGMENTS SPHERES
```

Replace the grammar tokens with locally verified values. There are no shipped
positions/models. Up to 32 camp anchors and 64 deposits are accepted; malformed
or duplicate-ID entries reject the entire catalog. Do not recycle deposit IDs
or reuse an existing profile for a different placement map. No respawn is
implemented. Temporary camp moves require updating the local anchor and restarting.
Camp ownership/unlocks are not automatically detected: only vetted player camps
belong in this developer catalog. An unrelated campfire alone never grants crafting.

The candidate streams at most eight resource instances/model requests nearby,
with bounded model waits and ownership decorators. Only its own spawned objects
can be removed. Models must already be registered by a separate local asset
installation; missing models are logged and skipped. Collection uses a native
one-second hold prompt within 1.5m and line of sight; receipt/reward commit before
object removal. Crafting uses the same native prompt at recognized campfire
props within 3m, with the fire within 4m of a trusted camp anchor. One original
prototype recipe exchanges **1 Paldium Fragment for 1 sphere** at either camp
type; this is not a claim of Palworld recipe parity. There is no crafting queue
or animation yet. Releasing/re-holding the input is required for another action.
Combat, aiming, mounted travel, pause/fade and recognized native menus block use.

Acceptance still requires actual pickup visuals/placement, main/temporary camp
detection and prompt coexistence on keyboard/controller, full inventory and
failed-save paths, streaming, receipt persistence, and native sphere equip/throw.
Crafted spheres currently feed the F9 developer capture inventory. Do not call
this a finished native throwable item or portable automatic acquisition system.

Publish only this source directory. Do not publish its parent workspace, game directories, generated assets, SDKs, downloaded dependencies, logs, dumps or backups. `.gitignore` is a convenience; `tools/check_repository.py` also rejects non-allowlisted files, including ignored files. Review text for accidental proprietary content and credentials before committing.

```powershell
git init
git config core.hooksPath .githooks
python tools/check_repository.py
git add .
python tools/check_repository.py --staged
```

No GitHub upload is performed by the installer. The project distributes original source/configuration only; ownership of games is not a guarantee of permission to redistribute derived content.

## Capture developer test (plugin 0.3.0)

This candidate adds a capture/companion foundation, not a usable native Pal sphere inventory item or throwing effect. Sheep appearance resources remain 0.1.3. Set `CapturePrototypeEnabled=1` and an explicit `CaptureProfile=lamball-test-01` in the startup INI. The feature defaults off; F8 is unchanged. Restart the entire game after an ASI or startup INI update.

- Aim at a live, unprotected Lamball within 25m and press **F9** for a capture attempt. No shot is required for the capture test. Invalid targets consume nothing. Accepted attempts consume one test sphere even on failure. Each new profile starts with ten test spheres and has one Lamball slot. Capture chance is an original prototype rule: 25% at full health, increasing toward 90% with injury; do not kill the target.
- Press **F10** to release the captured Lamball with a friendly follow task, or recall the deployed companion. Press **F11** for slot/sphere status. These are keyboard developer controls; native item/controller integration and enemy combat are unfinished.
- **Recall before saving, loading another save, or quitting.** The separate test roster is stored beside the local prototype log as `capture-test-<profile>.txt`; it is not synchronized with RDR2 save slots. Use one profile for one test save. Held ownership and test-sphere counts persist independently of game saves.
- Writes use a flushed temporary file and replacement. World creation/removal is journaled before mutation. Interrupted transactions, a deployed companion at process restart, or a missing/dead deployed companion require recovery and block automatic respawning. Recovery deliberately fails closed to avoid duplicate Pals; an automatic reconciliation UI is not implemented. Never delete/reset a profile to recover a deployed companion without first resolving its world entity.

Targeting/state-transition tests pass; native capture, release, follow and persistence behavior still require Story Mode testing. Companion combat must eventually hold fire until either the player or a hostile enemy fires the first shot in the encounter; that combat system is not implemented by this build.

## Companion combat candidate (plugin 0.3.1)

User confirmed failed/successful capture, release and following in 0.3.0. Candidate 0.3.1 adds opt-in `CompanionCombatEnabled=1`: a deployed companion holds fire until the player or a confirmed hostile shoots. Enemies must be actively fighting the player/companion, within 35m of the player and 40m of the companion. Mission-owned, mounted, vehicle-bound, hogtied, dead and nonhuman actors are excluded from this first combat test. After five seconds without an eligible hostile the encounter gate resets. F10 recall takes priority; no valid enemy means return to following.

This sheep-based rig has no verified combat animation. The prototype uses pursuit and a scripted 12-damage contact strike, at most once every two seconds, within 1.5m and clear line of sight; it does not apply ranged damage or implement a custom roll animation. The first-shot logic is unit tested; movement/contact attacks and disengagement require in-game validation. Gunfire is polled per frame against a periodically refreshed ped list; encounter timing remains an in-game acceptance check.

The first combat test uses a separate `CaptureProfile=lamball-combat-01`. The earlier local test profile was left deployed at shutdown, so automatic recovery would quarantine it. It is retained untouched rather than reset or silently duplicated. Capture a new test Lamball for this isolated combat check. Recall before saving/loading/quitting.

## Combat response candidate (plugin 0.3.2)

User confirmed release/recall and Lamball fighting a human in 0.3.1. Candidate 0.3.2 retains the existing lamball-combat-01 roster and adds reciprocal enemy targeting: after combat opens, existing enemies select the nearer visible opponent between the player and a fighting companion. Target choice is reconsidered at two-second intervals; native combat tasks handle aiming, firing and damage. Neutral actors and fleeing hunted game are not made aggressive. Recall redirects assigned enemies toward the still-nearby player before deleting the companion. Actual enemy firing and damage to Pals need in-game validation.

The companion also helps hunt non-owned, non-horse game the player deliberately shoots at: an aim sample within 250ms of firing selects that animal for up to 15 seconds within the existing distance/eligibility limits. This is a shot-at-target prototype, not verified projectile-hit attribution; aiming alone does not authorize hunting. Actively attacking wildlife can trigger immediate defense without a shot (the user explicitly selected this exception). This animal-only exception does not authorize unshot human targets. Gunfights still require the player/hostile first-shot gate. Mission, owned-Pal, mount, vehicle and other eligibility protections remain. Sheep assets and roster format are unchanged; no extra spheres or resets are granted.

## Native weapon wheel investigation

Source-only throwable registration experiment: `prepare_sphere_metadata.py`
now writes additive local PSO candidates for a dedicated sphere slot,
`weapon_thrown_pal_sphere`, `ammo_pal_sphere`, and two native catalog records.
It preserves original mapping records/unknown bits, verifies the native signed-byte
checksum convention before writing, isolates copied mutable metadata, retains
only one basic sphere ammo mode, zeros known configured damage/force fields and
removes native knife pickup/reward links. This is not proof of non-damaging native
physics or successful in-game registration. No candidate metadata is installed.

With `RDR2PAL_LOCAL_WORKSPACE` set, inputs are the private
`private-generated/weapon-wheel-research/raw/update_4.rpf/x64/packs/base/data/ai/weapons.ymt`
and `.../x64/data/itemdatabase/catalog_sp.rpf/catalog_sp.ymt`. An external PSO
reader must supply matching `<input>.schema.json` containing Data, Blocks, RootId
and Structs; the local RPFView.Meta reader was used. These are game-derived files,
not repository content. Script output stays under the private research folder.
Run without Python optimization because validation assertions are required.

Re-export **both original inputs and newly generated candidates** with an
independent PSO reader to `<file>.reexport.xml`, then run
`python -B experiments/validate_sphere_metadata.py`. The validator removes only
the expected new records/slot from its comparison and requires every old weapon
and catalog definition to match. The local candidates pass this check; synthetic
pointer/clone/checksum/failure tests also pass. Generated PSO/XML/JSON files contain
game data and must never enter the repository or distributable packages.

Still required before an in-game test: locally prepared sphere model/archetype,
texture/icon and localization, native inventory path/equip/count synchronization,
actual throw/impact attribution and capture/consumption integration. The selected
companion design is a **fourth Pals page** after Weapons/Items/Horse, with the
existing party-cycle/release/recall controls; its native page/provider integration
remains unverified. An extra tab label alone would not implement that page.

Requested: spheres in throwables, plus one companion item whose D-pad left/right cycles the party and whose use toggles selected-Pal deployment. Neither is implemented by 0.3.2. The roster still has one slot.

Local game inspection located the updated single/dual-wield wheel widgets, their engine quick-select providers and native cycle-next/previous bindings, plus weapon and inventory definitions. Those game-derived XML exports remain private. The available SDK exposes inventory enable/equip/slot operations and giving registered weapons, but inspection has not established registration of a new wheel item/provider. Adding an original icon alone is insufficient. A [RedM wheel research project](https://github.com/draobrehtom/redm-weapon-wheel) concerns runtime appearance replacement in RedM, and is not proof of a native Story Mode companion item. Do not deploy speculative widget replacements over the working native wheel. Next gate: locally generated sphere weapon/item registration and actual non-damaging throw detection, then a companion party provider with selection/use events and controller verification.

## Expanded automatic roster candidate (plugin 0.4.0, assets 0.1.4)

Local candidate installed: native sheep retain Lamball 0.1.3 resources unchanged;
goats use Caprity, and pigs/boars use Rushoar. Appearance replacements apply
automatically when those native species stream in; no F8 activation is needed.
This preserves native entities/model identities rather than deleting ambient
animals. Asset-level overrides also affect matching species used in scripts;
mission compatibility remains unverified even though entity identity is retained.

The two new Pals were exported locally through FModel, imported into Blender
and fitted as a first-pass native-rig adaptation. Every original native geometry
buffer is retained at its original vertex/index counts and collapsed in place;
custom models are appended. Goat/boar/pig resources preserve 35/19/19 buffers,
respectively, with 12 appended body LODs each. Shared BC3 textures retain alpha,
including Caprity's masked foliage. Re-export/geometry/weight/texture checks
passed; four model dictionaries passed an independent reader with zero warnings.
Rendering, movement, proportions, death/skinning and streaming need in-game
confirmation. Do not equate parser success with those checks.

`ExpandedSpeciesEnabled=1` enables developer capture of these four native model
identities. The one-slot save now writes `PALS_CAPTURE_2` with a species index,
and reads old Lamball `PALS_CAPTURE_1` saves without resetting health or spheres.
Release uses the saved native species; a pig-based Rushoar remains mechanically
a pig and a boar-based Rushoar remains a boar. The party is still one slot.

Owned companions are explicitly damageable and targetable. Death clears their
ownership slot and releases the existing corpse for native skinning; it does
not delete the corpse or permit F10 resurrection. Actual friendly-Pal skinning
prompts/rewards need in-game verification. Tests cover migration, species
round trips and death preventing re-release.

Private local generation uses `prepare_species.py`,
`build_quadruped_species.py`, `prepare_species_textures.py`, external BC3/native
writers and fresh re-exports checked by `validate_species.py`. All scenes,
textures, resources, external tools and reports remain outside this repository.
The existing public single-species Install.bat/Install.ps1 is not a multi-species
roster installer; do not use it over this local roster installation. A portable
local-generation/roster installer remains unfinished. World spheres, Paldium
collection, native campfire crafting, native wheel integration and legendary
capture are required future work, not completed by this candidate.

## Chikipi candidate and standalone capture DLL

Plugin **0.4.1**, assets **0.1.5** add Chikipi to the same automatic roster.
Both native chicken body components (head_000/head_001) used by all three
configured chicken outfits are covered. Four original geometry buffers are
preserved/collapsed, with four appended body LODs. Shared textures and weights
passed validation; independent model reader reported zero warnings. Dominique
and other-breed rendering/animation still need in-game confirmation. The four
previous model identities and saved species indices remain unchanged; chicken
is appended as the fifth supported capture model. Other Pal assets and the
existing capture save were preserved during local installation.

The generic local converter now also handles this chicken skeleton; despite
the historical build_quadruped_species.py filename, `-- chicken` selects a
separate bird bone mapping and private roster-chicken-v015 output. No game
assets, binary outputs or dependencies are in this repository.

[Capture Kit Bridge](bridge/README.md) is a separately built, tested Windows
x64 DLL for reusable capture chance and shake rules. It compiles one externally
supplied MIT-licensed Capture Kit module through an original C interface. The
runtime DLL, header, import library and license/notice bundle are private build
outputs. It is not installed into or used by the RDR2 ASI yet. Sphere crafting,
native throwables-wheel registration and animation/world adapters remain the
next implementation work.

## Logging

The plugin logs UTC times, version, settings, native-call BEGIN/END markers, post-swap frames, combat target changes and heartbeat messages in `%LOCALAPPDATA%\RDR2PalPrototype\RDR2PalPrototype.log` (temporary-directory fallback). `freeze_probe` can capture local thread stacks from a frozen process with appropriate Windows permissions. Do not commit those captures.

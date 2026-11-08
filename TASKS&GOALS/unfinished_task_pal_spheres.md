# Pal spheres and capture

Status: unfinished. User confirmed capture failure, subsequent successful capture, release and following in 0.3.0. Recall, persistence and interrupted-transaction recovery remain unverified. Combat and native wheel integration are separate unfinished work.

Latest local candidate installed: **0.6.0** adds a native Pals pane and five-member
party, with name/status display and cycle/use controls. Native navigation and
rendering/controls require testing; portraits remain pending. Save v4 migrates
the old single Pal and inventory unchanged. A guarded `mudtown4` mission-sheep
capture path permits one per profile while another mission-owned sheep survives;
final handoff/completion remains unverified. Generic mission protection stays.
See BUILD.md. The older single-slot paragraphs below are milestone history.

Animation request: adapt Capture Kit's procedural throw/release/capture timing
and shake/effect sequence with compatible RDR2 character animations. Capture Kit
does not supply ready RDR2 skeletal clips. Use a separately registered sphere
weapon based on knife-style native throwing, with a locally prepared sphere model
and zero configured damage plus verified non-damaging physics. Preserve ordinary
knives/dynamite. This animation/throwable work is not enabled by 0.6.0.

The developer candidate uses F9 aimed capture, F10 release/recall, one persisted Lamball slot and ten test spheres per explicit profile. It journals world mutations and blocks uncertain recovery rather than spawning duplicates. Friendly follow is assigned on release but unverified in game. Recall before save/load/quit; see BUILD.md for limitations and controls.

## Requested behavior

- **Latest user-selected companion UI:** a fourth native wheel page,
  **Weapons → Items/Consumables → Horse → Pals**. Investigate and implement
  page navigation/provider integration while preserving the existing pages.
  The earlier one-item selector controls still apply within the Pals page:
  left/right cycles the party; using the selection releases/recalls it;
  browsing does not deploy. Spheres remain under weapon throwables. A separate
  radial overlay is not the selected design. A native pane candidate is installed;
  placement/navigation and interaction are not yet verified in-game.

- Show the selected Pal's **party-menu portrait** in the companion selector,
  as requested by the user. Prepare/cache portraits locally from the user's
  Palworld installation; do not ship extracted icons or game data. Keep the
  selected Pal's name visible and provide a text fallback when no local portrait
  is available. Changing selection updates portrait/name without deploying it.

- Provide usable Pal spheres in RDR2 Story Mode through native inventory and throwing controls while preserving the weapon wheel.
- Pal spheres must appear in the native weapon wheel under **throwables**, with an accurate sphere count, sphere name/icon, normal equip/aim/throw controls and capture on impact. Preserve ordinary throwable weapons and their damage/behavior; do not simply relabel a damaging throwable as a working sphere. Prepare any game-derived resources locally outside the public source boundary.
- Optional requested extension: a **Pal companion slot and selector in the native weapon wheel**, showing the owned roster/current companion and offering selection, release and recall. Investigate native wheel support before promising an extra segment. Support keyboard/mouse and controller and preserve native weapons/items and mission behavior. A custom overlay or existing-slot replacement is not automatically equivalent to a native companion slot.
- Clarified interaction: expose **one companion item** in the wheel, not one item/segment per Pal. With that item highlighted, **D-pad left/right cycles Pals in the party**. Using the item toggles the selected Pal between deployed and recalled. Cycling selection alone must not spawn/recall a Pal. Keep selection distinct from deployment and preserve selected/active identities across wheel closure. Define behavior for selecting another party member while one is deployed before implementing multi-Pal deployment; the prototype currently has only one slot.
- Implement capture attempts, capture chance, feedback and effects, and a persistent owned-Pal roster.
- On successful capture, record the captured Pal and remove its world representation exactly once. Failed or interrupted attempts must not lose the animal or duplicate ownership.
- Release an owned Pal as a companion; preserve its identity and state across recall, release, streaming and save/reload.
- Captured animals/Pals become friendly to the player. Released companions follow the player and defend them by attacking enemies who are hostile to the player. Do not attack neutral/friendly actors merely because they are nearby; define hostility checks, engagement/disengagement range, recall priority and mission restrictions before enabling combat. Preserve allegiance and companion settings across recall/release and save/reload.
- User-selected combat trigger: companions hold fire until **either the player or a hostile enemy fires the first shot in the encounter**. Hostility alone, aiming, shouting, and gunfire from unrelated neutral actors must not trigger an attack. After that trigger, engage only confirmed hostiles; reset the hold-fire gate when the encounter ends. Friendly follow must remain available while holding fire.
- User clarified: hostile **animal attacks trigger immediate defense**, even without gunfire. That exception must not make humans attackable before the first-shot rule permits it. Help hunt a wild animal the player deliberately shoots at; do not select unrelated nearby wildlife or other owned Pals. Investigate reliable shot-target/impact association; mere aim must not trigger hunting.
- Enemies must be able to fight back against Pals. Among the player and actively fighting Pals, prefer the nearest viable opponent; reassess as distance/visibility changes. Do not manufacture hostility in neutral NPCs or fleeing hunted game. Preserve mission protections and leave enemies' weapons, ammo and damage native. Verify actual aiming/shooting at the Pal and real Pal damage, not only a combat-task assignment.
- Define sphere crafting/acquisition, consumption rules, capture eligibility and companion controls before implementing them. Keep mission actors protected until dedicated compatibility validation passes.
- Required acquisition: visible Pal spheres in the world and crafting at the player's campfire/crafting area. Support the same sphere recipe at main and temporary camps. World pickups and crafted spheres must feed the same persistent inventory/throwable count; no separate unlimited developer inventory in finished gameplay.
- Add locally prepared **Paldium Fragments** as visible collectible resources, potentially through spawned world deposits/pickups. Use persistent pickup/deposit IDs and controlled respawn rules; prevent duplicate rewards after streaming/reload. Keep placement out of mission paths and ensure crafting consumes materials once. Recipe quantities and additional ingredients remain to be chosen; do not claim exact Palworld recipe parity without implementing it.
- Friendly and owned Pals remain damageable, killable and skinnable. Death must remove deployed ownership eligibility so recall/release cannot resurrect the skinned animal or duplicate its rewards. Preserve the corpse for native skinning; friendship must not imply invincibility or hide the skinning prompt.
- Run gameplay in RDR2 without requiring Palworld to run concurrently. Read or prepare any required content locally from the user's own game installations.

## Mission-origin companions and herd handoffs

User requirement: once a mission-origin animal is legitimately captured and
released as an owned Pal, it follows the player. It must not rejoin the mission's
herd, be sold/handed off with the remaining animals, or disappear during mission
cleanup. Explicit regression case: **The Sheep and the Goats**.

Implement a dedicated mission-aware capture path with persistent ownership and
source-entity reconciliation. The released companion must be a separate mod-owned
representation, not the original mission entity or a member of its script/herd
groups, cleanup lists or objective counters. Preserve the Pal's species, health
and ownership after the herd's final handoff. Mission-specific code must resolve
the source animal's objective state without duplicating the Pal or mission reward.
The user suggested that the source might be treated as killed; evaluate that per
mission rather than automatically emitting a death or bypassing script protections,
since native objectives may fail or award penalties for missing/dead livestock.

This does not mean clearing the mod's own script-ownership/cleanup-protection flag
from a deployed companion: engine mission-entity protection is distinct from
membership in a story mission. Keep the current generic capture guard on protected
mission animals until the dedicated path is verified. Do not automatically respawn
an uncertain missing companion and risk creating two owned animals.

Acceptance: mission-origin Pal remains with/follows the player while the rest of
the herd is delivered, survives mission completion/cleanup and later recall/release,
and retains ownership through supported save/reload. Verify mission completion,
herd accounting, rewards/failure branches and no duplicate animals; ordinary owned
Pals must also remain outside nearby herd handoffs. Native implementation pending.

## Domestic animal capture and witnesses

Queued user request: capturing someone else's domesticated animal at a farm,
ranch or similar property should provoke the owner's/nearby citizens' livestock
theft response. Prefer the native response to stealing an animal; investigate
the existing animal-theft/killing witness and law systems before creating a
separate crime simulation.

- Identify owned livestock from reliable native ownership/property context;
  do not treat every sheep, goat or other domesticated species as stolen merely
  because of its model. Wild animals and the player's own Pals are distinct.
- User clarification: react **as soon as an observed Pal sphere is thrown at
  owned livestock**, before the capture result. Nearby citizens who can perceive
  the attempted theft should object (for example, "Hey, you shouldn't do
  that! Put it back"). Use appropriate native dialogue/behavior if supported;
  exact custom spoken lines are not promised and no game audio is distributed.
- Allow a short, clearly communicated opportunity to return that same animal.
  Returning it must restore its previous ownership/context and relinquish the
  player's captured ownership exactly once; releasing a different Pal or a
  distant duplicate must not satisfy the demand. Return is a dedicated action,
  not an ordinary friendly-companion release.
- If the player keeps the animal, allow a witness to report the livestock theft
  and involve lawmen through native crime/witness rules where feasible. Preserve
  native reporting interruption, witness perception, pursuit and bounty behavior;
  do not summon lawmen from an unseen capture or report the same theft repeatedly.
- Failed capture attempts and capture success are separate events: the observed
  throw can trigger an attempted-theft response even when capture fails. Demand
  return only if the animal was actually captured; do not demand an impossible
  return or falsely report a completed theft. No reward/capture rollback
  may duplicate the animal, inventory or crime reports.
- Preserve mission/script ownership protections. This feature does not authorize
  removing protected mission livestock or breaking ranch/hunting objectives.

Acceptance: an observed sphere throw at owned livestock produces immediate
objection before success/failure, returning the same captured animal
within the allowed window resolves the demand before reporting, keeping it leads
to a native witness report/law response, and an unseen capture creates no remote
witness. Verify wild captures, owned companions, multiple witnesses, interrupted
reporting, repeated interaction, streaming and reload without duplicate animals
or repeated reports. Behavior after an already-filed report follows native law
rules rather than silently erasing a bounty on return. Not implemented yet.

## Distribution boundary

The mod is original integration/bridging code only. Include no files or data from Palworld or RDR2 in the public repository, release packages or downloads supplied by the mod. This includes extracted or converted models, textures, animations, audio, game databases, metadata dumps, and generated content derived from either game. Keep such local inputs, caches and outputs outside the repository and distribution staging directories. Obtain external tools and dependencies separately under their own licenses.

MIT covers only original project code and related original files; CONTENT-NOTICE.md excludes third-party content. Local preparation and ownership of both games do not grant redistribution rights. Bridging the installations does not require simultaneous execution of both games.

## Acceptance

- Throw a sphere in live RDR2 Story Mode with Palworld closed; verify success, failure and interruption paths and their sphere-consumption rules.
- Capture, save/reload, release and recall the same Pal without duplicate ownership, duplicated entities or state loss.
- Verify keyboard/controller controls, native inventory and weapon-wheel behavior, protected actors and streaming.
- Verify sphere presence/count/selection under throwables, actual non-damaging throwing and capture impact, ordinary throwable coexistence, depletion/refill and save/reload. For the optional companion wheel selector, verify roster display, selection/release/recall and no accidental weapon use on either input method.
- Verify the single companion item shows the selected party member, D-pad left/right cycles correctly (including wraparound and empty/one-member parties), and using it releases that member or recalls it if already out. No deployment changes from browsing; repeated use must never duplicate a Pal. Test selection changes with an active companion and keyboard equivalents separately.
- Verify a captured/released Pal is friendly, follows on foot and while the player travels, and attacks a confirmed hostile enemy. Neutral/friendly actors must remain unharmed; verify combat ends when appropriate, recall works during combat, and allegiance persists across save/reload. Check mission-sensitive encounters separately.
- Test the first-shot gate separately: hostile presence/aiming without a shot stays peaceful; the player's first shot enables combat against confirmed hostiles; a hostile enemy's first shot also enables it; unrelated neutral gunfire does not. A later encounter starts with the gate reset. Behavior for melee-only encounters remains to be decided; do not silently treat melee as gunfire.
- Verify immediate defense against attacking wildlife before gunfire, deliberate shot-at-game pursuit, no pursuit from aim alone, no attacks on unrelated/owned Pals, and human hold-fire during an animal-only encounter. Human melee-only handling remains undecided.
- Verify an enemy switches to a closer visible fighting Pal and can damage/kill it, then can switch back to a closer player; obstructed targets and recall must not leave the enemy shooting a deleted entity.
- Audit the complete repository and release package: original code/configuration/documentation only, no game files, game data, derived content or bundled downloaded tools. Verify local content stays outside both boundaries.
- Record in-game results separately from build or offline validation.

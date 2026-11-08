# World spheres, Paldium and campfire crafting

Status: required; source-only 0.5.0 world/crafting adapter candidate, not installed
or verified in-game. Native sphere weapon registration/equip/throw is unfinished.

Source-only foundation added: capture save v3 stores Paldium fragments,
spheres and sorted stable pickup receipts in one atomic save. Legacy v1/v2
profiles migrate without changing Pal ownership, health or remaining spheres.
Collection commits its reward and receipt before authorizing pickup removal;
crafting commits ingredient debit and sphere credit together. Failed writes,
insufficient materials, capacity overflow and duplicate pickups grant nothing.
The receipt ledger is bounded at 4096 IDs and fails closed when full; no
respawning or eviction is implemented. Inventory counts are bounded at 999.
Recipe costs/yields are adapter-supplied and no final recipe is selected yet.
The candidate now streams bounded locally configured resource models, uses native
hold prompts, persists receipts before removing pickups, and offers one shared
prototype recipe (1 fragment -> 1 sphere) near recognized campfires within private
registered player-camp anchors. It does not identify camp ownership automatically;
temporary camp anchors need local updates when moved. Main and temporary camps use
the same recipe. No game-derived art or map coordinates are shipped. Missing or
invalid local catalogs/models fail closed. It defaults off, has no automatic
resource placements/unlocks/respawn, and has not been installed or tested in-game.
See BUILD.md for local input format and acceptance limits. Five offline suites
pass, including catalog validation and trusted camp/proximity checks.

**Next implementation priority, explicitly queued by the user:** craftable
Pal spheres at the player's camp/crafting area, available in the native
weapon wheel under **throwables**, with real equip/throw/capture behavior and
one persistent material/sphere inventory. Finish this acquisition-to-use loop
before further optional combat testing or roster expansion.

## Capture Kit (Bevy) research lead

User supplied **[Capture Kit (Bevy)](https://github.com/ufodriverr/capture-kit-bevy)**.
Reviewed revision `04c8554fbb44c077afb6a559fdc32294e519f52c` (1.0.0): original
creature-capture starter kit, code MIT OR Apache-2.0, not a Palworld rewrite.
Its animation module uses procedural primitive-model poses, not ready-made
RDR2 skeletal clips. A first engine-independent capture-rules/shake DLL was
built under the MIT option using the external capture module, with original C
ABI wrapper/header, retained license/notices and passing real-DLL C++ tests.
See bridge/README.md. The installed ASI does not use this DLL yet; native wheel,
crafting, visible sphere/fragment resources and animation adaptation remain.

Inspect the actual LICENSE, copyright/attribution requirements, dependency
licenses and separate animation/art licenses at a recorded revision. Confirm
that any offered assets are original/licensed material rather than extracted
Palworld content. Check animation availability, skeleton compatibility and
RDR2 conversion needs; a Bevy animation/runtime is not automatically an RDR2
animation resource. If suitable, adapt only clearly permitted components and
retain their notices; third-party code is not relabeled as original MIT project
code. Keep evaluation checkouts/tools/assets outside the public source boundary.
The earlier no-game-content distribution requirement remains in force.

Provide visible, collectible Pal spheres in the RDR2 world and sphere crafting
at the player's campfire/crafting area, including main and temporary camps.
Introduce visible Paldium Fragments through locally generated resource pickups
or deposits with suitable placement, prompt, collection and respawn behavior.

All acquisition routes must feed one persistent inventory used by the weapon
wheel and capture system. A displayed prop alone is not a collectible item.
Developer spawning may test the pipeline but is not the finished acquisition
loop. Define recipe quantities/ingredients, unlocks and allowed spawn habitats
before expanding. Native camp/crafting integration must preserve normal use.

Acceptance: collect one fragment once, save/reload without duplication, craft
one sphere by consuming the required materials exactly once, equip/throw it
through the native wheel and verify capture/consumption. Repeat at main and
temporary camps and test insufficient materials, interruption, streaming,
pickup/deposit respawn and keyboard/controller prompts. Legendary capture and
killable/skinnable friendly Pals remain required in the linked gameplay tasks.

Original integration code only in public source. Sphere/resource art, game
metadata and converted resources must be prepared locally from the user's
installations outside source/release boundaries. No downloaded/extracted
Palworld or RDR2 content may be distributed with the mod.

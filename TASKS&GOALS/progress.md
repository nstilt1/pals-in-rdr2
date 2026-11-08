# Progress

- User confirmed Lamball displays in RDR2 with asset build 0.1.2 and plugin 0.2.3.
- F8 nearest-compatible-animal conversion implemented.
- Shared compressed textures separated from model variants.
- Corrected native hidden-component buffer lengths and restored embedded masks after repeated hangs.
- Added versioned per-stage logs and local stack-capture helper.
- Source-only publication folder and local-file installer prepared.
- Sheep-based Lamball asset candidate 0.1.3 installed locally with ASI 0.2.3 unchanged; confirmed goat-based 0.1.2 preserved privately for rollback.
- Candidate uses native sheep identity to address the reported goat UI label and goat skin reward. Three body variants, nine body LODs, 23 preserved hidden buffers, embedded masks, shared textures and weights passed offline checks; independent reader reported zero warnings. In-game sheep appearance, identification and rewards remain pending.

In-game confirmation does not establish that all planned gameplay and stability checks pass.

- November 8, 2026: user confirmed the sheep-based Lamball correction worked. Preserve asset 0.1.3 with plugin 0.2.3 as the current working build. Further feature work must not overwrite its assets.
- Built developer plugin candidate 0.3.0 with aimed capture, one-slot private persistence, release/recall and friendly-follow task assignment. Targeting and capture transaction tests pass; native item, throw, combat and in-game validation remain unfinished. Sheep assets are unchanged.
- Installed capture developer candidate 0.3.0 after the user closed RDR2; verified plugin and backup hashes and unchanged sheep assets/LML configuration. Capture profile lamball-test-01 enabled. Awaiting in-game capture/release/recall checks.
- User confirmed a failed capture followed by successful capture, release and following in 0.3.0. Combat was absent as expected. Recall/persistence checks remain separate. Requested native throwables-wheel sphere entry and optional companion selector recorded.
- Combat candidate 0.3.1 builds and passes targeting, capture and encounter-gate tests. Adds first-shot-gated pursuit/contact strikes; in-game validation pending. Native wheel widgets/providers and inventory operations inspected privately; no wheel integration claimed.
- Installed combat candidate 0.3.1; verified unchanged sheep/LML assets and backed up working 0.3.0. Uses isolated lamball-combat-01 profile; original deployed lamball-test-01 retained unchanged. Awaiting combat/hold-fire/recall test.
- User confirmed release/recall and Lamball fighting a human in 0.3.1. New 0.3.2 candidate adds nearest viable enemy-opponent selection, deliberate shot-at-game hunting and immediate defense against attacking wildlife. Human hold-fire rule retained. Three test suites pass; actual reciprocal shooting, hunting and animal defense remain in-game acceptance checks.
- Installed 0.3.2 after user closed RDR2. Previous 0.3.1 backed up, installed hashes verified, sheep/LML files unchanged, and existing recalled Lamball/test-sphere count preserved in the same profile.
- Installed 0.4.0 + new local roster assets 0.1.4: automatic native goat=Caprity, boar/pig=Rushoar appearance replacements; working sheep=Lamball assets unchanged. Offline native-buffer, texture/alpha, weight and independent-reader checks passed; new appearance/animation/skinning pending in game. Species-aware single-slot capture saves migrate old Lamball records. Companion death retains the corpse and removes ownership instead of allowing resurrection.
- User prioritizes roster expansion over further combat tests. Friendly Pals must be killable/skinnable; legendaries must be catchable. Visible world spheres, campfire crafting and collectible/spawned Paldium Fragments are required and remain unfinished.
- Capture Kit reviewed at pinned 1.0.0 commit. Built a separate capture-math/shake DLL under its MIT option with an original C ABI, header/import library and retained notices. Actual DLL-load/interface tests pass; not yet connected to the RDR2 plugin.
- Installed 0.4.1/assets 0.1.5 Chikipi candidate for both shared native chicken components/all three configured outfits (including requested Dominique coverage). New native/resource checks pass; other Pal resources and capture save unchanged. In-game chicken appearance/animation pending.
- Source-only 0.5.0 adds bounded local resource streaming, native hold prompts,
  shared one-fragment/one-sphere camp recipe and durable pickup receipts feeding
  capture inventory. Trusted camp anchors/local models must be prepared privately;
  automatic camp ownership, placements, visuals and native sphere wheel/throw
  registration remain unfinished. Five offline suites pass; candidate not installed.
- Companion selector should show the selected Pal's locally prepared party portrait.
  Witnesses should object immediately to an observed sphere throw at owned farm
  animals; successful theft adds a safe-return window before native law reporting.
  Both requirements are queued, not implemented.
- Original additive PSO builder now prepares private sphere weapon/ammo/catalog
  metadata with a dedicated weapon slot. Independent re-read preserves all old
  definitions; configured sphere damage is zero with one ammo mode. Six suites
  pass. Sphere models/icons/localization, native inventory/throw/capture and the
  user-selected fourth Pals wheel page remain unfinished; no metadata installed.
- Mission-origin Pals must follow the player outside herd handoffs/cleanup,
  particularly The Sheep and the Goats. Dedicated mission reconciliation is
  required; generic protected-actor capture remains blocked.
- Installed 0.6.0 party/native-Pals-pane/mission-capture candidate with a complete
  0.4.1 startup/profile/appearance backup. Five-member save v4 retains legacy
  ownership and inventory. Independent native RBF roundtrip/UI preservation and
  all eight suites pass. One mudtown4-owned sheep can enter the guarded capture
  path while another survives, with one durable claim per profile. Page display,
  controls, mission handoff/completion and follow persistence are pending live QA.
  Existing eleven Pal resources and profile were hash-verified unchanged.

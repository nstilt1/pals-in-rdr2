# Expand the species roster

Status: new Caprity/goat and Rushoar/boar/pig candidates installed locally with plugin 0.4.0; offline validation passed, in-game validation pending. Other mappings below remain proposals.

Latest direction: prioritize making more Pal species and **automatic replacement
of eligible native animals**, without requiring F8 for ambient encounters.
Begin with Caprity/goat and Rushoar/boar as the next conversion candidates.
Keep native entity identity/AI/rewards where possible; do not repeatedly
spawn/delete ambient or mission animals. Retain the working sheep/Lamball build.

User requested deer, buck, squirrel, different birds, elk, panther/cougar,
bison, turkey, rabbit, chicken, dog, cat if present, goat, pig and boar.
Keep sheep -> Lamball working and preserve species identity/reward behavior.

User explicitly reported **Dominique chickens** need replacement. Cover native
chicken breed/outfit/component variants with Chikipi rather than changing only
a single visible chicken variant. Validate Dominique and other native chickens,
egg/harvest behavior where applicable, damage/skinning and streaming. Chicken
candidate is now installed in plugin 0.4.1/assets 0.1.5. Both head_000/head_001
body components referenced by all three configured native chicken outfits are
replaced with Chikipi. Four native buffers and four appended body LODs, DDS
payloads/alpha and weights passed checks; independent reader zero warnings.
Actual Dominique/other-breed appearance and animation still need confirmation.

Proposed visual/theme matches for discussion:

| Native species | Candidate Pal |
| --- | --- |
| Deer and buck | Eikthyrdeer |
| Elk | Reindrix |
| Squirrel | Lifmunk |
| Hawk | Galeclaw |
| Eagle | Nitewing |
| Owl | Hoocrates |
| Crow/raven | Cawgnito (ground-oriented; flight needs separate work) |
| Duck | Fuack |
| Panther | Grintale |
| Cougar | Blazehowl |
| Bison | Mozzarina |
| Turkey | Tocotoco (stylized bird choice) |
| Rabbit | Ribbuny |
| Chicken | Chikipi |
| Dog | Yakumo; Direhowl is an alternative, particularly for wolves |
| Cat | Mau |
| Goat | Caprity |
| Pig and boar | Rushoar (domestic/wild presentation can differ) |

These are design recommendations, not verified rig compatibility. Validate
local asset availability, proportions, locomotion, native variants, collision,
damage, skinning/rewards and streaming for each. Bipedal/floating/ground-bird
designs must not be assumed compatible with quadruped/flying rigs. Species can
share a Pal family; do not invent a purported pig Pal to force unique mappings.

Suggested first additions for evaluation: deer/buck, goat, pig/boar and chicken.
This ordering is provisional, not a claim of measured conversion difficulty.
Add species-aware capture/ownership and companion behavior; existing runtime
capture/release and ownership only support Lamball. Do not globally swap new
appearance resources and leave capture releasing every species as sheep.

Legendary counterparts have a separate task. Preserve mission/legendary
identities and do not enable generic deletion-based transformation for them.
All extracted/converted content and tools stay outside repository/releases;
publish only original integration code, configuration and documentation.

# Legendary animals become associated Pal types

Status: required; not implemented. User explicitly requires legendary Pals to be catchable.

Map legendary animals to the Pal family chosen for the corresponding ordinary RDR2 species. Consider a rare/variant or Alpha-style presentation where suitable; the user has not approved a final species/variant roster. Do not randomly assign unrelated Pals.

Preserve the legendary animal's native identity and entity references, tracking/clues, encounter conditions, hunt progression, death/skinning interactions, unique rewards, compendium state, trapper/fence unlocks and save state. Appearance replacement must not silently turn a legendary animal into an ordinary animal or erase its unique rewards. Implement legendary capture with defined capture-versus-hunt consequences; unique rewards and ownership must not duplicate. Protection in the current generic routines is an interim compatibility restriction, not a decision to exclude legendary capture from the finished mod.

Keep the current spawn/delete F8 and capture routines away from protected legendary/mission actors until dedicated compatibility handling is implemented and verified. Do not assume that mission-entity checks identify every legendary encounter. Add explicit legendary eligibility protection before expanding general species capture.

Acceptance:

- Verify one legendary encounter's normal behavior, then its species-associated Pal appearance without replacing the underlying hunt entity with an ordinary animal.
- Complete tracking, encounter, kill/skinning and native legendary reward/unlock flow.
- Verify save/reload, failed hunt/retry, streaming, unique-animal persistence and no duplicate legendary rewards.
- Verify required legendary capture, its defined hunt/progression outcome and mutually exclusive capture/death rewards.
- Expand to other legendary families only after the first encounter passes. Keep game-derived resources and metadata local, outside repository/releases; publish only original integration code.

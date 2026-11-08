# Lamball uses sheep identity and rewards

Status: user confirmed the sheep change worked on November 8, 2026. Broader checks below remain unfinished unless separately confirmed.

The user reported that a sheep converted to Lamball was identified as a goat and yielded goat skin. The old appearance replacement used `a_c_goat_01` underneath. A cosmetic rename alone would leave the wrong rewards.

The local 0.1.3 candidate uses `a_c_sheep_01`, native sheep component identities and the sheep rig. The working 0.1.2 goat-based assets and plugin were backed up privately; plugin 0.2.3 remains unchanged. No game resources or derived content are included in the public repository.

Acceptance:

- Restart RDR2 fully in Story Mode and verify sheep appear as Lamball without freezing.
- Identify a Lamball: the UI must say Sheep or Lamball, never Goat. A custom Lamball label is not implemented by this candidate.
- Kill and skin one: native rewards must correspond to sheep, with no goat skin from that animal. Verify actual inventory changes, not only an on-screen notification.
- Check F8 on an eligible non-sheep animal: it becomes a sheep-based Lamball; a sheep already using the configured model is left in place.
- Check movement, damage, corpse appearance, streaming and save/reload. Keep mission actors excluded from F8 and do not claim mission compatibility from appearance alone.
- Confirm goats retain their native appearance after retiring the old goat override.

Offline resource validation passed. The user's general confirmation establishes that the requested sheep correction worked; detailed streaming, mission and long-session checks are not inferred from that confirmation.

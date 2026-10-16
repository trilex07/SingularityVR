# Reverse-engineering journal

2026-10-16: Centurion `ch_inview_alt_pistol_mesh.psk`: measured 5799 wedges,
79 bones; rigid `clip` subtree yields 1067 UV vertices / 1252 triangles and
zero crossing triangles. Read-only native reload observation added; no direct
Reload ABI inferred from exported names. Evidence, hashes, falsifiers and
remaining gates: [Centurion reload](docs/centurion-manual-reload.md).

Continuation: inspected all three orthographic views of the private clip cache:
six-chamber cylinder. Validated RvGame exports name the pistol-cylinder component
and shared weapon Reload state/functions. Added read-only per-weapon UFunction
binding identity, shared-interpreter discriminator and ammo-change records;
no direct native ABI/call or inventory write inferred. Synchronized current
MAIN db4a261 into this own branch, preserving both sides of documentation conflicts.

Continuation: original Alt_pistol Reload PSA is 53 frames /24 Hz; clip local
motion returns to bind-relative start. Track 54 differs from PSK bone 57;
name-map through original parent cog_alt_pistol, never PSA placeholder parents.
Original mesh resolves material AR_Pistol and d/n/s/rim textures; offline parent
shader imports remain missing, so no claim of complete native material parity.
Validated Reload notification array has AnimNotify_RefillClip at 1.47052908 s;
RvGame names its owner RvSharedPawn. Added player-pawn-only read-only observation,
not a direct call or inventory write. Bounded PSA/tag readers + synthetic tests
and exact asset roundtrip; private original assets/reports remain ignored.
HUD freshness/identity/capacity guards and physical-left ownership strengthened;
360000 deterministic lifecycle frames pass, without Steam or headset claims.

PR37 base advanced during continuation. Merged its committed f0b96ad only into
own branch; resolved one input conflict preserving both left-grip arbitration
and MAIN's new knife impulse attack window. No accepted-source rollback:
canonical INI, knife, cinema and muzzle policy files match new base exactly.
Clean 9c164eb /O2 x86 PE32 rebuilt and candidate hash verified; receipt in doc.

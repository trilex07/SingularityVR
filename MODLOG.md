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

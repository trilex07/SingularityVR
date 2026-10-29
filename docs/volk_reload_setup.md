# Volk S4 reload mechanics — source contract and handoff

## Evidence and deliverables

Source is the original `ch_inview_shotgun_mesh.psk` (SHA-256 `941c871b695480e608ac8eb0ca25b99731344eca57383c9c5927cf71e7baf1ab`), **not** the machinegun PSK. The inspected source has 79 bones, 8297 wedges and a rigid, cleanly extractable `clip` magazine (242 UV vertices, 198 triangles, no mixed skin-boundary triangles). The original `Shotgun.psa` was not present in this workspace. An existing private `volk_reload.svwa` contains the previously validated 108-frame / 36 Hz `Reload` descendant tracks, but is not a substitute for the original PSA when authoring a Blender rig.

Run `python tools/reload/export_volk_mechanics.py --psk <original ch_inview_shotgun_mesh.psk> --output build/volk-reload-mechanics`. It creates `volk_magazine.glb`, `volk_magazine_preview.png` and `manifest.json` from the original rigid clip only. These are private local outputs and intentionally ignored by Git. GLB has original UVs, no material or altered appearance. It is in **clip bone-local coordinates**, converted at 52.5 Unreal units per metre; the source PSK is the attachment authority. Do not treat the GLB origin as a measured controller grip or physical magazine socket.

## Hierarchy and names

Keep the original PSK hierarchy and bind pose. Programmer aliases are descriptive helpers, not renamed source bones:

| Desired helper | Source bone / meaning |
| --- | --- |
| `volk_root` | `Root` (0); preserve original ancestor chain |
| `volk_body` | `cog_shotgun` (53), below `cog_gun` (52) |
| `volk_magazine` | `clip` (55), child of `cog_shotgun`; detachable rigid object |
| `volk_trigger` | `Trigger` (57), child of `cog_shotgun` |
| `volk_reload_part_01` | `release` (54), child of `cog_shotgun`; source name remains authoritative |
| `volk_muzzle` | `muzzle_flash` (58), child of `cog_shotgun` |
| `volk_grip` | helper to be fitted to the HD model / physical hand; no verified source grip socket |
| `volk_mag_socket` | helper at `clip` bind pose relative to `cog_shotgun`; use source bind matrix |
| `volk_left_hand_hint` / `volk_right_hand_hint` | optional helpers; require measured fitting |

`Slide` (56) is a separate source moving part and must stay separate. All five reload-related descendants (`release`, `clip`, `Slide`, `Trigger`, `muzzle_flash`) keep their PSK parent, local bind, rotation and scale conventions. Do not recenter or apply transforms to the original skeleton. A detached magazine retains `clip` local geometry and receives a world transform from MAIN; after insertion it can be reattached to the `clip` bind/socket pose.

## Modular motion support

The existing `volk_reload.svwa` is a 108-frame / 36 Hz source-derived transform track with the gun root frozen. Its palette is `[clip, Slide, cog_shotgun, release, Trigger]`. Position changes show clip travel around frames 22–27, 40–43 and 69–74; Slide travel around 20–24 and 75–76. These are **observed authored-track intervals**, not interaction timings or proof of the physical insertion axis. `release` and `Trigger` show no significant positional travel; any rotation needs separate review against the original PSA.

Recommended optional clips, only after the original PSA and HD part pivots are available:

- `mag_release`: small `release` movement only if its original rotational track confirms it; otherwise a state change with no animation.
- `mag_insert`: do not animate the magazine along a fixed path. MAIN follows the player's tracked hand until a validated socket snap.
- `mag_seat`: a short local settle from the final player-driven pose into the original `clip` bind pose; authored only after the socket and insertion axis are measured.
- `bolt_move` / `bolt_return`: use `Slide` source pivots and source-derived intervals if the weapon's real mechanical action calls for them. Keep these independent of magazine hand motion.

MAIN should drive Circle release, detachment, left-shoulder acquisition, hand-follow, insertion decision, physics, native ammo and final reattachment. Source `muzzle_flash` and body are visual/attachment references, not reload actuators. The currently generated GLB and preview pass offline extraction and export checks; a complete Blender scene, full-body weapon GLB/FBX and authored modular animation files remain pending original PSA availability plus a Blender-capable environment and HD-asset handoff. No Steam or PSVR2 validation was performed.

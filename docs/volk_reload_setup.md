# Volk S4 reload mechanics — source contract and handoff

## Evidence and deliverables

Source is the original `ch_inview_shotgun_mesh.psk` (SHA-256 `941c871b695480e608ac8eb0ca25b99731344eca57383c9c5927cf71e7baf1ab`), **not** the machinegun PSK. The inspected source has 79 bones, 8297 wedges and a rigid, cleanly extractable `clip` magazine (242 UV vertices, 198 triangles, no mixed skin-boundary triangles). The original `Shotgun.psa` was not present in any inspected local worktree. The private `volk_reload.svwa` (SHA-256 `cdb998651dea731cc552cb9c59836c0804a9678fce2f20c18daf792551d4d750`) contains the validated 108-frame / 36 Hz `Reload` descendant tracks. Its four tracks are sufficient to reproduce their original keyframes deterministically, but do not contain the complete PSA for all 79 bones or the complete textured weapon geometry.

Run `python tools/reload/export_volk_mechanics.py --psk <original ch_inview_shotgun_mesh.psk> --output build/volk-reload-mechanics`. It creates `volk_magazine.glb`, `volk_magazine_preview.png` and `manifest.json` from the original rigid clip only. These are private local outputs and intentionally ignored by Git. GLB has original UVs, no material or altered appearance. It is in **clip bone-local coordinates**, converted at 52.5 Unreal units per metre; the source PSK is the attachment authority. Do not treat the GLB origin as a measured controller grip or physical magazine socket.

Run `python tools/reload/build_volk_mechanical_glb.py --psk <original ch_inview_shotgun_mesh.psk> --svwa <validated volk_reload.svwa> --output build/volk-reload-mechanics`. It creates `volk_mechanical_rig.glb`, `animation_socket_manifest.json` and `volk_mechanical_preview.png`. The GLB has all **79 original bone names and parent links** as an importable Blender transform hierarchy, original local bind pose, five separately movable original rigid parts (`volk_body`, `volk_reload_part_01`, `volk_magazine`, `volk_slide`, `volk_trigger`), and six separate glTF animation actions. All 10,835 original source triangles are assigned to exactly one source bone, with original UVs. This is an animatable transform hierarchy, not an Armature skin. The actions use original SVWA keys and leave `Root`, `cog_gun` and `cog_shotgun` static. A textured FBX or `.blend` cannot be generated on this machine without Blender and the original material handoff. The complete untextured GLB is the delivered export format.

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

Delivered glTF actions:

- `mag_release_state`: frame-0 static `release` key. The source release track has zero position and rotation travel; this is an event/state marker, not a fabricated moving latch.
- `mag_remove_reference`: original `clip` frames 21–27. Reference only; do not play while a hand owns the magazine.
- `mag_insert_reference`: original `clip` frames 39–43. Reference only; source-authored return movement, not a physical insertion path.
- `mag_seat`: original `clip` frames 68–74, a short 1.62 UU source movement. Its final position is 0.56 UU from the PSK clip bind in local Y, so MAIN must resolve the handoff to its attachment pose deliberately.
- `slide_move`: original `Slide` frames 19–24.
- `slide_return`: original `Slide` frames 74–76.

The source does not justify a visible `release` actuation, a controller grip offset, or an insertion axis. MAIN should follow the player's tracked magazine until attachment and only use optional source clips when their start pose matches the physical state. The manifest records exact PSK bind translations and rotations for the relevant bones; the Asset agent can match these by source bone name/index rather than by a remastered mesh origin.

MAIN should drive Circle release, detachment, left-shoulder acquisition, hand-follow, insertion decision, physics, native ammo and final reattachment. Source `muzzle_flash` and body are visual/attachment references, not reload actuators. The mechanical GLB, preview and source-key/hierarchy test pass offline; Blender/FBX export and physical weapon fitting are not verified. No Steam or PSVR2 validation was performed.

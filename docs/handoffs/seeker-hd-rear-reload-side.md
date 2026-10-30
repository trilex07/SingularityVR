# Seeker SIDE asset handoff (offline candidate)

Private package: `build/seeker-private/final_handoff/` in the dedicated SIDE worktree. It contains original cooked/decoded source, source PSK/PSA, exact and PBR GLBs, 4K maps, an updated 64 px wheel atlas, previews, validation and SHA-256 manifests. None of these game-derived bytes are committed.

## What is ready

- The Seeker wheel illustration was edited from the verified original PSK silhouette. The private atlas replaces only `dev-seeker` at `(256,128,64,64)`; all other atlas pixels compare byte-for-byte after decode. It retains the existing catalog ID, ownership gate, and selection path. Its 1024 px transparent master and the approved generated-art master are in the private package.
- Source rig GLB preserves all 85 bone names and parent relationships. Its 54-frame, 24 FPS `Reload` reference drives descendants only; the weapon/controller root remains static. The source magazine is 410 triangles in the `clip` subtree with zero cross-boundary triangles. Separate origin-centered magazine GLBs preserve the original clip-local pivot. The socket manifest contains the source bind matrix, not a fitted hand offset.
- 4096×2048 UV-compatible maps carry authored 4K machining height, microtexture and material response. The HD GLBs embed albedo, packed metallic/roughness, and a glTF Y-converted tangent normal. The private standalone normal PNG retains DirectX Y. The native shader binding and metallic mask require visual review before use.
- Offline structural validation passes for GLB import, source rig/animation, separated magazine, maps, tangent normal lengths and atlas isolation. `trimesh` reimport passed for both HD GLBs. Blender import/export was not run because Blender is unavailable in the SIDE environment. The supplied original-versus-HD render uses identical simple lighting; it is a UV projection and does not demonstrate the full PBR response.

## Native reload gate

`Shared_ARSteerable` is a heavy `RvWeaponShared_RocketLauncher`, with `ERAT_ARSteerable`. That class inherits through `RvWeaponShared_RememberProjectile` to `RvWeaponShared`; neither intermediate class defines a script `AttemptReload`, `ServerStartReload`, `RefillClip`, or `ReloadFinished` override. The original Seeker PSA includes a 54-frame reload. **These facts do not establish that a live Seeker reload increases clip and decreases reserve.** The required decompressed PE text witness and a same-transaction authoritative clip/reserve receipt were unavailable to SIDE. The physical reload was therefore not connected, and no ammo or gameplay state was changed.

Before enabling a default-OFF native candidate, MAIN must capture the actual Seeker `AttemptReload` vtable target and one legitimate source reload with local weapon identity, clip/reserve before and after, and native acknowledgment. A method pointer or animation notify alone is insufficient. Then integrate the existing tracked-left-hand carry and permit path with the source clip socket, approach-axis rejection, and per-eye replay; test empty reserve, full clip, cancellation, switching, reset, and guided fire. Steam and PSVR2 are MAIN-owned and not attempted here.

## Reproduce private package

Run the three `tools/seeker` builders against the SHA-pinned private PSK/PSA, original texture PNGs and atlas. `build_seeker_icon.py` also requires the approved generated-art master at the hash pinned in that script. Its art prompt was to polish the verified long-barrel Seeker PSK render into a transparent industrial sci-fi weapon illustration while preserving the rear mechanism and silhouette. Run `validate_seeker_package.py --package <private-package>` after assembly. The generated icon master is private and must accompany the package for exact reproducibility.

Status: **OFFLINE ASSET STRUCTURE PASS; HD visual acceptance pending PBR review; NATIVE RELOAD BLOCKED by missing ammo contract; Steam/PSVR2 NOT RUN.** This is a draft integration handoff, not a completed physical reload.

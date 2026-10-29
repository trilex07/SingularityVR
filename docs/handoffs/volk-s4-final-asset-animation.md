# Volk S4 final asset and animation handoff — offline Blender PASS

Private local package: `C:/Users/Trilex/vr-mods/volk_s4/final_handoff/`. This branch commits scripts and this note only. The package contains source-derived game geometry/animation and HD maps, so it stays outside Git.

## Package

- `volk_s4_hd_animated.blend`: saved Blender 5.2.2 scene with 79 original named transform nodes, 11 separate meshes, packed PBR maps and six modular actions.
- `volk_s4_hd_animated.glb`: textured HD weapon and six actions; independent Blender re-import confirms 11 meshes and six action names.
- `volk_s4_hd_magazine.glb`: five independently detachable magazine meshes, including the HD ribs/fasteners, in original `clip` bone-local coordinates.
- `maps/`: 4K albedo, converted glTF/Blender normal, roughness, metallic and optional height. Native HD normal is left untouched in the visual source package.
- `source/`: byte-identical original PSK and validated SVWA key cache for reproducibility.
- `animation_socket_manifest.json`, `assembly_manifest.json`, `normal_conversion_manifest.json`, `blender_validation.json`, `blender_pose_validation.json`, `final_handoff_manifest.json`: identities, bone/socket bindings, action frames, hashes and offline gates.
- `previews/`: existing original-vs-HD comparisons. `animated_inspection_bind.png`, `animated_inspection_slide_move.png` and `animated_inspection_mag_seat.png` are controlled-lighting Blender renders from this assembled scene.

SHA-256: original PSK `941c871b695480e608ac8eb0ca25b99731344eca57383c9c5927cf71e7baf1ab`; SVWA `cdb998651dea731cc552cb9c59836c0804a9678fce2f20c18daf792551d4d750`; animated GLB `5a0d3e21068ecd178a846901c42b84d256752a0121aa2ce4b3a02df57a045741`. The local final manifest hashes every delivered file.

## Bind and motion contract

Visual and mechanical inputs have the same 79 bone names/parents and the same PSK SHA. Every visual manifest bind matrix matches the PSK raw-space bind **exactly** (maximum matrix entry difference: 0 UU). Original five rigid partitions account for all 10,835 PSK triangles; six HD detail meshes add 1,892 triangles. Detail OBJ points were transformed from source global PSK units to their named bone-local coordinates with original inverse bind; maximum roundtrip error is below `3e-14` UU before float32 export. There are no new bones or socket offsets. `volk_body` and body fasteners bind to `cog_shotgun`; magazine, its ribs and fasteners bind to `clip`; moving part and fasteners bind to `Slide`; trigger and release bind to their original names.

Blender pose gate PASS: `cog_shotgun` and body remain static in all six actions; magazine and all its HD details move together with `clip`; Slide detail moves with `Slide`. A world-preserving detach followed by a 0.1 m magazine translation leaves the body fixed. `mag_release_state` has no source motion and remains a state marker. `mag_remove_reference` and `mag_insert_reference` are only original source-motion references; MAIN must use tracked-hand movement for the physical magazine. `mag_seat`, `slide_move` and `slide_return` remain independent optional actions. The source-authored seat ending differs from PSK clip bind by 0.56 UU in local Y, so MAIN must handle the attachment handoff explicitly.

## Material and native integration boundary

The visual package documents UE3/DirectX normal encoding. The local Blender/glTF copy flips only normal-map G (`255-G`); R/B are byte-identical. The final package gate compares every converted pixel to the unmodified HD source. The [glTF 2.0 specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html) defines the +Y normal texture and tangent `w` handedness. Blender exported `NORMAL`, `TEXCOORD_0` and `TANGENT` for all five original parts; every exported tangent `w` is ±1 and tangent length error is below `0.001`. Controlled-lighting renders were inspected for gross inversion or missing surfaces. This is an offline Blender material validation, not a UE3 shader proof.

Existing named native path: mesh `ch_inview_shotgun_mesh`, material `AR_Shotgun`, diffuse `AR_shotgun_d`, normal `AR_shotgun_n`. The existing texture override route recognizes exact diffuse/normal identities; a format-compatible DDS version of the HD albedo/normal may be considered through MAIN's guarded texture workflow. The GLB geometry, six actions and roughness/metallic/height maps require separate native geometry/animation or shader integration. Do not assume the UE3 material or override bridge consumes PBR maps or GLB directly. No runtime code, installed Steam file, DLL or configuration changed here.

## Reproduce

Use the official Blender 5.2.2 portable distribution; its Windows x64 ZIP was checked against the published SHA-256 `3849d17a682cba006075aaa3f3597ecb5c9c30ec31035b2e092c53e40679b535`. Blender supports [headless Python execution](https://docs.blender.org/api/main/info_tips_and_tricks.html). With private visual package and SVWA available:

1. `python tools/assets/assemble_volk_final.py --package <volk_s4> --svwa <volk_reload.svwa> --output <final_handoff>`
2. `python tools/assets/convert_volk_normal.py --source <volk_s4/textures/volk_s4_hd_normal_4k.png> --output <final_handoff/maps/volk_s4_hd_normal_gltf_4k.png>`; copy the other four HD maps into `maps/`.
3. `blender --background --factory-startup --python tools/assets/blender_finish_volk_final.py -- <volk_s4> <final_handoff>`
4. `blender <final_handoff/volk_s4_hd_animated.blend> --background --python tools/assets/blender_validate_volk_final.py -- <final_handoff>`
5. `python tools/assets/package_volk_final.py --root <final_handoff>`

Status: **OFFLINE_BLENDER_PASS**. Steam runtime, native geometry/shader integration and physical PSVR2 acceptance: **NOT RUN**.

# Seeker native texture candidate for PR 79

The production slot-65 bridge now selects the two Seeker textures by verified original compressed mip-0 content. Original geometry, skinning, UVs, shader, emissive/lens/specular resources and reload code are retained. This is native diffuse/normal replacement; it adds neither mesh replacement nor native PBR support.

Original first-person identities, recovered directly from the SHA-pinned logical `wp_SP_ARSteerable_SF.u` and `Textures_P0.tfc`:

- `ar_steerable_d`: 2048x1024 DXT5, FNV64 `5E9335E323F8FC14`, mip-0 SHA256 `e52761517224b225348f2f687ffcb5dd2dbe5c0f240be0c41b8570357404e0c4`.
- `ar_steerable_n`: 2048x1024 DXT1, FNV64 `7CF4F0B9A81D0522`, mip-0 SHA256 `5d7c55de625b1498efca3c5f975813c826abbdc529e8d987966e79b9063cda7b`.

These are the source `e99_steerable` material's first-person maps, not the separate `W_ar_steerable_*` world maps. The cooked normal is `TC_Normalmap`, linear, with `UnpackMin=-1`. The native replacement is corrected to DXT1. Diffuse alpha comes from the original RGBA export; glTF opacity does not replace that data. Metallic/roughness are not sent to native shaders.

## Production lifecycle

`TextureBridgeOwned=1` in SingularityVR.ini and `bridge_owned=1` in texture_override.ini select the existing production-owned bridge. No additional SetTexture detour is armed. The independent startup setting `[general] seeker_material_mode=0|1|2` defaults to 0: OFF, log-only, apply. Restart after changing it.

Production CreateDevice invokes `SingularitySeekerLifecycleV1(device,0)` outside drawing. It loads the immutable CPU pair, validates exact SHA256 pins, dimensions, DXT format, every compressed mip size and the 13-level chain, then uploads on the same device-owning thread. Files live in `vr-assets/reload/seeker/AR_Steerable_d.dds` and `AR_Steerable_n.dds` relative to Binaries. Both must pass before either GPU resource is published. Missing/invalid preload or upload failure leaves original textures active.

Reset/ResetEx invokes event 1 before native Reset and event 2 only after success. Event 1 releases the GPU pair; event 2 reconstructs it from CPU data without file access. D3D9 uses MANAGED textures; Ex uses DEFAULT plus temporary SYSTEMMEM upload textures. Device recreation invokes event 0, releasing the previous pair. Explicit device teardown users must invoke event 1 before releasing their owned device, as the production Reset path and GPU fixture do. The callback never creates resources on the preload/reporting worker.

The Seeker bind route performs existing content identification once per resource, stores only a scalar identity in resource-owned metadata, and selects an already ready texture. It does not store replacement pointers in source metadata, so Reset cannot leave stale pointers. Source writes invalidate the identity. Failure is not stored as a permanent negative classification. Mode 1 counts matches but never substitutes; the existing periodic reporter emits aggregate counters outside the bind callback.

## Private package and tests

Private package: `build/seeker-private/final_handoff`. `sha256_manifest.json` pins each file; `source/native_texture_identity.json` records exact package/cache provenance and bulk ranges. `runtime/` contains the paired DLL candidates, DDS files and unchanged source-derived reload caches. No private assets are committed.

Generate identities with `extract_native_texture_identity.py`, corrected DDS with `prepare_seeker_native_textures.py`, then regenerate registry and SHA pins with `register_native_targets.py`. `test_native_preload.py` checks recovered hashes, generated pins, modes and absence of DDS/file/allocation/upload calls in the Seeker bind route.

The actual bridge GPU fixture covers plain/Ex upload and pixel readback, both stage/eye indices, OFF/log-only forwarding, 10000 binds with unchanged read/upload counters, source-write invalidation, Reset/ResetEx, CPU-backed recreation, failed-preload fallback and explicit teardown without retained COM references. Existing texture scope/cache/forwarding tests and the production bridge integration fixture are run separately. Offline GPU results do not prove headset appearance.

Validated locally: optimized x86 bridge and production DLL builds; all texture-cache build fixtures; actual production bridge/shadow integration fixture; exact target/pin/no-bind-work checks; Seeker source animation, rear corridor, carry and native/projectile adapter tests; 30000 original-cache-backed legacy reload transactions; Centurion/hand/holster/knife tests up to the source-pose replay stage; remaining synthetic animation, native bridge, refill, four-weapon visual-seat and carry-ray fixtures run directly. The full reload runner's optional private Valkyrie/Volk/Kasimov source-pose replay cannot run because those three `.svwo` oracles are absent locally. This is a fixture availability limit, not a claimed PASS. Seeker's original 1704-pose oracle is present and passes.

## One MAIN acceptance route

MAIN backs up its accepted installation and stages the two candidate DLLs plus the private Seeker runtime files. Keep all Seeker reload flags OFF initially. With consistent bridge ownership, start in mode 1 and acquire a legitimate Seeker: confirm exact Seeker match counts and zero substitutions, owned wheel icon, standard/guided firing and support grip. Restart in mode 2: confirm ready-pair receipts, both-eye native material appearance, preserved alpha/emissive/lens/specular behavior, scope/menu coexistence and pause/load/Reset recovery. Compare mode 0 from the same save if needed.

The native ammunition gate remains default OFF. A shared AttemptReload target is not an ammo receipt. Before approving physical reload, MAIN records one legitimate source Seeker reload's local pawn/weapon identity, authoritative clip/reserve before/after and acknowledgment using the existing bounded native transaction diagnostics. Only then test Circle extraction, left-shoulder acquire, physical-left carry, rear-only insertion, one conserved acknowledgment, cancellation and weapon switch. SIDE has not deployed or launched Steam.

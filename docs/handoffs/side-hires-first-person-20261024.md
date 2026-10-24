# First-person texture candidates for MAIN

Branch: `feature/side-hires-weapon-textures`. Parent is PR #57 head `5cc9150a`. Offline asset preparation only; Steam, accepted DLL and INI untouched.

## What is ready

- **First A/B: Volk and original first-person gloves.** `AR_shotgun_d/n` and `inview_glove_d/n` are staged at `<assets-local>/material-remaster-hires/ready/<identity>.dds`. Originals are verified 1024×1024 top mips; candidates are 2048×2048 DXT with 12 levels. Source identities and exact SHA256 values are in [`hires-first-person-manifest.json`](../textures/hires-first-person-manifest.json). `prepare_hires_first_person.py` makes these from the original top mips with channel-safe Lanczos and restrained edge clarity; normal vectors are normalized before DXT1 encoding. The output contains no new invented material scratches. 2× sampling does **not** recover information beyond source mip0.
- **Second A/B: Valkyrie.** `AR_E99Rifle_alt_d` and `AR_E99Rifle_n`, 4096×2048, reused unchanged from the verified batch-2 set. This is a source-proven pair, but PR #57 `PriorityMaterial` currently admits only Volk and gloves; MAIN must separately review the bridge scope before enabling Valkyrie.
- **Later single-map A/B: Kasimov normal.** `ar_sniper_rifle_n`, 4096×2048, unchanged from batch 3. The diffuse source is only 1024×512 against UE3's 2048×1024 object, so a complete Kasimov set is blocked.
- **Centurion: blocked.** The available `handgun_d` capture is 1024×1024 DXT5 while the UE3 object is 2048×2048 PF_DXT1; `handgun_n` is likewise not proven to be a top mip. The captures are not promoted to a replacement.

All seven staged private filenames, full SHA256 values, source SHA/FNV identities, dimensions, formats, confidence and preview paths are recorded in the manifest. The ready directory has no extra DDS. All previews stay outside public Git, under `<assets-local>/material-remaster-hires/volk-glove-clean-v2/previews/` and the existing batch-2/batch-3 comparison folders.

## Quality finding

The previous native-resolution Volk remaster introduces conspicuous grain and colored speckles on smooth parts in the offline preview. It was **not** used as the base of this 2× set. The new diffuse derives directly from the verified original mip0. Its original DXT color/alpha data still includes some low-alpha speckles when viewed as raw RGB; treat visual acceptance as open. The clean-v2 private preview is the comparison to inspect. No game-render quality claim follows from this image.

## MAIN preflight and suggested gates

Run from the repository root (replace the placeholder with the private root):

```powershell
python tools/textures/verify_hires_set.py --stage '<assets-local>\material-remaster-hires\ready' --originals '<assets-local>\textures\original'
```

Only copy the four Volk/glove maps for the first default-OFF, controlled A/B. Preserve the accepted DLL and remaster configuration and back them up with hashes. Compare both eyes, close-up leather/metal appearance, alpha seams, normals, shadows, UI exclusion, Reset and frame pacing. Record actual `BRIDGE MATCH` and `BRIDGE SUBSTITUTE` receipts; asset readiness alone does not establish D3D9Ex render correctness. For Valkyrie, review and test an independently gated scope extension before copying its two maps. Test the Kasimov normal independently. Do not deploy Centurion or Kasimov diffuse from the current captures.

No runtime code changed on this branch, and there is no PSVR2 validation.

## Offline validation record

- `verify_hires_set.py --stage ... --originals ...`: PASS, seven replacements matched public hashes; original SHA256, FNV64, dimensions and FourCC matched the compiled target table; full mipchain byte counts matched each file.
- `python -m unittest discover -s tools/textures/tests -p test_verify_hires_set.py`: PASS, including mismatched target, changed hash and missing mip rejection.
- `spikes/texture_cache/build.ps1`: PASS, x86 PE32 `/O2`, D3D9 and D3D9Ex DXT1/DXT5 GPU readback, protected-source bypass, Reset/ResetEx and hook forwarding. Offline `dinput8.dll` SHA256: `767EE283A4675FEB9BBF1C60BBF9E2111FD71EBEEC57C15C761C7A25F7E1DA2D`. The DLL was not modified for these assets or deployed.
- Downsampling the four new Volk/glove candidates to source dimensions yielded RGB mean absolute differences of 6.693, 2.086, 2.357 and 4.515 respectively (diffuse then normal per asset); Volk diffuse alpha difference was 0.759 after DXT recompression. These are fidelity measurements, not perceived-quality or VR PASS results.

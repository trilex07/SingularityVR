# loading_screen.bik — producer evidence (offline L1/L2)

| Field | Value |
|---|---|
| Path (sandbox) | `RvGame/Movies/loading_screen.bik` |
| SHA-256 | `764E701B5049F182968D2DFFF4EE9ACCD13443B0D68B66B3FD1C0FB2F059E27D` |
| Container | Bink 1 `BIKi` |
| Video | 1280×720, **2.0 s**, **24 fps**, ~48 frames, **no audio** |
| When it plays | UE3 full-screen Bink during **level load** (between maps), not the pre-IIS boot chain |

## Decoded content (frame review)

Samples under `offline-deliverables/loading-vya-v1/analysis/` (identification pass).

1. **Top third:** flat white **SINGULARITY** wordmark with soft bloom (2D glow, not the chrome 3D boot movie).
2. **Center:** back of a gloved hand wearing the **TMD**; orange filaments in the side vials and core icon pulse slightly across frames.
3. **Background:** dark grey smoke/mist sliding horizontally; hand + logo stay centered.
4. **Animation:** subtle orange intensity change on the device (~2 s loop); logo geometry is static.

## VR branding approach (candidate)

Rejected composite (v1): a bright-pixel bbox included the hand, then an opaque black rectangle was pasted back. That cut the fingers and clipped the suffix into side boxes. Not a VЯ read.

Replacement: stamp two glyphs on the letter row only (y≈85–165), to the right of Y. Finger region vs stock before encode: mean channel delta 0. Bink still recompresses the whole frame.

Deliverable: `offline-deliverables/loading-vya-v1/candidate/loading_screen_vr.bik` + proof PNGs decoded from that `.bik`.

# SIDE → MAIN — boot `SNG_logo_vr.bik` (BIKi)

Worktree: `SingularityVR-boot-logo-vr` · branch: `fix/boot-logo-sng-vr-bik` · **no Steam/DLL/INI install from SIDE**.

## Startup movies (evidence)

| Question | Answer |
|---|---|
| Black pre-menu card + first SINGULARITY film the same asset? | **Yes — one file:** `RvGame/Movies/SNG_logo.bik`. |
| Second SINGULARITY-only `.bik` before IIS? | **No.** `Copyright_Sing.bik` adds the legal footer; ATVI/Raven/UE3 are other logos. |
| Tooling | `tools/title/analyze_boot_movie_sequence.py` → `deployment/hands-dubbing/boot-logo-v2/boot-movie-sequence.json` (gitignored). |

## Stock (backup before replace)

| Field | Value |
|---|---|
| Path | `RvGame/Movies/SNG_logo.bik` |
| SHA-256 | `FE40BA3B4521C4D0BFAC9EFD65B7BAF2C9F0F07F2E637A703DB1E7B29EA2B03C` |
| Video | 1280×720, 11.0 s, 30 fps, 330 frames, `BIKi` |
| Audio | 44100 Hz stereo Bink DCT (unchanged in candidate via remux) |

## Candidate (offline, gitignored)

| Output | SHA-256 | Notes |
|---|---|---|
| `SNG_logo_vr.bik` | `D85FC26835C61DB795AD6626F5AA7C9D0FD01B5724D3E845CBAE679DB994E441` | Real **Bink 1 / `BIKi`**; magic verified |
| `reference-vr-patch-preview.png` | `0DC5A249382A5AE849001EF8E923EA9E1852A6124F883EAB5AA28937E6B81224` | Wordmark + VR (A/R glyph clone) |
| `SNG_logo_vr.intermediate.mkv` | `3F5FED3D86269DF7C0CE5C4B76E4029873140828F4552306AD8689676B265395` | Pipeline only; **do not** ship to game |

Location: `deployment/hands-dubbing/boot-logo-v2/candidate/` under this worktree.

**Validation:** `SingularityVR-bink-dubbing/tools/bink_media.py validate-video` **PASS** (geometry, fps, duration, stereo audio layout; full decode).

Pipeline: `prepare_boot_logo_bink.py` → `binkenc` (libbink) → `remux_bink_stock_audio.py` (stock stereo audio + patched video). One-shot: `tools/prepare-boot-logo-full.ps1`.

## MAIN install (L4)

1. Backup stock `SNG_logo.bik`; re-hash to stock SHA above.
2. Copy candidate **`SNG_logo_vr.bik` → `RvGame/Movies/SNG_logo.bik`** (same file name).
3. Boot: black card + early SINGULARITY film both show **SINGULARITY VR**; IIS/menu title package unchanged.
4. Restore stock on failure; re-hash.

## Encoder note (SIDE host)

Live `git clone https://github.com/jmarshall23/libbink.git` returns **404** here. SIDE built `binkenc` from Wayback zip `third_party/libbink-wayback.zip` (not committed). See `tools/build-binkenc.ps1`.

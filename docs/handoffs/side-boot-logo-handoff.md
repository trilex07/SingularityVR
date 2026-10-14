# SIDE handoff — boot card SINGULARITY VR

Scope: black pre-menu wordmark only. **Do not** change `iis` character 10,
`SPMenus` character 166, `UI_SinglePlayer_SF.xxx`, DLL, INI, or Steam from this
track unless MAIN explicitly leases the movie file below.

## Producer evidence (offline, not L4)

| Candidate | Verdict |
|---|---|
| `RvGame/Movies/SNG_logo.bik` | **Producer** — 1280×720, 11.0 s, 30 fps, 330 frames; sample frame 180 is black field + centered wordmark only (matches user boot card). |
| `Copyright_Sing.bik` | Rejected for this card — same wordmark plus legal footer band. |
| `ui_logos.logo` (512×128 DXT5 in `ui_logos_SF`) | Related typography, HUD-sized; **not** proven as boot draw. |
| `iis` / `WFontLogo` HTML | **Out of scope** — `RvUIGFxMovie_IIS` menu field (MAIN title work). |

Falsifiers for MAIN witness: live `BinkOpen` path before IIS focus; replacing
only `ui_logos.logo` changes boot without touching `SNG_logo.bik`.

### Hashes (Steam stock, 2026-10-14)

| Asset | SHA-256 |
|---|---|
| `SNG_logo.bik` | `FE40BA3B4521C4D0BFAC9EFD65B7BAF2C9F0F07F2E637A703DB1E7B29EA2B03C` |
| `Copyright_Sing.bik` | `3FEAF76162AC15E9F1587F5B71A0221D9BAFF6A49FA7FBF0874B59F15707ED91` |
| `ui_logos_SF.u` (offline decode) | `6D3655ABAAC9DBAF1278E53D80558178798D0D04EB07F4A2788FE0EC38E1008A` |
| `ui_logos.logo` mip0 (DXT5) | `CBD287980B64FF3961E06E97FC8A69C3D1C8EBDE37421F92FFE02F06BC59382F` |

## SIDE candidate (private tree, not in git)

Run from worktree (requires local game Movies + offline `control.u` / `ui_logos_SF.u`):

```powershell
.\tools\prepare-boot-logo.ps1
```

Outputs under `deployment/hands-dubbing/boot-logo-v1/` (gitignored):

- `producer-evidence.json` — structured proof
- `candidate/boot-logo.json` — manifest
- `candidate/SNG_logo_vr.intermediate.mkv` — 11 s / 30 fps, audio from stock
- `candidate/reference-vr-patch-preview.png` — offline visual check only

Latest offline prep (append-only VR suffix cloned from frame A/R glyphs):

| Output | SHA-256 |
|---|---|
| `SNG_logo_vr.intermediate.mkv` | `96984BA727A6DC2A392A5C0214A20C4E9E0915B152AFFA44620D883B4A257C2C` |
| Reference patch preview PNG | `0DC5A249382A5AE849001EF8E923EA9E1852A6124F883EAB5AA28937E6B81224` |

**BIKi candidate:** see [`side-boot-logo-bik-handoff.md`](side-boot-logo-bik-handoff.md)
(`SNG_logo_vr.bik`, `validate-video` PASS, offline under `deployment/hands-dubbing/boot-logo-v2/`).
Build path: `tools/build-binkenc.ps1` + `tools/prepare-boot-logo-full.ps1`.

`texconv.exe` was not found on the SIDE host: not on `PATH`, and no
`texconv.exe` under the searched `Program Files` directories (about 47 s,
exit 0, empty result). That does not block this candidate. The boot card is
`SNG_logo.bik` via the MKV plus a later `binkenc` step, not a DXT5 replacement
of `ui_logos.logo`. If a 512×128 texture is required later, MAIN can add
DirectXTex `texconv` or another BC3/DXT5 encoder.

## MAIN install gate (physical L4)

1. Backup stock `SNG_logo.bik`; verify SHA-256 above.
2. Deploy candidate `SNG_logo_vr.bik` **same file name** under `RvGame/Movies/`
   (or lease via agreed manifest — movie only).
3. Boot: confirm black card shows **SINGULARITY VR** (same glow/weight), then
   IIS/menu unchanged relative to accepted MAIN title package.
4. Restore stock `.bik` on failure; re-hash.

No extracted game pixels are committed to git.

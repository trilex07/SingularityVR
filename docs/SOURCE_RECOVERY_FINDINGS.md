# Offline texture source recovery — findings (SIDE, no Steam contact)

**Branch:** `feature/texture-source-recovery`
**Status:** investigation + recovery attempt. Everything below is read-only
forensics plus UModel exports into scratch locations; no Steam launch, no
deploy, no renderer/injector/hands/shadow/menu changes.

## Question

Why are `handgun_d` / `handgun_n` (first-person pistol) captured at 1024px
when their UE3 objects declare 2048px — and can genuine mip0 be recovered
offline, with the same question for the sniper / railgun / TMD diffuses?

## Answer (short)

No. All locally available archives provably lack the required bulk data:

- `handgun_d` / `handgun_n`: object 2048px, package `RvGameMPLoader_SF`,
  cache `Textures_P0`, `MipTailBaseIdx = 11` (all 11 mips streamed, none
  resident). UModel export fails reading mip1 from the local `Textures_P0.tfc`
  with `assertion failed: Size == 0` — the package-declared TFC offsets do not
  yield valid data. mip0 was never reachable.
- The local `Textures_P0..P3.tfc` are four byte-identical copies of one
  1,257,087,323-byte file (12 spread 1 MB samples + head + tail identical),
  and the sandbox `coalesced.tfc` samples identical to them. There is exactly
  one distinct TFC content blob on this machine.
- The same TFC serves other textures fine (`AR_shotgun_s` mip1,
  `ar_sniper_rifle_d` mip1 both export cleanly), so the failure is
  texture-specific, not a globally unreadable file.
- `tools/uedecompress` cannot decompress ANY local package (fails identically
  on 32 KB music packages and the 11 MB weapon package): it parses
  `0x656E6F4E` ("None", the folder-name string) as chunk-table data, i.e. its
  assumed `FPackageFileSummary` layout does not match Raven v584/lic126.
  Direct bulk forensics is blocked on that tool, not on missing effort.

## Evidence

### E1 — TFC duplication forensics (VERIFIED)

`asset-tools/textures/tfc_forensics.py` samples N spread 1 MB windows per file
and compares SHA-256:

- local P0..P3: identical at all 12 sample offsets + identical head/tail
- sandbox `coalesced.tfc` (same byte size): identical at all 8 sample offsets
- conclusion: one distinct TFC blob exists locally; P1–P3 names carry no data
  of their own. Any texture whose cache is P1/P2/P3 is served P0's bytes.

### E2 — UModel export matrix (VERIFIED, method 1)

Package `RvGameMPLoader_SF` (byte-identical local vs sandbox, SHA `871c047b…`):

| texture | object | requested | result |
|---------|--------|-----------|--------|
| `AR_shotgun_s` | 1024 | mip1 512 | OK, exported |
| `ar_sniper_rifle_d` | 2048x1024 | mip1 1024x512 | OK, exported |
| `handgun_d` | 2048x2048 | mip1 1024 | FAIL `Size == 0` reading `Textures_P0.tfc` |
| `handgun_n` | 2048x2048 | mip1 1024 | FAIL `Size == 0`, same call chain |
| `handgun_d` via `wp_SP_ARPistol_SF` | — | — | not an export of that package at all |

Full error chain:
`appReadCompressedChunk <- FByteBulkData::SerializeDataChunk <-
FByteBulkData::SerializeData <- UTexture2D::LoadBulkTexture: File=Textures_P0.tfc
Mip=1 <- UTexture2D::GetTextureData: handgun_d`.

UModel deletes the partial `.dds` on failure; successful probe exports were
deleted from the worktree after the experiment.

### E3 — object declarations (VERIFIED, UModel `-dump`)

`handgun_d`: `SizeX = 2048, SizeY = 2048, Format = PF_DXT1`,
`TextureFileCacheName = Textures_P0`, `MipTailBaseIdx = 11` — all 11 mips
streamed, zero resident. The object promises data the local archives cannot
deliver.

### E4 — decompressor incompatibility (VERIFIED, method 2 attempted)

`tools/uedecompress` on 3 packages (32 KB, 32 KB, 11 MB): identical failure
`chunk 0 runs past end of file`; its table parse yields the same impossible
sums (`sum(compressed) 1701736270`) regardless of input. Header dump shows it
reads the folder-name `FString "None"` (`0x656E6F4E`) as chunk-table data —
its summary layout predates Raven's v584/lic126 changes. UModel reads the same
files correctly, so the files are intact; the tool is not.

A branch-local `asset-tools/textures/mip_forensics.py` parses decompressed
package headers/tables (validated layout shared with `tools/uepkg`), but it
cannot run without decompressed input, which is exactly what is blocked.

## Interpretation (clearly labelled)

- HYPOTHESIS (not proven): the single local TFC blob is a partial/stale
  snapshot (streaming cache or mismatched build), not the shipping full TFC.
  Consistent with: some offsets valid (shotgun, sniper), handgun offsets dead.
  NOT proven: proving it needs the shipping TFC, which is not available offline.
- NOT claimed: that mip0 ever existed in any local file; that UModel is at
  fault (it reads neighbouring textures from the same file correctly); that
  upscaling the 1024px captures would constitute mip0 (it would not — policy
  stands).

## Recovered in this task

Nothing new at mip0. Final status per target:

| target | result |
|--------|--------|
| `handgun_d` / `handgun_n` (pistol, 2048px objects) | NOT recovered. mip1 reads fail (`Size == 0`); mip0 never reachable. |
| `ar_sniper_rifle_d`, `elite_beam_d` (2048px objects) | NOT recovered. mip1 reads succeed; mip0 not delivered by the archives. |
| `TMD` diffuses | NOT recovered. No local export at any mip; `TMD_new_n` mip0 re-verified readable (already in batch 3). |
| `door_industrial_04_S1/S2` d+n, `door_locked_al_N` d+n, `door_prison_gate_N` d+n+s | NOT recovered. All 9 fail at mip1 reads (`Size == 0` or `H.Tag == PACKAGE_FILE_TAG`); mip0 never attempted. |
| `revert_d2`, `elite_beam_n`, `ar_sniper_rifle_n`, `TMD_new_n`, `AR_E99Rifle_alt_d` | Re-verified readable at mip0 in the current staging (no new files; confirms the failure is selective, not global). |

No new remaster batch was generated: there is no new genuine mip0 source to
build one from. Batch 1–3 local outputs and manifests are preserved untouched.

Future-candidate note (not recovered, not processed): `t_decal_metal_d_v4`
(128px) and `bsplat3_d` (256px) are true-mip0 local already; they belong to a
future decal/FX batch, not to source recovery.

## Runtime capture proposal (for MAIN, later date — NOT executed)

The injector already ships a capture mode (`texture_override.ini`: `capture=1`,
`capture_min`, `capturesdir`, `marker_hash` ring isolation); no new tooling is
proposed. Minimal MAIN-owned session, only after the shadow work lands:

1. Deploy the injector on the Steam install in capture-only mode
   (`mode_apply=0`, `capture=1`, `capture_min=512`); record `d3d9.dll` SHA-256
   before launch per the deployment preflight.
2. Load a save with the pistol visible (mip0 streams at close inspection),
   then the sniper / railgun / TMD / door locations. Use `marker_hash` with
   the laser texture hash to isolate pistol-adjacent binds if the dump is
   noisy.
3. Collect `capture/*.dds`; for each, verify decoded dims equal the object
   mip0 size and the FNV-1a64 differs from all known lower mips. Anything
   short of object dims is logged as a lower mip, never promoted.
4. Roll back via `restore.ps1` + timestamped backup verify; transfer captures
   to `SingularityVR-assets-local` for the next offline batch.

Acceptance for handgun: 2048x2048 decoded `handgun_d` + `handgun_n` with fresh
FNV hashes. Until then the pistol stays explicitly deferred.

## Files added on this branch

- `asset-tools/textures/tfc_forensics.py` + `tests/test_tfc_forensics.py`
- `asset-tools/textures/mip_forensics.py` (parser; blocked input documented)
- `docs/SOURCE_RECOVERY_FINDINGS.md` (this file)

# Evidence summary for knife-fix agent — Steam `4B34CE06`

**Archive:** `docs/evidence/steam-4b34ce06-20261023/`  
**Evidence commit:** (see git after push)  
**Do not re-run this session; logs are archival only.**

## Identity

| Field | Value |
|-------|--------|
| DLL SHA-256 | `4B34CE06F93DDB2387D75C5A4152BCD6AADD900ED3DBA9E6459F86A677BE496C` |
| Git HEAD | `05a94ab645d29e5da84ba3128a5ccd013af40b95` on `fix/sol-tmd-raised-weapon-knife` |
| Code | `9593169` (TMD right `BuildGunC` grip), `0cf6c17` (knife route ownership), `6e15e77` (Impulse/ADS/R2 witnesses) |
| Log SHA-256 | `592023235301C7A57EAFBC1113CAF13C12FD136C31E89E311A8875D866CB7F26` (828 991 bytes) |
| Session window | Log attach **2026-10-23 21:18:17** → last gameplay lines **~21:19:19** (pause/menu exit) |

Correlation: log opens **one minute** after `d3d9.build.json` `builtAt` 21:17:16; runtime uses **`Impulse=`** / **`tmdCuff=`** receipts introduced in `6e15e77`/`0cf6c17`. Installed Steam DLL at capture time still **`4B34CE06…`**.

## Effective INI

Full file: `SingularityVR.ini.effective` (SHA `F5FD4613…`). Candidate deltas vs accepted `514bb03`:

- `[Input]` `DualWeaponTmdReady=1`, `DualWeaponTmdGunScale=1`, `DualWeaponTmdRightWrist=1`, `VolkAuthoredGrip=1`, `HipKnifeIgnoreTMD=0`
- `[Render]` `TmdShortGauntlet=1`, `TmdSupportGauntlet=1`
- Volk reload: `VolkManualReload=1`, `ReloadNativeMagazine=1`, `ReloadPalmGrip=1`

No repository INI template; this is the **actual Steam file** from the test machine after the session.

## Knife rendering — runtime (confirmed from log)

1. **Single route per pickup (fix target for 3929868):** After `hip knife: PICKED` (L6243), only **`reserve TMD cuff joint`** + **`native joint … tmdCuff=1`** in **both eyes** (frames 3495–3496). **`standalone overlay suppressed`** on reserve lines.
2. **No `held standalone route`** line anywhere in this log (contrast archived `3929868` log line 5390).
3. **Sheath:** L6332 `hip knife: SHEATHED` — no second pickup/reacquire captured in this short session.

## Weapon / TMD diagnostics (same session, for cross-check)

- **`single-hand weapon grip: … BuildGunC … gunVerts=8297`** (Volk) — L4391
- **`gun-scale: ready reflected`** + **`scale=0 -> 1`** with **`Impulse=1`** on witness lines — e.g. L4695–4728
- **`tmd-right-wrist final`** with **`Impulse=1 ADS=0`** stereo pairs — e.g. L4696–4730
- **`tmd-support-gauntlet`** receipt L4415 `support=0 eyes=2 tris=7110 indexRestore=OK`

## Physical grades

**Not recorded in repository for this session.** This archive is **log + INI + build provenance only**. User-visible PASS/FAIL was not submitted with this retrieval request.

## Instructions to knife-fix agent

- Treat **`0cf6c17` route ownership** as **runtime-proven** for this pickup (native TMD cuff, no standalone duplicate in log).
- Use **`knife-events.txt`** and full **`view_matrix.log`** for frame-aligned analysis; do not synthesize missing reacquire or second-pickup behavior.
- Compare against **`3929868`** evidence (`55BCAA84…`) where **`held standalone route`** + **`native joint`** coexisted.
- If investigating remaining visual issues, note session did **not** log a second knife pickup after sheath.

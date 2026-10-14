# SIDE audio investigation — 2026-10-14 (offline, L3 file identity)

Branch `fix/recover-accepted-opening`, HEAD at investigation time `d3afeb6`.
No Steam writes, no game launch, no restore guards armed by SIDE.

## Executive summary

| Question | Answer | Evidence level |
|----------|--------|----------------|
| Why did Polish NPC/radio work at PID37660 then fail? | Test lease **38 asset delta v4** (Polish VI2 banks + font + manifests) was installed; **post-exit guards restored stock**; a later step persisted **only 3 runtime files + 992 xlingual WAVs**, not the 38-pack. | L3 logs + manifests |
| Why did 992 xlingual WAVs not fix NPC/radio? | NPC/radio uses **native `SoundNodeWave` Vorbis inside `SP_VI2_*_LOC_INT.xxx`**, not `vr-assets` PlaySound. `PolishMaybeSpeak` runs only on **Localize text** (`chars >= 40`). PID35568 / manual retest: **zero** `polish voice:` lines. | L3 `session.log` + code |
| Why did notes fail when dubbing failed? | Same 38-pack: **`polish_text.jsonl` + `ui_GFx_fonts.xxx` (60E72871)**. Stock font/manifests after guard restore → English / missing glyphs. | L1 manifest + L4 history PID37660 |
| What is on Steam **now** (SIDE read-only verify)? | **38/38 assets**, **992/992 story WAVs**, **3/3 accepted runtime** (EA9EFFB1 / F5137234 / INI 757F414B). **Intro bank intentionally stock** (890C6172… not 4904F281…). | L3 `steam-verify-latest.json` |
| Is new xlingual audible in native banks yet? | **No.** `native_replaced=87` suppresses WAV overlay when `NativePolishSpeech=1`; **bank PCM must be rebuilt** via `native_story_package.build` / `prepare_native_story.py`. | L1 manifest math |
| Next user test without new work? | Should hear **previous integrated Polish in banks** (FB03741D… era), **Polish notes if font+manifest present**, **not** refreshed xlingual acting in native lines until bank integration. | L3 install + L4 pending |

## Timeline (mechanism, not narrative)

1. **PID37660** — exact4+38+1886; user L4: Polish first-area NPC/recordings + note glyphs (`docs/first-area-accepted-and-ui-witness.md`).
2. **Exit** — three guards restore **1927** files to baselines (`first-area-accepted-and-ui-witness.md` L11–16).
3. **PID35568** — promoted EA9EFFB1 triple + xlingual 992; **banks/font/manifests stock** → English NPC/radio; note unchecked.
4. **~21:29 MAIN** — `test-prepared-assets.ps1` reinstall **38 v4** (`asset-backups/20261014-212951-40600`).
5. **SIDE verify ~21:36** — Steam matches **38/38 asset candidates** + runtime + 992 WAVs (`tools/audio/verify-polish-install-state.ps1`).

## Log proof (broken text path)

`deployment/hands-dubbing/recovery-live-20261014-2035/session.log`:

- `ini: NativePolishSpeech=1`
- `polish localize saw:` — menu/UI only
- **No** `polish voice:`

Same pattern in `manual-test-20261014-2111/session.log` (~25 s gameplay): menu strings only, **no** `polish voice:`.

## Manifest math (v4 payload)

Source: `deployment/hands-dubbing/recovery-ready/assets/payload/.../polish_story_voice.jsonl`

| Metric | Value |
|--------|------:|
| Story rows | 997 |
| `native_replaced=1` | 87 |
| Join to xlingual `(package, asset)` | 82 / 87 |
| Missing xlingual keys (5) | duplicate/help barks across banks — retain native audio until mapped |
| `text_en` length &lt; 40 (all rows) | 468 |
| Kismet rows &lt; 40 chars | 22 (native_replaced on most radio/NPC barks) |
| Xlingual manifest rows | 992 |
| Filename intersection story ↔ xlingual | 992 / 992 |

**Implication:** even with perfect 38-pack, **short native-owned barks never use `PolishMaybeSpeak`**; they **must** come from bank Vorbis.

## Installed identity (Steam, read-only)

Report: `deployment/hands-dubbing/current-install-freeze-20261014/steam-verify-latest.json`

- **Assets:** 38/38 match `recovery-ready/assets/asset-manifest.json` **CandidateSha256** (includes font **60E72871**, manifests **6518581E / 0D632BF4 / 22B755EB**, banks **FB03741D…** family).
- **Runtime:** d3d9 / dinput8 / INI — accepted recovery triple.
- **Story WAVs:** 992 files under `Binaries\vr-assets\audio\polish\story`.
- **Core manifest mismatch (expected):** `recovery-ready/core/test-manifest.json` includes **intro candidate** `4904F281…`; Steam intro remains **stock** `890C6172…` (protected). **Do not** treat 4/4 core manifest as the live baseline; use **3-file runtime + optional intro policy**.

## First-building producer map (offline keys)

Example rows in v4 story manifest (all `native_replaced=1` → bank-owned when `NativePolishSpeech=1`):

| Asset | Package | English (truncated) |
|-------|---------|---------------------|
| VI1_0113_CP2 | SP_VI2_kismet01_LOC_INT | Spartan team do you read? |
| VI2_5020_DEMY | SP_VI2_kismet02_LOC_INT | My leg is hurt… |
| VI2_5020_DEMY | SP_VI2_kismet04_LOC_INT | (duplicate cue in other bank) |

Xlingual WAVs exist (`feaf10f92a120574.wav`, `d358719827e8b8e6.wav`, …) but **only affect playback after bank integration**.

## Bank decode status (RE blocker for SIDE)

Attempted read-only decode of installed `SP_VI2_kismet01_LOC_INT.xxx` (FB03741D…):

- `tools/uedecompress/uedecompress.exe` — **ERROR** (non-stock chunk layout; repacked via `native_story_package.pack`).
- `tools/uepkg/uepkg.exe` — name table parse fails on repacked physical.

**Next RE step (offline):** use the same **decoder + logical directory** pipeline as `prepare_native_story.py` (pinned in repo), or add a read-only inspector for **physical** packages produced by `native_story_package.build`. Do not guess SpokenText from filenames alone.

## Authorized test baseline (do **not** use stale runners)

| Use | Path / action |
|-----|----------------|
| **Asset lease** | `deployment/hands-dubbing/recovery-ready/assets/asset-manifest.json` |
| **Runtime lease** | EA9EFFB1 + F5137234 + INI 757F414B (see `current-install-freeze-20261014/freeze.json`) |
| **Story WAV lease** | `deployment/hands-dubbing/story-xlingual-installed-20261014/story-install.json` |
| **Verify before/after** | `tools/audio/verify-polish-install-state.ps1` |
| **Forbidden without new manifests** | `tools/run-recovery-trial.ps1` (pins `recovery-ready` + **quiet-revoice-v2** `68569A31…`) |

Guard restore target after a **future** test must be **this** 38+992+3 state, **not** stock baselines in `recovery-ready` core/asset **BaselineSha256** fields (those are rollback-to-English for lease tooling).

## Fix plan (MAIN, ordered)

1. **Keep** current 38+992+3 on disk; re-run verifier after any deploy.
2. **Headset acceptance (scoped):** first-building save — NPC/radio + **open note** (font+manifest now present). Distinguish **old bank Polish** vs **new xlingual** by ear only where lines differ.
3. **Bank integration (offline first):** for 82 joined xlingual rows (+ resolve 5 key gaps), run `prepare_native_story.py` / `native_voice_fit.py` with **new** WAV hashes; produce new candidate `.xxx`; extend asset manifest v5; fake-install test only.
4. **Never** run broad `install_polish.ps1` (sets `PolishLanguage=0`, overwrites UI/config).
5. **Intro** stays stock unless user explicitly scopes intro candidate `4904F281…`.

## Evidence gates

- **This document:** L3 file identity on real Steam install + L1 manifest/code + historical L4 PID37660.
- **Not claimed:** new xlingual quality in native lines, full note QA, intro/helicopter — pending L4 after bank work or scoped retest.

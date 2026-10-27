# Polish UI lektor QA — narrator restore — 2026-10-14

Branch: `agent/narrator-lektor-qa-20261014` (from `602a5c9` on `feature/polish-dubbing-full-qa`).

Frozen comparison baseline (unchanged):

`audio/polish/repair_candidate_20261013/full_revoice_final_pass/assembled_reviewed/`

## 1. Which “previous lektor” was found

| Role | Evidence | Engine / profile |
|---|---|---|
| **Previous menu/loading lektor (user-preferred era)** | `audio/polish/full_voicepack/` — all `narrator_user` rows; manifest profile `narrator_user`; conditioning **`audio/polish/user_reference/spokoj_reference.wav`** (documented in `docs/POLISH_POCKET_PROFILE_RESTORATION.md`) | Pocket-TTS Polish 6L, UI temperature **0.45** |
| **Current staged lektor (reported “łamliwy”)** | Same line IDs in `assembled_reviewed/voicepack/manifest.jsonl` with `reference_file` → **`intro_narrator_reference.wav`** (10 s excerpt 8–18 s from approved intro) | Pocket-TTS Polish 6L @ **0.45** |
| **Intro narration (user-approved, separate role)** | `C:\Users\Trilex\Music\SNG_intro_polish_voice.wav` | Human recording — **not** replaced |

The intro excerpt and `spokoj_reference` are **different SHA-256 sources** (`aa57a0fe…` vs `90ab214406…`). Replacing UI conditioning with the intro excerpt changed **all 890** UI/system narrator lines versus `full_voicepack`, even when Polish text was unchanged.

## 2. Restore decision (reversible)

**Applied:** UI/system narrator (`pack == full_localization` + `profile_before == narrator_user`, **890 lines**) now uses the **spokoj / full_voicepack lektor identity** again.

| Action | Count |
|---|---:|
| Restored prior WAV from `full_voicepack` (unchanged spoken text, valid PCM) | **643** |
| Regenerated with **spokoj_reference** @ 0.45 (corrected text and/or legacy non-PCM MPEG-in-.wav sources) | **247** |
| Story / character lines | **996** unchanged from intro-based `assembled_reviewed` |

Reversible manifest:  
`audio/polish/repair_candidate_20261013/narrator_ui_lektor_20261014/narrator_ui_restore_decisions.jsonl`

New full staging (1,886 WAV, 24 kHz mono PCM16 validated):

`audio/polish/repair_candidate_20261013/narrator_ui_lektor_20261014/voicepack/`

**Not installed** to Steam or game packages.

## 3. A/B listening — pilot reel

Path: `audio/polish/repair_candidate_20261013/narrator_ui_lektor_20261014/narrator_lektor_ab_pilot.wav`  
Index: `narrator_lektor_ab_pilot.json`

For each of **12** representative UI strings (Loading, Saving, console hints, achievements, MP menu, etc.):

| Label | Meaning |
|---|---|
| **A** | Previous lektor — `full_voicepack` (spokoj era) |
| **B** | Current intro-conditioned `assembled_reviewed` |
| **C** | This restore candidate (`narrator_ui_lektor_20261014/voicepack`) |
| **D** | Pilot **assertive** spokoj @ **0.55** (`pilot_assertive_spokoj_055/`) |

**Automated A/B preference:** not performed (no perceptual model).  
**Working hypothesis for your listen:** restore **C** should track **A** on unchanged lines; **D** is an optional slightly firmer spokoj variant if **C** still feels soft.

Per your earlier feedback (“previous lektor better”), **C is the selected candidate** pending confirmation on the pilot reel.

## 4. Licensed PL/RU/UK references

See `docs/POLISH_VOICE_REFERENCES_LICENSED.md` (Piper per-voice cards, Pocket CC-BY-4.0, user references, Common Voice CC0 caveats).

## 5. Audit / repair counts (this pass)

| Metric | Value |
|---|---:|
| Full corpus size | 1,886 |
| UI lektor lines touched | 890 |
| Lines regenerated (spokoj) | 247 |
| Lines file-restored (full_voicepack) | 643 |
| Unchanged intro-candidate story/character rows | 996 |
| Prior final-pass staging | **read-only** (unchanged) |

## 6. Technical QA vs perceptual QA

### Technical (this candidate voicepack)

- **1,886 / 1,886** root WAVs pass `wave` PCM check (24 kHz mono PCM16).
- **76** legacy `full_voicepack` UI files were MPEG bytes with `.wav` extension — **not** linked; those IDs were moved to spokoj regeneration instead.

### ASR (regenerated spokoj subset only, base Whisper)

Scope: `regenerated_spokoj/` (**247** files)

| Status | Count |
|---|---:|
| PASS_AUTOMATED_SCREEN | 54 |
| REVIEW_OR_REPAIR | 193 |

ASR flags are review prompts only (short menu strings, names, acronyms). They do **not** prove naturalness or lektor quality.

### Perceptual QA

| Item | Status |
|---|---|
| Pilot A/B reel | **Ready for human listen** |
| Full 890 UI lektor lines | **Not odsłuchane by agent** — spot-check via pilot + priority menu lines recommended |
| Full 1,886 dubbing approval | **Not claimed** — baseline `assembled_reviewed` listening backlog still applies to story/character rows |

## 7. Reproduction

```powershell
cd C:\Users\Trilex\vr-mods\SingularityVR-cutscene
python tools/audio/narrator_ui_lektor_restore.py `
  --output-dir audio/polish/repair_candidate_20261013/narrator_ui_lektor_20261014
```

Use `--skip-regen` after `regenerated_spokoj/` is complete to rebuild voicepack + A/B only.

## 8. Status summary

**Ready for human listening:** pilot reel + optional spot-check of Loading / main menu / MP lobby in `voicepack/`.  
**Requires further work if listen fails:** switch selected UI lines to pilot **D** (0.55 spokoj) or hybrid (intro for long story narrator, spokoj for short UI only — already the policy encoded in this staging).

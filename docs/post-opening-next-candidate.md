# Next post-opening candidate — controlled preparation

No Steam launch/deployment while preparing. The last installation was independently
restored (1924 exact original hashes). Runtime acceptance remains scoped to the
user's report in post-opening-feedback-20261014.md.

## Hypotheses and falsifiers

- Free right glove: the inherited pistol pose change snapshots the uncorrected
  aim target before wrist reconstruction. Select the corrected primary target
  at consumption time for non-pistol draws, retaining BuildGunC for the accepted
  pistol grip. Falsifier: the correctly selected target still produces an offset
  in first-area no-weapon draws. Cover both target identities and actual ride
  cancellation offline before the headset trial.
- Save dialog safety: accepted suppression policy is retained, but the inherited
  input-thread hook releases render-owned COM resources. Publish an atomic
  invalidation request; consume it only on the existing render/Present path.
- Cinema: the current projection-layer pose changes every frame, so the fitted
  picture is not a stationary screen. Default-OFF CutsceneWorldScreen submits
  the proved composed image as a core OpenXR quad in LOCAL space, anchored once
  on scene entry. Keep the anchor through pause, reset on scene/session/reference
  space exit, preserve source aspect and native camera. Falsifier: anchor pose
  changes on head motion, transition leaks the quad into gameplay, or L4 shows
  motion/doubling. Math/lifecycle tests first; no new capture route or engine hook.
- Logo and pointer: use the actual native GFx movies and field/glyph producers.
  An offline initial-text edit is not proof that runtime text or font lookup uses
  it. Investigate native ActionScript before patching, and preserve exact input
  hit tests. No global replacement of the word Singularity.
- Voices: SIDE audits remaining first-area dialogue import/cue/wave ownership and
  reuses exact existing translated WAVs. MAIN encodes, round-trips and prepares
  bounded native bank leases. No full-game dubbing claim from a first-area patch.
- Polish notes: the existing CP1250 round-trip cannot add missing font glyphs.
  Preserve Unicode; consume independently verified SIDE glyph evidence rather
  than repeating the conversion guess.

OpenXR quad contract: https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrCompositionLayerQuad.html
The pose is evaluated in its named space, size is metres, and front face is +Z.

## Prepared evidence and remaining physical gates

- Python suite: 26 PASS. Sandbox tempfile root explicitly selected under owned
  deployment; default sandbox Temp failed permissions, not font structure.
- Nine optimized x86 fixtures and accepted runtime/source policy: PASS.
- Hand native replay: 29.859–47.245 UU original right residual; corrected
  reconstruction below 0.00003 UU. Controller-to-glove pixels remain unverified.
- UI candidate `5E46B6199BED0DE7693A1C3E66C78838011F9E83A60A4E7BA992371BBD736CAF`:
  control/candidate native LZO round trips; iis/SPMenus exclusive native Mouse
  leaf and exact post-Initialize title conditional. Other headings unchanged.
- Audio SIDE e285652f: 16 WAV/23 native spoken exports, three first-area banks;
  nine nonverbal exports retained. Three ASR REVIEW entries explicitly pending.
  Native bank PCM/timing/loudness/platform/cue/property and LZO checks PASS;
  playback producers still need headset confirmation, not full-game dubbing.
- Font SIDE219c6fa (MAIN cherry59da9f9): original3F7BF0BE → candidate60E72871,
  sixteen outlined slots only; original ASCII, Ó/ó, other faces, kerning intact.
  The supplied raster preview uses bounding-box hole classification, so overlapping
  contours (especially Ł/ł) may look cut out there. It is NOT a native GFx raster
  proof. Explicitly inspect strokes/acutes/ogoneks/dots and wrap in PSVR2; reject
  and restore the font bank if wrong, without expanding to other fonts/assets.
- Asset manifest next-assets-v4: exact38 independent targets. Fake WhatIf,
  install/hash/backup corruption refusal/conflict preservation/restore PASS.
- Current Steam files are not these candidates. No launch before fresh start.

Headset script after MAIN verifies all leases: New Game/save confirmation →
intro/helicopter (look away from screen then back; pause/Options/return) →
free right glove near face before pickup → pistol grip → first-area dialogue,
recording and note → main menu/title/native ring pointer → close normally.
No chat/tab switching during the trial. Stop early for discomfort or a crash.
Three independent guards restore core/assets/WAVs after exit or no-launch timeout;
MAIN independently checks exact originals afterward. Do not infer visual fixes
from offline tests, successful xrEndFrame, or package load alone.

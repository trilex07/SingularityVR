# SIDE audio: correct ten timing-safe takes with incomplete meaning

Continue `side/final-audio-20261026` from `c5b56ad1`. MAIN independently
recomputed WAV hashes, PCM format, guarded duration and tempo for all 551 takes:
technical PASS. This is not listening or translation approval.

MAIN blocked the ten exact package/asset identities below from native-bank
integration. Do not regenerate the other 541 or alter the previously packed 707.
The five MAIN codec recoveries are already included in 707; do not add them again.

Machine-readable filenames, original hashes, full English lines and timing
limits: `docs/handoffs/final-dubbing-551-main-review-20261026.json` on
`integration/final-completion-20261026`.

## Required corrections

1. `RT4_1002_KAT` in both `SP_RT4_kismet01_LOC_INT` and
   `SP_RT5_kismet01_LOC_INT`: "Let's follow the track and... shit!!" became only
   "Cholera." Restore following the tracks. Suggested Polish:
   **"Idźmy torami... cholera."** Native budget 1.802833333 s;
   maximum guarded source duration 2.406825 s.
2. `VI4_3705_GST` in all five packages:
   `SP_VI4_kismet01_a_LOC_INT`, `SP_VI4_kismet01_b_LOC_INT`,
   `SP_VI4_kismet01_LOC_INT`, `SP_VI5_kismet01_LOC_INT`,
   `SP_VI5_kismet03_LOC_INT`. "Renko, flank right; I'll go left" became
   "Z prawej." Restore the player's right flank AND the friendly left flank.
   Suggested: **"Obejdź z prawej, ja z lewej."** Native budget 2.013833333 s;
   maximum guarded source duration 2.691675 s.
3. `VI4_4405_GST`, `SP_VI5_kismet03_LOC_INT`: the locked-door objective and
   promise to cover became only "Cholera." Suggested:
   **"Zamknięte. Znajdź sposób, ja osłonię."** Keep the door-opening objective
   unambiguous in the final wording. Native budget 3.6174375 s;
   maximum guarded source duration 4.856540625 s.
4. `VI4_4410_GST`, `SP_VI5_kismet03_LOC_INT`: "Go on, Renko. Find a way to
   unlock the door. I'll hold up here" became **"Stój."**, reversing the
   instruction. Suggested: **"Idź otworzyć drzwi. Ja osłonię."**
   Native budget 3.112125 s; maximum guarded source duration 4.17436875 s.
5. `VI6_1010_KAT`, `SP_VI6_kismet02_LOC_INT`: "Good, now down the stairs and
   turn right" became only "W prawo." Restore the stairs and sequence.
   Suggested: **"Teraz schodami w dół i w prawo."** Native budget
   2.581208333 s; maximum guarded source duration 3.45763125 s.

Suggestions may be improved idiomatically, but must preserve actions, directions,
negation and objectives. If natural speech cannot fit, report that exact identity
as BLOCKED rather than dropping essential information or clipping speech.

## Evidence and delivery

- Keep each identity's established speaker/profile; do not mix speakers.
- Actual synthesis code in `c5b56ad1` loads
  `hf://shefowl/pocket-tts-polish-6l/config.yaml`. Name the engine actually used;
  do not report XTTS for Pocket-TTS output. If changing engines, record that change.
- New candidate directory, same manifest filenames, mono PCM16 24 kHz,
  non-silent and unclipped, peak within MAIN's current limit (29204 PCM units).
- Use MAIN `native_voice_fit.trim_guarded` without trimming internal pauses,
  fricatives or final phonemes. Recompute
  `max(1, guarded_duration / (native_duration - .020)) <= 1.35`.
  Timing PASS alone is insufficient: listen to all ten corrected takes and check
  the full intended sentence. State who listened and what remains unapproved.
- Return a ten-record delta manifest: package, asset, original runtime filename,
  old/new SHA256, speaker/profile, engine, English, actual spoken Polish,
  measured duration/tempo/peak and listening status. Preserve exact identity even
  when the same asset occurs in several banks.
- Commit/push tools and the manifest on your own branch; WAVs stay private/ignored.
  No Steam access, bank deployment, UI, gameplay or renderer changes.

MAIN will independently recheck the corrected PCM and rebuild only these bank
exports. No approval of the remaining 541 performances is implied by technical
integration. The separate V3 startup crash remains MAIN's blocker.

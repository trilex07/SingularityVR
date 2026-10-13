# Polish dubbing repair candidate — 2026-10-13

Offline repair and QA checkpoint for the current accessible story and UI voicepacks. This is a reversible candidate overlay: original voicepacks and the Steam installation were not changed.

## Results

- Inventory covers 1,886 recordings: 992 story lines and 894 UI/localization lines.
- 205 recordings with hard failures in the prior QA pass were regenerated: 56 story and 149 UI.
- 79 line occurrences received text corrections based on the English source. The correction data is local-only and excluded from Git.
- Automated screen: 194 passed; 11 remain for targeted listening. All 205 passed format/waveform checks: 24 kHz, mono, PCM16, with no clipping or edge-silence flags.
- No original audio was overwritten. Candidate WAVs and manifests stay in the local staging directory and are excluded from this commit.

## Reproduce locally

Run from the repository root with the configured local Piper, Whisper and SciPy environments:

```powershell
python tools/audio/repair_full_corpus_failed_lines_20261013.py
python tools/audio/qa_repair_candidates.py
python tools/audio/finalize_repair_candidate.py
```

The correction map must be present locally at `audio/polish/repair_candidate_20261013/corrections.local.json`; it is intentionally ignored by Git. The scripts write candidates and the full line inventory under:
`audio/polish/repair_candidate_20261013/`

`full_audit/manifest.jsonl` maps each replacement to its prior file and source line. `qa.json` contains ASR and signal measurements. The short flagged-only reel is `targeted_listening_set_20261013.wav` with its index JSON.

## Quality limits

The replacement engine is local Piper Polish medium (Darkman profile only for the matching Demichev profile); other repairs use the available MC Speech voice. This does not preserve the prior Pocket-TTS/user-reference identities, so the regenerated files are not approved for final integration. Piper's available API has no random-seed control; a rerun can produce different audio, so keep the exact staged WAVs as the reviewed candidate. Source emotion metadata is absent. Whisper and signal metrics cannot establish acting quality, character identity, emotional performance or correct stress. The 11 flagged clips and the wider candidate set still require perceptual listening; the clean automated screen is not a dubbing-quality pass.

Do not merge the candidate WAVs into the voicepacks or install them into Steam before that review. Integrate only by applying the manifest to a backed-up copy, then verify scene/key coverage and retain a rollback copy.

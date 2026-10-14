# XLINGUAL DEPLOY — info dla main agenta (2026-10-14)

## Co wdrożyć
Pełny polski dubbing fabuły: **992 pliki WAV, PCM16 mono 24 kHz, peak −1 dBFS** (format gry),
głosy oryginalnych aktorów EN mówią po polsku (Pocket-TTS Polish 6L, dawca = własny oryginał kwestii).

Źródło (lokalne, NIE w gicie — patrz .gitignore):
`C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual\*.wav`

Shortcut: `C:\PL-dubbing`

## Jak wdrożyć (jedna zmiana w installerze)
W `SingularityVR-production/tools/polish/install_polish.ps1` linia 39:
```powershell
# BYŁO:
$SourceAudioStory = Join-Path $SourcePath "..\SingularityVR-cutscene\audio\polish\story_voicepack_nllb"
# MA BYĆ:
$SourceAudioStory = Join-Path $SourcePath "..\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual"
```
Nazwy plików IDENTYCZNE jak w `story_voicepack_nllb` (drop-in) — mapowanie
`polish_story_voice.jsonl` (package|asset → file) działa bez zmian.
Cel: `Binaries/vr-assets/audio/polish/story/` (jak dotąd).

## Czego NIE ruszać
- Intro (`SNG_intro_polish_voice.wav`) — zostaje.
- Menu/UI (`full_voicepack/`, 894) — głos zostaje.
- Multiplayer (linie `MP.*`) — głos zostaje.

## Status QA
- Batch 992/992 + rundy napraw R1–R6 (logi: `REPAIR_LOG.json`, `REPAIR2_LOG.json`, `REPAIR3_LOG.json`).
- 13 tekstów NLLB wyczyszczonych ręcznie (`CLEAN13.json`) — bełkot typu „Titanem/dupku/Pobieg osobisty" poprawiony.
- ASR-weryfikacja (whisper-small): batch w większości czysty; pliki po naprawach sprawdzone pojedynczo.
- Dawca `5339b0068ab0840e` na CZARNEJ LIŚCIE (halucynacje niezależnie od temperatury).
- Reszty do ucha reżysera: `6ba243d8f3f9a7f9.wav` (akcept), `db5a9bb47d436a5a.wav` (OK),
  `825b608f491f738b.wav` (do potwierdzenia środka kwestii).
- Znane ryzyka: wymowa nazw własnych (Barisov/Katorga/Demichev — respelling tam gdzie trzeba),
  Edge-TTS fallback MARTWY (403, Microsoft odciął endpoint).
- Backupy: `_backup_before_split/`, `_backup_r2/`, `_backup_r3/`, `_backup_r4/`, `_backup_r5/`, `_backup_r6/`.
- Manifest generacji: `manifest_xlingual.jsonl` (tekst, dawca, czas generacji per plik).

## Odtworzenie packu od zera (gdyby coś przepadło)
1. `python batch_xlingual.py` (~21 min CPU) — wymaga HF cache `shefowl/pocket-tts-polish-6l` + `.pocket_tools`.
2. `python repair2_xlingual.py` / `repair3_xlingual.py` / `repair4/5/6` — tylko flagged (patrz `QA_FLAGS.json`).
3. `python louden_xlingual.py` — PCM16 + peak −1 dBFS.
4. ASR: `python asr_qa_batch.py` (wymaga `_asr_queue.txt`).

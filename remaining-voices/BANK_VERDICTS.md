# Punkt 1 — werdykty 3 upadłych banków (dowody, nie zgadywanie)

Metoda: `unpack_chunks.exe` (zbudowany lokalnie ze źródeł worktree) + `ue3_sound_package.inspect`
na dekodowanych `.u`. Pliki gry tylko do odczytu. Raporty MAIN:
`SingularityVR-recovery/deployment/hands-dubbing/xlingual-banks/{out,full/out}/xlingual-banks.json`.

## Bank A: SP_RT1_fx01_LOC_INT — status: FAILED (no eligible voices)
**Werdykt: SŁUSZNY. Zostaje EN. Nic do generowania.**
Dowody (SpokenText z pakietu):
- `RT2_1003_REN` — "Aaaah!" — 44100 Hz mono, 0.85 s
- `RT2_1004_REN` — "Unf!" — 44100 Hz mono, 0.66 s
Oba to niewerbalne wysiłki gracza (Renko). Dźwięki wysiłkowe nie mają wersji językowej.
(Uwaga: raport mówi „1 linia", w pakiecie są 2 assety — oba niewerbalne, wniosek ten sam.)

## Bank B: SP_RT3_Vignette_LOC_INT — status: FAILED (unsupported PCM metadata)
**Werdykt: CZĘŚCIOWO SŁUSZNY. 5/6 do wygenerowania, 1 z decyzją.**
Dowody:
- 5 assetów: mono 48000 Hz, werbalne kłótnie naukowców SC1/SC2 (2–10 s budżetów) — ELIGIBLE.
- `RT99_0105_SC1`: **stereo, 44100 Hz** (jedyny taki w banku) — to on wysadził bramkę
  `NumChannels==1` w pipeline MAIN. Kandydat: mono-downmix (dialog centralny, strata żadna).
**Dostarczone (ten branch, `remaining-voices/out/vignette/`):**
- 6 plików `RT99_*.pl.wav` (Pocket-TTS 6L, dawcy = własne oryginały EN; SC1 z downmixu mono).
- Teksty PL wyczyszczone ręcznie (NLLB miało: „straż kolejowa", „czekasz na dzień" — naprawione
  z pełnego EN, m.in. dopisane poucinane zdania).
- `manifest_rows.json` w schemacie banków (asset, sha256, english, polish, tempo, peak, budget,
  rate/ch, qa_note). Tempa 0.60–1.20 (limit 1.85 z zapasem).
- ASR-screen: 0106/0107/0111 PASS; 0105/0108/0112 REVIEW (treść mostly obecna, do ucha).
- Decyzja dla MAIN: przyjąć 6 linii albo odrzucić tylko SC1-stereo (wtedy 5/6).

## Bank C: SP_VI6_kismet03_LOC_INT — status: FAILED (no eligible voices)
**Werdykt: SŁUSZNY dla banku TTS. Zostaje EN jako bank; okrzyki werbalne idą do punktu 3.**
Dowody: 262/262 SoundNodeWave to okrzyki bojowe (rozkład: 156×1 słowo, 35×2, 39×3, max 7 słów,
np. „Keep me covered!", „Let's get out of here!"). Zero pełnych kwestii.
(Uwaga: raport mówi „1 linia za długa" — niepotwierdzone; w pakiecie nie ma żadnej długiej linii.)

## Środek wytopu (do TASK.json pipeline MAIN)
- `manifest_rows.json` (Vignette, 6 wierszy) — integrację robi MAIN.
- Pliki WAV: `remaining-voices/out/vignette/RT99_*.pl.wav` (float32 z generatora;
  przed integracją znormalizować do PCM16/−1 dBFS jak reszta packu).

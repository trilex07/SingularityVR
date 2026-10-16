# SIDE PROMPT — poprawki tłumaczeń + cichy dubbing

Wklej to agentowi Side. On zakłada własny worktree/branch
(`side/dubbing-corrections`, z `origin/main`) i nie dotyka Steam ani cudzych branchy.

---

Jesteś Side agentem (wording + audio QA). Zakazy: Steam, cudze worktreebranche,
 nadpisywanie zaakceptowanych tekstów/audio, regeneracja 1886 WAV-ów bez
uzasadnienia, merge bez zgody. L1/L2 tylko (CPU/build/testy); zero testów headset.

## Wejścia (czytaj, nie edytuj)

- `tools/localization/backlog.json` + `GAPS.md` (gałąź z installerem):
  2637 rekordów, statusy, `occurrences` dla powtórek.
- `localization/pl/polish_story_voice.jsonl` (1672), `remaining-voices/*-pl.json`,
  `audio/polish/story_voicepack_xlingual/` (QA_FLAGS, ASR_QA, manifest 992).
- `audio/polish/louden_xlingual.py` — istniejący normalizer (peak −1 dBFS),
  UWAGA: ma zahardkodowany OUTDIR innego worktree; nie uruchamiaj bez poprawki.

## Zadanie A — poprawki tłumaczeń

1. Bierz STALE `TRANSLATION_NEEDS_CORRECTION`, scope campaign (multiplayer pomijasz).
2. Zacznij od chapterów z największymi liczbami w GAPS.md (SP_VI5_KISMET03…).
3. Każdą korektę zapisuj jako NOWY rekord `{id, proposed_pl, note}` we własnym
   `corrections_<zakres>.jsonl` — NIGDY nie edytuj `backlog.json`/`GAPS.md`
   (generowane) ani istniejących `*_pl.json`.
4. Standard: naturalna polszczyzna zgodna z angielskim znaczeniem (nie dosłowność
   ASR); nazwy własne i terminologia TMD spójne z już zweryfikowanymi 42 wierszami
   `full_native_text_review`.
5. Raport: ile wierszy, ile pewnych/niepewnych (flaga `needs_human`),collisions
   terminologii.

## Zadanie B — dubbing za cicho (najpierw MIARA, potem gałka)

Fakty startowe (zweryfikowane): story WAV peak śr. −2,2 dBFS (NIE za cicho na peaku),
ale 1401/1667 plików to 24 kHz; skipped mean −6,4/min −15,4 dB; bark/mp/intro nie
mają danych peak w manifestach.

1. Zmierz NA KOPIACH lokalnych WAV (ffmpeg `volumedetect` + `ebur128`, tylko odczyt):
   peak/RMS/LUFS per pack (story/bark/mp/intro/vignette/skipped). Porównaj ze story
   i między packami. Sprawdź też rozkład sample_rate (24k vs 44.1k/48k).
2. Hipotezy do rozstrzygnięcia DOWODEM (osobno na pack): niski RMS mimo peaku,
   zły sample_rate vs bank, wybrany zły (cichszy) track, ciche konkretne pliki.
3. Fix TYLKO po pomiarze: parametryzowany skrypt gain (jak `louden`, ale bez
   hardcodów, sufit peak −1 dBFS, kontrola true-peak i clippingu), praca na kopiach,
   tabela `{plik, gain_db, peak_przed/po}` w MANIFEST. Bez regeneracji głosów.
4. Walidacja offline: ponowny pomiar + pełny dekod próbek. Zero twierdzeń
   o odsłuchu i o mikście z muzyką.

## Oddanie

Branch push, pliki: `corrections_*.jsonl`, tabela gain, `HANDOFF.md` (co zrobione,
liczby, co NIE działa, następny krok). Bez merge. PR otworzy człowiek (brak gh auth).

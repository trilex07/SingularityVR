# HANDOFF — side/dubbing-corrections (wording + audio QA, L1/L2)

Branch: `side/dubbing-corrections` z `origin/main` (fb73156). Bez merge (PR otwiera człowiek).
Zakazy dotrzymane: zero Steama, zero edycji cudzych branchy/worktree, zero edycji
plików generowanych (`backlog.json`, `GAPS.md`, `*_pl.json`, WAV-y), zero regeneracji
głosów, zero twierdzeń o odsłuchu/miksie. Wejścia z innych branchy tylko do odczytu
(`tools/localization/backlog.json` + `GAPS.md` z brancha installerowego,
WAV-y z worktree `SingularityVR-cutscene` i `SingularityVR-polish-remaining-voices`).

## Zadanie A — korekty tłumaczeń (campaign, największy chapter)

Plik: `dubbing-corrections/corrections_sp_vi5_kismet03.jsonl`
- **131 rekordów** `{id, proposed_pl, note}` + flaga `needs_human`.
- Wszystkie id zweryfikowane: status `TRANSLATION_NEEDS_CORRECTION`, scope `campaign`,
  chapter `SP_VI5_KISMET03` (największy w GAPS.md: 418; wzięto 228 story, poprawiono 131
  unikalnych id z tego chaptera; bark 187 i skipped 3 z tego chaptera poza zakresem wordingu story).
- Pewne: **123** (`needs_human: false`), niepewne: **8** (`needs_human: true`).
- Standard: 42 wiersze `full_native_text_review` z `localization/pl/polish_story_voice.jsonl`
  (glosariusz: Barisow/Demiczew, Katorga dwanaście, E dziewięćdziesiąt dziewięć,
  Osobliwość, kalutron/piec, tryb rozkazujący lm.).

Klasy błędów (zliczone z pliku):
- obcięcia pod budżet TTS (pominięte zdania/okrzyki): ~13
- inwersje sensu (głównie groźne): VI99_2205_SC1 (ludzie vs strażnicy), VI5_4013 (Say again),
  SD_SPT2_FIRE3 (osoba), SD_SPT1_MOVE_RETREAT3 / SD_SPT2_MOVE_RETREAT2 (fall back),
  SD_GST_SEE_TARGET3 (strona), SD_SPT2_FIRE1 + VI5_2011 (take out), VI4_4405 (while)
- kalki wojskowe (cover/kryjówka, pinned/uwięziony, fire/otwórz ogień, frosty/mroźny,
  dead ahead/martwy, take cover/ukryj się): ~40
- rodzaj/przypadek (tej karabiny, granata, wroga widziany): ~8
- liczba (rozkazy lp. do drużyny): ~15
- systematyczny artefakt wiodącego `"- "` w draftach: VI4_3005, SD_GST_MOVE_IN3,
  SD_GST_MOVE_OUT1, SD_GST_MOVE_OUT2, SD_ESP1/GRD1/SPT3_SRCH_FIND1, VI5_4018

Kolizje terminologii (do decyzji MAIN przy scaleniu):
- `On me!` → `Do mnie!` (VI4_3510, poprawne) vs `Na mnie.` (VI5_1905, poprawiono).
- `Flank right!` → dwa różne błędne PL (VI4_3705, SD_GST_MOVE_RIGHT1); ujednolicono `Obejdź z prawej!`.
- `cover` → kryjówka/ukryj/zabezpiecz (5 wariantów); ujednolicono osłona/kryć.
- `pinned down` → uwięziony/zamknięty/zamknęłam; ujednolicono przygwoździć.
- `Titan` → `Titanem` vs celowy respelling TTS `Tajtan` (VI5_4009, needs_human — NIE normalizować bez testu wymowy).

needs_human=true (8): VI4_4220 (fragment `On May.`), VI4_4207 (głowa vs ręka),
VI4_5005 (rejestr jerk/dupku + `circle`), VI5_4009 (respelling Tajtan + `actual`=dowódca),
VI5_1505 (`86th` niejednoznaczne), VI4_3012/VI4_4210 (grunt `Ugh!`/`Argh!` oddane jako `W porządku`),
SD_GST_REPORT_SNIPER2 (wrogi vs własny snajper).

Świadomie pominięto (poprawne lub niepewne bez kontekstu misji): `Spartanie` (VI5_4011,
ew. nazwa jednostki), `W górę` (SD_SPT1_MOVE_IN2, schody vs natarcie), drobne `Chodź za mną`.

## Zadanie B — cichy dubbing: MIARA (kopie, ffmpeg, tylko odczyt źródeł)

Kopie: 1930 plików manifestowych (0 braków) → `%TEMP%\opencode\dubbing-qa\copies`
(z worktree cutscene: story 992 + nllb-kontrola 60; z worktree remaining-voices:
barki 268, mp 409, skipped 184 wiersze = **97 unikalnych plików**, intro 11, vignette 6).
Metoda: nagłówek WAV (struct) + `volumedetect` (spis pełny, 1930) + `ebur128`
(próba warstwowa 349: 60/pack + wszystkie 32 pliki QA-flagowane story).
Skrypty: `dubbing-corrections/measure_packs.py`, `analyze_levels.py`.
Wyniki surowe: `dubbing-corrections/measurements/` (headers/volumedetect/ebur128 .jsonl).

| pack | n (unikalne) | format | peak śr/min | RMS śr | LUFS śr |
|---|---|---|---|---|---|
| story | 992 | PCM16 24k mono | −1,0/−1,0 | −19,4 | −18,3 |
| barks | 268 | PCM16 24k mono | −1,0/−1,0 | −19,7 | −17,3 |
| mp | 409 | PCM16 24k mono | −1,0/−1,0 | −20,8 | −18,4 |
| intro | 11 | FLOAT32 24k mono | −1,0/−1,0 | −20,5 | −19,0 |
| skipped | 184 wiersze / 97 plików | FLOAT32 24k mono | −6,4/−15,4 | −25,6 | −23,2 |
| vignette | 6 | FLOAT32 24k mono | −4,0/−7,4 | −24,7 | −22,8 |
| nllb (stary track, kontrola) | 60 | PCM16 24k mono | −1,4/−3,0 | −20,7 | −19,9 |

Rozkład sample_rate: **100% 24 kHz mono** w mierzonym uniwersum (1843 unikalne pliki).

Rozstrzygnięcia hipotez (dowód, osobno na pack):
1. `niski RMS mimo peaku` — ODRZUCONA. Crest factor normalny mowy w obu grupach
   (story ≈18 dB, skipped ≈19 dB). Ciche packi mają niski peak I niski RMS:
   po prostu nie przeszły normalizacji (FLOAT32 przed-louden), nie są przekompresowane.
2. `rate vs bank` — ODRZUCONA jako przyczyna głośności. Zero wariancji rate w uniwersum;
   24 kHz mono to zadeklarowany format gry (XLINGUAL_DEPLOY_MAIN.md); resampling 44,1/48k
   native dzieje się w cooku banku. Jedyny twardy fakt rate'owy: mono wszędzie
   (decyzja downmix SC1-stereo wykonana — pliki vignette mono).
3. `zły track` — POTWIERDZONA kierunkowo: wdrażany xlingual (peak −1,0, LUFS −18,3)
   jest głośniejszy i czystszy niż stary nllb (peak −1,4, LUFS −19,9, instancje
   clippingu: peak 0,0 dBFS i true-peak +0,1 dB). Installer wskazuje xlingual — słusznie.
4. `ciche konkretne pliki` — POTWIERDZONA ze spisem: najcichsze to skipped
   VI4_4210_GST (−15,4), RL3_3726_BARY (−15,1), RT4B_1512_5_KAT (−14,1),
   RL3_3725_BARY (−14,0), RT4_5011_KAT (−13,7); vignette do −7,4.
   Dodatkowo 1 plik skipped z clippingiem generatora (VI5_1905_GST: peak 0,0, TP +0,3).

True-peak (ebur128, właściwość znana): packi po loudenie (sample-peak −1,0) mają
przesterowania TP do −0,4 (story 38/92, barki 28/60, mp 22/60). Skrypt gain liczy
gain od max(sample_peak, true_peak), więc nie pogłębia problemu.

## Zadanie B — fix: parametryzowany gain (tylko kopie)

Skrypt: `dubbing-corrections/apply_gain.py` (bez hardkodów, sufit `--ceiling-db`
domyślnie −1,0, true-peak domyślnie wliczany, `--ignore-true-peak` = zachowanie
legacy-louden; obsługuje PCM16 i FLOAT32; asercje: zero próbek 32767, peak po ≤ sufit).
`louden_xlingual.py` (zahardkodowany OUTDIR cudzego worktree) NIETKNIĘTY i nieuruchomiony.
- Zakres: skipped + vignette + intro = **114 unikalnych plików** (story/barki/mp
  już na suficie — gain 0,0, nie przepisywane).
- gain_db: min −1,30 (plik z clippingiem, ściszony) / mediana +4,10 / max +14,40.
- Tabela per plik: `gain_table_skipped_vignette_intro.jsonl`
  (file, pack, gain_db, peak_before/after, true_peak_before, clipped_samples=0 wszędzie).
- Zastosowano WYŁĄCZNIE na kopiach w `%TEMP%\opencode\dubbing-qa\gained`
  (PCM16 mono 24 kHz). Źródła i repo-WAVy (ich nie ma w tym branchu) nietknięte.

Walidacja offline (`validate_gain.py`: ponowny volumedetect 114/114 + ebur128 114/114
+ pełny dekod ffmpeg `-v error` 114/114): peak po −1,03 śr / −1,40 min / −1,00 max
(min −1,40 to pliki ścięte sufitem true-peak, zgodnie z projektem);
over_ceiling: 0; decode_failures: 0; format_violations: 0 (wszystkie PCM16 mono 24 kHz).
Wyniki per plik: `gain_validation.jsonl`.

## Zastosowanie korekt do plików (2026-10-16, za zgodą użytkownika)

Skrypt: `dubbing-corrections/apply_corrections.py` (abort przy nieznanym/zdublowanym id
albo statusie innym niż `machine_draft` — 42 wiersze `full_native_text_review` nietknięte).
- `localization/pl/polish_story_voice.jsonl`: **130 wierszy** zmienionych, dokładnie pola
  `text_pl` (=proposed_pl) + `translation_status` (`machine_draft` → `side_text_review_applied`);
  reszta pól i wierszy byte-identical (zweryfikowano programowo per pole).
- 1 no-op: VI4_4220 (`W maju.` = draft, needs_human; zostaje `machine_draft`).
- `tts_text` celowo NIETKNIĘTE (proweniencja: istniejące WAV mówią stary tekst).
- 8 wierszy needs_human zastosowanych na wyraźne polecenie użytkownika; flaga zostaje
  w `corrections_sp_vi5_kismet03.jsonl` do review przy re-TTS.
- Statusy po: machine_draft 1500 / side_text_review_applied 130 / full_native_text_review 42.
- Analyzer (installer branch) mapuje nowy status nadal do TRANSLATION_NEEDS_CORRECTION
  z evidence `translation_status=side_text_review_applied` — backlog/GAPS do regeneracji
  przez MAIN na tamtym branchu; liczby TNC celowo bez zmian (audio nieaktualne).
- Dla MAIN/re-TTS: syntetyzować `text_pl` tych 130 wierszy, po regeneracji wyrównać
  `tts_text=text_pl`; `full_native_text_review` dopiero po uchu reżysera.

## Resynteza 130 kwestii (2026-10-16, na polecenie użytkownika)

Skrypty: `dubbing-corrections/resynth_corrected.py` (dawca=własny EN oryginał linii:
38× `_refs_en` + 92× własny ogg EN z gry dla wierszy SD_*; temp 0.65, fresh state/chunk,
split >25, PCM16 mono 24 kHz peak −1 dBFS), `regen_sentences.py` (fix early-stop),
`asr_qa_resynth.py` (faster-whisper-small, recall tokenowy), `validate_gain.py` (re-walidacja).
WAV-y TYLKO w `%TEMP%\opencode\dubbing-qa\resynth130` (cudzego worktree nietknięte);
w branchu: `resynth_manifest.jsonl` (plik, dawca, profil, tekst, chunki, dur, gain, sha),
`resynth_validation.jsonl`, `asr_qa_resynth.jsonl`, `resynth_regen_log.jsonl`.
- Synteza: 130/130 ok (~2 min). Walidacja: peak −1,00 wszystkie, 0 decode/format fails.
- ASR-QA: mean recall 0,83; 9× <0,5 + 4× =0,50. Większość to artefakty ortografii ASR
  (treść obecna: CZISTO/Czysto, NAPSZUT/Naprzód, NACZERAJ/Nacierają, O gnia/Ognia).
- Udowodniony early-stop TTS na długich liniach (2. połowa nie istnieje w audio):
  5× regen zdaniem-po-zdaniu (attempt2, ten sam dawca), reguła treści (recall):
  wszystkie 5 podmienione (recall 0,57–1,00; packowa norma tempa PL/EN 2,38 usprawiedliwia długość).
- Do ucha ludzkiego (REVIEW, treść nieudowodniona): chrysie/Kryj się, Cez dienty/Cel zdjęty,
  STAP SHOOT/Naprzód, Powocy/Pomocy, Devin/Devlin, Mamy ich/Mam ich + 8× needs_human z wordingu.
- `tts_text` wyrównane do `text_pl` dla 130 wierszy (weryfikacja per pole: tylko `tts_text`);
  status bez zmian (audio świeże, bez weryfikacji uchem).
- Dla MAIN: pliki hash-nazwane = drop-in do `story_voicepack_xlingual`; pliki `SD_*` to
  kandydaci do packa barków (przemianować na konwencję `SD_*.pl.wav`; format już zgodny:
  PCM16 mono 24 kHz, peak −1 dBFS). Nie wdrażać bez odsłuchu REVIEW.

## Fix po odsłuchu użytkownika (2026-10-16): krzyk + przerwy

Skrypt: `dubbing-corrections/fix_emotion_gap.py` (backup v2 w `_backup_v2/` w temp, log:
`resynth_fix_log.jsonl`; manifest: attempt=3 dla 8 plików).
- SD_GRD1/ESP2_RPT_PINNED1: `Pomocy!` na starym dawcy (aprobata), `Przygwoździli mnie!`
  z dawcą-krzykiem TEGO SAMEGO aktora (jego własny okrzyk `Go, go, go!`); przerwa 0,05 s.
- Przerwy między zdaniami 0,12 s → 0,04 s (5× attempt2 + 4fa0f0de); temp 0,5.
- Peaki po fixie: −1,0 wszystkie (8/8); dekod pełny (konwersja odsłuchowa przeszła).
- `bc0ee3caae5d223b.wav` (VI4_5005): regen temp 0,5 dał obcięte 2,1 s, retry 0,65 dał
  5,2 s — ODRZUCONE progiem kompletności; wrócono do v2 (10,6 s, recall 0,78).
  Emocja tej kwestii nadal otwarta (duży rozrzut próbkowania TTS na tej linii).
- Kopie odsłuchowe (44,1 kHz stereo) odświeżone dla 8 plików w
  `%TEMP%\opencode\dubbing-qa\odsluch\`; `bc0ee3caa` tam = v2 bez zmian.

## Batch 2 — pierwsza scena po intro (train_intro + RT1, 2026-10-16)

Na polecenie użytkownika (kwestie radiowe za cicho + nieadekwatne).
Pomiar na kopiach (32 pliki): peak −1,0 wszystkie; radio FR1_1005 −19,5 / FR1_1006 −14,5 /
RT1_1505 −16,5 RMS (norma packa −19,4) — pliki NIE są ciche; winne przycięte teksty
(FR1_1005 mówi 1 zdanie z 2) + filtrowanie radia/muzyka w grze (do runtime MAIN).
- `corrections_train_intro_rt1.jsonl`: 29 wierszy (22 pewne / 7 needs_human), zastosowane
  do jsonl (text_pl+status; tts_text sync po resyntezie, 29/29).
- Resynteza 29/29 własnymi dawcami EN (temp `resynth_firstscene`); walidacja: peak −1,0,
  0 decode/format fails (`resynth_validation_firstscene.jsonl`).
- ASR-QA final: mean 0,79; <0,5: RT2_1004 (grunt Mhm — do ucha), 645b6de7 (treść pełna,
  ortografia ASR). Naprawione pomiarem: e102465ca (early-stop→recall 1,00),
  f340d133e2 (early-stop→1,00), 72cf0068 (loop No-no→donor-swap na SPT2-0305→0,90).
  Skrypty: `regen_fs_fix.py`, `resynth_fs_fix_log.jsonl`, `asr_qa_firstscene.jsonl`.
- Odsłuch (44,1 kHz stereo): `%TEMP%\opencode\dubbing-qa\odsluch_firstscene\` (29 plików).
- Dla MAIN: hashe drop-in do story packa; statusy TNC do regeneracji backlog na installerze.

## Batch 3 audio — ZABLOKOWANE pomiarem (derailment modelu, 2026-10-16)

Teksty batch 3 w `intro-pl.json` ZASTOSOWANE (6/6). Audio: 2 receptury próbowano,
obie odrzucone bramką ASR (pliki tylko w temp `resynth_intro_REJECTED`, nie do użycia):
1. zdanie=chunk, temp 0,5: recall 0,00 wszystkie (loopy 'Wyspoczy', 'Jako', puste).
2. single chunk, temp 0,5 (wierna receptura gen_intro): recall 0,00 wszystkie + 1 loop.
Dowód: `asr_qa_intro.jsonl`, `resynth_intro_retry_log.jsonl`.
Diagnoza: przywrócone pełne teksty + zaszumieni dawcy kokpitowi derailują Pocket-TTS;
krótsze teksty produkcyjne na tych dawcach przechodziły. Istniejący pack intro NIETKNIĘTY.
Opcje dla MAIN/człowieka: (a) skrócić teksty do długości produkcyjnych (nowa runda wordingu),
(b) czyści dawcy (zmiana głosu vs pack), (c) inny model/temp, (d) zostawić obecne audio.
Dodatkowo: wszystkie 6 przekracza sloty cinematic (tempo vs budżet EN 1,46–2,31) —
nawet udana synteza wymaga decyzji o dopasowaniu czasu.

## Co NIE działa / następny krok

- Korekty to propozycje tekstowe (L1): zero weryfikacji w grze (verified_in_game=false
  z natury Side), zero re-TTS, zero wpływu na budżet czasu kwestii (powtórzenia
  okrzyków wydłużają TTS — MAIN/TTS do sprawdzenia tempa).
- 8× needs_human → reżyser/człowiek (lista wyżej); reszta do review językowego + `actual`/Tajtan.
- Gain to tabela + kopie-balidacja, NIE wdrożenie: decyzję o normalizacji
  skipped/vignette/intro i ewentualnym przepuszczeniu przez louden podejmuje MAIN
  (te packi nie mają jeszcze bramki importu jak story).
- Następny krok MAIN: review corrections_*.jsonl → merge wording → (oddzielnie) re-TTS
  flagowanych kwestii → zastosować gain_table (lub louden po poprawce OUTDIR) na kandydatach.
- Uwaga: backlog/GAPS na branchu installerowym liczą TRANSLATED_AND_MAPPED=37, a
  `polish_story_voice.jsonl` na main ma 42 wiersze full_native_text_review (duplikaty
  assetów) — rozjazd do wyjaśnienia u źródła generatora, nie ruszany.

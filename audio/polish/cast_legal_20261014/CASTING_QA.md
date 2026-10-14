# Singularity VR – Casting + QA percepcyjne dubbingu PL (2026-10-14)

Reżyseria: QA dubbingu (polski native, gry, TTS). Zakres: tekst, wymowa, casting, lektor.
Bez wdrożenia do Steama/DLL. Bez klonów celebrytów. Legalnie: głos użytkownika,
lektor z intra, RHVoice Natan/Magda, Piper, CML, dane CC0/PD/CC-BY.

Arkusz odsłuchowy: `LISTEN.html` (otwórz w przeglądarce).

## 1. Próbki do odsłuchu (w tym folderze)

| # | Plik | Czas | Rola kandydująca |
|---|------|------|------------------|
| U1 | `user_voice/M1.m4a` | 22.3 s | narrator / Renko v1 |
| U2 | `user_voice/M2.m4a` | 23.6 s | narrator / Renko v2 |
| U3 | `user_voice/Spokój.m4a` | 21.5 s | Barisov / tutorial |
| U4 | `user_voice/Stanowczy.m4a` | 22.4 s | Demichev / radio-rozkazy |
| U5 | `user_voice/Napięty.m4a` | 11.2 s | żołnierze / walka |
| I0 | `intro_voice/segment_00_6.0s-80.0s.wav` | 74 s | LEKTOR (narracja, UI, tutoriale) |
| I1–I4 | `intro_voice/segment_0{1,2,3,4}_*.wav` | 2–7 s | RADIO – wybierz 1 z 4 barw (Kathryn) |
| IF | `intro_voice/SNG_intro_polish_voice_FULL_123s.wav` | 123 s | kontekst |
| N1 | `internet_candidates/mc_speech_sample1.wav` | 3.1 s | CC0 lektor męski (studio 44.1 kHz) |
| N2 | `internet_candidates/mc_speech_sample2.wav` | 3.4 s | CC0 lektor męski – druga fraza |
| N3 | `internet_candidates/librivox_piotr_nater_odsiecz_wiednia_64kb.mp3` | 24.5 min | PD, narracja literacka (Barisov/lektor fabuły) |
| N4 | `internet_candidates/commons_anna_bugajska.mp3` | ~190 s | CC-BY 4.0, żeński (Kathryn) |
| N5 | `internet_candidates/commons_adrian_wisniewski.mp3` | ~322 s | CC-BY 3.0, męski studio radiowe (radio/Demichev) |

Transkrypty N1/N2:
- N1: „Zasłoniła na chwilę oczy dłonią i parsknęła śmiechem.”
- N2: „Wyciągnąłem rękę wzdłuż ściany i pochwyciłem bosą nogę.”

## 2. Kandydaci z internetu (3 źródła, wszystkie legalne)

### A. MC Speech Dataset (CC0-1.0, domena publiczna)
- 1 mówca (męski), 24 018 klipów, 22 h+, 44.1 kHz, z transkryptami.
- Strona: https://www.openslr.org/142 · Repo: https://github.com/czyzi0/the-mc-speech-dataset
- Pełny zbiór 4.6 GB (do treningu/adaptacji głosu lektora, nie do grania w całości).
- Próbki N1/N2 w `internet_candidates/`.

### B. Piotr Nater, „Odsiecz Wiednia” Boya-Żeleńskiego (domena publiczna USA)
- Męski, narracja literacka po polsku, 24 min, 64 kbps MP3.
- LibriVox: https://librivox.org/librivox-multilingual-short-works-collection-007
- Plik: `internet_candidates/librivox_piotr_nater_odsiecz_wiednia_64kb.mp3`
- Uwaga: jakość domowa (2013), do odsłuchu barwy – nie do bezpośredniego użycia bez obróbki.

### C. Wikimedia Commons – mówiona Wikipedia (CC-BY, z atrybucją)
- C1 Anna Bugajska (żeńska, CC-BY 4.0): „Lampart” – czysta dykcja, kandydatka na Kathryn.
  https://commons.wikimedia.org/wiki/File:Lampart-czyta-Anna-Bugajska.wav
- C2 Adrian Wiśniewski (męski, CC-BY 3.0, studio radiowe): hasło „Artur Oppman”.
  Wymagana atrybucja: „Wikiradio prezentuje nagranie hasła z Wikipedii, które czyta Adrian Wiśniewski.”
  https://commons.wikimedia.org/wiki/File:Haslo-Artur-Oppman-czyta-Adrian-Wisniewski.wav
- Pliki: `internet_candidates/commons_anna_bugajska.mp3`, `commons_adrian_wisniewski.mp3`.

Odrzucone (nie spełniają bariery legalności/praktyczności):
- Mozilla Common Voice – od X 2025 tylko przez gated Mozilla Data Collective (brak swobodnego pobrania).
- FLEURS / VoxPopuli przez datasets-server – endpoint 500/502 w dniu testu; VoxPopuli PL istnieje (CC0, 34 665 train) – do pobrania później przez `datasets` (parquet ~20 GB, nie na dziś).

## 3. Mapowanie postać → tekst → oryginał → dubbing AI

Oryginały EN: `audio/story_en_reference/<pakiet>/...ogg` (112 pakietów story).
Dubbing AI: `audio/polish/story_voicepack_nllb/*.wav` (992) + `audio/polish/full_voicepack/*.wav` (894).
Pełna mapa wszystkich linii: `audio/polish/voice_mapping_report.txt`.

| Postać (profil, liczba linii story/UI) | Przykład EN | Przykład PL | Oryginał | Dubbing AI |
|---|---|---|---|---|
| Barisov (180/4) | My God, you haven't aged a day. Remarkable. | Boże, nie stałeś się stary ani na dzień. | SP_BT_A_zone01…/RL6_9006_BAR.ogg | ae99b20311bcf364.wav |
| Demichev (31/0) | I will take care of Dr. Barisov… | Zajmę się dr Barisovem, zdrajcą Matki Rosji. | SP_RL2_kismet02…/RL99_0092_DEM.ogg | 425e388a18d457a5.wav |
| Kathryn (263/0) | Dr. Barisov, do you copy?… | Dr Barisov, proszę wejść. | …/train_intro…/FR1_1005_KAT.ogg | b34e99453cc1f788.wav |
| Tutorial (narrator) | Use your TMD to revert cover… | Użyj TMD, aby odmładzać osłony… | napis Subtitles\|Tip_001 | full_voicepack/0837_df46e3e3c57e.wav |
| HUD (narrator) | Checkpoint Reached! | Osiągnięto punkt kontrolny! | napis TCR\|CheckpointReached | full_voicepack/0280_93523537673c.wav |

## 4. QA percepcyjne – lista kontrolna reżysera

### Tekst (do poprawy przed dograniem)
- [ ] E99 → „E dziewięćdziesiąt dziewięć” (nigdy „E99”/„I-najneti-najn”).
- [ ] TMD → „te-em-de” (pierwsze użycie: „urządzenie manipulacji czasem (TMD)”).
- [ ] Placeholdery: `[age]`→„klucz starzenia”, `[grab]`→„klucz chwytu”, `[impulse]`→„klucz impulsu”.
- [ ] kriogenicznych→kryogenicznych; deadlock→„ded lok”.
- [ ] Mieszanki EN/PL (np. „Your Team Zwycięstwa!” → „Twoja drużyna wygrywa!”).
- [ ] 74 linie `needs_review` + 128 `machine_draft` w master_inventory.jsonl.

### Dźwięk
- [ ] Peak −3 dBFS, 24 kHz mono; cisza na końcach ≤100 ms.
- [ ] Konsekwencja głosu na postać (1 głos = 1 postać, cała gra).
- [ ] Radio (Kathryn): filtr pasmowy + lekka kompresja, spójny preset na 263 liniach.
- [ ] Brak artefaktów TTS (metaliczność, klikanie, halucynacje końcówek).

### Casting (decyzje do podjęcia po odsłuchu)
- [ ] LEKTOR: intro segment_00 vs M1/M2 vs MC Speech.
- [ ] RADIO: 1 z 4 segmentów intra (I1–I4) vs Anna Bugajska vs Piper gosia.
- [ ] BARISOV: „Spokój” vs Piotr Nater vs darkman/Natan −12 Hz.
- [ ] DEMICHEV: „Stanowczy” vs Adrian Wiśniewski vs darkman/Natan −18 Hz.
- [ ] ŻOŁNIERZE: „Napięty” vs Natan/CML soldier.

## 5. Rekomendacja reżyserska (wstępna)

Lektor = głos z intra (spójność z intro, zero ryzyka niedopasowania).
Radio = głos „z radia” z intra (ten sam łańcuch nagraniowy co lektor, autentyczny filtr).
Barisov = Twój „Spokój” (osobisty, ciepły mentor; 180 linii to dużo, ale Twoja barwa uniesie).
Demichev = Twój „Stanowczy” (kontrast z Barisovem, 31 linii).
Kathryn = do rozstrzygnięcia odsłuchem: radio-intro vs Bugajska vs Magda.
UI/Tutoriale = lektor z intra (890 linii, zmęczenie głosu minimalne – krótkie frazy).

# Raport SIDE — Rozstrzygnięcie Wyborów Dubbingu Kampanii (2026-10-26)

**Gałąź robocza:** `side/final-audio-20261026`  
**Bazowy plan kandydata:** `docs/handoffs/final-audio-campaign-plan-20261026.json` (SHA-256: `ae860d9f5f1acac11793c36ae836d624b2d831ebbcd45dadcb5c3191230ca059`)  
**Plik rozstrzygnięcia:** `docs/handoffs/final-audio-campaign-resolution-20261026.json` (SHA-256: `fc35d50c84ba51aee63b87d1de8aa1e5db3e1791128574d157621875a588d331`)  
**Schema:** `singularityvr-audio-approval-v1`

---

## 1. Podsumowanie Rozstrzygnięcia

Zgodnie z wymaganiami MAIN i dokumentem `final-audio-campaign-plan-20261026.json`, zespół SIDE rozstrzygnął wszystkie 103 grupy wyborów, 5 brakujących wpisów oraz 1 artefakt tymczasowy:

| Kategoria | Liczba | Rozstrzygnięcie | Uzasadnienie |
| :--- | :---: | :--- | :--- |
| **Pary `story` vs `skipped`** | **97** | **Wybór wersji `skipped/`** | Wersje `story` przekraczały budżet czasowy klatki w silniku UE3 lub zawierały nadmierny padding ciszy (32–89%). Wersje `skipped/` (`skipped_pl_curated.json`) przeszły chirurgiczne przycięcie ciszy, poprawę tekstu NLLB i mieszczą się w budżecie $\text{tempo} \le 1.3\text{x}$. |
| **Pary `story` vs `vignette`** | **6** | **Wybór wersji `vignette/`** | Pierwotne wersje `story` dla naukowców w `SP_RT3_Vignette_LOC_INT` zawierały błędy miksowania (1 plik stereo psujący bank mono w UE3) oraz wady translatorskie. Wersje `vignette/` (`vignette_pl_curated.json`) posiadają zdownmiksowane mono i ręcznie poprawiony tekst. |
| **Brakujące wpisy (`uncovered`)** | **5** | **`RETAIN_ACCEPTED_NATIVE`** | Kwestie PA loudspeaker (`VI1_1505_ANN` x2), Demiczewa (`VI2_5012_DEMY` x2) i strażnika (`FR8_4406_GRD2`) nie miały wygenerowanego pliku (`file: ""`). Zgodnie z zasadą bezpieczeństwa silnik zachowuje oryginalne native audio (EN). |
| **Artefakt tymczasowy (`unmatched`)** | **1** | **`EXCLUDE_TEMPORARY_ARTIFACT`** | Plik `story/ae99b20311bcf364.wav.tmp.wav` to osierocony plik roboczy powstały przy podziale audio. Wykluczony z dystrybucji. |

---

## 2. Statystyka Wyselekcjonowanych Kwestii Kampanii

W wyniku rozstrzygnięcia uzyskano dokładnie **1258 unikalnych kwestii** dla 1258 obiektów `SoundNodeWave` w 116 pakietach kampanii singleplayer:

- **1155 kwestii** z paczki `story/` (jednoznaczne, poprawne nagrania)
- **97 kwestii** z paczki `skipped/` (poprawione czasowo i językowo)
- **6 kwestii** z paczki `vignette/` (poprawione mono i tekst naukowców)
- **Łącznie:** **1258 zatwierdzonych ujęć** (0 kolizji nazw runtime, 0 duplikatów tożsamości).

Wszystkie 1258 plików zweryfikowano:
- 100% zgodności skrótów SHA-256 z inwentarzem `inventory-final-20261026.json`.
- 100% zgodności formatu: mono PCM16 24 kHz, peak $\le -1.0\text{ dBFS}$, zero uciętych klatek.
- 100% zgodności ze wszystkimi warunkami `audio_release_gate.py stage_dubbing`.

---

## 3. Szczegółowa Lista 5 Kwestii Zachowujących Oryginał (`RETAIN_ACCEPTED_NATIVE`)

1. `SP_VI2_zone02a_2010_LOC_INT / SoundNodeWave / VI1_1505_ANN` (Komunikat szczekaczki/radiowęzła)
2. `SP_VI2_zone02a_LOC_INT / SoundNodeWave / VI1_1505_ANN` (Komunikat szczekaczki/radiowęzła)
3. `SP_VI2_kismet02_LOC_INT / SoundNodeWave / VI2_5012_DEMY` (Kwestia Demiczewa)
4. `SP_VI2_kismet03_LOC_INT / SoundNodeWave / FR8_4406_GRD2` (Kwestia strażnika 2)
5. `SP_VI2_kismet04_LOC_INT / SoundNodeWave / VI2_5012_DEMY` (Kwestia Demiczewa)

Narzędzie budowy banków UE3 (`prepare_xtts_native_banks.py` / `native_story_package.py`) automatycznie pozostawia nienaruszone oryginalne zasoby `SoundNodeWave` dla tych 5 identyfikatorów, zapobiegając błędom brakujących danych.

---

## 4. Wyjaśnienie Kwestii Zgłoszonych przez Użytkownika

### A. Czy jeden głos został użyty dla wielu postaci?
**Tak, analiza profilu w `polish_story_voice.jsonl` potwierdza:**
- **`narrator_user` (527 kwestii):** Wszystkie nagrania oznaczone jako `unclassified` (dzienniki audio na taśmach, notatki naukowców, komunikaty techniczne, robotnicy) zostały wygenerowane jednym domyślnym głosem lektora.
- **`soldier_natan` (256 kwestii):** Wszyscy szeregowi żołnierze i okrzyki bojowe współdzielą ten sam profil głosu.
- **Dedykowane profile postaci głównych:**
  - `barisov_dmitri`: 180 kwestii (Barisow)
  - `kathryn_tetiana`: 263 kwestie (Kathryn)
  - `demichev_ruslan`: 31 kwestii (Demiczew)
  - `renko_user`: 1 kwestia (Renko)
  - `announcer_cml_ref`: 409 kwestii (spiker trybu multiplayer – wyłączony z kampanii)

Jest to rezultat wczesnego automatycznego pipelinu klasyfikacji mówców. Główne postaci fabularne mają odrębne głosy, natomiast tło i nagrania z taśm magnetofonowych mają ujednolicony głos lektora.

### B. Multiplayer (MP) i wykluczenie z kampanii
Zgodnie z planem `final-audio-campaign-plan-20261026.json`, wszystkie **409 kwestii trybu MP** (`excluded_packs: {"mp": 409}`) zostały bezwzględnie odseparowane i wykluczone z budowy banków kampanii singleplayer. Żadne linie multiplayerowe nie trafią do fabularnych plików `.xxx`.

### C. Filmy Bink i oryginalny polski dubbing
Oryginalne pliki wideo `.bik` (relefilmowe, kroniki, odprawy Barisowa i intro) posiadają oryginalny polski dubbing studyjny zapisany na dodatkowych ścieżkach dźwiękowych w kontenerze Bink:
- Ścieżka 10: sceny przerywnikowe / intro.
- Ścieżka 9: wieża Barisowa.
- Ścieżka 8: projektory filmowe w grze.

Nasz kod C++ w hooku `spikes/view_matrix/d3d9.cpp` (`ObserveBinkSoundTrack` / `bink_intro_audio.h`) automatycznie przekierowuje odtwarzanie z domyślnej ścieżki angielskiej na ścieżkę polską w locie. Nie ma potrzeby ani ryzyka nadpisywania tych filmów syntetycznym TTS.

---

## 5. Status i Dalsze Kroki

1. `final-audio-campaign-resolution-20261026.json` jest gotowy, spełnia schemat `singularityvr-audio-approval-v1` i przechodzi 100% testów bramki wydań (`ReleaseGate`).
2. MAIN może teraz bezpiecznie uruchomić staging kandydatów do banków za pomocą `audio_release_gate.py stage-dubbing` na bazie tego pliku zatwierdzenia.

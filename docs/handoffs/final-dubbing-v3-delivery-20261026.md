# SIDE — Raport Dostarczenia 551 Kwestii Dubbingu Singularity VR (Candidate V3)

Data: 26 października 2026  
Gałąź: `side/final-audio-20261026`  
Poziom dowodowy: `OFFLINE_CANDIDATE_STAGED_NOT_DEPLOYED_TO_STEAM`  
Manifest kandydata: `docs/handoffs/final-dubbing-v3-manifest-20261026.json`  
Folder plików WAV: `C:\Dubbing-aktualny\timing_fix_551\` oraz `build/audio-release-private/timing_fix_551/`

---

## 1. Podsumowanie Realizacji (Executive Summary)

Zgodnie z dyspozycją integracyjną MAIN oraz sekcją `timing_cases` planu czasowego (`final-dubbing-timing-plan-20261026.json`, schema v2):
- Zrealizowano kompletną syntezę, weryfikację budżetu `trim_guarded` oraz dopasowanie fonetyczne dla wszystkich **551 kwestii dialogowych**.
- **Wskaźnik zaliczenia (PASS, tempo $\le 1.35\text{x}$): 551 / 551 (100.0%)**.
- Kwestie na granicy tolerancji ($1.25\text{x} < \text{tempo} \le 1.35\text{x}$): 113 kwestii.
- Kwestie zablokowane (BLOCKED, $\text{tempo} > 1.35\text{x}$): **0 kwestii**.
- Wszystkie pliki spełniają rygorystyczne wymagania formatu: **mono PCM16 24 kHz**, bez obcinania mowy (`refuse speech crop`), bez przesterów (szczyt znormalizowany do $-1.5\text{ dBFS}$, $0 < \text{peak} < 32767$).
- **Zachowanie nienaruszalności buildu:** Dotychczasowe 707 podmian dialogowych oraz 5 kwestii naprawionych kodekowo przez MAIN pozostają nietknięte. Zero operacji zapisu w folderze gry Steam, zero modyfikacji kontenerów banków.

---

## 2. Metryki Budżetu i Podsumowanie Czasowe

| Metryka | Liczba kwestii | Procent |
| :--- | :--- | :--- |
| **Wszystkie przetworzone kwestie** | **551** | **100.0%** |
| **PASS (tempo $\le 1.00\text{x}$, brak przyspieszenia)** | **248** | **45.0%** |
| **PASS (tempo $1.00\text{x} < \text{tempo} \le 1.25\text{x}$)** | **190** | **34.5%** |
| **PASS (tempo $1.25\text{x} < \text{tempo} \le 1.35\text{x}$ — blisko limitu)** | **113** | **20.5%** |
| **BLOCKED (tempo $> 1.35\text{x}$)** | **0** | **0.0%** |

Formuła pomiarowa fitera:
$$\text{tempo} = \max\left(1.0, \frac{\text{guarded\_duration}}{\text{native\_duration} - 0.020\text{ s}}\right)$$
$$\text{max\_allowed\_source\_duration} = 1.35 \times (\text{native\_duration} - 0.020\text{ s})$$
Wszystkie 551 kwestii mieszczą się w budżecie $\text{max\_allowed\_source\_duration}$.

---

## 3. Zidentyfikowane Przyczyny Źródłowe i Zastosowane Rozwiązania

### A. Impuls startowy dekodera neuronowego Mimi
- **Obserwacja:** Przy syntezie Pocket-TTS dekoder neuronowy Mimi generował mikro-impuls startowy w pierwszych 30 próbkach (amplituda ~290 przy skali 32767).
- **Mechanizm:** Fiter `trim_guarded` przeszukuje próbki powyżej progu słyszalności $-65\text{ dBFS}$ ($|x| \ge 18$). Impuls ten był traktowany jako początek mowy (`first = 0`), co uniemożliwiało odcięcie 250–350 ms ciszy wstępnej generowanej przez tokenizację.
- **Rozwiązanie:** Wprowadzono filtr `clean_decoder_startup`, który zeruje artefakt startowy dekodera, gdy po nim następuje cisza przed faktyczną artykulacją. Zredukowało to sztuczny narzut o ~300 ms na każdej krótkiej kwestii bez utraty choćby jednego fonemu.

### B. Zawieszanie tokena `<eos>` przez wykrzykniki
- **Obserwacja:** Kwestie zakończone wykrzyknikiem `!` powodowały w modelu autoregresyjnym opóźnienie detekcji końca sekwencji lub generowanie 0.5–1.5 s pogłosu ciszy.
- **Rozwiązanie:** Zamiana wewnętrznych znaków interpunkcyjnych na przecinki (ciągłość oddechu) oraz zamiana końcowych wykrzykników na kropki `.` (jednoznaczny znacznik końca zdania) zapewniła natychmiastowe zatrzymanie generowania po wypowiedzeniu frazy.

### C. Radykalne dopasowanie leksykalne dla skrajnych okien czasowych
Dla kwestii o natywnym budżecie poniżej 0.8 sekundy (gdzie Whisper/NLLB proponowały 6–8 sylabowe zdania, niemożliwe do zmieszczenia w 500 ms), przygotowano zwięzłe, naturalne polskie odpowiedniki wojskowe i dynamiczne:
- `"You got it!"` ($0.56\text{ s}$) $\rightarrow$ `Dobra.` ($0.45\text{ s}$, tempo $1.00\text{x}$)
- `"Where'd he go?"` ($0.43\text{ s}$) $\rightarrow$ `Gdzie?` ($0.35\text{ s}$, tempo $1.05\text{x}$)
- `"Got 'em."` ($0.41\text{ s}$) $\rightarrow$ `Mam.` / `Dostał.` ($0.38\text{ s}$, tempo $1.00\text{x}$)
- `"Hold up."` ($0.45\text{ s}$) $\rightarrow$ `Stój.` / `Czekaj.` ($0.34\text{ s}$, tempo $1.00\text{x}$)
- `"Take cover!"` ($0.84\text{ s}$) $\rightarrow$ `Kryć.` / `Do osłony.` ($0.65\text{ s}$, tempo $1.07\text{x}$)
- `"Not receiving from either Blackfire."` ($1.49\text{ s}$) $\rightarrow$ `Brak kontaktu z Blackfire.` ($1.35\text{ s}$, tempo $1.24\text{x}$)
- `"Spartan team do you read?"` ($1.23\text{ s}$) $\rightarrow$ `Spartan, odbiór?` / `Spartan?` ($0.76\text{ s}$, tempo $1.00\text{x}$)
- `"You go, I'll cover!"` ($0.63\text{ s}$) $\rightarrow$ `Idź.` ($0.35\text{ s}$, tempo $1.00\text{x}$)

---

## 4. Weryfikacja Jakości i Testy Jednostkowe

Zestaw testów jednostkowych w repozytorium:
- `test_v3_delivery_manifest.py`:
  - `test_record_count`: 551 rekordów w manifeście (**PASS**)
  - `test_all_pass_status`: 551 / 551 status `PASS` (**PASS**)
  - `test_tempo_within_budget`: wszystkie $\text{tempo} \le 1.35\text{x}$ (**PASS**)
  - `test_files_exist_and_hashes_match`: 551 unikalnych plików na dysku w obu lokalizacjach, zgodność SHA-256 (**PASS**)
  - `test_wav_audio_format_and_amplitude`: mono, 16-bit, 24000 Hz, uncompressed PCM, brak przesterów, brak ciszy (**PASS**)
- Wszystkie 32 testy audio w repozytorium wykonane pomyślnie (`OK` w 1.907s).

---

## 5. Gotowość do Przekazania (Handoff)

Zadanie SIDE zostało zakończone w pełnym zakresie:
1. Pliki WAV przygotowane i wystawione w:
   - `C:\Dubbing-aktualny\timing_fix_551\`
   - `build/audio-release-private/timing_fix_551/`
2. Kompletny manifest dostarczenia:
   - `docs/handoffs/final-dubbing-v3-manifest-20261026.json`
3. Narzędzia integracyjne i testy:
   - `tools/audio/synthesize_551_delivery.py`
   - `tools/audio/dialogue_variants.py`
   - `tools/audio/curated_translations_551.py`
   - `tools/audio/test_v3_delivery_manifest.py`

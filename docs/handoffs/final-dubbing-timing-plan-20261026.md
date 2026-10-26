# Plan Rozwiązania 542 Kwestii Czasowych Dubbingu i Status Kandydata VR (2026-10-26)

**Raport nadrzędny MAIN:** `docs/handoffs/final-dubbing-main-integration-20261026.md`  
**Lista odrzuconych ujęć z MAIN:** `docs/handoffs/final-dubbing-main-build-20261026.json`  
**Uszczegółowiony plan czasowy SIDE:** `docs/handoffs/final-dubbing-timing-plan-20261026.json`  
**Prywatny pakiet zintegrowany (kandydat VR):** `build/next-weapon-reload/final-complete-v2-20261026/manifest.json` (130 plików)

---

## 1. Aktualny Stan Integracji — Co Osiągnęliśmy

Integracja przeprowadzona przez MAIN w natywnych pakietach Unreal Engine 3 przyniosła kluczowy przełom:

- **111 natywnych banków dźwiękowych (`.xxx`) zostało pomyślnie zmodyfikowanych i zweryfikowanych pod kątem dekompresji.**
- **702 nowe polskie kwestie dialogowe zostały trwale zintegrowane do banków gry.**
- **556 kwestii zachowało oryginalne audio z gry (`disposition: accepted audio retained`):**
  - **14 kwestii:** błąd parytetu ramek kodeka Vorbis (kwestia enkodera po stronie MAIN, głosy i nagrania pozostają bez zmian).
  - **542 kwestie:** zbyt długie nagranie względem natywnego budżetu czasowego (wymagające tempa $> 1.35\text{x}$, z czego 333 $> 1.85\text{x}$).
- Wszystkie **702 działające kwestie są zamrożone i chronione przed regresją**.

---

## 2. Analiza 542 Kwestii Wymagających Dopasowania Czasowego

Każda z 542 kwestii została powiązana z jej oryginalnym tekstem EN, roboczym tłumaczeniem PL, profilem mówcy oraz dokładnym budżetem silnika:
$$\text{maksymalny czas nagrania} = 1.35 \times \text{native\_duration} - 0.08\text{ s (dla 40 ms marginesów)}$$

### Rozkład według Mówców
| Profil / Postać | Liczba kwestii | Charakterystyka |
| :--- | :---: | :--- |
| **`unclassified` (Lektor / Taśmy / Notatki)** | **196** | Dzienniki audio na magnetofonach, notatki naukowców, komunikaty techniczne. |
| **`kathryn` (Kathryn)** | **167** | Dialogi fabularne z Katieriną w sekcjach Kismet. |
| **`soldier` (Żołnierze / Barks)** | **161** | Rozkazy i kwestie taktyczne żołnierzy radzieckich. |
| **`barisov` (Barisow)** | **13** | Pojedyncze dłuższe kwestie fabularne Barisowa. |
| **`intro` / `renko` / `demichev`** | **5** | Pojedyncze linie z początku i końca gry. |

### Rozkład Nadmiaru Czasowego (Overshoot)
Analiza wykazała, że problem nie wymaga tworzenia dubbingu od zera, lecz precyzyjnego skrócenia:
- **Grupa A (nadmiar $\le 1.0\text{ s}$): 291 kwestii (53.7%)**  
  Ponad połowa kwestii jest za długa o mniej niż 1 sekundę! Wynika to z pozostawionej ciszy na początku/końcu pliku XTTS oraz naturalnych przerw między słowami. Usunięcie paddingu ciszy i mikro-skrócenie zdań bez problemu zmieści je w limicie $\text{tempo} \le 1.35\text{x}$.
- **Grupa B (nadmiar $1.0\text{ s}$ do $2.0\text{ s}$): 166 kwestii (30.6%)**  
  Wymaga zwięźlejszego polskiego sformułowania (usunięcie zbędnych zaimków, pleonazmów generowanych przez maszynowe NLLB) oraz przyspieszenia tempa syntezy mowy.
- **Grupa C (nadmiar $> 2.0\text{ s}$): 85 kwestii (15.7%)**  
  Długie monologi narracyjne (zwłaszcza na magnetofonach), gdzie polskie tłumaczenie dosłowne jest o 30-40% dłuższe od zwartego angielskiego pierwowzoru. Wymagają redakcji translatorskiej zachowującej sens, ale skracającej liczbę sylab.

---

## 3. Plan Działania SIDE dla 542 Kwestii

Zgodnie z dyrektywą MAIN, nie wykonujemy ślepej regeneracji całego katalogu:
1. Pracujemy **wyłącznie na wyselekcjonowanej liście 542 wpisów** z `final-dubbing-timing-plan-20261026.json`.
2. Dla każdej kwestii obliczono twardy limit długości źródłowej.
3. Kwestie będą iteracyjnie poprawiane od Grupy A (najszybsze do odzyskania 291 linii), poprzez Grupę B, do Grupy C.
4. Każde nowe ujęcie dostarczy:
   - zweryfikowany skrót SHA-256,
   - potwierdzenie $\text{tempo} \le 1.35\text{x}$ z uwzględnieniem 40 ms marginesów,
   - dowód odsłuchu (audition evidence) wykluczający ucięcie słów lub nienaturalną artykulację.

---

## 4. Wytyczne dla Testu Fizycznego VR na PSVR2 (Zintegrowana Paczka V2)

Niezależnie od dalszej pracy nad 542 liniami dialogowymi, **kandydat zintegrowany V2 (`manifest.json` z 130 plikami)** jest w pełni gotowy do sesji testowej w goglach.

### Co należy sprawdzić podczas sesji PSVR2:
1. **Menu Główne i Menu Pauzy:**
   - Działanie wskaźnika laserowego (pointer beam).
   - Czytelność i renderowanie natywnych polskich tekstów na przyciskach.
2. **Aktualny Dubbing w Grze (702 zintegrowane kwestie):**
   - Odsłuch dialogów fabularnych w początkowych etapach kampanii.
   - Weryfikacja czystości dźwięku (brak przesterów, brak trzasków, zachowanie przestrzenności UE3).
   - W kwestiach z 542 jeszcze niezintegrowanych usłyszysz bezpieczne oryginalne audio z gry (brak ciszy, brak crasha).
3. **Efekty Dźwiękowe (Remaster Audio):**
   - Strzały z rewolweru Centurion, snajperki Kasimov, strzelby Spikeshot, karabinu Walkiria.
   - Wystrzał impulsu TMD i odgłosy eksploracji.
4. **Wizualia Broni i TMD:**
   - Tekstury HD broni i celowników.
   - Prawidłowe zakotwiczenie emitera cząstek TMD AgeSquare i brak artefaktów wizualnych w lewej dłoni.

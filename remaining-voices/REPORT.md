# Raport SIDE — dokończenie dubbingu (reszta po v7)

Branch: `agent/polish-remaining-voices` (z origin/main). Zero Steama, zero cudzych plików.
Wszystkie WAV: mono PCM16/24 kHz, peak −1 dBFS. Integrację do banków robi MAIN.

## 1. Trzy upadłe banki — werdykty (`remaining-voices/BANK_VERDICTS.md`)
- **SP_RT1_fx01** (2 assety: „Aaaah!", „Unf!"): niewerbalne wysiłki Renko → **zostaje EN**.
- **SP_RT3_Vignette** (6 kwestii naukowców): winny 1 plik **stereo** (reszta mono).
  Dostarczone: 6 WAV PL + `out/vignette/manifest_rows.json` (tempa 0,60–1,20).
  ASR: 0106/0107/0111 PASS, 0105/0108/0112 REVIEW. SC1-stereo jako mono-downmix (decyzja MAIN).
- **SP_VI6_kismet03**: 262/262 okrzyki (max 7 słów) → **bank zostaje EN**; werbalne w punkcie 3.

## 2. ~184 pominięte (tempo) — `out/skipped/` + `skipped_final.json`
- Metoda: regeneracja + **trim ciszy** (32–89% czasu generacji to padding!) + czyszczenie bełkotu NLLB.
- Wynik: **97 CANDIDATE (≤1,3)** + 53 REVIEW (≤1,85) + 33 STAYS_EN (fizycznie niemożliwe, np. zdanie w 0,5 s).
- Duplikaty assetów między pakietami: 225/226 ten sam tekst (1 plik starcza); wyjątek
  `RT4B_1512_6_KAT` (sp_rt3_katya vs SP_RT4) ma wariant per pakiet.

## 3. Okrzyki bojowe — `out/barks/` + `bark_rows.json`
- Inwentarz SpokenText: 546 unikalnych SD_ (268 werbalnych / 278 nie), `bark-speech.json`.
- Tłumaczenia NLLB + glosariusz militarny (nadajnik/rój), `bark-pl.json`.
- TTS dawcami frakcyjnymi (najdłuższy okrzyk w grupie), temp 0,3.
- Wynik: **93 CANDIDATE + 24 CANDIDATE_645B_DONOR + 139 REVIEW_TEMPO + 12 STAYS_EN**.
- Lekcja: krótkie okrzyki się zapętlają (no-EOS); temp 0,2 nie pomaga, inny dawca czasem tak.
  Reszta to REVIEW do ucha, nie kolejne rundy zgadywania.

## 4. Multiplayer — osobny kawałek (tu: inwentarz + tłumaczenia + TTS gotowe)
- Inwentarz: 897 kwestii, 11 pakietów (`mp-speech.json`).
- Tłumaczenia: 409 werbalnych (`mp-pl.json`).
- TTS: 293 CANDIDATE + 116 REVIEW_TEMPO (`out/mp/` + `mp_rows.json`).

## 5. Nowe intro (`SP_VI_intro_Kismet`)
- Inwentarz 138 (11 narracji + 81 okrzyków + reszta SFX), `intro-speech.json`.
- 10 linii PL wyczyszczonych ręcznie, TTS własnymi oryginałami, poziom emisyjny −1 dBFS.
- ASR: większość PASS; „Nimitz" łamie model we wszystkich pisowniach (testowane 4x) —
  1006/1010 bez imienia, 1005 wariant „mówi baza" + oryginał do decyzji reżysera.
- INTRO_1015 (krzyk) zostaje EN jak RT1_fx01.

## 6. 825b — nowa wersja jednym dawcą 645b, ASR pełnej treści (padło wstępne „Pomocy!").

## Narzędzia (do powtórki)
`gen_vignette/regen_vignette4`, `gen_skipped`, `finalize_skipped`, `translate_batch.py`
(barks/mp), `gen_barks`, `retry_loops`, `retry_645b`, `translate_intro`, `gen_intro`,
`regen_intro[4567]`, `gen_mp`, `gen_825b`, `assemble_all`, `fix_pl_enc`, screeny ASR.

## Znane pułapki (dla następnych)
- NLLB wypluwa pojedyncze bajty cp1250 (ó=F3, ł=B3) — naprawa chirurgiczna per-bajt,
  NIGDY blanket-ftfy (psuje dobre „ł"→„³").
- Dawca 5339b0068ab0840e jest toksyczny (halucynacje niezależnie od tempa) — blacklist.
- Izolowane okrzyki („Pomocy!", „Syreny!") derailują model; doklejone do zdania działają.
- `import gen_barks` wykonuje batch (side effect) — nie importować, kopiować funkcje.

## Decyzja reżyserska 2026-10-15: MULTIPLAYER PO POLSKU
Poprzednie ustalenie („multi zostaje jak jest") ZASTĄPIONE: skoro gotowy jest pełny
dubbing MP (409 kwestii PL w tym PR), w grze zostaje NOWY. MAIN integruje paczki MP.

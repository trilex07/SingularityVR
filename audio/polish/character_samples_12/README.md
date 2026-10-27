# 12 próbek głosowych postaci

Próbki są polskimi kwestiami demonstracyjnymi, wyrenderowanymi lokalnie przez Pocket-TTS Polish 6L.

| Pliki | Postać | Referencja | Charakter |
|---|---|---|---|
| `01`, `02` | Renko / główna postać | własne nagrania użytkownika | neutralny, napięty |
| `03`–`05` | Barisov | rosyjski profil Piper `ru_RU-dmitri-medium` | rozkazujący, briefing, gniewny |
| `06`–`08` | Demichev | odrębny rosyjski profil Piper `ru_RU-ruslan-medium` | chłodny, techniczny, groźny |
| `09`–`10` | Kathryn | ukraiński profil Piper `tetiana` | naukowy, pilny |
| `11`–`12` | Devlin | ukraiński profil Piper `mykyta` | męski, powściągliwy, alarmowy |

Rosyjskie i ukraińskie próbki referencyjne są syntetyczne i pochodzą z modeli Piper udostępnionych na Hugging Face. Nie użyto nagrań prawdziwych aktorów. Polski tekst został wygenerowany osobno przez Pocket-TTS, więc próbki są testem brzmienia akcentu, a nie jeszcze finalnym dubbingiem.

Poprzednia wersja używała `dmitri` dla Barisova i Demicheva oraz domyślnego ukraińskiego speakera dla Devlina. Zostało to rozdzielone: Demichev ma teraz `ruslan`, Kathryn `tetiana`, a Devlin `mykyta` (męski speaker z pakietu ukraińskiego).

Manifest z tekstem, rolą i referencją znajduje się w `manifest.json`.

## Ważna uwaga jakościowa

Próbki `01_renko_neutral.wav` i `02_renko_tense.wav` z poprzedniego przebiegu
zostały odrzucone: każda miała tylko 1,28 s, mimo dłuższego tekstu, a kontrolny
ASR nie rozpoznał polskiej kwestii. Nie należy ich używać do odsłuchu ani jako
referencji. Zastępczy narrator oparty na zatwierdzonym intro jest generowany
przez `tools/audio/generate_intro_reference_candidate.py` do lokalnego katalogu
`audio/polish/repair_candidate_20261013/intro_reference_experiment/`.

Pocket-TTS dobrze przenosi barwę referencji, ale akcent może być delikatny. Jeśli po odsłuchu rosyjski lub ukraiński profil będzie zbyt neutralny, następną iterację zrobimy przez XTTS-v2 cross-language albo przez lekką obróbkę formantów. Nie będziemy przesadzać z akcentem kosztem zrozumiałości polskich kwestii.

# Pilot polskiego TTS

To jest pierwszy odsłuch syntetycznych polskich głosów. Zawiera sześć
przykładowych porad z `Subtitles.int`, aby ocenić barwę, wymowę nazw `TMD` i
tempo. Nie są jeszcze podmienione w grze i nie udają finalnych kwestii
dialogowych.

- `MarekNeural`: narrator męski, spokojny, lekko zwolniony.
- `ZofiaNeural`: narrator żeński, spokojny, lekko zwolniony.
- `Piper Darkman`: dodatkowy męski kandydat do Demicheva.
- `Piper MC Speech`: drugi dodatkowy męski kandydat do Demicheva.

Po akceptacji brzmienia ten sam generator będzie pracował na manifeście
rzeczywistych `SoundNodeWave` z pakietów scenicznych. Wtedy każda linia
dostanie `source_asset`, czas referencyjny i profil postaci.

`character_demo.jsonl` zawiera dwie wymyślone kwestie demonstracyjne dla
Barisova i Demicheva. Służą wyłącznie do oceny profilu głosu; nie są jeszcze
przypisane do konkretnego nagrania z gry.

Próbki `demichev_darkman.wav` i `demichev_mc_speech.wav` są wygenerowane z
tych samych słów, żeby porównać samą barwę. Piper jest lokalnym silnikiem
offline; modele mają osobne karty licencyjne, które trzeba zachować razem z
dystrybucją.

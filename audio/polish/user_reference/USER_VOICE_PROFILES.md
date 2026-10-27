# Profile głosu użytkownika

Przygotowano profile na bazie własnych nagrań użytkownika. Zgoda na użycie tego głosu została udzielona w rozmowie.

## Główna postać

`main_user_neutral.wav` powstał z `spokoj_reference.wav`. To neutralny, naturalny profil głównej postaci.

## Postacie poboczne

Pocket-TTS używa próbek użytkownika jako referencji, a następnie zmienia realizację wypowiedzi:

- `side_user_command.wav` — mocny, rozkazujący sposób mówienia;
- `side_user_cold.wav` — chłodny, powściągliwy sposób mówienia;
- `side_user_shifted.wav` — bardziej pobudzony sposób mówienia.

Profile zachowują wspólną bazę głosu, ale różnią się referencją i ekspresją. To celowe: Pocket-TTS nie tworzy automatycznie całkowicie nowych aktorów; większe zmiany płci, wieku lub formantu wymagają osobnej obróbki i odsłuchu.

## Przypisanie robocze

Robocze mapowanie znajduje się w `user_voice_profiles.json`: główna postać używa profilu naturalnego, a Barisov, Demichev, Kathryn i Devlin używają różnych wariantów Pocket-TTS.

To są gotowe profile demonstracyjne. Nie zostały jeszcze wstrzyknięte do pakietów gry ani do DLL. Następny etap to wygenerowanie właściwych kwestii z listy dialogowej i redirect/repack audio po zatwierdzeniu brzmienia.

## Parametry techniczne

- wejście referencyjne: mono WAV 48 kHz, bez clippingu;
- wygenerowane demonstracje: mono WAV 24 kHz;
- poziom demonstracji: peak -3 dBFS;
- model: `shefowl/pocket-tts-polish-6l`;
- model ma licencję CC-BY-4.0 — zachować informację o źródle przy dalszej dystrybucji.

## Wariant akcentu rosyjskiego/ukraińskiego

Możemy dodać osobną gałąź obsady: polskie kwestie z lekkim rosyjskim albo ukraińskim akcentem. Najlepsza technika to model wielojęzyczny, który dostaje krótką, legalnie udostępnioną próbkę rosyjskiego lub ukraińskiego mówcy jako referencję, ale tekst wejściowy pozostaje po polsku. Nie będziemy kopiować głosu aktora bez zgody ani używać przypadkowych nagrań znalezionych w sieci.

Piper ma osobne modele `ru_RU` i `uk_UA`, ale są to głosy językowe, a nie gotowy mechanizm polskiego tekstu z obcym akcentem. Do testu cross-language lepiej użyć XTTS-v2, który deklaruje polski i rosyjski wśród obsługiwanych języków; dla ukraińskiego trzeba najpierw sprawdzić jakość na konkretnej próbce. Pocket-TTS Polish 6L pozostaje prostszą ścieżką dla wariantów z Twojej własnej próbki.

Docelowo akcent powinien być subtelny i czytelny: rosyjski profil dla wojskowych/technicznych postaci, ukraiński profil dla naukowców i personelu pomocniczego. Najpierw przygotujemy po jednej krótkiej próbce testowej na profil, dopiero potem pełny dubbing.

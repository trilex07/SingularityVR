# Obowiązkowy odsłuch dubbingu

Przed uznaniem polskiego dubbingu za gotowy do integracji trzeba ręcznie odsłuchać kolejkę QA.

Źródła:

- `research/localization/POLISH_DUBBING_REVIEW_QUEUE.md`
- `research/localization/polish_dubbing_review_queue.json`
- `audio/polish/story_voicepack_nllb/`
- `audio/polish/full_voicepack/`

Kolejność:

1. Dialogi fabularne: naturalność polskiego, nazwy własne, akcent i przypisanie głosu.
2. Długie komunikaty lokalizacji oraz opisy perków.
3. Krótkie komendy, liczby i nazwy, dla których Whisper zgłasza konserwatywny `REVIEW_REQUIRED`.

Automatyczny raport potwierdza obecność i poprawny format wszystkich 1886 WAV-ów, ale nie zastępuje odsłuchu człowieka. Do czasu zakończenia odsłuchu status pozostaje `CONTINUE DUBBING QA`.

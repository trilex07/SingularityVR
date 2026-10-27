# Twoje próbki głosu — jak je podać i co z nich zrobimy

## Gdzie wrzucić pliki

Skopiuj swoje nagrania (tylko **Twoje**, z Twoją zgodą) do:

`C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\user_reference\incoming_modulations\`

Proponowana nazwa plików (żeby było wiadomo, co to za emocja):

- `mod_spokoj.wav` — spokojny lektor / narrator  
- `mod_stanowczy.wav` — stanowczy UI, krótkie komunikaty  
- `mod_napiety.wav` — napięcie, alarm  
- `mod_chlodny.wav` — chłodny, techniczny  
- `mod_cieply.wav` — cieplejszy, fabularny (nie intro — intro zostaje osobno)

Możesz dodać krótki `notes.txt` w tym folderze (1–2 zdania na plik: „to ma być lektor menu”, „to Renko pod stresem”).

## Format techniczny

- **WAV**, mono, **48 kHz** lub **24 kHz**, PCM16 (bez MP3 w środku).  
- **10–60 s** czystej mowy (minimalny szum, bez muzyki i gry w tle).  
- Jedna modulacja = jeden plik; nie muszą być idealne — wybierzemy najlepszy fragment.

## Co zrobimy po przesłaniu (lokalnie, bez wysyłki na zewnątrz)

1. Wycinamy najczystszy odcinek (~8–18 s, jak przy intro).  
2. Test Pocket-TTS Polish 6L: 3–5 zdań lektora + 1 linia fabularna na każdą modulację.  
3. Reel A/B: Twoja modulacja vs Natan/Magda/spokoj vs obecny lektor UI.  
4. **Ty wybierasz** zwycięzcę; dopiero potem ewentualny pilot na 20–30 linii menu.

Skrypt (po wrzuceniu plików):

```powershell
cd C:\Users\Trilex\vr-mods\SingularityVR-cutscene
python tools/audio/prepare_user_modulation_candidates.py
```

Wynik trafi do (ignorowane przez git, tylko u Ciebie):

`audio/polish/user_reference/modulation_candidates_YYYYMMDD/`

## Intro

`SNG_intro_polish_voice.wav` pozostaje **zatwierdzonym** nagraniem intro. Nowe modulacje służą głównie **lektorowi UI**, ewentualnie Renko — nie zastępują intro bez Twojej wyraźnej decyzji.

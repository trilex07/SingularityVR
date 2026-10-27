# Legal Polish cast pack (NC free mod) — 2026-10-14

Mod **non-commercial, free** → używamy głosów **NC / CC0 / CC-BY** z jawną atrybucją.  
**Nie klonujemy** rozpoznawalnych zmarłych lektorów (TV, radio, aktorzy). Szukamy **podobnej roli** (lektor, żołnierz, naukowiec) z **legalnych głosów TTS**.

## Odsłuch — jeden plik

`C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\cast_legal_20261014\cast_listen_reel.wav`  
Indeks: `cast_listen_reel.json`  
Manifest: `CAST_MANIFEST.json`

## Proponowane przypisanie (do akceptacji po odsłuchu)

| Rola w modzie | Referencja | Licencja | Uwaga |
|---|---|---|---|
| **Lektor UI (stanowczy męski)** | `references/rhvoice_natan_reference.wav` | Natan **CC0** | Pełna jakość po instalacji SAPI (patrz niżej) |
| **Lektor UI / PA (kobiecy)** | `references/rhvoice_magda_reference.wav` | Magda **CC BY 4.0** | Atrybucja w README moda |
| **Kathryn (alternatywa bez uk-accent)** | ta sama Magda + Pocket | CC BY 4.0 | Tylko jeśli odsłuch wygra nad `uk_tetiana` |
| **Żołnierz / Spetsnaz pool** | Natan ref | CC0 | Krótkie rozkazy |
| **Naukowiec / cywil (kobieta)** | `references/piper_gosia_reference.wav` | Piper MODEL_CARD | Parametryczny TTS, nie klon osoby |
| **Lektor neutralny (alt)** | `references/piper_mc_speech_reference.wav` | Piper MODEL_CARD | Męski, czytelny |
| **Demichev (NC alt)** | `references/piper_darkman_reference.wav` | Często **NC** | OK przy **darmowym** modzie |
| **Barisov / Devlin** | bez zmian: `accent_reference/` + `character_samples_12/` | Piper + Pocket | Akcent RU/UK w polskim tekście |
| **Intro fabularne** | `SNG_intro_polish_voice.wav` | Twoje nagranie | Bez zmian |

Demos Pocket (jak br br brzmi w grze):  
`audio/polish/cast_legal_20261014/pocket_demos/` — m.in. `lektor_natan_ref.wav`, `lektor_magda_ref.wav`, `kathryn_magda_ref.wav`.

## RHVoice Natan / Magda — pełna wersja (opcjonalnie, 1× admin)

Instalatory są już pobrane:

- `tools/rhvoice_sapi_local/installers/natan_setup.exe`
- `tools/rhvoice_sapi_local/installers/magda_setup.exe`

Uruchom **jako administrator** (instalacja SAPI w Windows), potem:

```powershell
cd C:\Users\Trilex\vr-mods\SingularityVR-cutscene
python tools/audio/build_legal_polish_cast.py
```

Skrypt wygeneruje długie `rhvoice_*_reference.wav` przez SAPI zamiast hybrydy demo+Piper.

**Bez admina** (obecny stan): referencje `rhvoice_*_reference.wav` to **hybryda** oficjalnego demo RHVoice + polski tekst z Piper — legalne, ale to **nie** pełny render RHVoice.

## „Zmarli / ci co się zgodzili” — co wolno w praktyce

| Źródło | Zgoda / prawo | Do czego używamy | Czy „klon osoby”? |
|---|---|---|---|
| **RHVoice Natan** (Edwin Tarka) | **CC0**, głos stworzony do TTS | Lektor, żołnierz | **Nie** — to licencjonowany głos syntetyczny |
| **RHVoice Magda** (Magda Roczeń) | **CC BY 4.0** | PA, Kathryn-alt, UI | **Nie** — ten sam model |
| **LibriVox / CML-TTS** | Wolontariusz oddaje **nagranie do PD**; dataset **CC-BY** | `pocket_cml_reference.wav` — **anonimowa** próbka | **Nie** identyfikujemy mówcy (warunek datasetu) |
| **Piper mc_speech / gosia / darkman** | MODEL_CARD Hugging Face | Parametryczne postacie | **Nie** osoba publiczna |
| **Twoje nagrania** | Zgoda w projekcie | Intro, spokoj | Twoja tożsamość — za Twoją zgodą |
| **Zmarli celebryci** (np. znany lektor TV) | Brak zgody na syntezę | **Nie używamy** | — |

LibriVox: [Public Domain](https://librivox.org/pages/public-domain/) — nagranie w PD; to **nie** pozwala marketingowo mówić „głos jak X”, ale pozwala używać **anonimowych** nagrań PD/CML jako materiału treningowego/referencji z atrybucją.

## Następny krok po Twoim odsłuchu

1. Wybierz lektor UI: **Natan**, **Magda**, **mc_speech**, **CML**, lub zostaw **spokoj**.  
2. Jeśli Natan/Magda wygrywają — podpinamy wybrany `references/*.wav` w generatorze lektora (osobny mały pilot, nie całe 1886 od razu).  
3. README moda: krótka atrybucja RHVoice Magda (CC BY), Pocket-TTS (CC BY), Piper, CML-TTS.

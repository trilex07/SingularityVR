# SIDE → MAIN — VR stills `.bik` (BIKi): boot, loading, 4K-exp

Worktree: `SingularityVR-bink-stills` · branch: `side/bink-stills-vr` ·
**no Steam/DLL/INI/movie install from SIDE. Runtime test + install = MAIN.**

## Slot evidence (dlaczego wystarczą 2 pliki na 3 życzenia)

Poprzedni handoff (`fix/boot-logo-vya-suffix`,
`docs/handoffs/side-boot-logo-bik-handoff.md`) udowodnił: czarna karta
pre-menu i pierwszy film SINGULARITY to **jeden asset**,
`RvGame/Movies/SNG_logo.bik` (1280×720, 11 s, 30 fps, 330 ramek, 1 strumień
audio 44,1 kHz stereo). „Pre-open" i „pierwsze intro (krótki filmik po boot,
zanim menu)" użytkownika to więc ten sam slot.

## Kandydaci (offline L1/L2 PASS, runtime PENDING)

| Plik | Slot (`install_as`) | Źródło | Wideo | Audio | SHA-256 |
|---|---|---|---|---|---|
| `SNG_logo_vr2.bik` | `RvGame/Movies/SNG_logo.bik` | `references/Metaliczne logo SINGULARITY VR.png` (1672×941) | 1280×720, 11 s, 30 fps, 330 f | stock remux (44,1 kHz stereo, pakiety stock, wideo nietknięte) | `B0D35C12229685142808C82252FDFEEAD9ACFE393B11389CD60B204F26C618EF` |
| `loading_screen_vr2.bik` | `RvGame/Movies/loading_screen.bik` | `references/Singularity_VR_4K_3840x2160.png` (downscale 3×) | 1280×720, 2 s, 24 fps, 48 f | brak (jak stock) | `4DBDFADA32C493D6D7F779F6B82A628987CBADE8732ACA3C60F4826F49D858EB` |
| `loading_screen_4k.bik` | `RvGame/Movies/loading_screen.bik` | jw., natywne 3840×2160 | 3840×2160, 2 s, 24 fps, 48 f | brak | `069253195939380407826A9DDDAB207702A3FA02287DC1B109250D17427234D0` |

Pełne SHA w `offline-deliverables/stills-vr-v1/MANIFEST.json`; klatki dowodowe
dekodowane z FINALNYCH bików w `proof-from-bik/` (po 3 na kandydata).

Metoda: stills → MKV (ffmpeg ffv1, keyframe tylko pierwszy) → `binkenc`
(LibBink open-source Bink 1, `third_party/libbink-src`) → dla logo remux
stock-audio (`tools/stills/remux_bink_stock_audio.py`, wierna kopia z brancha
boot-logo) → walidacja: magia `BIKi`, kodek, geometria, fps, układ audio,
pełny dekod, klatki dowodowe. Regeneracja:
`py -3 tools/stills/prepare_vr_stills_bink.py [--only NAZWA]`.
Pośrednie MKV/WAV nie są commitowane (`work/`, `.gitignore`).

## Ostrzeżenia

- **`loading_screen_4k` to EKSPERYMENT.** Bink 1 w 4K nigdy nie był
  slot-proven w dekoderze UE3 z 2010 (alokacje/pamięć/wydajność). Zalecany
  `loading_screen_vr2` (720p). 4K instalować tylko do testu, z backupem.
- `SNG_logo_vr2` to statyczna plansza 11 s (stock miał animację + ten sam
  dźwięk). Zamierzony efekt: karta tytułowa VR.
- Stock SHA do weryfikacji backupu: `SNG_logo.bik`
  `FE40BA3B4521C4D0BFAC9EFD65B7BAF2C9F0F07F2E637A703DB1E7B29EA2B03C`,
  `loading_screen.bik`
  `764E701B5049F182968D2DFFF4EE9ACCD13443B0D68B66B3FD1C0FB2F059E27D`.

## Instalacja MAIN (L4)

1. Backup stock `.bik` + re-hash do SHA powyżej.
2. Skopiuj kandydata pod nazwę stockową do `RvGame/Movies/`.
3. Boot: karta + film logo pokazują metaliczne `SINGULARITY VR`; loading:
   art TMD 4K. Przywróć stock przy niepowodzeniu, re-hash.

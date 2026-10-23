# SIDE → MAIN — enemy/gore upscale batch (8 DDS + brama agemap)

Worktree: `SingularityVR-enemy-upscale` · branch: `side/enemy-upscale-batch` ·
**zero Steam, zero instalacji. Runtime/injector = MAIN lub właściciel assetów.**

## Zbudowano (`processed/enemy-upscale-batch/`, MANIFEST.json)

| Tekstura | Native → cel | Format | Metoda | VRAM |
|---|---|---|---|---|
| `phasetick_d/n/s/e` | 512 → 2048 | DXT1/DXT5/DXT1/DXT1 | sharpen / renorm-normal / sharpen / sharpen | 2.7/5.3/2.7/2.7 MB |
| `bsplat3_d`, `t_blood_splat02_d` | 256 → 2048 | DXT5 | sharpen | 5.3 MB ×2 |
| `bsplat_decal_d` | 128 → 2048 | DXT1 | sharpen | 2.7 MB |
| `revert_t` (transmission) | 512 → 1024 | DXT1 | Lanczos (maska danych) | 0.7 MB |

Suma: **27.3 MB**. Każdy DDS: pełny łańcuch mipów do 4 px, rozmiar
bajtowy zgodny z fourcc, FNV-1a64 mip0 w MANIFEST (bramka injectora),
kontrola wierności: downsample→native MAD 1.2–2.7 (szum samej kompresji
BC — brak wymyślonej treści, brak korupcji).

## Korekty względem propozycji (dowody)

- **Specular Reverta wycofany**: materiał `ch_revert...ch_revert_env` nie ma
  slotu `specularmap` (tylko diffuse/normal/transmissionmask) — tekstura
  `_s` nigdy nie zostałaby spróbkowana. Zamiast tego `revert_t` 512→1024.
- **8K nie ma i nie będzie** w tym batchu (patrz `docs/enemy-detail-proposal.md`).
- **`agemap_body/head`: PENDING.** Źródeł nie ma lokalnie, a lokalne kopie
  `Textures_P*.tfc` są niekompletne (UModel: assertion na bulk read).
  Ekstrakcja dla MAIN (read-only, 1 komenda na teksturę):
  `umodel -path=<scratch z hard-linków pakietów> -export -dds -out=<dir> RvGameMPLoader_SF agemap_body`
  (oczekiwane: PNG 256 DXT1 → batch robi 1024 sam; ścieżki już wpisane
  w `tools/enemy_upscale_batch.py`, status trafi do MANIFEST `pending_*`).

## Integracja (nie tykać plików innego worktree — scalić ręcznie)

`MANIFEST.json → targets[]`: gotowe wpisy `{name, format, px, fnv1a64_mip0}`
do `asset-tools/texture-injector/targets_generated.h` (gałąź assetów).
Regeneracja: `py -3 tools/enemy_upscale_batch.py [--only NAZWA]`
(deterministyczne: te same FNV co pierwsze przejście).

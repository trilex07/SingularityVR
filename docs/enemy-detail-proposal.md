# Propozycja: detale wrogów + odcięte kończyny (Side, bez kodu)

Status: **propozycja na dowodach, do akceptacji**. Zero zmian kodu i zero
instalacji na tym etapie. Podstawa: gałąź `plates-l3-main @ b850ac2`,
`research/texture-upscale/manifests/classification.json` (gałąź
z pipeline tekstur), `processed-textures/` (58 DDS), commit `6245c25`
(Revert 4K, Spetsnaz/PhaseTick).

## 1. Odpowiedź: NIE, wrogowie nie mają tekstur 8K

Pełny inwentarz tekstur postaci (stan na dziś):

| Wróg | Diffuse | Normal | Specular | Rozmiar | Uwagi |
|---|---|---|---|---|---|
| Revert | `revert_d2` | `revert_n2` | **BRAK** | 4K | max w projekcie; bez specular |
| Spetsnaz_MP | Body/helmet `_d` | Body/helmet `_n` | Body/helmet `_s` | 2K | pełny triplet + `helmet_e` |
| PhaseTick | `phasetick_d` | `phasetick_n` | `phasetick_s` | 1K | + niesklasyfikowane `phasetick_e` |
| (ludzie?) | `agemap_body`, `agemap_head` | — | — | ? | UNKNOWN, do zbadania |

W całym projekcie zero tekstur 8K (maks to 6 plików 4K: bronie/normale).
`Quality=8K` w INI nie ma treści. Żołnierze SP, cieżcy, bossowie: brak
w pipeline. Tekstur ran/mięsa/kości: brak (krew to tylko decal
`bsplat3_d` **512 px**).

## 2. Rekomendacja: NIE iść w 8K dla wrogów

- Koszt VRAM: 8K BC1+mipy ≈ 85–90 MB, normal BC5 ≈ 170 MB **na teksturę**;
  w ścieżce injectora D3D9 z 2010 to ryzyko crashy/swapowania.
- Zysk: żaden na dystansie walki — rozdzielczość kątowa gogli nie pokaże
  różnicy 4K vs 8K na wrogu 3–10 m od gracza.
- Lepszy wydatek tego samego budżetu (kolejność wg stosunku zysk/koszt):
  1. **Specular dla Reverta** (jedyna dziura w triplecie; bez `_s` skóra
     wygląda płasko w świetle VR).
  2. **PhaseTick 1K → 2K** (najsłabszy wróg w zestawie).
  3. **Krew 512 → 2K + warianty** (`bsplat` decal — widać po KAŻDYM
     zabójstwie, więc zwrot największy).
  4. **Klasyfikacja `agemap_body/head`** (jeśli to blend starzenia ludzkiej
     skóry, odblokowuje detale wszystkich ludzkich wrogów).
  5. **Pokrycie**: żołnierze SP / ciężcy / bossowie (inwentarz z pakietów,
     tym samym pipeline co commit `6245c25`).

## 3. Odcięte kończyny: najpierw brama dowodowa, potem treść

W modzie **nie ma kodu gibów** (grep `Gib|Dismember|Sever` w `d3d9.cpp`:
puste) i nie wiadomo, co natywnie potrafi silnik w tej grze. Proponowany
pakiet dopiero PO przejściu bramy:

**Brama (MAIN/runtime, 1 sesja):** dump property klasy wroga
(`DumpClassProperties`, ta sama metoda co run 235 dla TMD) + odpowiedź:
czy istnieją klasy/meshy gibów i capów ran? Bez tego nie ma co teksturować.
Falsyfikator: brak klas gibów = kończymy na decalach krwi (punkt 3 powyżej),
nie budujemy fikcyjnego systemu.

**Pakiet gore (po bramie):** tekstury wnętrza rany (mięso/kość, cap na
kikut), materiał krwi na gibach, trwalsze decale krwi pod ciałem.
Wszystko default-OFF, testy jak reszta assetów (mip0 gate, target-only
deploy, proof z dekodera).

## 4. Decyzje dla Ciebie

- [ ] Kierunek 8K dla wrogów: odradzam (powyżej). Zielone światło na punkty 1–5?
- [ ] Brama gibów: zlecić sesję runtime (MAIN) z dumpem klas wroga?
- [ ] Krew 2K mogę przygotować od razu jako kolejny asset (ta sama procedura
      co loading/biki: offline, manifesty, proof) — robić?

# Singularity VR standalone texture injector (research)

A 32-bit `dinput8.dll` proxy that substitutes selected game textures with
enhanced DDS files. It is deliberately standalone: the game statically imports
`DINPUT8!DirectInput8Create` and no `dinput8.dll` ships in `Binaries`, so this
DLL loads at process start and co-exists with the VR mod's `d3d9.dll`.

Nothing about the VR renderer is touched. Reverting is deleting the deployed
files (see `restore.ps1`).

## How matching works

`IDirect3DDevice9::SetTexture` is hooked. For every texture bound, the mip0 DXT
bytes are hashed (FNV-1a 64) and compared against `targets_generated.h`, which
lists the dimensions, format and hash of the original textures. On a match the
injector builds the replacement DDS into a managed texture and binds that
instead. Unknown textures pass straight through.

## File naming (one rule, enforced both sides)

The replacement file the injector looks up is

    <texturesDir>\<target name>_<quality tier>.dds

with the tier suffix coming from `texq_naming.h`:

| tier | suffix |
|------|--------|
| 2x   | `_2x`  |
| 4K   | `_4K`  |
| 8K   | `_8K`  |

`build.ps1` computes the very same suffix when it deploys, and
`tests/test_tier_naming.py` compiles `texq_naming.h` and runs a real deploy into
a scratch directory for all three tiers to prove the two agree.

## Build and deploy

```powershell
# compile the DLL (x86 /O2, needed for the SHA provenance check)
.\build.ps1

# deploy the material remaster package plus the pipeline candidates
.\build.ps1 -Install -Quality 2x -RemasterDir ..\..\material-remaster
```

`-Install` needs a game `Binaries` folder and is MAIN-owned; see `AGENTS.md`.

## Material remaster package

The three prototype materials live in `material-remaster/` in the repository:
`weapon_shotgun`, `glove` and `env_door`. Each has a `manifest.json` with
SHA-256 checksums, the true-mip0 proof for every source map, the DDS validation
of every produced file, fidelity measurements against the original and an
`evidence` block labelling each claim VERIFIED / ESTIMATED / NOT TESTED.

Only the diffuse and normal maps are wired for injection, because those are the
two whose source is provably true mip0. The specular maps are exported one mip
lower by UModel, so injecting them would reduce resolution; enhanced specular
maps are produced for review (`*_specular_review.dds`) but are never deployed.

## Licence note

`material-remaster/**/*.dds` and `*_compare.png` are derived from game textures
owned by their publisher. They are in this repository for review of the
prototype only and are not licensed for redistribution.

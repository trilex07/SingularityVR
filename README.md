# Singularity VR — by Trilex

**Play Singularity in VR with tracked hands, physical weapon handling and OpenXR stereo.**

A VR conversion of *Singularity* (2010, Raven Software, Unreal Engine 3),
created and maintained by **Trilex ([trilex07](https://github.com/trilex07))**.
The mod brings motion controls, manual reloading, a wrist-mounted TMD and
VR menus to the original campaign.

**Mod VR autorstwa Trilexa.** Obsługuje PSVR2 na PC, ręczne przeładowanie,
śledzone dłonie, TMD oraz polską lokalizację i rozbudowany pakiet dubbingu.

**First-release baseline accepted on 26 October 2026.** The combined V7 build
passed the user's latest PSVR2 session and is the current stable development
baseline. The installer and downloadable first-release package are being
prepared; they have not been published yet. A source ZIP is not an installer.

[Downloads / releases](https://github.com/trilex07/SingularityVR/releases) ·
[Report a bug](https://github.com/trilex07/SingularityVR/issues) ·
[Accepted baseline](docs/release/first-release-baseline-20261026.md) ·
[Development status](STATUS.md)

## Features

### VR and hands

- Native OpenXR stereoscopic rendering with 6DoF head and controller tracking.
- Independent tracked gloves, physical knife handling and weapon holsters.
- Original-style left-hand TMD with a short gauntlet and animated hardware;
  use the TMD while keeping the weapon in the right hand.
- Two-handed weapon support and scoped-rifle aiming with R1 stabilization.
- Cinema presentation for videos and scripted scenes, animated story/medkit
  hands, and corrected in-world projector stereo.
- Dynamic world and first-person shadows, plus targeted input and renderer
  optimizations.

### Weapons and interaction

- Controller-directed weapon aiming and muzzle effects.
- Manual reloads for **Centurion, Valkyrie, Volk and Kasimov**: take fresh
  ammunition from behind the left shoulder and insert it into the weapon.
- Centurion cylinder opening, upward case ejection and gesture-based closure.
- Weapon-mounted 3D ammunition displays and a smaller translucent health HUD
  that appears when the left wrist is raised; contextual hiding while supporting
  a weapon or using the scope.
- An expanded ordinary-weapon inventory and remastered selection wheel, with
  native WeaponLocker integration.
- Native gore/dismemberment and controller haptic feedback.
- Seeker support and native projectile integration are included; its guided
  rocket behavior and original ammunition rules remain in use. Broader Seeker
  retrieval and save/load coverage is still limited.

### Menus, image and sound

- VR Settings with controller navigation, laser selection and optional PL/EN
  reload help/tutorials.
- Remastered Settings, weapon-wheel icons, main/pause menu buttons and backgrounds,
  with contextual Sense prompts and legacy pad-label cleanup.
- Live NIS sharpening/upscaling controls and optional OLED color presets.
- Selectable **2X / 4K / 8K** texture quality for supported replacements;
  coverage and actual source detail vary by asset. Texture-quality changes
  require a restart.
- A fitted right-eye desktop/TV mirror and Steam screenshot support.
- Polish UI, subtitles and note glyphs; selection of the original Polish audio
  tracks in supported Bink movies.
- Expanded Polish campaign dubbing: **1,248 native replacements across 116
  banks**, combining new and previously preserved takes.
- **Seven sound-remaster sets** integrated with native audio playback.

These features describe the complete integrated payload. Some are configurable,
require additional assets, or have narrower test coverage than the main PSVR2
setup. One successful session does not certify every level or dialogue line.

## Requirements

- Windows 10 or 11 and a legally owned PC copy of *Singularity*.
- An OpenXR headset with its active runtime. The current baseline is tested
  with **PSVR2 + PC adapter + SteamVR OpenXR + the Steam game**.
- [Visual C++ 2015–2022 Redistributable **x86**](https://aka.ms/vs/17/release/vc_redist.x86.exe).
  The game and mod are 32-bit, including on 64-bit Windows.

Other headsets, runtimes and game editions have less coverage; the current GOG
build is not verified. SteamVR is used through OpenXR.

## Installation

**Wait for the complete first-release package and its installer instructions.**
The existing source archive and legacy `packaging/make-release.ps1` output do
not reproduce the full accepted installation.

The runtime lives beside `Singularity.exe` in `Binaries` and includes
`d3d9.dll`, `dinput8.dll`, `openxr_loader.dll`, `SingularityVR.ini` and
`texture_override.ini`. Hands, HUD, localization, textures and audio require
additional assets and, where applicable, locally generated game-package
patches. Replacing a DLL alone is not a complete update.

Close the game before any update. Back up the affected game files and your
settings. Development deployments use the project's
[guarded deployment tool](tools/deploy-singularityvr.ps1) with explicit profiles,
change plans, timestamped backups and hash verification; see the
[deployment workflow](docs/STEAM_MAIN_DEPLOY_WORKFLOW.md).

Start the headset/runtime, then launch Singularity through Steam. Keep
**High Quality Decals = ON** for the accepted dynamic-shadow configuration.
Headset resolution is detected and cached in the INI. Do not import another
user's `AutoResX` / `AutoResY` values.

For uninstalling a complete installation, restore its backed-up game packages
and assets as well as remove the mod runtime. Removing only `d3d9.dll` disables
VR but does not revert patched UI or audio packages.

## PSVR2 controls

The accepted right-handed manual-reload profile uses these controls. Native
TMD unlocks, energy and item availability still apply.

- **Left stick:** movement. **Right stick:** turning; up to jump.
- **R2:** fire; select an item with the menu laser.
- **Circle (O):** start manual reload; back in menus.
- **Left Grip:** take ammunition behind the left shoulder, or support the weapon
  when the left hand is near its support position.
- **R3:** hold to open the weapon wheel, select a slot, then release to equip.
- **Cross (X):** use a health item.
- **L2:** TMD Impulse. **Square:** TMD Age / Renovate.
- **R1 with a supported Kasimov:** steady scoped aiming; bring the optic toward
  the eye to use the scope.
- **Triangle:** menu. Use the laser and R2 in VR Settings.

**Centurion:** press O to open the cylinder, raise the muzzle to empty the cases,
take fresh ammunition behind the left shoulder, insert it, then flick the weapon
to close the cylinder.

**Valkyrie / Volk / Kasimov:** press O to eject the magazine, take a fresh one
behind the left shoulder and insert it into the weapon. These paths do not use
the Centurion closing flick.

The knife is at the left hip; the ordinary-weapon holster is on the right.
The optional in-VR tutorial and reload hints can be selected in VR Settings.
Settings are stored in `Binaries/SingularityVR.ini`; some changes require a
restart. L3+R3 can recenter when `RecenterChord=1`; it is not a screenshot binding.

## Coverage and remaining limits

- The latest combined PSVR2 session was accepted by Trilex. It was not an
  exhaustive campaign, per-weapon save/load, or dialogue-listening test.
- Polish dubbing is expanded, **not complete**: ten rejected replacement takes
  retain earlier/base audio, and five further native identities remain uncovered.
  The current generated batch uses Pocket-TTS Polish 6L; it is not an XTTS release.
- One proposed TMD Impulse sound replacement is withheld. The original core
  package and its native sound are preserved because the modified core failed
  startup tests.
- HD replacements are selective and can contain upscaled source material.
  In particular, the Kasimov diffuse candidate is interpolated from the highest
  available extracted mip, not recovered native 4K artwork.
- A brief dip can remain in the early collapsing-building sequence. Performance
  depends on scene, headset resolution and hardware; no universal 90/120 Hz
  guarantee is made.
- Later scenes, Seeker retrieval/save-load, controller hotplug and other runtime
  combinations need more coverage.
- The first-release installer and distributable asset/patch assembly are pending.

## Reporting problems

[Open an issue](https://github.com/trilex07/SingularityVR/issues) with the mod
build, headset/runtime, game edition, relevant settings and reproduction steps.
Say whether the issue affects one eye, both eyes, or just the monitor/capture.
Include the relevant part of `%LOCALAPPDATA%\SingularityVR\view_matrix.log`.

For headset tests, complete the agreed route and close the game normally before
returning to chat; switching focus mid-test can interrupt VR rendering.

## Building and contributing

```powershell
.\spikes\view_matrix\build.ps1
```

Requires Visual Studio 2022 C++ x86 tools, a Windows SDK and the OpenXR SDK.
Set `OPENXR_SDK` or use the [SDK resolver](tools/Resolve-OpenXrSdk.ps1).
Production artifacts use **x86 PE32 /O2**; the build records provenance and
SHA-256 in `spikes/view_matrix/d3d9.build.json`. Building does not deploy the mod
or assemble the complete release payload.

Start with [AGENTS.md](AGENTS.md), [STATUS.md](STATUS.md) and the
[skill index](docs/AGENT_SKILLS_MANIFEST.md). Engine research is in
[ENGINE_NOTES.md](ENGINE_NOTES.md); historical experiments are in
[HISTORY.md](HISTORY.md). Offline tests, installed-game evidence and physical
headset acceptance are recorded separately.

## Credits and licences

**Singularity VR is a mod by Trilex / trilex07.** Thanks to contributors and the
VR modding projects referenced in
[Reference VR implementations](docs/REFERENCE_VR_IMPLEMENTATIONS.md).

Project code is MIT; see [LICENSE](LICENSE) and
[THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt) for component-specific terms.
Original Singularity assets remain the property of their respective owners.
Extracted assets and locally patched game packages are separate from the mod's
source licence and are not automatically redistributable under MIT.

Not affiliated with or endorsed by Activision or Raven Software.

# Singularity VR

A native VR mod for _Singularity_ (2010, Raven Software, Unreal Engine 3).
The 32-bit `d3d9.dll` proxy renders the game through OpenXR with stereoscopic
6DoF head tracking, motion controllers and controller-directed weapons.

**Alpha — actively developed and tested on PSVR2 with the PC adapter,
SteamVR and the Steam version of the game.** Other hardware and later levels
have less coverage. A successful build is not a completed headset test.

[Releases](https://github.com/trilex07/SingularityVR/releases) ·
[Report a bug](https://github.com/trilex07/SingularityVR/issues) ·
[Current development status](STATUS.md)

## Requirements

- Windows 10 or 11 and a legally owned PC copy of Singularity.
- An OpenXR headset and its active OpenXR runtime. Current physical testing uses
  PSVR2 through SteamVR; earlier testing used Quest 3S with Virtual Desktop/VDXR.
  The current GOG build and other headset/runtime combinations are not verified.
- [Visual C++ 2015–2022 Redistributable **x86**](https://aka.ms/vs/17/release/vc_redist.x86.exe).
  The game and proxy are 32-bit, even on 64-bit Windows.

SteamVR is supported through **OpenXR**. A native OpenVR renderer is not required.
A wireless router is relevant only to a wireless headset setup, not PSVR2.

## Installation and updating

Use the instructions and payload attached to the specific release you download.
A GitHub source archive is not a ready-to-install mod.

The core runtime files belong beside `Singularity.exe` in `Binaries`:

```text
Binaries/
  d3d9.dll
  openxr_loader.dll
  SingularityVR.ini
  vr-assets/             # when supplied by the feature/release payload
```

**The current development installation contains more than these three files.**
Original UI/font packages can be patched locally, and localization, audio,
glove/knife and HUD features have additional assets. Copying only a newly built
DLL does not reproduce the complete tested installation.

The existing [make-release script](packaging/make-release.ps1) still packages
only the two DLLs and INI. It does not yet assemble every current feature or
generate all required game-package patches. Do not treat that archive as a
complete distribution of the development setup.

For development integration, [the guarded deployment tool](tools/deploy-singularityvr.ps1)
checks an accepted profile and an explicit change plan, creates timestamped
backups and verifies hashes. Read the [deployment workflow](docs/STEAM_MAIN_DEPLOY_WORKFLOW.md)
and [accepted-profile guard](docs/accepted-profile-guard.md) before using it.
Close the game before replacing any installed file. Preserve your own INI and
asset payload when updating.

Start SteamVR with the headset connected, then launch Singularity through Steam.
Keep **High Quality Decals = ON** for dynamic shadows. Resolution is detected
from the headset and cached; if the first launch has the wrong resolution,
close the game normally and launch again.

To uninstall a full development installation, restore its backed-up game packages
and configuration as well as remove the mod runtime files. Deleting `d3d9.dll`
alone disables the proxy but does not undo UI/font/audio package changes.

## Current features

The following systems are implemented; acceptance applies to the tested scenes
and configuration, not automatically to every weapon, level or headset.

- Native per-eye stereo, 6DoF head tracking, head roll, snap/smooth turning,
  controller aiming and live weapon alignment.
- Controller-tracked hands/gloves, weapon holstering, a left-hip knife and
  physical melee interaction.
- Native pause/menu integration and in-headset VR settings. The remastered
  VR Settings plates (P0) and localized styling (P1), plus the remastered
  four-gun weapon wheel (P2), all default-off in source, were **physically
  passed on PSVR2**; more detailed weapon silhouettes are an accepted
  cosmetic follow-up.
- Bounded OpenXR TMD-power controller pulses with native power
  classification and local-player-only ownership: **physically passed on
  PSVR2** in a combined session (short Impulse-scale pulses; cooldown and
  foreign-owner rejection verified in receipts).
- Reconstructed dynamic world and first-person shadows. Keep High Quality Decals
  enabled; see [accepted shadow/input fixes](docs/accepted-shadow-input-fixes.md).
- Input-enumeration caching to avoid the previously measured button-held stalls.
- Cinema presentation for videos and scripted scenes, with an anchored screen
  and return to stereo gameplay.
- Polish localization, locally installed voicepacks and Polish note glyphs.
  These require their corresponding package/audio payloads; an INI switch alone
  does not install them. See [voicepack installation](docs/story-xlingual-install-and-recovery-acceptance.md)
  and [note fonts](docs/polish-note-glyphs.md).
- Manual reload work for Centurion, Valkyrie, Volk and Kasimov. Centurion has
  received repeated physical testing; the remaining weapon paths need their own
  full gameplay acceptance. See [native reload integration](docs/centurion-native-reload.md)
  and the [reload handoff](docs/handoffs/centurion-reload-compact.md).
- First-building projector stereo correction and measured renderer optimizations;
  see [projector evidence](docs/projector-stereo.md).
- Contextual menu arrow suppression and laser/ring alignment passed the latest
  reported PSVR2 test. See [cursor evidence](docs/contextual-cursor.md).

Some features are opt-in and require an integration profile. Their presence in
source does not imply that every published release enables or includes them.

## Controls and configuration

Bindings vary with controller type, selected options and integration profile.
The older Touch-only layout is not a reliable description of the current
PSVR2/manual-reload setup.

With the requested right-handed `ReloadControlLayout=1` profile:

- Right **Cross (X)** uses a health item.
- Right-stick **up** jumps.
- Right **Circle** operates manual reload.
- Fresh reload ammunition is taken with the left hand from behind the left shoulder.
- Centurion uses cylinder opening, upward case ejection, fresh-ammo insertion and
  a closing gesture. The other weapon reload paths use their weapon-specific
  insertion procedure; their physical validation remains separate.
- Left-hip knife and right-side weapon holster remain separate from the ammo zone.

Use the in-headset settings and the documentation accompanying your tested
profile for other actions. Recentring can use L3+R3 when `RecenterChord=1`;
that chord is not a screenshot shortcut.

Settings live in `Binaries/SingularityVR.ini`. Restart after changing options
that are not exposed as live settings. Avoid duplicate keys: Win32 INI reads
can silently use the first occurrence. `AutoResX` and `AutoResY` are a cache
for your headset and must not be copied from another user's setup.

## Known issues and work in progress

- **Gameplay location/information HUD repositioning:** implemented offline
  (`ShowInformation` → `locationTxt` → `LocationTextDisplay`; targeted GFx
  path `_root.txtMsgContainer.LocationTextDisplay`) with the movie-identity
  checks and native GFx setter safeguards preserved. **OFFLINE PASS,
  PHYSICAL PENDING** — `[Render] VrGameplayTextSafeArea=0` and
  `[Diagnostic] GameplayTextProbe` must stay default OFF; with those
  switches disabled no gameplay HUD coordinates are modified and no extra
  text is drawn. It will be activated only in a later controlled headset
  session.
- **TV/desktop mirror, Steam screenshots and health HUD:** confirmed in the
  October 18 PSVR2 candidate test. Steam VR captures required opaque alpha on
  the final XR copy (`XrCaptureOpaqueAlpha=1`); the source RGB was already correct.
  The smaller translucent health plate appears on face raise. These scoped
  results do not promote the temporary test bundle to a persistent install.
  See [capture evidence](docs/menu-test-regression-20261018.md).
- **Notes and control hints:** the latest test exposed note pages clipped at
  opposite eye edges and outdated pad/reload labels. Corrections are in progress.
- **Knife melee:** a prior test reported right-hand lowering during a left-knife
  swing; native weapon-palette evidence is collected, acceptance remains open.
- A short FPS dip remains during the early collapsing-building sequence.
  Smaller general hitches improved, but no complete fix for that scene is claimed.
  There is no universal 90/120 FPS guarantee.
- Later weapons, later levels, controller hotplug and other runtime/hardware
  combinations need more coverage.
- Full reproducible release packaging for all locally patched packages/assets
  remains work in progress.

[STATUS.md](STATUS.md) contains dated evidence and historical entries.
Read newer scoped results before relying on older “LATEST” sections.
Test candidates are not automatically promoted to the accepted installation.

## Reporting a bug

[Open an issue](https://github.com/trilex07/SingularityVR/issues) with your headset,
runtime, game edition, mod build/commit, relevant INI settings, scene and steps
to reproduce. Attach the relevant portion of
`%LOCALAPPDATA%\SingularityVR\view_matrix.log`; older runs may be timestamped
beside it. Mention whether the problem affects one eye, both eyes, or only the
desktop/screenshot.

For PSVR2 tests, agree the complete route before putting on the headset and
report after normal game exit. Switching between the game and desktop chat
has caused focus loss and invalidated earlier tests.

If the unmodified game also fails to start, check its original dependencies.
The optional [PhysX helper](spikes/view_matrix/setup_physx.ps1) uses an existing
local copy; inspect its instructions before running it.

## Building

```powershell
.\spikes\view_matrix\build.ps1
```

Requires Visual Studio 2022 C++ x86 tools, a Windows SDK and the OpenXR SDK.
Set `OPENXR_SDK` or use the repository's
[SDK resolver](tools/Resolve-OpenXrSdk.ps1).
The production build is optimized **x86 PE32 /O2** and records provenance and
SHA-256 in `spikes/view_matrix/d3d9.build.json`. Build checks include static
analysis for format-string defects.

The active renderer is `spikes/view_matrix/`, including `d3d9.cpp` and its
supporting headers. Earlier spikes are research references, not interchangeable
current deployment baselines. Building does not deploy or launch the game.

## Development and licences

Start with [AGENTS.md](AGENTS.md), [STATUS.md](STATUS.md) and the
[agent skill index](docs/AGENT_SKILLS_MANIFEST.md).
[ENGINE_NOTES.md](ENGINE_NOTES.md) records engine research;
[HISTORY.md](HISTORY.md) retains earlier experiments, including superseded results.
Deterministic checks, builds, installed-game tests and physical headset
acceptance are recorded separately.

Project code is MIT; see [LICENSE](LICENSE) and
[THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt) for component-specific terms.
Original game content remains the property of its owners. Locally extracted
assets and patched game packages must be handled separately from mod source;
do not assume the source-code licence covers those assets.

Not affiliated with or endorsed by Activision or Raven Software.

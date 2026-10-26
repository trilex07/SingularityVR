# First-release baseline — Trilex, 26 October 2026

Trilex accepted the latest combined V7 PSVR2 session: **“wszystko wydaje się
dobrze”** (“everything seems good”), and explicitly requested promotion to main,
README attribution and preservation for the first release.

The exact tested runtime and asset payload is now the **persistent accepted
installation**. The first-release installer and public distribution are pending
the user's requirements. No release version, Git tag or downloadable installer
has been created by this promotion.

## Identity

- Runtime source: `38ca72f88112bfcbd26addb0d6aa4f76095aa15e`.
- Main-history integration parent: `5667aa5ca6c68635e5a498b7a1b4373008ee267d`.
- Renderer: `23A3A768C44F1C6EB5479D899141924C4C44D5C6A73A3175A2827241105370B1`.
- Bridge: `F58CB3A805E46C3D3F7E88137BD605AF21A8A8106C289385132F677A968E95FD`.
- Asset manifest: `df6a7a3ff4ae8e012f4c9b2837537333977a25e79376b852000f73030541fa82`.
- New accepted profile: `895A8E845304F601F1DFCC37AF6DFD88E24D69F83656C04095207F25D7419E8D`.

Full INI, texture INI, protected EXE/core identities and backup paths are in the
[machine-readable receipt](first-release-baseline-20261026.json).

## Promotion and verification

The previous test guard completed its exact restore and exited. MAIN verified
the game and restore writer were absent, then used the production deployment
tool for the four runtime files and 134 asset files. No new game was launched.

All **138 installed source comparisons**, **164 accepted file pins**, protected
EXE/core checks and the accepted INI check passed. Headset resolution cache
values were preserved. The runtime is the same x86 PE32 `/O2` artifact that the
user tested; the main-history merge and README work introduce no runtime-source
changes. Existing offline regressions remain attached to those exact artifacts.

Timestamped runtime and asset backups remain private. The new
`deployment/accepted-profile.json` is also private; its hash is published above.
Never silently regenerate it from unknown installed bytes.

**Old V3–V7 test runners target AB348E06 as their restore destination. They are
historical receipts, not runners for the new baseline.** Future tests must bind
their plans and backups to this accepted V7 profile.

## Accepted scope and retained limits

The user accepted the grouped session. No individual check results were supplied
in that feedback, so this is not an exhaustive campaign, dialogue-listening,
Seeker retrieval/save-load or refresh-rate certification. The visible OpenXR
session and submitted stereo are recorded separately from the user feedback.

The payload includes 116 campaign banks / 1,248 dubbing replacements, seven
sound-remaster sets, native main/pause buttons and ten DDS candidates, alongside
the integrated hands/TMD, HUD, reload, aiming, menu, localization and rendering
work. Retain these limitations in release preparation:

- Original `RvGame.xxx` and TMD Impulse audio remain intact. The eighth sound
  replacement was withheld after modified-core startup failures.
- Ten refused replacement takes retain earlier/base audio; five additional
  native identities are uncovered. The SIDE-generated batch is Pocket-TTS
  Polish 6L, not XTTS. Full listening/acting approval is not implied.
- HD coverage varies. Kasimov diffuse is interpolated from the highest available
  extracted mip, not recovered native 4K artwork.
- Broad Seeker and later-level coverage, other headset/runtime combinations
  and universal 90 Hz operation are not established.

## Next gate

Use this accepted baseline for the first-release installer once Trilex provides
its requirements. Assemble the complete distributable/locally generated patch
payload with appropriate notices; the legacy DLL-only ZIP script is incomplete.
Do not redistribute private extracted game packages as MIT source assets.

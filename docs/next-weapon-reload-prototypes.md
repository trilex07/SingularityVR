# Next weapon reload prototypes — SIDE / offline

Scope: continue beyond Centurion without changing its runtime, left knife or
right gun holster. ZERO Steam. These are CPU gesture/asset prototypes, NOT
installed or headset-ready reload implementations. All enable gates default OFF.

## Evidence before implementation

Original private PSK exports have independently movable rigid `clip` subtrees:

- ARMachineGun / Valkyrie: 11387 wedges, clip bone 54; 547 extracted UV vertices,
  596 triangles. Original MachineGun Reload is 52 frames /24 Hz (2.166667 s).
- ARShotgun / Volk S4: 8297 wedges, clip bone 55; 242 UV vertices, 198 triangles.
  Original Shotgun Reload is 108 frames /36 Hz (3 s).
- ARSniperRifle / Kasimov: 11389 wedges, clip bone 56; 170 UV vertices,
  180 triangles. Original sniper Reload is 239 frames /60 Hz (3.983333 s).

All three clips have zero mixed skin-boundary triangles, material slot 0, and
original UVs; encoded FLOAT32 bind roundtrip passes at <=2e-5 UU. PSA records
roundtrip byte-exact. Track indices differ from PSK indices: resolve by NAME.
At explicit 52.5 UU/m, part dimensions are respectively 4.17 x34.93 x21.62 cm,
6.35 x18.31 x14.05 cm, and 3.65 x17.44 x18.12 cm. These are authoring conversions,
NOT physical controller scale calibration. Live material/socket/pose and native
ammo semantics remain UNKNOWN. AnimSet names/lengths do not prove an engine ABI.

The old SIDE wheel catalog incorrectly labels mesh 8297 as Spikeshot; its
actual PSK is `ch_inview_shotgun_mesh`, and CommonText.ARShotgun = Volk S4 in
the existing localization source report. Railgun mesh has 8543 wedges. Do not
change the accepted wheel here or pretend this proves its live cycle order.

Hypothesis: a reusable remove/discard/pouch/insert recognizer can support these
three detached clip geometries, with strict per-profile identity and calibration
gates, while native inventory remains untouched. Reuse the tested Centurion
CPU recognizer, NOT its authored socket, cylinder assumptions or runtime flags.
No shell-by-shell, pump/bolt operation, automatic-reload suppression or direct
native invocation is inferred from the `clip` name.

Falsifiers: unknown/mismatched mesh/object can acquire the hand; profile switch
continues the old transaction; a held grip retriggers after cancellation; one
gesture requests twice; or ammo pickup without equal reserve loss is acknowledged.
These are deterministic CPU checks. Runtime and PSVR2 evidence remain separate.

## Intended first slice

Each profile has its own opt-in CPU gate and MUST receive calibrated socket,
pouch, reach limits and live identity from a future caller. No profile selection
from vertex count alone. A profile/identity/calibration change cancels and holds
ownership until physical release. The recognizer emits an abstract intent only;
it does not publish XInput, call a native function or write engine/HUD data.

Asset preparation reuses the existing rigid-clip/PSA validators and creates only
private, ignored mesh caches, reports and wireframe previews. No game assets in
Git; texture/material binding stays a separate measured step.

## Implementation and limits

`spikes/view_matrix/weapon_reload_prototypes.h` contains three allocation-free
CPU profiles. `Frame.enabled`, `liveIdentityVerified` and calibration readiness
default false. These are API gates, NOT installed INI settings. No runtime hook
or caller exists yet. Object names are expected base FNames for future MAIN
verification; even matching names + wedge counts cannot set the verified bit.
Only right-dominant/physical-left interaction is accepted in this first slice.

The existing Centurion transaction is reused without editing it. A changed
profile, revision OR calibration values cancel even if an observer reports
balanced ammo in that frame. Ownership persists until physical grip release.
The result emits `reloadIntent`, not XInput nor a direct engine operation.
Native automatic reload/animation and weapon handling remain unchanged.

Preparation (repeat per profile) takes explicit private inputs:

```text
python tools/reload/prepare_next_weapon.py --weapon valkyrie --psk <ch_inview_machinegun_mesh.psk> --psa <MachineGun.psa> --preview
python tools/reload/prepare_next_weapon.py --weapon volk --psk <ch_inview_shotgun_mesh.psk> --psa <Shotgun.psa> --preview
python tools/reload/prepare_next_weapon.py --weapon kasimov --psk <ch_inview_sniper_mesh.psk> --psa <sniper.psa> --preview
```

Fixed private output: `build/centurion-reload/next-weapons/prepared/<profile>/`.
Each contains `<profile>_clip.svrk`, `report.json` and optional wireframe PNG.
All three previews were inspected: distinct original curved/box magazine
geometries, no fabricated universal cylinder. The shared preview helper only
gains an optional label; default Centurion extraction/render behavior is intact.

Native-cache hashes:

- Valkyrie: `C6420D9A1C96E09C73C23EC4FB86CB45F4F91C51FD37533F6B365DEA2D5756D0`
- Volk: `9029FDA62D9179CDDC9D2DDA8C16F882FE69D03C43D2C016BE797FF854EBDAE6`
- Kasimov: `B9387A88E9BB50D898EF28A1FFBA1C9E4E59E474CA92F90120F252CF931191D1`

The C++ prototype test also accepts these three paths in that order and loads
them through the existing bounded SVRK decoder. This proves CPU consumption,
not GPU execution or physical scale/grip. The normal reload suite includes
synthetic profile/calibration/identity/ammo/ownership tests and 30000 complete
transactions; asset preparation has six new Python regression tests.

## Next integration gate (MAIN, not performed here)

First finish the native ammo/notify experiment on Centurion. Then for EACH
additional weapon independently prove its live pawn/weapon/mesh identity,
HUD owner, native reload interval/commit and native material/socket. Provide
calibration and a single default-OFF runtime dispatcher; arbitrate the physical
left grip before knife/TMD/support actions. Only then add carried rendering,
native magazine suppression and a native backend in separate controlled steps.
Do not deploy this CPU-only prototype as a working manual reload.

## Verification receipt

L1_PASS: nine optimized x86 CPU executables +33 Python tests. The new recognizer
completes 30000 synthetic conserved transfers (10000 per profile, 360000 frames)
and passes profile/calibration changes in all five active phases, unknown/full/
empty-reserve gates, drops and cancellation ownership. The actual three private
caches also pass the C++ decoder/count checks above; their wireframes were
inspected. Native UE3 calls, textures, GPU draw, Steam and PSVR2 remain UNKNOWN.
No existing runtime source or INI was edited; the new header is compiled by the
CPU test, not included by `d3d9.cpp`. Full DLL build is a separate regression
gate and cannot establish that these prototypes are wired into the game.

The independent single-player mesh exports match the research PSKs byte-for-byte:
machinegun from RvGame, shotgun from wp_SP_ARShotgun_SF, sniper from
wp_SP_ARSniperRifle_SF. Respective SHA-256:
`B445A4A2C27E9EEDFC487A2CD5358B6DB1BD755B184CAFD663C4A2454991B442`,
`941C871B695480E608AC8EB0CA25B99731344ECA57383C9C5927CF71E7BAF1AB`,
`8FEAEFCE7B0B4880A084B2F31CAE10C2B0C3D98B57415EF040EA87AFADE033A7`.

L2_PASS offline build scope: clean source
`F519DE6A9E0D731137FCC2D646B6183CBA0C39F9`, `/O2`, x86 PE32 (`014C/010B`),
empty manifest working-tree status. Regression DLL SHA-256:
`A61CEBF5D45F11C8291B32DEB41BCE5782BA333D31AE02AA7C753D1DF08FF84F`.
Prototype test executable also verifies PE32/x86; SHA-256:
`B7DDAB7B1BFCB08BB435C6E05AE278FAACA3A02F6DE0440B08A7401CDE67658A`.
The DLL has unchanged runtime behavior and was NOT copied to the private
Centurion candidate or Steam. Existing candidate remains source e7a0c89 / SHA
`23AB074B813499DE2338EAC625079C3F5B0A5A1AB97196F6299F0B95BDCAB77A`.
The prototype header and prepared parts are not yet a runtime implementation.
Compiler retains inherited warnings, with no build error/fatal format gate issue.

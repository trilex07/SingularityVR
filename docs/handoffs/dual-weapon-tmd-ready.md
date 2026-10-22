# Dual weapon / TMD presentation candidate

Base: `641fcd5b00cc8ab10802604021a3f3fcaeb72ab6`.
Branch: `feature/dual-weapon-tmd-ready`. SIDE only: no installed files changed,
no game launch, no save changes, no merge. Default OFF.

## Evidence and hypothesis

The user's weapon-disappearance report is not by itself proof of omitted native
geometry. In archived `view_matrix_2026-10-23_00-20-40.log`, line 19894 reports
HANDS_TMD=1; the immediately following foreground signature includes 8297 (the
known Volk gun), alongside 7444/4818 TMD geometry. Around lines 20580–20584 the
log also reports a 700 ms melee window followed by HANDS_TMD and the gun signature.
Signature collection precedes `FgHidden`. This establishes native submission of
the matching draw signature in that session, not correct pixels or coverage of
every weapon. It disproves the blanket claim that HANDS_TMD always omits the gun.

Two proven mod-side mechanisms:

1. Impulse arms a native-hands/melee window at BOTH its rising-edge handler and
   the final XI_RSHOULDER forwarding handler. `NativeAnimatedHandsNow` then
   disables `FgMoved` and `PrepareIndependentLeft` for 700 ms. The native TMD
   impulse was being treated as a knife animation by presentation logic.
2. `FgHidden`'s broad flesh-section rule discards the mixed TMD base BEFORE
   Sol's hardware-only index cut/wrist palette can execute. Keeping only the
   downstream cut is insufficient. The candidate admits that exact section
   only when the existing independent palette/cut can safely handle it.

Falsifiers: gun raw draw count becomes zero in HANDS_TMD; or it passes the stereo
route with no false melee window but still disappears. Those distinguish native
omission/root animation from the mod-side failures repaired here. Do not respond
by forcing HANDS_WEAPON or enabling an unverified draw replay.

## Native route (offline disassembly, not a new hook)

Owned EXE SHA256:
`BD459ACD87D4B82693928DEE4ED437C086D5928A8E0428F3AD54B6FC114C0F51`.
Archived unpacked text at base `00402000`, SHA256:
`858F2FC33B1C99B3A4A27A817A11205DA66414EE1B030A03DF821F1AD6A0C78A`.

`audit_dual_weapon_tmd.py` resolves native registrations and verifies the
RvPlayerPawnSP vtable at `019D0788`:

- SwitchToTMD: wrapper F62C90, slot AA8, FD17F0, 355 bytes, RET 4.
- SwitchToWeapon: wrapper F62D10, slot AAC, FD1960, 542 bytes, RET 4.
- IsTMD_Equipped / IsWeaponEquipped: slots B04/B08, FBE360/FBE370;
  both compare byte `[ecx+105C]` to their mutually exclusive state.
- Switch bodies call AC7960 for three blend nodes (+1024/+1028/+102C), with
  inverted weights. AC7960 writes target blend +58 and duration +5C, RET 4.
  This is an animation transition, not a proven SetHiddenGame/mesh-list removal.

No native method, state value, blend weight, UObject, animation or gameplay rule
is patched. Native transition animation may still lower a weapon: this is an
explicit unverified boundary, not claimed fixed by the signature evidence.

## Implementation and ownership

`[Input] DualWeaponTmdReady=1` opts in; zero preserves baseline. Initial candidate
supports the requested RIGHT weapon / LEFT TMD arrangement only. Left-handed
mode retains baseline behavior; it is not advertised as dual-ready.

- Separate presentation predicate treats an equipped gun as present during
  HANDS_TMD. Original `WeaponInHands`, `TmdRaised`, and native state stay intact.
- Support grip, holster/draw, wheel eligibility, weapon steady and reload/UI
  presentation use that predicate. Holstered and no-weapon guards remain.
- Native L2 Impulse and Square Age/Rejuvenate still forward to the game; the
  candidate removes only their false melee presentation window. Real knife and
  health/cinematic/restraint animation ownership is retained.
- Existing support-grip solution is reused. During support/reload/wheel,
  invalid left tracking, pause/menu/cinema, TMD power buttons are reserved.
  Release-to-rearm prevents a held L2/Square firing as support is released.
- TMD material-1 hardware section is hidden during support or invalid left
  tracking. Release restores selection on the next draw. No new device mesh.
- Existing `PrepareIndependentLeft` applies the same physical-left wrist delta
  to original hardware bones. Existing 49 index runs preserve 6527 hardware
  faces and exclude 1177 arm faces. No edits to Sol's geometry/palette headers.
- Draws stay inside the native `StereoPair` path with original material, buffers
  and animations; existing index/palette restoration is reused. No draw replay,
  cached weapon draw, extra scene render, added GPU query or hot-path allocation.
- No haptics changes. Native power/energy/ammo/cooldowns remain authoritative.

Prerequisites for the hardware selection route: existing IndependentHands ON,
GloveIsolate>=2, normal VR gameplay and acquired TMD. Preserve the accepted
TmdOnOffHand/TmdNativeAim and rendering configuration; this flag doesn't silently
enable experimental aim or replace configuration. No assets to deploy.

## Bounded diagnostic and limitations

Optional `[Diagnostic] DualWeaponTmdProbe=1`, default zero. In eligible gameplay,
report every 450 frames, maximum 5400 frames / 12 groups of 6 rows. Counts split
HANDS state and gun/hardware signature: before hiding, reached stereo preparation,
gun ride, native-animation ownership. No per-draw logging or readback. Counters
are render-thread-only, fixed storage. `reachedStereo` is NOT actual draw success,
resource identity, pixels or proof of independent firing. Different weapon
identities can occur within a 450-frame group; do not compare them as one mesh.

This is an independently switchable implementation candidate, not proof of full
coexistence. Native R2/L2 arbitration, lower/raise animation, TMD presence during
every weapon state's submission, stereo depth/shadow appearance and frame time
remain physical/runtime questions. The already-existing 49 hardware runs add
GPU draw work when made visible; no measured FPS/performance PASS is claimed.

## Repeatable offline verification

- `tools/tests/test-dual-weapon-tmd.ps1`: optimized x86 policy/transition,
  native TMD adapter, wrist isolation, holster, reload, measured aim, WeaponLocker
  preview/layout and OLED D3D11 CPU-vs-GPU regressions; source-wiring guardrails
  explicitly labeled as structural tests.
- `tools/gloves/test_tmd_hardware_cut.py --psk <private original PSK> --cooked
  <private decompressed RvGame>`: full index roundtrip and hardware face census.
- `tools/fx/audit_dual_weapon_tmd.py --exe <owned EXE> --text <archived text>`:
  native registration, vtable, full body disassembly and animation switch checks.
- Production `spikes/view_matrix/build.ps1`: x86 PE32 `/O2`.
- `tools/reload/test_locker_compiled_abi.py spikes/view_matrix/d3d9.dll`:
  emitted `[eax+20Ch]` and FName factory stack layout unchanged.

The broader inherited `test-vr-interaction.cpp` was also attempted and FAILS at
line 78: it expects R32F 256x256 NOT to be a projected shadow map. The test and
all its headers are byte-identical to the requested base; current shadow policy
accepts that size. This unrelated baseline inconsistency is not silently fixed
or counted as PASS. The focused runner instead exercises the actual support
helpers across four weapon profiles and 25 rotations, plus the existing grip
transition test. Compiler analysis still reports pre-existing warnings.

## MAIN review / one physical route

GO for review and a controlled opt-in test after L1/L2, NOT production promotion.
With an owned TMD and gun, check gun/TMD coexistence and R2/L2/Square; then hold
left Grip at the support point, hold L2, release Grip (no accidental impulse),
release/repress L2 (normal impulse). Finally switch weapon, holster/draw,
reload, pause/resume and inspect both eyes: no duplicate gun/device, long arm,
lost tracking, abnormal shadow or new hitch. Stay in the headset until normal
exit; MAIN reads the bounded census. If the gun remains absent despite passed
native draws, the next missing evidence is its original root/palette during the
native blend transition, not permission to duplicate the whole scene.

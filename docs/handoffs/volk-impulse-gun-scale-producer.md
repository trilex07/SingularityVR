# L2 disappearance — original gun_scale producer correction

Branch: `fix/volk-impulse-palette-producer`, base `362f205` (support-gauntlet
change `d512c38`, preserving gameplay baseline `cac4ce3`). SIDE only:
no Steam read/write, deployment, launch or merge. Physical validation OPEN.

Clean source build: `032ab8af4b61effa1f8ed0f18c5a30e02eccb420`.
DLL SHA256: `7C473E25D295158AFDED3A06B2F11D07EF9C0F45DE98ABB15076AB2F15D0B991`.
Local artifact: `spikes/view_matrix/d3d9.dll`; provenance in `d3d9.build.json`
records a clean tree, x86 014C/PE32 010B and `/O2`. The subsequent
handoff-only commit does not change compiled sources.

## Evidence and first violated contract

The archived `8fbb9f4` capture, SHA256
`5D3CA90DE512F8293276045E23EC5B6F757CB98564690DB16FF1706BC444CBC6`, shows
all five incoming Volk skin matrices collapsed in hands=1. Both eyes submit
the collapsed matrices with successful HRESULTs. It does NOT identify the
original upload caller, and its budget missed the exact L2-held/R2 interval.
That original producer-attribution classification remains E, not retroactively A.

New offline native/asset evidence establishes a specific authored producer:

- Original `RvPlayerPawn.PostBeginPlay` export 12220 assigns `mGunScaleCtrl`
  (export 12061) from **mInviewMesh** (12030).FindSkelControl(`gun_scale`).
  `Pawn.Mesh` would be the wrong component; it is not used by this adapter.
- `inview_shared` (17332) binds control 34896 to **cog_gun**. The controller
  is a `SkelControlSingleBone` with BoneScale=0, initial strength/target=0,
  BlendInTime=BlendOutTime=0. It is an authored presentation switch.
- RvPlayerPawnSP vtable `019D0788`, +AA8 -> `00FD17F0` SwitchToTMD:
  loads `[esi+1024]`, pushes 1 at FD18E0, calls AC7960 at FD18E2.
  SwitchToWeapon (+AAC -> FD1960) pushes 0 at FD1A85, calls AC7960
  at FD1A87. AC7960 sets control strength/target with the native blend time.
  The +1024 field is independently required to resolve as `mGunScaleCtrl`
  at runtime; no unvalidated fixed-offset mutation occurs.
- SingleBone constructor B43AA0 sets vtable `0189F0A8`; +140 points to
  **B3E400**, `fld [ecx+7C]; ret8`. The reflected BoneScale offset must be 7C.
- Native consumer AEC25D obtains this virtual getter, pushes component then
  bone index, and calls it at AEC26A. It computes
  **1 + strength * (returnedScale - 1)**, multiplies the bone atom scale,
  and composes the space-base matrix. Returning 1 neutralizes only this
  scale operation. Authored rotation/translation and child animation survive.
- Original Volk PSK: `cog_gun` is bone 52; all five consumed bones descend
  from it: 55 clip (c5), 56 Slide (c8), 53 cog_shotgun (c11), 54 release (c14),
  57 Trigger (c17). This accounts for the whole 10835-face weapon, not just
  its 198-face clip. Source geometry/weights were checked, not inferred from c5.

**Proven offline mechanism:** the native TMD presentation intentionally scales
the complete gun subtree to zero, while the dual renderer submits that native
pose unchanged. The semantic mismatch is native single-presentation versus
dual presentation, not invalid skin indices or a D3D9 draw failure.
**Still unverified:** application of this exact local controller in the user's
next live session and final visual/recoil behavior. New receipts distinguish
ownership/signature refusal from successful producer execution.

## Minimal correction

`[Input] DualWeaponTmdGunScale=1`, default 0, requires the existing
`DualWeaponTmdReady` context. A six-byte fingerprinted getter is detoured;
the original runs exactly once. The result changes from 0 to 1 only for:

- the live camera-owned local SP pawn's reflected `mGunScaleCtrl`;
- exact SingleBone class, ControlName gun_scale, reflected BoneScale offset;
- that pawn's current **mInviewMesh** component and audited caller return AEC26C;
- engine input/animation thread, live equipped weapon, non-holstered dual VR;
- no cinematic, restraint, healing/native-hands ownership.

Other controls (clip_scale, grenade, recoil), other pawns/components, unknown
callers, nonzero scale, invalid objects, other threads and flag OFF forward
the original value. Ownership is rechecked; no cross-level pose cache.
The existing right-controller transform consumes the current native animation.
No edits to FgHidden, RideTargetFor, shader constants, shadow/stereo projection,
input/power mechanics, haptics, gun aim, reload or inventory.

Hook ABI: at native caller ESP=S, push component -> S-4, push bone -> S-8,
CALL -> S-12. Callee `[ESP]=return`, `[ESP+4]=bone`, `[ESP+8]=component`,
ECX=control. Original and detour return float in ST(0), RET8 restores S.
The detour uses MSVC fastcall only to receive this in ECX (EDX ignored);
its original function pointer is explicitly thiscall. No manual ESP repair.

Bounded receipts (12 state/ready messages total, once per refusal reason):

```text
ini: DualWeaponTmdGunScale=1
dual-tmd gun-scale: ready reflected mGunScaleCtrl=1024 BoneScale=7C ...
dual-tmd gun-scale: local gun_scale scale=0 -> 1 bone=52 ... hands=1 L2=...
```

An installer receipt alone is NOT application proof. A `refused ...` means
the original value was retained. No per-frame log stream or GPU readback.

## Validation

- `tools/tests/test-dual-tmd-gun-scale.ps1`: real own-process x86 /O2
  MinHook of the six-byte getter, native-style virtual caller at the audited
  return address, 10000 calls, exact ESP and float return, actual production
  adapter; refusal cases for OFF/thread/component/control/GC/holster/native
  hands/caller/name/scale. No game code/process is executed.
- `tools/reload/audit_impulse_gun_scale.py`: private native disassembly,
  virtual slot, script field/component link, authored zero-scale node, complete
  five-bone ancestry and all 10835 faces. Three animated poses, row-vector
  inverseBind * animatedBone convention and original reflected-Y PSK data.
  Corrected maximum skin-position error: **2.85e-14 UU**; original zero-scale
  vertex separation from collapsed root: **1.43e-14 UU**. This is a mathematical
  reproduction, not a sampled post-fix game palette.
- Dual-TMD suite: input/support, holster, Centurion/next-weapon reload, measured
  aim, cuffs, transitions, WeaponLocker preview/policy/layout and wiring PASS.
- Short-gauntlet suite: exact early route, 2x7110 draw dispatch, failure/reset,
  IB restore, original asset identity, 414 mixed sleeve faces, rotations PASS.
- Runtime regression CPU tests PASS. Its sandbox HAL fixture could not create
  D3D9. The same compiled fixture outside sandbox passed D3D9 **and D3D9Ex**:
  7110 faces/eye, 3304/3304 pixels, original IB restored. Synthetic VB only.
- OLED CPU and D3D11 GPU tests PASS (all three presets, <=1 LSB reference error).
- Compiled WeaponLocker ABI test preserves the literal `[eax+20Ch]` call and
  FName stack setup. Build x86 PE32 /O2 PASS; pre-existing warnings remain.
- `tools/reload/test_impulse_compiled_abi.py spikes/view_matrix/d3d9.dll`
  verifies the actual emitted adapter: original getter exactly once, ECX,
  bone/component stack order, original return-address witness, x87 float,
  saved EBX and RET8. This is disassembly of the production DLL, not a
  source-text assertion.

Private inputs stay outside Git. Reproduction accepts explicit archived paths:
`audit_impulse_gun_scale.py --exe ... --text ... --package ... --volk ...`.
PSK SHA256 `941C871B695480E608AC8EB0CA25B99731344ECA57383C9C5927CF71E7BAF1AB`;
archived text SHA256 `858F2FC33B1C99B3A4A27A817A11205DA66414EE1B030A03DF821F1AD6A0C78A`.
The accepted SVTG is unchanged, SHA256
`452EA0C0DDFB4721A7F9B9718629975CF021F9E079E1CD92235C076E9E376192`.

## MAIN controlled acceptance after review

Keep the accepted INI/assets, add only `DualWeaponTmdGunScale=1` to the existing
dual-TMD candidate. The already implemented `[Render] TmdSupportGauntlet=1`
remains a separate switch for the user's new keep-device-on-support requirement.
Do not deploy this sample INI wholesale. No requested save/inventory changes.

With Volk: raise TMD, press/hold/release L2 twice; right gun must stay visible
in both eyes and track the right controller. Hold L2 and fire R2: actual shots,
recoil and live TMD must coexist. Check reload and weapon switch, then Left Grip
support/release: connected short sleeve/device remains on the supporting glove,
without stretching or duplicate geometry; power release-to-rearm is unchanged.

GO for code review / controlled physical candidate, NOT a stable-build claim.
Unknown animation thread or reflected layout refuses safely, with a bounded
reason. Controller/FX absolute alignment still needs physical confirmation;
this commit does not disguise it with offsets or claim that all TMD defects
are solved.

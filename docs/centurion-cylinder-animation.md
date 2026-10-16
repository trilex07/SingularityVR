# Native Centurion cylinder animation — SIDE, 2026-10-16

## Review of the supplied feedback / SIDE c70b36f

The user reports three blocked native AttemptReload calls and an empty clip.
This supports that scoped guard result, not "100% of all engine reload paths".
No new native method patch or ammo/inventory write is added here. A successful
close still needs the existing conserved clip-gain/reserve-loss acknowledgement;
6/6 is possible only when enough reserve exists. Log counts alone do not prove it.

Accept SIDE's separate 16cm insertion radius: observed 9.3cm release was outside
7.5cm. Preserve 7.5cm old-grab acquisition; don't enlarge knife/support regions.
Prefer `ReloadInsertRadiusCm=16`, bounded5..20cm; legacy SIDE
`ReloadSocketRadiusCm` remains an alias for INSERT only. Regression includes
9.3cm acceptance, 16.1cm refusal and no increased old-grab radius.

Reject negative left-distance as a mathematical fix: Pouch negates local X
already, so negative22 selects right in its yaw frame. No evidence that OpenXR
quaternion X was inverted. The runtime supplied the **recenter yaw**, allowing
the reference shoulder to become right of the currently facing user after a
physical turn. Use actual tracking head yaw when the reload begins, latch that
yaw during the transaction so looking back doesn't chase the supply. Keep
shoulder offsets positive magnitudes; migrate legacy -22 to abs22. Same computed
anchor drives grabbing and rendering. Accepted knife/weapon hip math unchanged.
Tests include 0/90/180/-90-degree orientations and the yaw latch; L4 still needed.

## Evidence before animation

Original PSK SHA `4E18A25D5241DE1B3F18100E3BD9373D464EE16BAF3E29D7C60289AE18FE73D4`;
PSA SHA `9A30ECE4C009BDBEA8672AEEA67B832F1AD840E946CB3887CEFBF584DAA412D3`;
decompressed package SHA
`0534C5C9A5878D280294168BD12711B99CB3948503D4ED53608A0316479D1B18`.
Parsed the actual cooked section/chunk/index array and cross-checked PSK skin
contributors. Native palette is **[57,53,56,55,58]**:
clip0 / cog_alt_pistol1 / hammer2 / Trigger3 / clip_release4. PSK bone57 and PSA
track54 are NOT shader slot57/54. One section, 5799 vertices,6767 triangles,
16-bit index-content FNV1a147026316. Counts alone are not resource identity.

Original Reload:53 frames/24Hz. It extracts the whole cylinder; there is no
separate hinge or per-cartridge bone. Offline geometry preview inspected at
frames0/12/19/26/33/48: cylinder separates, returns and closes; other geometry
unchanged. No invented revolver hinge, procedural cartridge model or generic
animation substituted. Individual flying brass/smoke/SFX are NOT implemented
by this feature: they need a verified original asset/particle producer.

## Controlled implementation

Default-OFF `[Input] CenturionCylinderAnimation=1`. Private3208-byte
`vr-assets/reload/centurion/centurion_reload.svca` stores only the original two
tracks plus audited bind transforms. Tool: `tools/reload/prepare_cylinder_animation.py`
with explicit --psk/--psa/--cooked/--output, optional --preview. Private cache SHA:
`85ABCDDE7C9F1971C7B3BD5457672D69987FF332F8EC8C52953604505A945EA6`.
No proprietary keys/assets committed.

Manual FSM clocks drive original clip/release poses: Unlocked0..10, Open10..12,
Dump/NeedAmmo12..19 then collapse only the rigid clip; Carrying keeps native
clip hidden; Loaded26..33; closing WaitingNative33..48. This is a render-only
retiming, not invoking native Reload or its refill notify. Native engine reload
animation/state after the permitted call is NOT disabled by this change.

At the exact native gun draw: read CTAB's actual BoneMatrices base; require
nonshadow foreground/scene/current Centurion identity + fresh100ms snapshot.
Verify actual IB bytes against the cooked witness once/resource (max4 attempts
per Reset); unreadable/unknown resources stay unmodified. With parent skin P:
`clipSkin = inverse(clipBind) * animatedLocal * parentBind * P`.
Override only clip0/release4; parent/hammer/trigger bytes retained. Native mesh,
UV, material, normal/specular and draw remain bound. One scoped palette change
covers both existing stereo draws; restore all15 saved registers afterward.
Failed restore latches visual OFF until Reset/XR init. No draw-time allocation,
asset load or file logging; first resource validation may need one driver read.
Bounded witness logs are emitted later in Present. Reset hooks release held IB.
World shadow passes are deliberately untouched; their visual coherence is L3/L4.

The detached spare retains the prior original mesh/size correction and its
albedo-only renderer. Do not claim this native **attached** material path fixes
the spare's full lighting. Input, native reload guard/tap and inventory ownership
are unchanged except reviewed insertion/yaw policy.

## Gates and MAIN handoff

L1: phase/time, synthetic parent transforms, untouched slots, hidden-cylinder
degeneracy,521 fractional poses; private original cache1042 rigid poses and idle
bind identity; cooked palette/index identity and malformed inputs; full reload
regressions. L2 receipt appended after clean optimized x86 build. L3/L4 UNKNOWN.

MAIN integrates onto CURRENT source (including c70b36f), resolving the yaw and
insert-key differences intentionally. Do not copy the old SIDE INI wholesale or
replace a newer MAIN DLL. The private staging helper accepts --style PowerShell
`-OriginalAssets <existing verified clip pair>` and `-AnimationCache <SVCA>`;
writes only this checkout's ignored animation-candidate and exact manifest.
No INI staged. SIDE did not launch/deploy/restore Steam.

Scoped test additions, preserving current accepted config:

```ini
[Input]
CenturionCylinderAnimation=1
ReloadShoulderLeftCm=22
ReloadInsertRadiusCm=16
```

Keep current manual/layout/cylinder/guard/tap flags as reviewed by MAIN. Verify
cache SHA and DLL/PE32/source identity, backup/accepted profile, no running game.
One headset route: empty Centurion -> Circle/lateral release -> muzzle down ->
left-shoulder spare -> release within16cm -> raise/flick close -> fire once;
pause/resume, switch weapon and return to menu, exit normally. No mid-test chat.
Expected: only cylinder/release moves, native texture remains, no free refill,
correct shoulder/size and native conserved ammo acknowledgement. MAIN must see
`cooked index-content witness=MATCH` and nonzero scoped draws before inferring
the visual path ran. MATCH/counter still isn't PSVR2 acceptance. On unreadable
IB/wrong visual, preserve trace identities and narrow the next experiment rather
than disabling validation or guessing a different palette register.

Resource-readability prerequisite: through the EXISTING CreateIndexBuffer hook,
the opt-in visual feature removes only the WRITEONLY hint from 40602-byte,
INDEX16 allocations. This is a capability filter, NOT an identity claim; format,
size, content and pool are retained (existing D3D9Ex pool translation unchanged).
The later exact-content witness is still mandatory. Ordinary D3D9 installs only
that existing creation wrapper when needed. No new resource Lock/Unlock hooks.
Own-process D3D9Ex test uploads/re-reads a synthetic same-sized IB, checks hash,
pool/flags, and compares OFF behavior. [Microsoft's index-buffer Lock contract](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3dindexbuffer9-lock)
requires balanced unlocks before drawing; no lock is retained across a draw/frame.
The local driver allowed both original WRITEONLY and readable fixtures; that
does not make reading WRITEONLY portable. The capability filter avoids depending
on that leniency. [D3DUSAGE documentation](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dusage)
also warns of performance cost for DEFAULT buffers without WRITEONLY; this is
limited to cooked-size IBs under the OFF-by-default visual flag, and MAIN still
must measure real gameplay frame times. No performance acceptance is claimed.

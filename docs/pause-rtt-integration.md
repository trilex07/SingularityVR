# Native pause RTT integration

Branch: fix/pause-native-rtt-integration; baseline: 726e1a7.

2026-10-12 physical package-only test: user reports unchanged clipped main
pause text at upper left and clipped Objectives at upper right. Visual status:
BROKEN. This does not establish whether the new target was initialized or used.
Original package, accepted DLL and INI restored with matching backup hashes.

Hypothesis: RenderTexture is either overridden during initialization or is
produced independently of the current VR presentation. Falsification: read the
actual focused movie's named reflected MovieInfo, RenderTexture and mode at
pause entry and again after initialization settles. Log target UObject identity
and reflected SizeX/SizeY. Null or a different target rejects the asset producer
assumption; RTT_VRPause with 1280x720 permits investigation of native resource
association. Size/name alone never establishes a D3D resource association.

The first diagnostic changes only native_ui_input.h and adds pause_rtt_probe.h.
It performs read-only observations on the existing input-thread focus path:
three snapshots per movie transition, 1.5 seconds apart, maximum 24 per process.
Property type and validated UE3 property-layout prerequisites must hold. Native
calls, target binding, texture suppression and renderer presentation are unchanged.
No resource association or completed RGB/alpha content is claimed by this probe.

Live probe PASS (2026-10-12 17:27): SPPause references RTT_VRPause,
TextureRenderTarget2D, SizeX=1280, SizeY=720, modeRaw=2. Reflected movie
offsets are 60/64/137 and dimensions 172/176. Reopening retains the target.
SPMain control references RTT_UI1. Options/Video focus transitions also appear.
Evidence is local deployment/pause-rtt-probe/evidence/probe-physical.log.
Tested DLL 70228BA9DBAEDA64FC4022CF1911B4DE1F19F6F80C705E398403D33314045DED.
Accepted baseline package/DLL/INI restored and hashes verified after collection.

Next probe: inspect bounded UObject pointer fields up to the last proven
dimension property. Find native nodes with an image-owned vtable and backlink
to that exact target. Only a unique node is considered an owner candidate.
At successful native RT0 binds compare its direct/nested pointers against the
actual bound surface and GetContainer texture. All matches remain UNVERIFIED
until native field semantics are corroborated. No speculative virtual calls.
Maximum 24 distinct surfaces per owner and 96 per process; no draw-hook work.

The resource scan initially faulted inside guarded diagnostics. Corrected local
range checks reject overflow and use fresh VirtualQuery; image ownership uses
AllocationBase rather than reading live PE headers. Autonomous Steam startup
on AD2AC766...06B9E6 found a unique RTT_UI1 owner at UObject+0xA4,
resource backlink+0x60, vtable 0x0182B7C8. An actual bound texture matched
resource+0x48 -> RHI+0x1C. Other pointer paths also matched a non-texture
surface: those incidental matches are not accepted as the UI texture.

Offline inspection of the existing owned text snapshot corroborates resource
InitDynamicRHI (vtable slot 1, observed 0x00895AB0): it reads owner+0x60,
dimensions at owner+0xAC/+0xB0, creates the texture and surface via RHI, and
assigns surface RHI at resource+0x48. Diagnostic runtime checks validate this
method's owner/dimension instruction signature as well as target+0xA4 and the
owner backlink. No virtual method is called on a scanned pointer.

Content probe retains only an actual successfully bound surface whose container
texture equals the verified RHI field. Readback occurs at Present after native
unbind and draw attempts, under the same movie token, into SYSTEMMEM. Up to
four RGB/alpha BMP pairs are written (one main control, three pause); no target
or render state is changed. Draw and sample counters describe attempts, not
successful semantic content. Pixels and callback freshness still need inspection.

Correction (2026-10-12 17:55): +0x1C was adjacent heap, not an RHI member.
Read-only inspection of live PID 18936 and offline constructors/creation
methods establishes opaque RHI handles are native wrapper base+0xC;
the owned COM pointer is base+8, i.e. RHI-4. FD3D9Surface vtable is
image+0x013B4474; FD3D9Texture2D is image+0x013B441C. Correct matching
requires both classes, target/resource/backlink, and exact bound surface
plus container texture equality. The old main RGB image is real, but its
original pointer association argument is superseded by this contract.

Native start remains read-only: pause receives RTT_VRPause (repair=0), and
the RTT list contains its active record. The corrected native pause surface
has thousands of draws. Missing capture was a probe bug, not producer failure.

## Physical provider failure, 2026-10-12 18:15

On DLL 9CF168A4...6EDA6CAF the user reports Pause still clipped/outside the
visible region, more visible when physical IPD increases. Laser highlights;
resume/reopen works. VR Settings/footer appears higher. Returning to main
has differing per-eye backgrounds; IPD causality is not established.
Status remains BROKEN, not physically verified.

Producer RGB is complete (1280x720); Objectives body has nonzero RGB with
zero alpha. ONE/INVSRCALPHA preserves it. Both provider draws succeeded,
but that is not pixel or headset proof. Auto captures used GetRenderTarget(0)
after provider restoration and show only the scene, not the submitted menu.

Next bounded candidate establishes a complete fixed-function sampling contract
for pause only (UV index 0, texture transform disabled, stage 1 disabled,
clamp sampler, fog/stencil off, ADD blend), inside the existing state block.
Hypothesis: inherited GFx state defeats the full-UV quad despite draw success.
Falsification: capture actual GetBackBuffer after the final overlays immediately
before the XR bridge copy; inspect text and panel bounds in each submitted half.
At most three pause captures and one return-to-main capture, with bounded
inherited state/quad logs. No gameplay stereo/projection or main policy changes.
Original package C8CDB2E1...6984F, accepted DLL 2FE5011C...B39A47 and INI
61AEC56C...FE378B restored and verified before preparing this candidate.

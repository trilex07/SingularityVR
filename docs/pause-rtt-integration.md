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

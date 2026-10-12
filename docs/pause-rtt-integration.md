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

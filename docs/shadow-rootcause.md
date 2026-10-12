# Stereo square artifact investigation

Base: latest main 9e92a4f, merged PRs #13/#14. Frozen native RTT pause, menu
layout, hands/weapons and gameplay stereo. No old 726e1a7/2FE5011C rollback.

Reviewed 7b33231 research, a894ce8 decal audit, HISTORY runs 40-48,
fp_shadow.h and live render-hook source. The old audit's "confirmed root cause"
is not established: map-pass drops and G16R16 drops left squares; narrow decal
classification matched zero live draws; c1/c13 remaps failed visually.
HQ decals OFF is a prior user observation, not current-main A/B proof.

Source finding: main has SetVS tracking but no SetPixelShader hook. Existing
RenderEvidence reads bytecode inside draw hooks, so it will NOT be used for this
trace. Need bounded final-draw state/producer-consumer signatures and shader
size-negotiated capture outside the hot path. Size/format is metadata, not identity.

Next experiment: read-only pass trace, default OFF. Compare one gameplay scene
with HQ decals ON/OFF, no shadow-drop or coordinate changes. Correlate exact
shader and RT/texture identities, primitive/range, viewport/scissor, original and
per-eye constants; prove candidate pass contributes incorrect pixels before fix.
No register fix or producer claim yet. Diagnostics themselves need an A/B control.

Restore baseline must contain accepted menu DLL 9FBA308C...99C4500 (5f05687),
INI 61AEC56C...FE378B and dedicated RTT UI 5B2EA469...673CDE. Installed older
accepted pause DLL 1635A8F0...D625219 is preserved separately before upgrading.

Current installed baseline is 9FBA308C...99C4500; full verified restore manifest
under deployment/shadow-backups/20261012-191007. Earlier DLL preserved by the
deployment runner's install-backups/20261012-191007. Snapshot directory naming
now includes milliseconds and rejects collisions instead of overwriting manifests.

First diagnostic implementation: one requested gameplay frame, at most 192 exact
shader/RT/texture/scissor/phase groups and 24000 final driver calls, overflow
reported explicitly, four frames per process maximum. Resource references held
only for groups, released at next Present/reset. Bytecode negotiated and written
only at Present; first 32 VS/PS constants sampled on a new group, not assumed to
cover all shader registers. Geometry/DS/texture dimensions are metadata only.
SetPixelShader slot 107 verified from Windows SDK d3d9.h alongside known slots
37 (RT) and 94 (VS constants). No pass is suppressed or remapped by this trace.
Flag is presence of explicit shadow-pass.request in runtime log directory;
no request = inactive. Request tool labels observations but does NOT change HQ
decals or establish the option state. The human must set ON/OFF separately.

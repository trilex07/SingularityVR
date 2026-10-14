# Physical PID16456 follow-up

User confirms Polish helicopter dialogue (native replacement path visual/audio
acceptance for speech presence), but too quiet against effects. Gameplay still
English: the ten-wave helicopter replacement is not all-game localization.
Hands almost correct, not aligned exactly; user currently seated on the floor
and cannot test standing. No button-related FPS difference observed; dips remain.
Return to menu shows two flat images. No full hands/FPS/menu acceptance inferred.

After exit all four originals independently verified: VR D8928180, injector
AAA07005, INI B3FEED43, native audio890C6172. UI5B,107DDS,texture config/original
forwarder unchanged. No profile request pending. Initial hidden guardian failed
before Steam launch because fresh WindowsPS lacked Get-FileHash; explicit PSHOME
module imports fixed it. Replacement guardian32192 completed real rollback.

Native stock INTRO1008 is -14.92 LUFS vs generated -20.78 LUFS (~5.86 LU quiet).
Other radio lines were ~8–11 LU below their stock counterparts. Loudness-only
follow-up uses measured gain, look-ahead peak limiting and encoded-result checks.
No global mixer, FX/music/native properties/timing changes. One-pass loudnorm
missed the short radio-line target; bounded gain corrections converged against
the actual measured output. Nine lines within1LU; exceptionally compressed stock
crash-radio line allowed1.5LU below original rather than stronger limiting.
Explicit per-line limits recorded; no final auditory quality acceptance claimed.
Every voice still mono48k/exact original PCM frame budget; encoded true peak
below-0.2dBTP. Original scream and128non-target sounds/properties retained.
Independent LZO roundtrip PASS, candidate package
84B9A419B3843E73DEFD3A96E63AE7C5A0E7F73D932D69D532C5BED7BAE07565.

Frontend ownership candidate described in docs/cutscene-frontend-handoff.md.
L1 matrix/sequence tests PASS; x86 PE32/O2 build source6628fa6,
DLL85CCCB607090A1A59C118BB63F92D82F1529F39F9B6B84C29CC8FF44C99EBC99.
Default OFF, proposed test ON only in a private leased INI. Existing noTMD
repair retained; controller offsets/ShowControllerArms untouched. Next hand
alignment trial must use the user's actual seated posture; no standing request
and no speculative pose correction from a nonspecific offset report.

Residual FPS cause remains open (first profile contaminated by menu/cinema,
second capture unavailable by design within that process). Do not bypass the
sampler's one-capture safety guard or reset active scope accounting. Next pure
gameplay capture needs fresh process and arm after current gameplay qualified;
no new test until fresh user start. Full native gameplay Polish voice delivery
is OPEN, not solved by WAV presence or the helicopter package.

2026-10-13 PID34980 follow-up ran the louder package84B9A419 with frontend
candidate85CCCB60. Steam/OpenXR and exact installed four-file hashes confirmed;
session completed and all originals independently hash-verified after guarded
restore. Physical speech balance/quality feedback is still PENDING. No claim
that all-game Polish dubbing is delivered. See cutscene-frontend-handoff.md for
runtime presentation and limited profile observations from this session.

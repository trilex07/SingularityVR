# PID24468 radio follow-up: one waveform, not another mixer change

User says menu is OK but radio did not speak. Do NOT promote this to a claim
that all native dialogue is silent; helicopter speech has not been reconfirmed
in this round. No audio-event probe was active, so the session does not prove
that the relevant Kismet action, native wave load or decoder executed.

Closed-session evidence: private three-items-live-20261014-085320/session.log
contains cinema pause/resume and return to GameplayStereo, then menu/video again.
Three guardians restored their disjoint targets; restore-verification.json
independently checks core4, asset32, WAV1886,107 frozen DDS and original forwarder.
No real installation changes or launches during follow-up preparation.

## Producer and hypothesis

Read-only decode of the user's SP_VI2_kismet01.xxx passes independent header/
export/serial tiling. Typed RadioConversation arrays on three RvSeqAct_PlayerRadio
instances resolve their mCharacterSoundCue references through FirstNode/ChildNodes
to17 SoundNodeWave imports. Exact main-sequence single-line object index7959
references snd_vo_SP_VI.Cue.VI1.VI1_1005_GSTCue and imported
snd_vo_SP_VI_wav.VI1_1005_GST. This identifies a real producer/resource chain,
NOT proof that it fired during PID24468.

Measured stock vs translated PC payloads: VI1_0105_CP2 -14.32 vs -24.37LUFS;
VI1_0105_NIM -13.02 vs -26.51; VI1_0506_GST -13.95 vs -20.94;
VI1_1005_GST -12.83 vs -23.16. Earlier preparation verified peaks/timing but
did not match these gameplay radio voices' integrated loudness. Hypothesis:
the translated radio can be masked by unchanged FX/radio mix. Falsifiers:
gain-matched target remains silent, action never fires, another wave/bank loads,
or unchanged original speech fails in the same exact action.

## Small experiment, offline PASS only

Change ONLY VI1_1005_GST PC payload volume in the previous three-item candidate.
Encoded -13.64LUFS (within1LU of native -12.83), true peak -1.37dBTP;
442418 frames/mono48k unchanged. Bounded measured gain +look-ahead limiter,
rendering each pass from the same source (no cumulative transcoding). No new
TTS/text, cue delays, SoundNodeWave properties, music/FX/mixer or render changes.
All other candidate voices stay byte-identical; `_a`/`_b` physical banks identical.
Original bank is independently decoded before trusting inventory; candidate LZO
re-decode equals expected logical image. Peak, finite loudness and PCM gates PASS.

Broad attempts volume-candidate and volume-candidate-v2 are REFUSED artifacts:
first short line missed1LU by .08 after five passes (monotonic measurements
justify the existing absolute eight-pass ceiling); another line required gain
above30dB. Do not lift that limit or deploy either incomplete batch. The safe
next experiment explicitly selects only the proven single-line target.

Private final paths under deployment/hands-dubbing/radio-event-investigation-20261014:

- single-radio-ready/radio-loudness.json: exact source/previous/candidate hashes.
- single-radio-ready/radio-graph.json: typed references, no copied speech text.
- delta-single-radio-ready/asset-manifest.json: exact32 lease, log-only textures.
- lease-test: synthetic32 installation/restore/corrupt-backup/conflict tests PASS.

Bank SHA256:
FB03741D672537CFC8117C8448361CFEABF819562B3955DADD7BBAD0945AECF8.
Thirty-one other asset delta hashes equal delta-log-final; core-ready DLL/INI/
intro and voice-delivery-v3 unchanged. NO new runtime DLL build needed: same
source9989bae, x86/O2 VR C1D904D0 and injector F5137234. Five synthetic loudness
guard tests PASS and original-data graph check confirms exact target.

## Next physical gate

Fresh start required. MAIN uses core-ready, existing WAV lease and this NEW delta
with all3 backup/hash/READY guards. Keep remaster_apply0 for this audio isolation;
do not add texture apply to a failed audio gate. Existing private start script
accepts -DeltaManifest to select this artifact instead of the old one.

One headset route without chat switching: reach post-crash stereo, proceed into
the first gameplay area and listen for the radio instruction to regroup near the
tower. No guarantee of trigger timing from our log. Do not stop merely at the
stereo transition; finish the short route and close normally. MAIN preserves
session and restored hashes; user reports radio language/audibility. If absent
again, next step is a native action/load/playback observation and exact one-wave
original-audio control, not another speculative global gain or PlaySound hook.

## Physical result PID17436

User heard Polish radio and generally accepted the current version. Exact native
callback/decoded resource execution was not instrumented; do not invent that
evidence from audio feedback. Session started with verified installed C1D904D0/
F5137234 DLLs and bankFB03741D, OpenXR ready and actual loaded module paths.
Session ended normally; three independent guardians and subsequent MAIN hashing
confirm restoration of4+32+1886 originals plus107 frozen DDS. Private evidence:
three-items-live-20261014-102151/session.log and restore-verification.json.

Accepted scope: Polish radio audible in the tested opening route. User reports
indoor recordings/people still English, notes untranslated, first pistol leaves
forearms and bad right grip after pickup, UI narrator loud/no toggle, and stacked
New Game save-slot/confirmation menus. These remain separate open follow-ups;
no full-game Polish, complete hands, branding typography or applied remaster
acceptance inferred. User requests a review PR to main, not automatic merge.

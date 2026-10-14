# Controlled next test: native Polish audio, original-style title, existing materials

User physically accepted E213B216 trial's hands, cinema pause/options, matching
eyes/menu and controls (PID23584/36340). Native opening radio stayed English.
No additional Steam install/launch today; next test waits for fresh `start`.

## Native audio

Mechanism: existing Localize/WavForEnglish/PlaySound is a UI overlay, not a
redirect of UE3 SoundNodeWave. SIDE's exact SpokenText audit found20 faithful
entries and22 inaccurate translations among the previous42. Those42 must not
be deployed blindly. SIDE b13b8dda/9a5ae536 supplies corrections and improved
guarded takes. There are existing files for all17 native VI1 radio identities,
but that does not prove which event plays after the crash.

Small experiment: preserve native wave properties, cue/event references, rates
and PCM budgets; replace only inline PC Vorbis payloads in the three first
gameplay banks. Retain ten previous Polish helicopter voices, add only bark
replacements passing the exact frame gate. Contiguous -65dBFS trim +40ms edge
guards protects quiet consonants; no speech cropping or tempo above1.85.
Independent decode count must equal original native PCM frames. Six short
attempts remain English/refused (including reload's unsafe full-word take).
No guessed codec-delay compensation or metadata volume offsets.

Current candidate:48 fitted new native waves across four banks, plus10 preserved
helicopter replacements. Native bank load/playback and new audio audition OPEN.
Exact fitted story IDs mark native voice ownership, default-OFF NativePolishSpeech:
prevents a second PlaySound overlay ONLY for those exact source strings. UI and
uncovered lines keep their original audio path. Lookup/WAV identity uses each
installed package/asset entry's existing filename, not SIDE's shared _a filename
for _b. Revoice delivery retains1886 IDs and explicit QA review status. Visual
token/markup mismatches keep the original text rather than losing substitutions.

Falsifiers: native playback still English for an actually replaced asset; package
not selected/loaded; duplicate overlay despite exact ownership; any changed event
timing/format/crash. These require event/runtime evidence, not another WAV copy.

## Branding

Reuse SIDE's SWF patcher from96275fd. Actual accepted UI5B decoded hashc5e7db29;
GFxMovieInfo `iis`, DefineEditText character10 contains the original logo text.
Change that exact field to SINGULARITY VR; fonts/style/bounds/placement, actions
and all other movies stay byte-identical. No window/process-title or gameplay
substring replacement. Repair declared GFx array/tag/file sizes and proven inline
texture offsets only. Dedicated RTT_VRPause is retained. Independent LZO control
and candidate roundtrips pass. Candidate UIE0F5A661. Font V glyph, clipping and
appearance remain physical questions, not inferred from an ASCII replacement.

## Existing remaster and hook/lifetime experiment

Reuse source0dcdcd5 and188c10a, not another asset-generation run:24 maps (6 native-
resolution prototype maps plus18 batch maps, mostly2x),91,229,592B including DDS
headers/mips. Original mip0 hashes/FNV/format/dimensions and replacement hashes,
complete mips verified. Specular/lower-mip review files and unmatched TMD excluded.
Derived game assets stay private and must not be added to Git/release downloads.

Observed source defects: legacy injector MinHook owns the function currently in
device slot65, while MAIN also patches slot65; positive cache pins replacements
for process lifetime and keys raw source pointers. This is load-order/lifetime
fragility even though existing Steam traces prove many injector calls succeeded.
Do not treat the stale audit's categorical 'always idle on Ex' as current proof.

Hypothesis: one MAIN slot65 owner with an explicit forward callback avoids hook
ordering; original identity reaches VR/menu/shadow classification, substituted
identity reaches the saved driver exactly once. Default-OFF TextureBridgeOwned
and bridge_owned opt in together; mismatches fail closed without MinHook. New
injector route never hooks CreateDevice/CreateDeviceEx: MAIN supplies the device.
Pool-aware replacement uses MANAGED on plain, DEFAULT +SYSTEMMEM/UpdateTexture
on Ex. Source-owned COM metadata owns replacement (no source AddRef cycle); source
destruction/write invalidation releases it. Existing MAIN Lock hook invalidates
positive/negative decisions on writes; enabled plain-device creation also arms
the same proven texture wrappers. RT/dynamic/depth and protected menu/shadow
sources bypass substitution. Legacy/default-OFF hook regression stays covered.

Hidden native plain/Ex tests exercise actual callback, red-pixel readback,
Reset/ResetEx, exact pointer/stage/failing-HRESULT/forward count, unknown ABI and
protected-source bypass, original destruction and replacement release. These are
offline graphics-content tests, NOT Steam black-material or remaster acceptance.
An additional offline test compiles actual MAIN wrappers +actual injector code
in separate translation units, without DllMain/game/OpenXR session. It proves raw
DEFAULT LockRect fails, wrapper redirects writes/readonly hashes, factory upload
works through actual CreateTexture hook, one MAIN->callback->driver chain keeps
original identity for MAIN while driver receives replacement, actual write
invalidates metadata, ResetEx retains pixels/shadow readback, and live shadows
return to zero. g_stageCount is a cumulative allocation metric, NOT a live count.
Falsifiers: chain recursion/lost binds, Lock failure on real matched targets,
black materials, failed Reset, leak growth, p95 added frame time >0.5ms.

## Deployment order and ownership

Four-file core lease contains new DLLs, experiment INI and combined native intro.
Independent exact32 asset delta: UI logo +3 gameplay banks +24 DDS +texture INI
+3 lookup manifests. Separate existing1886-WAV lease. No overlapping targets.
All backups/hashes before writes; guards require READY before Steam launch;
600s no-launch rollback,10s after closed game. User edits are preserved as
conflicts, never silently overwritten. Original absent DDS removed only if its
installed hash equals the owned candidate. No broad folder deletion/rename.

The24 maps have a separate texture-override-remaster folder; accepted107 DDS,
base mode_apply1,groups/tier/directory stay unchanged. remaster_apply0 logs exact
named matches and binds the existing accepted base files; remaster_apply1 selects
ONLY24 whitelisted remasters. Other glyphs/FX/environment replacements stay frozen.
Next test order: remaster log-only first (title/audio/menu/hands +candidate identity,
zero remaster=1 substitutions), then a separate cold-start apply round after that gate.
Never change DLL/INI/assets while game runs. Existing accepted shadows/input/hand
mesh logic, saves, registry, Bink movies and gameplay stereo remain outside scope.

## Prepared handoff, 2026-10-14

Runtime source commit: `9989baef05858f4cd1f16499272a38a4ab840c67` on
`fix/cutscene-physical-observation`. Both final builds are x86 PE32 `/O2`,
without performance instrumentation. Build provenance travels with core-ready.

- d3d9: `C1D904D0D05B1ADDC48A7ED52A8BD1E06EC65FA874E7C2B1D4A5FC5F0666FE20`.
- dinput8: `F5137234C9BACCCF636A6A387B63CCD9865C726C9E662AD56B6E30C74770F07A`.
- INI: `A40C449ECEBA682002BA4DCC5AECD8B9C736AADE4BC5B0F037118ACBDFD71040`.
- Native intro: `4904F2818B9CA9505AE636CB33ADE88C2B2E173AD1D2DB2A85BC43EDD49E1290`.
- Title UI: `E0F5A661AD6E6EE7E56C7D6E9B914AFA61D83B38D118D2E8136D8CA086A3CC64`.

Private artifact paths below are relative to this checkout's
`deployment/hands-dubbing/three-items-prep/`. Do not add them to Git.

- Core lease: `core-ready/test-manifest.json` (all six owned flags listed;
  HandAnchorProbe OFF). `core-final` has identical bytes but older descriptive
  IniChangeOnly metadata; prefer core-ready.
- Audio source: `native-voices-v3/native-story.json`; fitted bank/control and
  independent LZO roundtrip outputs are alongside it. Its historical legacy_wav
  report labels must NOT drive deployment: `_a`/`_b` source names were shared
  in SIDE data. Correct original entry identity is verified by voice-delivery-v3.
- WAV candidate/QA: `voice-delivery-v3/revoice/`; PLAN evidence:
  `revoice-plan/revoice-lease.json`. 1886 entries pass identity/format/hash gates;
  146 retain explicit QA review status. Three lookup files in voice-delivery-v3
  mark 45 fitted story IDs (48 native waves include three intro barks).
- First-round asset lease: `delta-log-final/asset-manifest.json`.
- Second-round asset lease: `delta-apply-final/asset-manifest.json`.
  These are the ONLY final delta artifacts. Earlier delta directories are
  superseded and must not be installed.
- Material provenance: `materials-final/material-files.json` and excluded.json.
- Final build logs: vr-final-build.log, bridge-final-build.log.
- Actual MAIN wrapper fixture: main-bridge-fixture-v2.log.
- Guard evidence: delta-guard-fixture/guard.out.txt (READY then RESTORED32,
  stderr empty), independently checked against all32 original fake hashes.

Preparation did not change any Steam file. Independent installation report
`installation-unchanged.json` at 01:50+02:00 verifies original4 core, all32 asset
targets (including absent remaster files),1886 original WAVs, accepted UI5B,
107 DDS and frozen texture config/forwarder. This proves restoration/unchanged
installation, not acceptance of candidate rendering or speech.

Next fresh `start`: first verify no Singularity process and all originals again.
Install core4 first (its UI preflight expects5B), then the separate1886 WAV lease
using fresh install evidence, then exact32 log-only delta. Verify candidates and
all three guardian READY messages before launching Steam app42670. Never reuse
fake-test backup paths as real rollback evidence. Guardians own disjoint targets;
after normal exit independently verify restoration of all files, not just DLLs.

Headset script, with no chat switching: new game/intro/helicopter; pause/options
and resume during helicopter; continue through standing up into stereo gameplay,
listen to radio, move both hands near face/outward, use L2/R2, return to main menu,
then close normally. MAIN records session/bridge/native bank evidence. User checks
branding typography/clipping, Polish audio and timing, accepted hands/menu/shadows
and FPS. Only after log-only passes, prepare a separate cold apply round for the
24 remasters, checking black materials, appearance, matching eyes and frame cost.
Retained English lines and unreviewed acting quality are explicit open limits;
this is NOT a claim of complete Polish dubbing or accepted remaster.

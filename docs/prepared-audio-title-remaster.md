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

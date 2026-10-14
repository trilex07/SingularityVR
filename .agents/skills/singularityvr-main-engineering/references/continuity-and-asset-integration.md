# MAIN continuity and asset integration

Use for takeover, multi-file tests, persistent promotion or native audio/font
integration. `AGENTS.md` and the current user request own permissions. This is
not an installer, a live baseline or a reason to expand an unrelated task.

## 1. Reconstruct the state, not the story

Compare the current branch/HEAD/diff and worktree inventory with the installed
files, current task receipts and the physical feedback. Another model may have
changed the checkout or deployed from a different one. Leave other owners' files
alone. Logs can outlive the process or be overwritten by a later launch.

For every affected component, distinguish:

- Installed identity: absolute target, existence and exact SHA-256.
- Accepted identity: source commit/artifact and the specific physical test scope.
- Proposed identity: candidate bytes, flags and dependency manifest.
- Restore identity: verified backup and ownership conditions for the next lease.

Include dependencies, not just the renderer: injector/forwarder, INI flags, UI
package, font package, localization manifests, native voice banks and WAV packs
as relevant. Account for required files that are absent. A promoted runtime may
coexist with still-original fonts or native banks; say so explicitly.

Do not read saves or change Steam/OpenXR configuration just to establish these
identities. A missing or contradictory receipt calls for read-only checks, not
silently blessing unknown installed hashes.

## 2. A test lease is not a persistent installation

A temporary candidate returns to its declared baseline after exit/no-launch;
a user-requested persistent installation remains installed, with its backup
retained. Neither physical success nor a guard's "RESTORED" line alone authorizes
promotion. Explain this distinction when the user sees a fix disappear on restart.

Before reusing a runner:

1. Read its actual install and restore targets, flags, package/WAV sources and
   backup identity. Private helpers may be absent or obsolete in this checkout.
2. Verify all planned sources and baselines. A historical run that installs an
   older voicepack can undo a newer story-only installation even if the DLL fits.
3. Verify active writers/guards. Do not promote while an older guard can still
   restore over the promotion. Resolve owned leases safely after confirmed exit;
   do not terminate another owner's process or change their files without authority.
4. Rebuild the lease against the authorized baseline and exercise it in a private
   fake installation when the deployment logic has changed. Check bad backup,
   unexpected edit preservation and exact restore, not only successful copying.
5. Use full process visibility before live writes. An empty sandbox process list
   does not establish that Singularity is closed. Never overwrite running files.

Independent guards need disjoint file ownership. Preserve user calibration and
settings through exact scoped edits/three-way reconciliation; do not replace an
INI wholesale just because a previous test used it. If restoration is incomplete,
report the actual conflict and remaining installed identities.

Promotion requires the user's direction and an explicitly scoped payload. Record
which files were retained or restored. Do not roll back a promoted build to a
historical "safe" DLL/INI in a later task. No unattended experimental candidate:
use the bounded cleanup behavior in the existing test playbook.

## 3. Follow the real asset/audio consumer

Matching a filename, toggling PolishLanguage or copying a WAV library is not proof
that a native dialogue consumes it. Separate Bink track selection, native UE3
SoundNodeWave/SoundCue PC Vorbis payloads and the mod's UI/story WAV playback.
Text-lookup narration may never receive a native radio/NPC event. A native-owned
line may intentionally suppress the standalone WAV overlay.

Identify the loaded package/wave/cue and producer with bounded existing hooks or
read-only reflection. Record what disproves the proposed path. For native bank
replacement, reuse audited package/timing tools if present; preserve event/cue
metadata, platform payloads, complete utterance, sample budget and loudness.
Validate an independent decode/roundtrip before Steam. Never crop speech blindly,
globally mute original audio or layer a duplicate player as an unreported fix.

For a requested drop-in story pack, verify exact filenames, formats, hashes and
backup scope; protect intro/UI/multiplayer as requested. Installing those bytes
is a legitimate completed install step, not full-game auditory acceptance. ASR
cannot prove acting quality; retain known text/pronunciation omissions as open QA.

Similarly, a Polish text manifest cannot supply missing font outlines. Prove the
actual GFx movie/font field and glyph producer; keep font-package and translation
acceptance separate. Asset dimensions or draw-success counters alone are not
evidence that the visible logo/cursor uses that resource.

## 4. Preserve evidence at transfer and release

Provide a small handoff with branch/commit, installed versus candidate/restore
identities, relevant flags, receipts/log paths, physical scope and untested items,
stale runners not to execute and the next justified action. Avoid raw transcripts
or fixed live hashes in this skill. Carry user test preferences forward without
turning them into permission to launch a new session automatically.

Fetch the actual base/head comparison before preparing a PR. The SIDE's last
commit may be audio-only while its branch includes old renderer changes. Check
for an existing or already-merged successor instead of creating a duplicate.
No merge without the project's applicable authorization rules. API 403, CLI
authentication absence or an open compare form does not mean a PR was created;
report the real state and use an authorized supported route when available.

## Behavioral checks

- New story pack promoted; old runner installs older WAVs: reject the stale lease,
  preserve the new pack and regenerate manifests before a future authorized test.
- Guard restored a menu test; TV then regressed: verify actual files/timing first,
  separate cleanup from runtime failure and do not patch a shader on that report.
- User accepts gameplay except English radio and omits the note: keep only scoped
  acceptance, investigate the native radio producer and leave font testing pending.
- SIDE says43 files but PR includes renderer changes: inspect the full branch diff,
  check for a clean successor and do not merge it under an audio-only description.

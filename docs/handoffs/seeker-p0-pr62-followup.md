# Seeker PR62 physical-failure follow-up

Status: ownership/launch activation correction implemented; left support glove NOT FIXED, bounded diagnostic only. NO-GO for a stable release; GO for MAIN review and one controlled acceptance/diagnostic session. No Steam access, installation changes, launch or merge by SIDE.

Branch: `fix/seeker-p0-retention-support`. Base: `19bd45dad101307a4977ff13c77f2dd2070e8d52` (PR62). The source/DLL receipt is in `seeker-p0-build-receipt.md`.

## Evidence: actual latest PR62 session, not PR60

Read-only snapshot: `build/seeker-p0/pr62-session.log`, copied from the application's local log (2026-10-25 08:55:06, 2,125,865 bytes). SHA256 `1EB237B8ED442C55A9E8CE41564C4B4FDCBA2163DB117290D21B3818C6D8B465`. Private log not committed.

- Line 6105: pawn `1BD46400`, current `3CDC3000`, refusal `weapon-owner-or-name-field`, hook=0, available=0.
- Line 13454: same pawn, Seeker current `3CDC2400`, same refusal.
- Line 16494: tracked right Grip, distance **0.106 m**, radius **0.160 m**, all busy gates zero, unavailable. Other attempts were outside the zone; this one proves moving the zone is not the first fix. Zone unchanged.
- Line 13544: Seeker launch readiness also refused at `weapon-owner-or-name-field`; no successful launch-vector receipt. Actual projectile correction was NOT armed in this session.
- Lines 13911-13916: support latch acquired and SupportTarget computed. These do not establish visible per-eye glove submission. Existing generic final-bound hand diagnostic was exhausted before Seeker (last relevant receipts around line 10283). No latest screenshot was supplied with this follow-up.
- Lines 96-98: global two-hand spread 100, Valkyrie 70, Valkyrie support factor 50 => **70% / 35%**. Existing policy and tests unchanged.

## Proven ownership defect and minimal fix

`NativeSeekerSlot::Read` required reflected `Instigator` offset **0x98** and read that address. Independent archived executable evidence:

- UTF-16 `Instigator` at `0x018236BC`, referenced by Actor serialization `push` at `0x008B64AE`.
- Named serialization reads `mov eax,[edx+0xCC]` at `0x008B64FB`.
- Native `PhysicalFireStart` independently loads its pawn via `mov ecx,[esi+0xCC]` at `0x00E6A1FB`, then pawn virtual getters.

The first code contract was impossible on the audited executable: correct reflection returns CC and the old comparison rejects it. The session's combined message cannot prove there were no additional failing subconditions. New receipts distinguish name-field, reflected offset, and mismatched owner.

Correction: require reflection to agree with audited **0xCC**, then read that field. Classify names before the owner check. Unknown inventory weapons do not veto the snapshot; recognized foreign weapons are skipped and NEVER retained/selected. Broken chain/cycle/dead object/name layout still fails closed. No inventory/ammo writes, spawning or artificial retention list. Native Heavy hook and existing native selector remain unchanged. `selection-requested` still waits for actual `Pawn.Weapon` equality (`selection-equipped`); timeout is not success.

The existing launch correction reuses this validated ownership snapshot. Fixing the refusal allows its original verified hooks to arm; no new aiming algorithm, spread change or rocket behavior change is included. Initial muzzle/ray correctness still requires final spawn receipts. Original main rocket `mIsDrunk` and alternate `mSteerable` remain native.

## Left glove: evidence gap, no speculative pose fix

No visibility mask, IK target, support offset, palette transform or TMD draw rule changed. Current offline SupportTarget algebra passes 15 transformed/scaled cases, but those are not evidence of the actual final game camera/palette/index route. Claiming a fix from these tests would repeat PR62's mistake.

An independent Seeker-only diagnostic now survives earlier ordinary/TMD gameplay. With existing `SeekerSupportGrip=1`, it records the first three frames of each HANDS_WEAPON/HANDS_TMD x support/release state, for exact noTMD/TMD glove source sections. It records:

- `Seeker glove source`: frame, hands/support, both tracking states, native draw range, foreground/animation/shader gates.
- `Seeker glove refused: FgHidden` if that early gate suppresses the source.
- `Seeker glove bound`: each actual eye callback, palette-register count, segments, native/SVNC route, faces, bound IB/VB, stride, ride type and cached target frame.
- `Seeker glove matrix`: final bound wrist skin, LocalToWorld and view-projection, before restoration.
- `Seeker glove completed`: eye callback count, draw HRESULT, IB and palette restoration HRESULTs. An eye callback with zero segments is explicitly NOT a successful visible submission.

No GPU readback, flush, blocking query, geometry allocation or pose mutation. Getter COM references released. This distinguishes missing source/early suppression from palette rejection/zero segments and successful submission at the wrong position. The smallest remaining evidence is one support/release sequence with these receipts. Until then: glove physical FAIL remains open; save/load remains UNTESTED.

## Verification

- Real engine-thread slot adapter: 100 owned native selector calls; byte-exact weapon/ammo conservation; unrelated foreign inventory ahead of real weapons; recognized foreign entries excluded; old 98 poisoned while CC valid; reflected wrong offset refused; owner, deletion, cycle, context, stale pawn, failed hook and actual-equip timeout gates.
- Seeker geometry/launch: 15 affine support/stereo cases; original launch cone and roll preserved; stale/foreign witness rejection; probe budgets independent per state and bounded.
- Real projectile adapter: native fire/start/spawn forwarding, native archetype preserved, scoped corrections and SEH TLS cleanup.
- Archived native + compiled DLL ABI audit: Actor named offset CC, native fire CC, emitted Read comparison and address calculation CC (no +98), hook stack cleanup and native fingerprints.
- WeaponLocker compiled factory still literal `[eax+20Ch]`, correct FName setup.
- 18 critical C++ programs and 12 Python tests plus preview wiring: PASS (dual TMD, hands, cuffs, holsters, reloads, carry, measured aim, accuracy, locker).
- Standalone D3D9 AND D3D9Ex real-device index tests: PASS valid/corrupt identity, both-eye queries, unchanged binding, reset/recreate. Sandbox GPU creation was unavailable; identical test executable passed outside sandbox. These are not game rendering tests.
- OLED CPU/D3D11 suite: PASS including GPU reference <=1 LSB and resource lifetime.
- x86 PE32 `/O2` build and format analysis: PASS. Physical PSVR2: PENDING, no physical PASS claimed.

## MAIN: one short session, after code review

Acquire Seeker legitimately, switch to an ordinary weapon, retrieve/store it with right Grip at the unchanged lower-right back zone; require `ready`, retained-owned and `selection-equipped`, not only requested. With Seeker visible, acquire/release left support twice and fire three shots into open space; record glove visibility and initial launch direction separately from subsequent native drift. Close normally and archive the new log. If retrieval works, check save/load separately before marking it passed. Preserve accepted Valkyrie 70/35, 20 cm reload, other configs/assets and baseline.

# Owned wheel, pickup retention and cinema handoff — 2026-10-20

## Observations and evidence

## Accepted native pickup result —2026-10-20

User physically accepts source663a49a: "super działa" and requests main plus
persistent baseline. Private log session-663a49a-accepted.log: adapter installed
at1508; local supported linked input-thread pickup keep0->1 at18646, chain
mask3->7/count3 at18647; existing selections5799/8297/11387 follow. Another
pickup at52127 changes mask7->F/count4; wheel namedF/iconsF at56896. This proves
native retention, not merely initialization. Initial saved wheel and selection
accepted. No fresh cinema-exit acceptance; persistence across later save reload
has not yet been independently tested.

Exact tested DLL7A1DE5854D1EC8635EE77200E3A39F0D4256C3A73ED6A69E99D3C26417B69495
and INI60224CA08A8E9B01E299AD4B14A069A1E854F2B5E46ADFBB0B01A86973A74ADC
reinstalled persistently after original guard completed; previous baseline
retained in timestamped backups. Canonical local accepted profile updated,
no active rollback guard. No campaign/font/audio/mesh packages changed.
Historical failed approaches below remain evidence, not the current status.

Physical result on source2cdff06: hero hands work in cinema and medkit use.
User had two actual saved guns but saw only Centurion on the wheel; picking a
third gun still discarded one; stereo briefly looked wrong after cinema.
Preserved private log: `build/combined-followups/session-2cdff06.log`.

The log resolves actual possession (masks6 then3, each count2), successfully
selects existing meshes5799/11387, then changes to mask5 after shotgun pickup.
It never records the proposed interpreter-entry pickup scope. This disproves
the claim that the previous implementation retained three guns. There is no
wheel draw-plan/pixel evidence in that session to isolate its missing icon.

## 2026-10-20 physical falsification of8e612e5

User: Centurion missing with Valkyrie equipped on load; third pickup still drops
the equipped gun; cinema exit better but briefly wrong. No acceptance/promotion.
Private evidence: build/combined-followups/session-8e612e5.log. The bounded
adapter installation line at269 is NOT success: there are NO suppression lines.
The exact proposed pickup path is falsified as a complete explanation. Other
callers/native entry or an individual scope refusal must be distinguished next.
Do not widen the receiver/caller policy just to force a hit.

PRE-explicit-load old-world chain publishes mask6 (Valkyrie+Volk). A subsequent
level change clears the pawn cache. First open has ownedReady0/named2 even with
all four atlas icons available; later mask3/named3 appears immediately after
the renderer reacquires the pawn. This establishes a readiness gap, not missing
texture bytes. It does NOT authorize spawning an old discarded Centurion.

Correction: the earlier claim that mask6 described the saved possession was
wrong. User confirms Centurion+Valkyrie in the save, Volk+Valkyrie before exit.
The8c7a022 session independently distinguishes those pawn/load epochs below.

## 2026-10-20 physical8c7a022 result and native pickup followup

User reports initial wheel good, third pickup still discards, game closed.
Private log: build/combined-followups/session-8c7a022.log. Old pawn2DDBA000
mask6 precedes explicit load; loaded pawn1BD76400 mask3 and wheel named3/iconsF
match the actual Centurion+Valkyrie save. After shotgun pickup the mask is6,
not7. ZERO throw receipts despite installed opcode adapter: old interception
does not cover the observed native pickup. Wheel passes only that physical
scope; retention remains BROKEN and cinema has no fresh result.

Owned archived text SHA070D20A0...7B0E1 and constructor-derived SP vtable prove
native virtual+D70 (audit VA010417C0), four stack arguments, ret16. Complete
748-byte FNV1a CC78CCB4EE3651A2. Its only fourth-argument condition at+1A4
bypasses direct TossInventory calls+1D4/+205; creation, selection and ammo setup
remain on the original path. Existing game-authored restore caller already
supplies (archetype,0,0,1). This explains why ThrowActiveWeapon saw nothing.
Verification: tools/reload/verify_native_pickup.py (no process access, no body
output, five mutated-body refusals). Audit addresses are NOT new hardcoded
runtime calls: runtime resolves the class vtable, checks independent HasWeapon
neighbour and FULL function fingerprint, otherwise refuses installation.

Hypothesis: pass keep-existing=1 only for a local supported pickup, preserving
the already-owned chain without spawning guns. New callback requires enabled
KeepAcquiredWeapons (default OFF), exact local SP pawn, linked manager, existing
input thread, supported archetype and no pause/cinema. Preserve all other
arguments and return value. Installation retries at most every2s; log at most12
pickup receipts. No save, ammo, chain or shared-script writes by the callback.

Current L1 PASS:160 policy checks,181 actual-handler/ABI/scope checks,12 Python
checks,16 preparation and6 lease scenarios; independent native verifier PASS.
These include disabled/paused/cinematic/wrong-thread/wrong-receiver/missing
manager/null-archetype refusals, unchanged argument/result forwarding and no
fake-object mutation. No native interception runtime claim yet.

Falsifiers: no installed native adapter/keep0->1 receipt, mask remains two guns,
new gun cannot equip, or pickup changes elsewhere. A receipt is not acceptance:
next runtime must show three actual owned guns and physical selection of each.

Followup fixes the gap with named reflected ownership, not guessed offsets:
Engine Camera.PCOwner export2684, Controller.Pawn1870, Pawn.Controller19809.
Require current live classes and reciprocal ownership; keep the legacy fallback
if any check fails. Cache offsets by class with bounded retries. Wheel-open
requests a snapshot on the existing input thread, not on XR/render threads.
Cached pickup function/script identity is invalidated on pawn/manager loss or
change. Unrelated script calls still forward identically. Up to12 matching
ThrowActiveWeapon opcode receipts report caller/object/receiver, exact code,
thread and identity predicates; up to8 native-entry receipts cover a non-opcode
producer. No per-draw logging or unrelated FName walk.

The exit's first scan finds0/24 valid candidates; only the later scan locks c0.
Recovery now scans at most8 consecutive frames. While recovering, a source with
zero split draws is still one full image and retains mono presentation; even
one split draw forbids that fallback. The original generic scanner remains
unchanged outside this bounded exit. Hands/medkit/VFX/shadow transforms unchanged.

## Pickup mechanism and smallest experiment

Read-only Engine inspection finds Pawn.ThrowActiveWeapon export20311 with36
script bytes; Vehicle override24533 has4. RvGame-only inspection missed these.
The previous flat byte solver is not a recursive UE3 decoder; it excludes
zero-operand tokens. Its nine-byte call claim is invalid: the audited call at200
includes FName201..208, EmptyParmValue209, EndFunctionParms210.

Archived owned ProcessInternal disassembly independently shows frame Node+16,
Object+20 and Code+24. Two opcode dispatch sites read the same table; the
interpreter increments Code before invoking the opcode member function with
the original two-argument thiscall ABI. Verification tool:
`tools/reload/verify_inventory_vm_dispatch.py` (explicit private Engine/text/base/
interpreter arguments; no process access or proprietary body output).
Engine SHA B6B481EF…71A34, text SHA070D20A0…7B0E1.

Hypothesis: intercepting VirtualFunction at its exact nested pickup expression
can suppress the unwanted throw while leaving normal pickup/equipping alone.
Implementation: discover the opcode table from TWO matching interpreter sites
and audited frame-access shapes, atomically replace only slot0x1B, forward all
other calls. Default OFF via existing KeepAcquiredWeapons. Suppression requires
the local pawn receiver, matching manager/frame Object, matching caller Node,
exact Code address, engine input thread and complete original script identity.
It consumes the remaining ten call bytes in THIS frame, not shared script data.
No weapon spawn, save write, ammo mutation or arbitrary drop suppression.

Falsifiers: adapter installed but exact suppression never occurs; suppression
occurs but chain still drops to two; pickup/equipping/quest behavior breaks.
Successful initialization is not runtime proof. No widening the scope on a miss.

## Wheel

Two concrete hazards are removed: renderer read mutable items/count while the
engine thread rewrote them; hand-anchored circle could extend outside an eye
or disappear when the hand projection failed. Rendering now uses one atomic
mask for labels/icons and fits the complete circle in central view. Selection
directions and catalog order are unchanged; no unowned weapons are displayed.
Bounded draw-plan log reports ownership readiness/mask, named sectors, resolved
icons, atlas, order, layout and mapped meshes. If named=3/icons=F but the second
gun is absent, the remaining cause is downstream rendering, not inventory.
These fixes are justified independently; missing-icon root cause remains
unproven until the new draw-plan evidence and physical result.

## Cinema handoff

Present advances classification AFTER completed scene draws, but composition
formerly read the NEW classification. Thus the last full-width mono frame was
interpreted as two unrelated halves. A pure completed-frame policy now preserves
that frame's old mode across cinema/gameplay edges; Bink/menu paths stay immediate.
The log additionally shows zero valid camera candidates and mono fallbacks after
the exit. The generic lock failure delay was60 frames, followed by rescanning.
Exit now revokes stale matrices/lock and arms a fresh scan AFTER scan reporting,
so the next frame can actually populate it. No artificial eye swap or yaw offset.

Falsifier: mono frame stamp is correct and fresh lock is acquired but the
perceived inversion remains; capture per-eye camera state next, no guessed signs.

## Gates / next physical route

L1 PASS:160 policy checks,153 actual-handler/ABI/scope checks,12 Python checks,
16 preparation and6 lease cases. The actual adapter is exercised on a fake
opcode table, including unchanged-script forwarding; all16 ownership masks
roundtrip without reading the mutable engine list. Six frontend/cinema CPU
programs PASS, including the completed-frame transition policy. Full reload
regression PASS (37 compiled programs and108 Python tests, synthetic/private
owned fixtures only). Wheel geometry includes1204 offscreen-hand fit cases.

Offline structural evidence and fake-engine tests are not L3/L4. Preserve the
accepted combined installation; prepare privately and wait for fresh `start`.
One headset route, no mid-test chat: load the two-gun save, open wheel immediately
and select both; acquire a third gun and select all acquired guns; watch the
cinema exit and medkit once; close normally. MAIN reads suppression, ownership
and wheel draw-plan logs independently. Stop if stereo causes discomfort.
Do not promote until the corresponding physical scopes pass.

# STATUS — session handoff

LATEST MAIN 2026-10-17 startup/Centurion candidate deployed; L3 pending.
User reported the prior candidate curtain only appeared briefly and the
Centurion cylinder was detached beside the gun. Prior runtime log showed
`ready=3F`, `hold=0 elapsed=110ms`: hook/pawn/XR readiness, not UE3 streaming.
Prior INI had all four Centurion visual/refill flags OFF, so it did not exercise
the tracked-cylinder path. Corrected candidate now has a 3000ms minimum visible
curtain (5000ms existing fail-open maximum), `StartupPrewarm=1`, and the reviewed
CenturionVisualFollowup/TrackedCylinder/PhysicalPolish/RefillOnly stack ON.
All other parsed INI values are held at the accepted baseline. UI package is
untouched. L2_PASS clean c0393dd, x86 PE32 /O2, DLL SHA256
789749E5D4DB8FC6C603DF599A38A4FB1EC00DE374E4E2FDD469B09B9C5F361B.
Deployment receipt `deployment/install-reports/20261017-192942.json` verifies
DLL `789749E5…F361B`, INI `8B0B0B4C…2CCFAF`, unchanged dinput8
`5B948DDB…729D59`, and backup `deployment/install-backups/20261017-192942`
with accepted DADE170B/INI/dinput8 hashes. UI package was not a target and is
still SHA256 `51CDAB5C…1A1A`. A 10-minute bounded restore lease is armed; it
waits for the game to exit and restores only if candidate hashes still match.
PSVR2 visual check is pending; the curtain is not proof that all UE3 assets
have loaded.

OFFLINE follow-up now adds a 3000 ms minimum visible hold, clamped to the
existing 5000 ms fail-open timeout. It is a visible-duration policy, not a claim
that all UE3 assets have streamed. The next candidate config will enable only
the reviewed Centurion stack: VisualFollowup, TrackedCylinder, PhysicalPolish,
and RefillOnly; all other accepted INI values remain unchanged.
L1_PASS: startup suite (6 optimized C++ fixtures, 5 Python guards) and full
Centurion suite (20 policy fixtures + own-process ABI and two D3D9 fixtures;
62 Python tests). All own-process GPU fixtures passed; no game/Steam proof.
L2_PASS clean c0393dd, x86 PE32 /O2, DLL SHA256
789749E5D4DB8FC6C603DF599A38A4FB1EC00DE374E4E2FDD469B09B9C5F361B.
Candidate source and test notes: docs/startup-readiness.md. New flags remain
default-OFF in the source; only the guarded test INI enables them.

LATEST 2026-10-17 SIDE startup readiness candidate (dedicated branch from92396ca):
user reports immobile visible gameplay while hooks initialize. Runtime cause of
native movement block UNKNOWN. Source/log evidence: pawn at index114610 after
bounded512-object slices, frame-count script/aim retry latency. Default-OFF
StartupPrewarm prefers reflected reciprocal Controller.Pawn/Pawn.Controller,
retains original bounded fallback, rearms offset retry on new pawn, retries
missing hooks by bounded wall time and does not mark failed installation ready.
Separate default-OFF StartupLoadingGate is presentation ONLY, stable100ms,
timeout5000ms, menu/pause/cine bypass, NO extra input/engine pause/clock writes.
Probe logs exact missing core readiness, not proof of native move acceptance.
L1_PASS6 own-process programs incl 2-round D3D9Ex/state/failure GPU fixture,
5 Python structural guards. L2_PASS clean e243f97, x86 PE32 /O2, DLL SHA256
CE297F0AF8BADFAE941FB500BE8A07D9D139DCCF15E427FCABD2A2CC3867F003.
Private build/startup-readiness/candidate-e243f97 (NO INI/assets). All new flags
remain OFF; no runtime speedup/native movement/curtain L3/L4 claimed. ZERO Steam.
See docs/startup-readiness.md for probe-only baseline, fast-path and curtain gates.

LATEST 2026-10-17 MAIN recoil-release L3+L4 PASS (build BEF82DC6 + 5 flags):
user PSVR2 "super" on first-shots-after-close route (native recoil back, no
frozen gun). Scoped to this build/config/route; user ordered merge to main.

LATEST 2026-10-17 MAIN recoil-release L3 DEPLOYED (SIDE 4962b7a, PR48):
candidate DLL SHA-256 BEF82DC6A5878CE036F66E15892118647987524E667E1CA15DA22B10E0E4CBCC,
PE32 x86 /O2, clean tree 146388e, manifest dirty=[], guard preflight+postflight
PASS; backup deployment/install-backups/20261017-144550. STATUS: CANDIDATE
ONLY — first-shots animation still needs PSVR2 (this test).
WHY: user reports first two shots missing animation (not reload): refill-only
close left the transaction idle pose + frozen right hand mounted (holds through
2600 ms tail). FIX (inspected): in refill mode, Time() goes inactive 280 ms
after close (close anim kept), root falls back to LIVE native constants, right
capture stops; OFF path byte-identical (ReleaseNative=false). Fail-closed
(LiveRoot rigid check). L1_PASS (EXIT=0 incl. 280ms/live-root + legacy tail),
L2_PASS (clean). Steam: DLL=BEF82DC6, INI=5314150A (same 5-flag set; pre-edit
backup in 144550 dir). Path: rollback to DADE170B (postflight PASS), redeploy
(guard model intact). Rollback: deploy Rollback -BackupPath 20261017-144550
(+plan paths) + drop the 5 added INI lines. AWAITING L3+L4: full manual reload
-> close -> FIRST shots must show native recoil/fire animation (no frozen gun),
counts exact; then normal behavior. Falsifier: shots still frozen -> release
theory wrong. MAIN reads post-close draws (no override) + user eyes.

LATEST 2026-10-17 MAIN MERGED TO main (user order): d32eb54 (merge of tested
stack: PR48 ports visual/tracked/native-space/polish/refill/diagnostics +
MAIN reseat; PR47 grip-integration already in main via 276a952). Source tree
spikes/tools identical to L1/L2-tested f3fe2a5; deployed Steam 6B0664ED
unchanged and still valid. Draft PR48 now superseded for ported commits (SIDE
branch untouched). Steam install + accepted profile unchanged by merge.

LATEST 2026-10-17 MAIN resolver-fix L3+L4 PASS (build 6B0664ED + 5 flags):
user PSVR2 "jest okej" on the counter-to-6 + fire-without-animation/shake
route. Scoped to this build/config/route; user ordered merge to main.
SIDE small fixes may still follow; NOT global acceptance.

LATEST 2026-10-17 MAIN resolver-fix L3 DEPLOYED (SIDE 56b3f16, PR48): candidate
DLL SHA-256 6B0664ED83EB579DBCACB2BFF4DE7BB38CF2B49A5CFFC8D6F906156739F5408F,
PE32 x86 /O2, clean tree f3fe2a5, manifest dirty=[], guard preflight+postflight
PASS; backup deployment/install-backups/20261017-142527. STATUS: CANDIDATE
ONLY — counter-to-6 + no-animation/no-shake still needs PSVR2 (this test).
WHY: 14:16 preflight proved ready=0 AFTER all 4 bindings matched (unrelated
tail field-name failed). FIX (inspected): stop walk at found==4; incomplete
walk still fails closed; all fingerprint/cast checks intact (py asserts order
+ single result=cached). L1_PASS (EXIT=0 full suite), L2_PASS (clean). Steam:
DLL=6B0664ED, INI=5314150A (same 5-flag set; pre-edit backup in 142527 dir).
Path: rollback to DADE170B (postflight PASS), redeploy (guard model intact).
Rollback: deploy Rollback -BackupPath 20261017-142527 (+plan paths) + drop the
5 added INI lines. AWAITING L3+L4: full manual reload -> counter to 6, then
fire WITHOUT Reload animation and WITHOUT camera shake; exact counts, no
double-add. MAIN reads `refill-only: result=2` (Conserved) + no ReloadFinished.

LATEST 2026-10-17 MAIN refill-diagnostics L3 DEPLOYED (SIDE 9c38667): candidate
DLL SHA-256 4105C00457D0C27A581D71B1A08B361F6A3EEDDC4D1E9F25703DDA31611645EB,
PE32 x86 /O2, clean tree ddbdb80, manifest dirty=[], guard preflight+postflight
PASS; backup deployment/install-backups/20261017-141545. STATUS: CANDIDATE
ONLY — fix open. WHY: close gesture arrives but resolver ready=0 (0 rounds).
WHAT: bounded read-only audit (named reject stages, behavior 1:1) + 3x
auto-probe on BLOCK (engine thread, >=1 s apart, no invocation, no gesture
needed). L1_PASS (EXIT=0 incl. refill policy/ABI), L2_PASS (clean). Steam:
DLL=4105C004, INI=5314150A (same 5-flag set; pre-edit backup in 141545 dir).
Path: rollback to DADE170B (postflight PASS), redeploy (guard model intact).
Rollback: deploy Rollback -BackupPath 20261017-141545 (+plan paths) + drop the
5 added INI lines. AWAITING SHORT MEASUREMENT (no full reload): empty
Centurion -> fire attempt -> close game. MAIN reads `centurion refill
resolver:` stage (which of: layout/object/class-name/children/field-*/wrapper/
vtable/method/cast vs ready) + `refill-only: result=/ready=`.

LATEST 2026-10-17 MAIN refill-only L3 DEPLOYED (SIDE 6d7b201, PR48): candidate
DLL SHA-256 4F54E47B970BD947625519DA0E439569F69BB45955C3B44EA288D2E41C4A97E7,
PE32 x86 /O2, clean tree 55d3f66, manifest dirty=[], guard preflight+postflight
PASS; backup deployment/install-backups/20261017-135058. STATUS: CANDIDATE
ONLY — NOT accepted. No-delay/no-shake still needs PSVR2 (this test).
Scope (inspected): Permit path calls ONLY fingerprinted RefillClip (+3 int
getters), same inventory cast as AttemptReload, conservation-verified
(clip+moved/reserve-moved) else sticky Fault; never falls back to Reload;
holstered/changed/stale/NPC states refuse; OFF keeps legacy dispatch.
L1_PASS (EXIT=0: refill policy + ABI refill/no-original + full suite),
L2_PASS (clean). Steam: DLL=4F54E47B, INI=5314150A (+VisualFollowup
+TrackedCylinder +PhysicalPolish +RefillOnly, VfxHost=1/HideMuzzleFx=0;
pre-edit backup in 135058 dir, all-CRLF). Path: rollback to DADE170B
(postflight PASS; guard also caught leftover HideMuzzleFx=1 from isolation
(b) — reverted), redeploy (guard model intact). Rollback: deploy Rollback
-BackupPath 20261017-135058 (+plan paths) + drop the 5 added INI lines.
AWAITING L3+L4 (NO screenshots): fire to partial -> Circle/open/UP-dump/
shoulder/insert/flick -> refill must be IMMEDIATE (no ~1.5 s wait), NO camera
shake, counts exact, no double-add; NPC/holster/store paths unchanged.
MAIN reads `centurion refill-only: result=2` (Conserved) + absence of
ReloadFinished after it.

LATEST 2026-10-17 MAIN VFX isolation (b) RESULT (user) + log: plugin-only shows
NO smoke + NO flash, but shot effect visible UNDER the gun instead of muzzle.
Log 13:41: init loaded, overlay n=1 x12 (budget saturated, items flow). Both
SIDE predictions confirmed: smoke was native (gone in (b)); anchor bug visual
(GunPivotWorld + 150 mm fwd, zero UP). (a)+(b) combined: native-only = total
silence, plugin-only = misplaced small effect. DECISIONS for SIDE (needs code+
stub+L1/L2, then MAIN L3): UP component/per-gun muzzle table (pivot!=muzzle,
150 mm short for rifles); bigger/longer stub flash (4.5 cm/33 ms invisible);
stay one-system (plugin-only (b) kept installed: VfxHost=1/HideMuzzleFx=1,
INI 52CCD7D4). Native contributes nothing visible in VR.

LATEST 2026-10-17 MAIN VFX isolation (b) RUNNING: VfxHost=1 + HideMuzzleFx=1
(plugin-only; INI 8D0E27E4->52CCD7D4, pre-edit backup in 132624 dir).
Session (a) RESULT (user): native-only shows NOTHING (no smoke, no flash) —
native FX invisible in VR regardless of seam. Expect (b): smoke gone (was
native), flash small/low per SIDE prediction. Then decisions (UP/per-gun
muzzle, bigger stub flash, one system).

LATEST 2026-10-17 MAIN VFX isolation (a) RUNNING: VfxHost 1->0 (native-only,
HideMuzzleFx=0 kept; INI 59C48A88->8D0E27E4, pre-edit backup in 132624 dir).
Expect zero VFX lines; user records smoke/flash below. Then (b) plugin-only.

LATEST 2026-10-17 MAIN physical-polish L3+L4 PASS (build 37AC0AD2 + 4 flags):
user PSVR2 "okej dziala" + log 13:29 session proves TWO full manual cycles
(opened->dumped->grab->loaded->inserted->close->ACK, clip 3->0, reserve
16->13): insertDist 0.064 m at Loaded (was 2.4-3.0 m), socketFresh=1,
nativeSpareDraws 636->2960, rightHeld/palmPublished counting, conserved ACKs.
Scoped to this build/config/route; SIDE small fixes still expected; NOT global
acceptance. VFX trigger L3 PASS same session: init plugin=loaded atlas=
vfx_atlas dir, overlay n=1 x12 (budget saturated). Visual placement (UP/size/
smoke-source) still needs isolation (a)/(b) + user eyes.
VFX flag check (SIDE ae62c51): installed INI = 59C48A88, VfxHost=1,
HideMuzzleFx=0 — NOT the silent combo; single INI copy under game tree;
13:29 log seams=1. The seams=0 session predates the flag edit. One earlier
unexplained revert to 7ED1239C (12:56) not reproduced since; flag re-verify
after every session continues.

LATEST 2026-10-17 MAIN physical-polish L3 DEPLOYED (SIDE 9ccbe96+d078491):
candidate DLL SHA-256 37AC0AD2AB667C1C3C5DBA18660188CAEEFB57B1478BD7495F02235DF76B6961,
PE32 x86 /O2, clean tree 5c800b7, manifest dirty=[], guard preflight+postflight
PASS; backup deployment/install-backups/20261017-132624. STATUS: CANDIDATE
ONLY — NOT accepted, NOT flagged 100%.
Scope (inspected): UP-only dump + rearward cases + palm-fit spare + right-hand
hold (flag-gated; OFF keeps legacy down-dump/slow-close/authored paths);
holster Stored cancels tap + revokes permit, native gate block-only when
holstered; health plate hides ONLY in fresh Carrying; deliberate close needs
QuickFlick (0.45 m/s or 2 rad/s <=100 ms); PhysicalPolish auto-disables without
native noTMD glove path (log line). L1_PASS (EXIT=0: 216-case polish + full
suite), L2_PASS (clean). Steam: DLL=37AC0AD2, INI=59C48A88 (+VisualFollowup
+TrackedCylinder +PhysicalPolish, VfxHost=1; pre-edit backup in 132624 dir,
all-CRLF). NOTE: INI found reverted to 7ED1239C before this deploy (all 3 prior
flags gone; cause unknown — panel save suspected; re-verify flags after every
session from now on). Path: rollback to DADE170B (postflight PASS), redeploy
(guard model intact). Rollback: deploy Rollback -BackupPath 20261017-132624
(+plan paths) + drop the 4 added INI lines. AWAITING L3+L4 (SIDE route, NO
screenshots): 3 shots -> Circle -> muzzle UP (cases toward shooter) -> shoulder
ammo (plate clears, clip between fingers) -> insert -> 1 s -> fast flick (only
cylinder moves, ONE refill) -> 2 shots -> Circle -> STORE w/o reload -> 3 s ->
draw (must NOT refill). MAIN reads `centurion physical` + `tracked cylinder`
rows + native ACK/BLOCK.

LATEST 2026-10-17 MAIN VFX isolation (a) NATIVE-ONLY staged (SIDE runda 2,
b957289): 12:29 log proves plugin path alive (init loaded, atlas=vfx_atlas
dir, overlay n=1 x6) — placement/size question only. Session (a): VfxHost 1->0
(HideMuzzleFx=0 kept; INI 9222CC60->D8050DF2, pre-edit backup in 125231 dir).
Expect: no host-init/overlay lines; native smoke+flash as before (seam defect
included). Next (b): VfxHost=1 + HideMuzzleFx=1 (plugin-only; expect smoke
gone, flash small/low) -> decisions: UP component/per-gun muzzle table, bigger
stub flash, one system at a time. Centurion flags untouched throughout.

LATEST 2026-10-17 MAIN native-space L3 DEPLOYED (SIDE 5d5afd7, PR48):
candidate DLL SHA-256 E58E3E57C4BA6CCACB6CA5CA49BA86038CC5DF09E080DB4770EB9FA52662E188,
PE32 x86 /O2, clean tree cf71a4a, manifest dirty=[], guard preflight+postflight
PASS; backup deployment/install-backups/20261017-125231. STATUS: CANDIDATE
ONLY — NOT accepted, NOT flagged 100%.
WHY: 12:29 session on 0742736 (04CC6CA2) proves the bug SIDE fixed — ammo valid
(clip 5->3 fired), plate drew, Circle->cylinder-opened, but tracked socket at
y≈-2.4 (2.4-3.0 m insertDist, grab/discard loop, zero inserts): ActorX/export
vs UE3-space mixing. FIX (inspected): ToNative undoes bind reflection ONCE on
parent/inverse (SVCA file stays v1); Sample skips legacy reflect only for
nativeSpace data; tracked Attached+Spare use NativeAnimation; legacy paths keep
export-space Data (OFF behavior byte-identical); fail-closed when unready.
shells.h change comment-only. LIMIT: NativeVertices GPU oracle needs SIDE
private assets — NOT run here (CPU + spare-GPU + IB fixtures green).
L1_PASS (EXIT=0 full suite incl. test_native_space.py), L2_PASS (clean).
Steam: DLL=E58E3E57, INI=9222CC60 (+VisualFollowup +TrackedCylinder, VfxHost=1
restored; pre-edit backup in 125231 dir, all-CRLF). Path: killed lingering
12:46 PID 37480 (user auth), rolled back to DADE170B (postflight PASS),
redeployed (guard model intact). Rollback: deploy Rollback -BackupPath
20261017-125231 (+plan paths) + drop the 3 added INI lines. AWAITING L3+L4
(SIDE route, NO screenshots): Circle -> LEFT open at gun (no meter jump);
tilt dumps, gun held; shoulder ammo -> rendered socket, insertDist <0.2 m;
WAIT 2 s open; flick ONE refill; repeat, pause/resume, exit. MAIN reads
`native space` + `tracked cylinder` rows + native ACK.

LATEST 2026-10-17 MAIN tracked-cylinder L3 DEPLOYED (SIDE 0742736, PR48):
candidate DLL SHA-256 04CC6CA23588D7BA929365BDB56A7B50EA0DAF970D9E3AF10FCA5BF797A8FE70,
PE32 x86 /O2, clean tree c738a97, manifest dirty=[], guard preflight+postflight
PASS; backup deployment/install-backups/20261017-122757. STATUS: CANDIDATE
ONLY — NOT accepted, NOT flagged 100% (SIDE corrections still expected).
Scope (inspected before port): Circle->Open/Opened with circleOpens (OFF keeps
legacy Unlocked); insert vs RENDERED socket + carried-mesh offset (OFF keeps
authored socket, insertOffset zero-finite); socketFresh defaults true (shell
gate unchanged when OFF); witness SRWLock + 100 ms + identity binding, resets
in Cancel/XR-init/Release; shell launch needs fresh witness only when ON.
OFF path byte-behavior preserved. L1_PASS (EXIT=0: tracked 2916 cases + wiring
py + full suite), L2_PASS (clean). Steam: DLL=04CC6CA2 on disk, INI=9222CC60
(+VisualFollowup=1 +TrackedCylinder=1, VfxHost=1 restored; pre-edit backup in
122757 dir, all-CRLF). Path: rolled back 49CF1838->DADE170B (postflight PASS),
redeployed (guard model intact, profile still pins DADE170B). Rollback: deploy
Rollback -BackupPath 20261017-122757 (+plan paths) + drop the 3 added INI
lines. AWAITING L3 log + L4 PSVR2 (SIDE route, NO screenshots): Circle with
spent Centurion -> immediate LEFT open; tilt dumps cases, gun stays in right
glove; LEFT-shoulder grip, ammo to open cylinder, release, WAIT 2 s (stays
open); flick -> ONE refill, no second sweep; repeat, pause/Options/resume,
normal exit. MAIN reads `tracked cylinder` rows (socketFresh/insertDist),
native ACK, accepted systems unchanged.

LATEST 2026-10-17 MAIN VFX trigger L3 (SIDE docs/vfx-trigger-diagnosis.md,
push 25e1c50): MAIN checks DONE. Installed: SingularityVfx.dll PRESENT
(E371EF77..., 10752 B, 7 required VfxPlugin* exports verified via dumpbin) +
vfx_atlas/ PRESENT (4 DDS + manifest v1 abi 1, CC0 packs). INI was VfxHost=0 /
VfxSmoke=0 / VfxScreenFlash=0 / HideMuzzleFx=0. Log 12:00 session (65 shots):
ZERO `VFX overlay:` + ZERO `VFX host init:` — fully explained by VfxHost=0
(init returns early line 21494, FILL+DRAIN gated; aim-mode gate never reached).
FIX (single variable): INI VfxHost 0->1 (hash 0579F9EB->ACB368DD, pre-edit
backup .../120644/SingularityVR.ini.pre-vfxhost, all-CRLF). Smoke/Flash/
HideMuzzleFx untouched (smoke deliberately out of headset path line 21446;
native flash stays as fallback so player never left with nothing). No DLL
change, no guard deploy. AWAITING L3: fresh boot, fire shots, expect `VFX host
init: plugin=loaded ... atlas=vfx_atlas dir` + `VFX overlay: n>0`; user reports
flash visible per eye / double-flash with native / nothing. Rollback: set
VfxHost=0 (backup hash 0579F9EB).

LATEST 2026-10-17 MAIN fix/centurion-visual-followup-integration DEPLOYED for L3
STATUS: CANDIDATE ONLY — NOT accepted, NOT flagged 100% working (user order);
SIDE visual followup still needs corrections. Preserved as-is, no claims.
(candidate DLL SHA-256 49CF183865487901C7A6958CC18DD98BD6892D7ED3B8587B012D4A87D3F16D01,
PE32 x86 /O2, clean tree e079e36, manifest dirty=[], guard preflight+postflight
PASS; backup deployment/install-backups/20261017-120644).
WHY (user: Circle dead, no plate, sessions 11:44+12:00): log proves Circle seen
(circle=1) but ammoValid=0 — RvHUD reads clip=-1/reserve=-2 persistently; plate
never drew either (shared FindHudObject latch, untouched code). New PR48 code
never executed (zero Unlocked) — exonerated. Root cause: latch keeps the FIRST
readable RvHUD-deriving instance; 12:00 session latched ONE zombie (1D429800,
same class+offsets as good reads) all session, while the 11:21 good session
re-latched 1D432000->33A06000 and read clip=5/6. Latch lottery, pre-existing.
FIX (smallest experiment): HudReseatPolicy in centurion_reload.h (pure,
203-case L1) + FindHudObjectExcept in d3d9.cpp — only a 5 s eligible invalid
streak reseats to a DIFFERENT instance (6/epoch max, 5 s cooldown, menu gaps
and valid reads reset; good sessions never trigger). Falsifier: reseat fires
but all instances stay invalid -> wrong-instance theory dead, pivot to timing.
L1_PASS (EXIT=0 full suite), L2_PASS (clean). Steam: DLL=49CF1838 on disk,
INI=live+VisualFollowup=1 (0579F9EB, pre-edit backup in 120644 dir, all-CRLF).
Rollback: deploy Rollback -BackupPath 20261017-120644 (+plan paths) restores
DADE170B; INI revert = drop the one added line (verified 7ED1239C roundtrip
earlier). Path here: 58F82A7C deployed, rolled back to DADE170B (postflight
PASS), redeployed 49CF1838 — guard model intact, profile still pins DADE170B
(unaccepted builds never re-pinned). AWAITING L3 log + L4 PSVR2: fresh boot,
ammo plate visible? (expect `HUD reseat:` then `ammo ... drew`), Circle ->
Unlocked -> compact open -> LEFT-shoulder insert -> WAIT unseated -> flick ONE
refill; pause/cancel, reload again, exit; no regressions.

LATEST 2026-10-17 MAIN fix/centurion-visual-followup-integration DEPLOYED for L3
(candidate DLL SHA-256 58F82A7C2B75F03532B8955A2EAA0B7AB4D485CDB097A402E8CCE1CC45996147,
PE32 x86 /O2, clean tree f7d601f, manifest dirty=[], guard preflight+postflight
PASS via re-pinned profile 4BF0D8B0 + files-only plan DADE170B->58F82A7C;
pre-deploy backup deployment/install-backups/20261017-114157 verified).
Steam verified: d3d9.dll=58F82A7C on disk; INI=live+CenturionVisualFollowup=1
(hash 0579F9EB, pre-edit backup .../SingularityVR.ini.pre-visualfollowup,
all-CRLF, single-key delta; CylinderAnimation=1+ShellEjection=1 retained).
L1_PASS (cmd rerun EXIT=0: 16 CPU + ABI + 6 Python + IB witness + spare-GPU
10609/10609 both rounds). L2_PASS (clean rebuild). Re-pin rationale: profile
now pins user-accepted DADE170B (L3+L4 PASS render-only "dziala"); all other
installed files hashed equal to old pins, live INI zero-drift vs old profile.
No new assets. Rollback: deploy Rollback -BackupPath 20261017-114157, or
restore DADE170B + INI 7ED1239C. AWAITING L3 log + L4 PSVR2 (NO screenshots):
(1) intro identical both eyes; (2) fire, Circle/open/dump, LEFT-shoulder ammo,
insert, WAIT 2s no flick (must stay visibly unseated); (3) flick close, ONE
refill, no second gun sweep; (4) pause/cancel, reload again, normal exit;
(5) no regressions (plates/rings/menu/shadows/FPS/monitor). MAIN reads
`ini: CenturionVisualFollowup=1`, `visual followup:` lines, nativeSpareDraws.

LATEST 2026-10-17 MAIN fix/centurion-visual-followup-integration (from db0aedb):
port of SIDE draft PR #48 source ONLY (5 files, verbatim from
origin/fix/centurion-visual-feedback@d6d427e: centurion_visual_followup.h,
d3d9.cpp wiring, 2 CPU/GPU fixtures, test-centurion.ps1; SIDE STATUS/docs NOT
ported). Symptom (user physical): cylinder full 42.34UU withdrawal, albedo-only
spare over hand/gun, Loaded auto-closes in 280ms, close re-runs native reload.
Experiment: default-OFF CenturionVisualFollowup (requires CylinderAnimation=1,
else forced 0): compact frame12 open, Loaded holds frame26 until flick,
26..32 only on close, native root held through tail, spare = second native DIP
same VB/IB/material/depth inside each eye, 15 CTAB regs restored. OFF path =
old centurionanim::Palette (accepted behavior preserved by construction).
Falsifiers: frame displacement > frame12; Loaded closes w/o input; non-gun draw
modified; material/RT/depth leak; spare still over nearer geometry.
L1: rerun via cmd in progress (first run: all 16 CPU + ABI + 6 Python suites +
IB witness + spare-GPU 10609/10609 both rounds PASS; outer exit=1 was PS
stderr-dot harness noise, not a suite failure). L2: PE32 x86 /O2 build SHA-256
0A496C3A76A867CC26B9580CD444D9D5F73EFC8C12B3BD2CE508ED42BBCAEFE2 (dirty-tree;
clean rebuild after commit). Installed Steam still DADE170B (prior L3+L4 PASS
render-only scope; this experiment NOT covered). Game closed. Next: commit,
push, clean rebuild (manifest dirty=[]), deploy DllOnly + INI
CenturionVisualFollowup=1, L3 log check, user PSVR2 route. Rollback: deploy
script timestamped backup + restore DADE170B on critical failure.

LATEST 2026-10-17 MAIN fix/centurion-grip-integration L3+L4 PASS (user PSVR2:
"dziala", render-only, NO screenshots per standing order): intro identical in
both eyes, single clamped ammo plate per eye, scripts hold then release on
fresh boot, holster/knife rings hidden in menu+pause, visible in gameplay,
Centurion shoulder/cylinder/shell behavior intact, no regressions. Deployed
build DADE170B05EB6BA45569926156ADAC4629DEEC9EB787B00A8371D054A86F25AE
(PE32 x86 /O2, guard pre/post PASS). Rollback path verified:
install-backups/20261017-111929 (d3d9.dll F5E925, INI a6f4b973) + remove
vr-assets/reload/centurion/. PR to main open for review (no self-merge).

LATEST 2026-10-17 MAIN fix/centurion-grip-integration DEPLOYED for L3
(candidate DLL SHA-256 D35A9EAD05EB6BA45569926156ADAC4629DEEC9EB787B00A8371D054A86F25AE,
PE32 x86, guard preflight+postflight PASS via change-plan-centurion-integration.json;
pre-deploy backup install-backups/20261017-105337 verified: d3d9.dll F5E925..., dinput8.dll 5B948D..., INI a6f4b973).
Steam verified: d3d9.dll=D35A9EAD (on disk), INI=live+CenturionCylinderAnimation=1+
CenturionShellEjection=1 (all else byte-identical to accepted baseline),
5 private assets staged+triple-hash-verified (side manifest==source==installed)
under Binaries/vr-assets/reload/centurion/. Accepted profile re-pinned to the
user-verified good state (d3d9 F5E925, Fill=1/Window=0/Capture=1, story_voice
4FEEC18C...E020==live, mtime Oct-16-stable); the 6600708D profile pin had no
matching binary on disk. Port scope: ONLY missing Centurion fixes (two-eye
mid-eye shoulder + latched yaw, cooked-IB native animation, cosmetic shell
ejection, NativeSize, DrawInstances); accepted features preserved (cuffs,
camera stabilization, menu rows, F12, L3+R3, mirror). L1/L2 PASS on final
tree (incl. GPU fixtures vs INSTALLED assets). AWAITING PSVR2 (NO screenshots):
(1) intro identical in both eyes, (2) Circle → sideways gun motion opens →
muzzle down ejects (watch for shell burst), (3) fresh cylinder from LEFT
shoulder, insert, flick to close, (4) no artifacts/regressions (hands/shadow/
menu/monitor single image/HMD normal), normal exit. On FAIL: restore backup
DLL+INI (hashes above), remove vr-assets/reload/centurion/, verify hashes,
report log lines (centurion shoulder/mismatch/witness).

LATEST 2026-10-17 MAIN fix/centurion-grip-integration (base local main
513013a): ported ONLY the missing Centurion grip/shoulder fixes from
fix/centurion-grip-shoulder — two-eye-midpoint shoulder + latched yaw,
cooked-IB native cylinder animation, cosmetic shell ejection, NativeSize
clip scaling, DrawInstances, LoadIni flags CenturionCylinderAnimation /
CenturionShellEjection (default OFF), INI InsertRadiusCm + shoulder
magnitude. Deliberately EXCLUDED: glove-cuff removal, camera-stabilization
simplification, PR_GLOVECUFFS/SHAKESCREEN/BOBSTABILIZE row removals, F12
stereo toggle, L3+R3 screenshot trigger, mirror/scope/guard reverts,
UpdateCameraStabilization deletion (accepted features preserved).
L1_PASS mirror ps1 + full reload suite incl. GPU fixtures vs private
original assets (shells L/R stereo pixels) + knife suite. L2_PASS clean
x86 PE32 /O2 DLL SHA-256
D35A9EAD05EB6BA45569926156ADAC4629DEEC9EB787B00A8371D054A86F25AE
(provenance: branch fix/centurion-grip-integration @76eedc7 clean tree;
rebuilt 2026-10-17, manifest dirty=[]).
Private assets (NOT in git, hash-verified read-only): centurion_shell.svrk
AB61034E..., centurion_shell.svrt F804945B..., centurion_reload.svca
85ABCDDE..., centurion_clip.svrk F2196114..., centurion_clip.svrt 17A3355A...
(vr-assets/reload/centurion/ at deploy). L3/L4 UNKNOWN. READY_FOR_PHYSICAL_ACCEPTANCE of render only
(NO screenshots per standing order).

LATEST 2026-10-16 SIDE fix/right-eye-mirror-steam-capture (PR #43): complete right-eye
aspect fit, opt-in monitor work-area window cap 1920x1080 (physical client px),
no changing XR/scene resolution commands. Fixed double-applied aspect scaling;
offscreen composition then commit avoids clear-before-failed-copy black frame.
L3+R3 no longer saves BMP; optional recenter/recorder/wheel remain. F12 no longer
switches debug stereo. Opt-in SteamCaptureBackbuffer scopes completed mirror RT0
around original Present, bounded identity/failure logs, exact RT/DS/MRT/viewport
restoration. Steam black screenshot hypothesis NOT runtime-proven.
L1_PASS geometry/fault CPU checks, own-process D3D9Ex pixels/source/swap/cinema/
RT0 scope/window cap/Reset and 6 Python contracts; six menu/cinema regressions.
L2_PASS clean source 4513db6, x86 PE32 /O2 DLL SHA-256
B8E9AE47DB0C9780B0FB684A766DAA167F404D09CC52CAA0152EC582DCA9EF73.
L3/L4 UNKNOWN. ZERO Steam or installed-file changes.
See docs/right-eye-mirror-capture.md. MAIN must integrate on current source,
preserve full accepted config/assets and validate actual saved Steam images and
UE3 WM_SIZE behavior before accepting the window-fit flag.

LATEST 2026-10-16 SIDE native Centurion continuation: offline native registration,
class-constructor/vtable and original BeginFire reload/fallback path identified.
New default-OFF CenturionNativeReloadGuard 1=observe /2=conditional suppression
experiment, derived ABI/slot + method-shape validation; one close-token, native
inventory untouched. L1_PASS 13 CPU executables + 31 Python checks, original
archived wrapper/body match and own-process x86 detour replay.
L2_PASS clean a2b77ac /O2 x86 PE32 DLL SHA256
89A5209153F194ED858553EAE2C3B99CC1BC2E52C9DD9B1DEBD9C43977458339.
Private candidate/manifest/assets checked, NO INI staged.
L3/L4 UNKNOWN. Actual cylinder animation OPEN.
See docs/centurion-native-reload.md. ZERO Steam, preserve current MAIN integration.

LATEST 2026-10-16 SIDE fix/centurion-grip-shoulder: user tested original
Centurion prototype and reported auto-reload, gun/knife grab conflicts, squares.
Follow-up implements left-shoulder source, bounded grip priority/no fake release,
default-OFF Cross health/right-stick-up jump/Circle cylinder gesture layout,
Circle/open/dump/shoulder/insert/close FSM and opt-in original stock/carried mesh.
Markers now independently OFF. Native auto-reload/cylinder animation NOT fixed;
native declaration/ABI/runtime producer still requires evidence, not guessed calls.
L1_PASS eleven /O2 /W4 /WX CPU executables +27 Python tests; knife 6 CPU/14 Python.
L2_PASS clean 80a5e34 /O2 x86 PE32 DLL SHA256
B5352D3C9DF60F705904A0AE992EDF478E96650A1E7B6888F4B51F3985384CAA.
Private candidate/asset hashes verified, NO INI staged. Full manual reload not ready.
ZERO Steam, no installed-file writes; L3/L4 UNKNOWN.
See docs/centurion-grip-shoulder.md; rebuild onto CURRENT MAIN before any test.

LATEST 2026-10-16: Prototypy przeładowania Valkyrie, Volk S4 i Kasimova (PR #37):
Przygotowałem prototypy przeładowania Valkyrie, Volk S4 i Kasimova: gesty, zabezpieczenia oraz oryginalne modele magazynków.
Przeszło 30 000 symulowanych przeładowań, 9 programów testowych C++, 33 testy Python i build x86 /O2. Zmiany są w [PR #37](https://github.com/trilex07/SingularityVR/pull/37).
To jeszcze prototypy offline — następny etap to podłączenie renderowania i natywnej amunicji. Steam oraz działanie Centuriona pozostały nietknięte.

LATEST 2026-10-16 SIDE fix/native-glove-cuffs (PR #41): original full native cuffs
(noTMD 10.786UU behind wrist instead of old 3.5UU cut), closed cuff backs for
free hands AND held-gun native noTMD/TMD sections. New default-OFF
NativeGloveCuffClosure uses separate native_cuffs.svnc (two exact profiles),
original VB/UV/skin/material/scale retained. No gun-finger/anchor changes;
knife private depth uses the selected colour IB's triangle count.
L1_PASS new topology/axial/skin-seam/source roundtrip +2048 runtime context
combinations/decoder/IB guard tests and existing glove/knife regressions.
L2_PASS clean b50e5c9 /O2 x86 PE32.

LATEST 2026-10-16 SIDE fix/frontend-menu-comfort (PR #40): default-OFF class2-only
FrontMenuCleanBackdrop replaces the glitch-prone UE3 backdrop with neutral dark
colour and retains both eye projections of the existing current-token GFx panel.
Scoped test values: FrontMenuScalePct=80 / FrontMenuDistanceMm=1800 (no INI
replacement). Backup-before-clear, full D3D state + RT/DS/MRT restoration,
failure pixel restore, Reset-owned DEFAULT-pool resources; captured successful
receipt disables legacy identical-left-crop for this frame only.
L1_PASS six /O2 /W4 /WX CPU executables: policy/generation/transaction failures,
panel/laser/parallax geometry, menu/pause/cutscene regressions.
L2_PASS clean 9b23e3f /O2 x86 PE32; DLL SHA-256
681D3A790951E9D192377CDD4AC7433F8652A01F7FE2B7D9649315805E878C1E.
ZERO Steam; L3/L4 UNKNOWN. No game assets/config/renders of hands/weapons changed.
See docs/frontend-menu-comfort.md; MAIN owns L3 and physical acceptance.

LATEST 2026-10-16 SIDE feature/accepted-profile-guard (PR #39): offline deployment
regression guard, complete semantic INI + exact inventoried SHA-256 and
reviewed per-candidate plan; denies silent Polish/hand/knife/calibration loss
and stale rollback. Narrow deploy runner integrated, legacy build -Install
blocked. No automatic acceptance/restore. Broad/cutscene writes fail closed.
other standalone runners remain OUT OF SCOPE until MAIN adapts them.
No live baseline created or promoted, ZERO Steam, no renderer/input changes.
L1_PASS 37 rejection cases on Windows PowerShell 5.1 and PowerShell 7.
L2_PASS clean 4e7828e /O2 x86 PE32, DLL SHA-256
3B83750AC8C5BCABFA4C1160791BB03055C8D6F84ACAF288B8715ABC2A67F1D1.
Build only, not staged; L3/L4 UNKNOWN. Not active on user's installation.
See docs/accepted-profile-guard.md for contract, coverage and migration limits.

LATEST 2026-10-16 SIDE feature/centurion-manual-reload, synced MAIN f0b96ad:
Compact import: docs/handoffs/centurion-reload-compact.md. Offline correlation
helper tools/reload/analyze_reload_log.py added: 13 synthetic tests PASS;
reload suite now eight CPU executables +27 Python tests PASS. It never proves
native reload/ABI, writes files or touches Steam. No runtime/DLL/assets changed;
existing L2 receipt below remains the candidate identity, L3/L4 UNKNOWN.
Continuation: CenturionClipPreview default OFF; original cylinder/albedo in
left hand only during Carrying, 100 ms identity/focus/tracking snapshot guard.
Shared knife loader keeps original defaults; no native cylinder suppression,
ammo/animation changes, fitted grip or native-lighting claim. Eight CPU
executables +14 Python tests PASS; actual mesh/albedo load and textured CPU
preview inspected. L2_PASS clean source e0fb5e1 /O2 x86 PE32; DLL SHA
00C49400DC5FCF6A9F9DA2F67ED35E47F7239B86FB8B1F15915AC5D3B90E502B.
Private candidate DLL/manifest +both assets verified; no INI staged/deployed.
Prior receipts below are superseded; user has NOT tested Centurion yet.
Centurion-only assisted gesture prototype, NOT completed physical reload.
Front-centre pouch, unchanged left knife/right gun holsters; allocation-free
remove/discard/pouch/insert/ack FSM, ownership/cancel guards, bounded calibration
overlay, named read-only HUD observers and existing-hook native reload probe.
All four flags OFF. Shared native Use/reload tap is independently opt-in;
no engine ammo writes, cylinder animation control or auto-reload suppression.
Original clip extracted offline: 1067 UV verts /1252 tris, no mixed boundary;
optional unlit carried preview implemented, not GPU verified. Initial L1 six CPU executables +four Python
tests + actual FLOAT32 asset roundtrip, plus six menu/holster regression exes.
Probe-only mode records named ammo changes and bounded per-weapon UFunction
binding identities (never called); original six-chamber cylinder inspected.
Continuation L1_PASS: fresh identity-bound HUD samples, missing-field retry,
physical-left ownership and capacity guards; 360000 reload lifecycle frames;
seven CPU executables +12 Python tests. Original Reload: 53 frames /24 Hz;
14 notifies include AnimNotify_RefillClip at 1.47052908 s on RvSharedPawn.
Fifth player-pawn-only probe event; no native calls. Material AR_Pistol resolved
offline; live shader/socket and dedicated reload ABI remain unproven.
Previous continuation L2_PASS source 9c164eb /O2 x86 PE32,
DLL SHA 99A00CB7C636856EAAA88A829ACD09E4548045D3B4A71FB30064812CFADDD051;
private DLL/manifest hashes verified, original textures staged offline only;
PR base advanced during work: merged its committed f0b96ad into this branch
only, preserving MAIN's new knife impulse path after reload ownership gate.
INI, native knife, cinema and muzzle policy files match that base exactly;
seven reload CPU/12 Python tests and six knife CPU/14 Python tests pass.
receipt in docs/centurion-manual-reload.md.
L3/L4 UNKNOWN. ZERO Steam. Integrate onto current accepted MAIN and rebuild,
never replace newer knife/menu work with this older standalone base.

LATEST 2026-10-16 SIDE feature/menu-holster-polish: class-2 frontend-only
scale/depth candidate 88% / 1400 mm, same provider/pointer/fallback geometry;
native source RT/crop, pause/boot and mod panel unchanged. Original thin
segmented hip cues, gun/knife icons, valid-hand proximity highlight; SAME
anchors and grab radii. Missing config keeps exact legacy layout/white ring.
L1_PASS six CPU test executables, original procedural CPU preview inspected;
L2_PASS clean44b4bf7 /O2 x86 PE32, DLL SHA-256
04C5B55AA6400F4F68FFABE246DF9120E967CF497D60B55A6A527F8E198BDDF8.
Private candidate/provenance verified, no wholesale INI; ZERO Steam, L3/L4
UNKNOWN. Isolated tree based on
MAIN committed17bf1ed, no MAIN dirty edits copied; integrate follow-up onto
current MAIN, never replace newer work. See docs/menu-holster-polish.md.

LATEST 2026-10-16 SIDE fix/knife-shading-thumb: physical feedback accepts
joint mount/size, rejects bright unlit knife and thumb interpenetration.
Default-OFF HandThumbPoseFix: original thumb rig only, separate knife pose and
11-key empty-fist path; zero measured crossings at 201 FLOAT32 samples/hand.
Default-OFF KnifeSceneLighting: original normal/specular, audited native scene
SH/direct/ambient light and depth-alpha encoding; exact captured shader gate.
This is a scene-lit approximation, NOT full Mat_Knife or a new shadow caster.
L1_PASS: 13308 original-rig golden vertices, 101 palette levels, unchanged
non-thumb/opposite-hand/rig/cuff/mount, SM3 compile + tangent/cache tests and
existing upload/state/holster/hand regressions. L2_PASS clean 230c62e /O2 x86
PE32, DLL SHA E71EF4E18ACF99418AC50F6A2E9BB0C294C2886559582ABB2F6EAD8DE8F086A2.
Base includes MAIN aef042f (joint PR32 + Bink stills); integrate follow-up onto
current MAIN, don't deploy stale standalone source. ZERO Steam, L3/L4 UNKNOWN.
See docs/knife-shading-thumb.md for flags, limits and two independent tests.

LATEST 2026-10-15 SIDE fix/native-muzzle-stereo, base fb73156: original FX
continuation, no Steam. Original pistol light ownership now separately proven
offline: mMuzzleFlashLights -> export918 RvGameShared_ExplosionLightComponent;
flash/smoke template remains FX_MuzzleLockedFX_MuzzleLocked. Stride72 identity
was DISPROVED historically, not reused. Implemented default-OFF idle/3-shot
driver capture + exact shader-pair drop discriminator + guarded CTAB-camera
alias repair. L1_PASS (cache/COM/restore/alias/sequence +6 Python + accepted
shadow/hand/holster regressions). Contributing live draw still UNKNOWN; alias
hypothesis NOT established. No generated replacement FX or native object edits.
L2_PASS clean469c99a /O2 x86 PE32; DLL SHA256
4A8DB99F20E354CFB6D274BA74F92E6DC84ACE85FE94DD749A6CDBF20314EA23.
Full receipt in docs/native-muzzle-stereo.md; L3/L4 requires MAIN. Do not enable
mode2 without the selected-pair/content and exact-alias evidence.

LATEST 2026-10-15 MAIN round 19 DEPLOYED (DLL
F303DA28CA6B387A5DEBBF31B72D9D1CE71F798B728EF315D10B9A3EB6A88C48/source
branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; game closed at deploy).
Why: User feedback:
1) "jeszcze troche w dół i w lewo odkładanie borni": HolsterTrimRightCm=-7 (left), HolsterTrimUpCm=-6 (down) written to SingularityVR.ini.
2) "nóż widzę ale nie mogę go podnieść": Log showed near=1 (left hand at belt) blocked because engine has mTMD non-null (+0x1058), which triggered postTMD=1 requiring right hand only (h==1) without weapon (noWpn=0). Fixed: g_hipKnifeIgnoreTMD defaults to 1 and explicitly sets f.nativeKnife=impulseOk, f.hasTMD=false. INI updated with HipKnifeIgnoreTMD=1. Now both hands can pick up the knife at the hip regardless of held weapon.
3) "mesh jest noża jest customowy a powinien być wzięty z gry": Analysis shows 2010_knife in knife.svrk IS the game's native geometry from RvGame.xxx, but knife_renderer.h renders it with D3DFVF_XYZ|D3DFVF_DIFFUSE (flat 2-color unlit vertex colors without textures or lighting), making it look like an untextured custom model.
L1_PASS (159 checks), L2_PASS (PE32 /O2). Pending physical PSVR2 verification.

LATEST 2026-10-15 MAIN round 18 DEPLOYED (DLL
D2E2ACD1E13298CEBB79F83BDB455A98BAD47874702B8924B6D2D706B67E04FD/source
5baaf5a branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-203638; game closed
at deploy). Why: r17 log (session 20:30, `build r17` marker) names the knife
render blocker: gameplay mask `vis=1 play=1 stereo=0` — input Context TRUE and
visible, but render gated on g_stereo, the F12 alternate-eye DEBUG method that
SetVrMode/F1 deliberately clear. Zero `draw=` in the whole log history proves
the mesh NEVER rendered in normal play (side's L1/L2 never covered pixels).
Fix: render gates on Rd(g_dupDraws) (TRUE stereo, same signal as the belt
ring). Same session's silent lines (postTMD=1, near=LEFT+grip=RIGHT or fresh
left squeezes) confirm pickup logic behaves per the requested right-only rule.
Plus user trim applied to INSTALLED INI (pre-edit backup
deployment/install-backups/20261015-203644/SingularityVR.ini.pre-trim):
HolsterTrimFwdCm=-2 (toward camera = back), HolsterTrimRightCm=-3 (left),
HolsterTrimUpCm=-2 (down); mapping stated so sign flips are one edit away.
L1_PASS (159). L2_PASS (PE32, manifest 5baaf5a clean). L3/physical PENDING:
restart game (new `build r18` marker), docked knife should render at left
belt; store should fire at the ring; knife = empty right hand + squeeze AT
belt. No fix claimed.

LATEST 2026-10-15 MAIN round 17 DEPLOYED (DLL
B94033554E1E8E6D77D2069AB7B2C6FD9BF0F40EE2CEA46F3667AA6B5A4B3C71/source
8cb58ca branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-202938; game closed
at deploy). Why: user disputes TMD (claims none) and reports the docked knife
MODEL never renders. Evidence: (1) TMD read is exact-FName `mTMD` +0x1058
depth 1, distinct from `mTMDArchetype` +0x1054 — archetype-confusion ruled
out; `hands: mTMD is NON-NULL` + every silent line postTMD=1 agree; story-wise
the device fuses to the wrist in the prologue while Centurion is a weapon, so
no contradiction. Still, new INI hatch HipKnifeIgnoreTMD (default 0 = trust
engine; =1 forces pre-TMD both-hands rule, restart required) unblocks the
user either way. (2) Render side: DrawHipKnife is called every Present but
this session never logs `draw=` nor GPU-failure — the gate blocks with only a
spent menu-time one-shot to show for it. Gate log is now per-DISTINCT-mask
(en/ready/vis/vr/play/stereo/eyes/paused/menu/panel/wheel/cine) and names the
gameplay blocker in one run. L1_PASS (159). L2_PASS (PE32, manifest 8cb58ca
clean). L3/physical PENDING: restart (new `build r17` marker), read the new
`gate blocked` mask in gameplay; knife = empty right hand + squeeze AT belt;
optional IgnoreTMD=1 test. No fix claimed.

LATEST 2026-10-15 MAIN round 16 DEPLOYED (DLL
AD1137A395587D79D1F9ABE063B6F98E6B1C4850FD6F67E7FE92ED8D44E7745A/source
f58a09f branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-202051; game closed
at deploy). Why: r15 log (session 20:15, `build r15` marker present) proves
(1) user HAS TMD (`hands: mTMD is NON-NULL`, postTMD=1 on every silent line
— Centurion is the weapon, TMD the wrist device, no contradiction); left-hand
belt squeezes with fresh edge (near=1 grip=1 edge=1) are blocked post-TMD by
the user's own right-only rule; one line shows right grip held while only the
left hand is at the belt (near=1 grip=2 edge=0); right hand was never at the
belt with a fresh edge, so no PICKED is correct behavior, not a bug;
(2) 3x STORED hand-hip vs aim gives 47/88/29 deg — mixed with one orthogonal
sample, so the gun-vs-controller axis hypothesis is NOT confirmed; all three
are front-edge contacts at radius ~11-12 (approach direction, not bias).
Change: user-tunable trigger-only trim INI (HolsterTrimFwdCm/RightCm/UpCm,
default 0 = trigger at ring, body frame, +/-30 cm, logged at startup);
ring and stored-gun render untouched. L1_PASS (159). L2_PASS (PE32, manifest
f58a09f clean). L3/physical PENDING: restart game (new `build r16` marker),
knife = empty RIGHT hand to belt + squeeze AT belt; holster = dial trim in
3 cm steps (felt zone behind+above ring → TrimFwdCm plus, TrimUpCm minus).
No fix claimed.

LATEST 2026-10-15 MAIN round 15 DEPLOYED (DLL
AB2F11F23D9E6A4D44636A5CD0BE846B3DDECF679F7EA0E39346F003F0B9F30A/source
d74a922 branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-201446; game closed
at deploy). Why: user retest on r14 reports (1) pistol stores behind+above
the drawn ring, (2) no docked knife at left belt despite no-weapon tries with
both hands — while the 20:05 r14 log PROVES one right-hand PICKED held=1 +
SHEATHED cycle and mTMD null (no TMD, pre-TMD both-hands path live). No
behavior change this round, only falsifying instruments: (a) startup build
marker `hip knife: build r15 ...` with holster/knife config (proves which DLL
runs); (b) silent-refusal probe `armed grip at belt but no pickup
(near/grip/edge/free/trk/noWpn/postTMD/preTMD)` — catches edge-missed grips
(held BEFORE entering radius), !free (weapon in hand), untracked hand;
falsifier: edge=1 on the near hand with no PICKED disproves edge-timing;
(c) STORED/DRAWN now logs aim dir — if (hip-hand) aligns with +aim across
samples the user aligns the VISIBLE gun while the trigger tests the
CONTROLLER point, and the fix is a gun-length offset along aim, not a hip
constant; orthogonal scatter disproves it. Ini strings updated (post-TMD
right-hand-only). L1_PASS (159). L2_PASS (PE32, manifest d74a922 clean).
L3/physical PENDING: controlled run — 3x store aligning CONTROLLER to ring,
3x aligning GUN to ring; knife right-hand no-weapon pickup; MAIN takes the
log. No fix claimed.

LATEST 2026-10-15 MAIN round 14 DEPLOYED (DLL
D065446C99A909DBAEFDE2A4F3D5E714715259BD96EBE2E6AE42E604E785E034/source
bcf8ec1 branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-200339; game closed
at deploy). Why (user request): with TMD present the knife stays usable but
right hand (h==1) with no weapon in hand only; pre-TMD both-hands path
unchanged. Frame gains hasTMD (TMD!=0 with intact impulse mapping) and
noWeapon (!WeaponInHands||holstered); Context accepts nativeKnife||hasTMD;
post-TMD pickup requires h==1&&noWeapon and a left-held knife resets when TMD
arrives. Refusal probe mask/log extended with hasTMD/noWeapon. Correction to
prior note: the old `mTMD NON-NULL` line came from another moment in the long
log, NOT the user's current no-TMD save — current save has Centurion knife,
no TMD, so hasTMD must read 0; the new raw tmd/hp/off/imp fields will prove
the actual blocker (offsets vs hp vs mapping). L1_PASS (hip-knife 159 checks:
147 retained + 12 post-TMD). L2_PASS (PE32 0x014C/0x010B, manifest commit
bcf8ec1 clean). L3/physical PENDING: pre-TMD either hand picks up; post-TMD
right hand empty-handed only; swing attacks. No fix claimed.

LATEST 2026-10-15 MAIN round 13 DEPLOYED (DLL
391B44BB4CF6B6CAAD922E166159CA13E3DD3B89766A8EE345E7476E0A366EEE/source
e181aa1 branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-195400; game closed
at deploy; knife.svrk + native gloves + INI flags retained). Why: round 12
(d39bb27, built 546DDAA9 but NEVER deployed — agent stalled during build)
subtracted 4 UU from the holster trigger Z based on 10x STORED edge contacts
(mean hand ~5 UU above hip); those are approach-direction edge hits inside
radius 12, not a calibration bias, and -4 moved the trigger AWAY from the
approaching hand. Round 13 reverts to trigger==ring==stored-gun point
(HolsterHipWorld TRUE) and enriches the refusal probe with offTMD/offHP/
tmd/hp/impIdx/impMask so post-TMD (by design pre-TMD only) vs unknown offsets
vs dead can be told apart in ONE log line. Log evidence: mTMD NON-NULL seen
earlier plus last refusal alive=0 nativeKnife=0 strongly suggests the user is
testing post-TMD, where the knife is correctly hidden. L1_PASS (hip-knife
147). L2_PASS (PE32 0x014C/0x010B, manifest commit e181aa1 clean). L3/physical
PENDING (pre-TMD save ONLY): belt ring at left hip, grip at ring picks up,
new refusal line names the blocker with raw values; store gun AT the gun
ring (no behind/above offset). No fix claimed.

LATEST 2026-10-15 MAIN round 11 DEPLOYED (DLL
00522E118FC59B5F0F6175D5934C2CD42583D3E3F6AA30AA069DE2282AFD835D/source
05e0042 branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-193527; game closed
at deploy; knife.svrk + native gloves + INI flags retained). Why: live log
19:28-19:30 (current DLL) proved gun ring DREW (flat ring at 1917,1985) while
the knife was never picked (no PICKED, belt ring gate held=-1 at startup) —
knife code simply never ran in gameplay yet. Changes: (1) holster trigger now
uses HolsterHipWorld(H,TRUE), the same point the ring draws and the stored
gun renders — the FALSE trigger sat hmdOffset above the ring, which is why
storing needed aiming above it; (2) belt ring shows while DOCKED too (held
gate removed) so the belt can be found — it marks where to grip; (3) one-shot
pickup-refusal probe (grip held at belt but no PICKED logs gameplay/focused/
alive/nativeKnife — nativeKnife=0 means post-TMD save, by design pre-TMD
only). L1_PASS (hip-knife 147). L2_PASS (PE32, manifest commit 05e0042
clean). L3/physical PENDING (pre-TMD save): belt ring visible at left hip,
grip at ring picks up, ring while held, release sheathes, swing attacks;
store gun AT the gun ring. No fix claimed.

LATEST 2026-10-15 MAIN round 10 DEPLOYED (DLL
66E63F943642244E73878F07F6DF57C1090A19BF0E208ECD3371A5D3E4061E8B/source
1ed9b12 branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-192322; game closed
at deploy). Knife level + ring: (1) installed HipKnifeDownCm=55 (holster
-36UU at WorldScale 80 = 0.549m below head; pre-edit INI backed to
deployment/install-backups/20261015-192322/SingularityVR.ini.pre-knifedepth;
file verified all-CRLF after edit); (2) NEW white knife-belt ring at the
LEFT belt while held (shared DrawFlatRingAt helper with the gun ring — one
mechanism; own gate/drew/proj logs); (3) one-shot gate log added to side's
DrawHipKnife (names the blocker if the knife stays invisible). L1_PASS
(hip-knife 147). L2_PASS (PE32, manifest commit 1ed9b12 clean). NOTE: a
botched PowerShell text rewrite of d3d9.cpp (UTF-8 BOM + double-encoded
emoji, 879-line noise commit dc68e62) was caught by diff-size check and
fully reverted — unpushed, abandoned via reset; round 10 was redone with
byte-safe edits only (99+/56-, mojibake grep clean). Lesson: never route
source files through PowerShell Get-Content/Set-Content; use the edit tool
or git-native writes. L3/physical PENDING (pre-TMD save): knife visible at
left hip at holster level, grip pickup, belt ring while held, release
returns, swing attack. No fix claimed.

LATEST 2026-10-15 MAIN knife live (DLL
CF5D9143941209D012CC660EE32D6631C77F06E6591165CE04D2D1E11711DB6A/source
cf99df9 = e89c41e code + STATUS record; x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-190214; game closed
at deploy). Verified on disk: installed DLL hash equals manifest; knife.svrk
6BC784FB present; installed INI now carries GloveNativeEnabled=1 (Render) and
HipKnifeEnabled=1 (Input) — both were absent despite the earlier handoff
claiming them, set now with pre-edit backup
deployment/install-backups/20261015-185357/SingularityVR.ini.pre-flags
(plus deduped an accidental double-write). GloveRendererEnabled=1 and
holster 6/18/-36 retained. Previous segment's L1 (147 knife checks) + L2
re-verified against this tree before deploy (same code, fresh link only).
L3/physical PENDING (pre-TMD save ONLY): knife at left hip, free-hand grip
pickup, release returns to belt, decisive swing -> native attack; hit counts
from camera (no 6DoF blade collision). TEXTURES NEXT (user scope set): upscale
installed 4K remasters to 8K DDS+mips into texture-override-8K (Quality=8K +
panel TEXTURES row already exist); loading screens need DDS encode + FNV
targets + loading group + dinput8 rebuild + panel selection row. No fix
claimed.

LATEST 2026-10-15 MAIN merged + deployed left-hip knife (DLL
886B6D31FF87791AFFCF5B94C568925912634CED3F2F9FCC4534A96750D6B272/source
e89c41e merge of origin/feature/left-hip-knife 043cfc0 (draft PR #31);
x86 PE32 /O2, built==installed via tools/deploy-singularityvr.ps1 -DllOnly;
backup 20261015-185357; game closed at deploy). Merge conflicts resolved by
keeping both sides (ReleaseWristResources releases all three renderers; MY
holster 6/18/-36 defaults kept + knife INI block added; STATUS blocks kept).
Installed knife.svrk to vr-assets/knife (hash 6BC784FB = side hashes.json,
verified after copy) + HipKnifeEnabled=1 in installed INI (pre-edit backed
to deployment/install-backups/20261015-182559/SingularityVR.ini.pre-knife).
L1_PASS (hip-knife 147 checks + renderer fake-D3D guard re-run on merged
tree). L2_PASS (PE32, manifest commit e89c41e clean). L3/physical PENDING
(pre-TMD save ONLY): knife at left hip, free-hand grip pickup, release
returns to belt, decisive swing -> native attack; hit still counts from
camera (no 6DoF blade collision). TEXTURES OPEN: no 8K DDS exists anywhere
(max installed is 4K 4096x2048; Quality=8K already selected in
texture_override.ini but has no 8K content); VR panel TEXTURES row (2x/4K/8K)
already exists; loading screens need the DDS encoder + dinput8 target rebuild
(doc docs/loading-screens-4k.md pipeline steps 1-4) — awaiting user scope
(upscale 4K->8K? user-supplied files? loading via injector vs splash-only?).
No fix claimed.

LATEST 2026-10-15 MAIN round 9 DEPLOYED (DLL
D119F664A08A693E6BA07A4FB75C4B78AA225D8254C2D764F27B8479A94FC026/source
e1353a9 branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-184201; game closed
at deploy; native gloves + GloveNativeEnabled=1 retained). Thumb fix: full
authored opposition (62.8 deg on bones 10-12/70-72, verified against the
private PSK bone list) drove the thumb through the fingers at full grip, so
Pose() now blends thumb bones with gain 0.7 (Thumb()/ThumbGain() in
glove_native.h, other fingers untouched, shape preserved). L1_PASS (native
test incl. new Thumb membership asserts vs real asset; structural checks
unchanged). L2_PASS (PE32, manifest commit e1353a9 clean). L3/physical
PENDING: slow grip — thumb must stop short of the fingers without looking
stiff (too far -> lower gain; stiff -> raise toward 1). No fix claimed.

LATEST 2026-10-15 MAIN adopted side native gloves (user auth: side agent at
limit, WIP uncommitted in their tree — read-only takeover, their tree
untouched). DEPLOYED DLL
97244AE7525EA21792D9ED9AC3DFED917B8B3971BEE893462306959B87A33A8B/source
be561e4 branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-183351; game closed
at deploy. Takeover method: exported side diff to patch file, applied all but
one hunk cleanly, hand-applied the last (Hook_DrawIndexedPrim restore block);
copied untracked glove_native.h + native test/tool/doc files; verified every
hunk landed by grep. Native variant: original VB/material/shaders stay bound,
scoped index-subset draw + finger palette on original bones, cuff with short
forearm, same native size (no rescale), default OFF (GloveNativeEnabled, takes
priority over the GLB renderer). Installed native_hands.svni (hash B4564A69 =
side report cache_sha256, verified after copy) + GloveNativeEnabled=1 in
installed INI (pre-edit backed to
deployment/install-backups/20261015-182559/SingularityVR.ini.pre-native;
GloveRendererEnabled=1 kept as fallback path). L1_PASS (native guard/decode/
101-level pose test vs real asset: triangles=3792 cap=98, plus layout, probe,
holster, vrpose, glove 276-check anim + state guard). L2_PASS (PE32, manifest
commit be561e4 clean). L3/physical PENDING: native material/cuff/fist in
headset (side's falsifiers: looks different from held glove in same light, no
cuff behind wrist, size change, grip/aim change). No fix claimed.

LATEST 2026-10-15 MAIN round 8 DEPLOYED (DLL
59598E6D3E0BDCBAFCBF6351DE7F5515935BA2C7525A9D4CA0A505D91F3ACD1B/source
97574db branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-182559; game closed
at deploy; user-supplied digit-free plates installed (old baked PNGs backed
to deployment/install-backups/20261015-180725/plate-*.baked.png); glove v2
pack + GloveRendererEnabled=1 retained). Changes per annotated screenshot:
(1) baked-digit covers REMOVED from DrawAmmoPlate — clean art needs nothing
covered except the empty-mag info (red box hidden iff clip>2; white mag,
icons, frames, labels never touched); cover helpers stay for L1 only;
(2) holster ring is now HORIZONTAL (flat to floor/ceiling): outer 0.1127m /
inner 0.0877m (diameter +1 inch, rim 5cm->2.5cm), XR x/z plane at hip height,
yaw-invariant; (3) side native-material glove variant NOT merged — side
worktree still holds uncommitted WIP (glove_native.h, prepare_native_hands.py,
native tests; origin still ea86587), per policy MAIN waits for their pushed
green L1/L2. L1_PASS (layout). L2_PASS (PE32, manifest commit 97574db clean).
L3/physical PENDING: clean plates (digits on empty cells, red iff <=2),
flat ring at hip with gun held, gloves v2. No fix claimed.

LATEST 2026-10-15 MAIN round 7 DEPLOYED (DLL
96F5947BDE7299734FD719EA0A753B8C46B3D77FBAC3006E7655D224B5ACC50B/source
99b2546 branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-180725; game closed
at deploy; glove v2 pack + GloveRendererEnabled=1 retained). From the user's
annotated screenshot: (1) RED (wrongly covered): pips band x0.190 covered
mag-icon bottom + corner tick + cell frame — column profile proves true pips
are only x272-380/y210-236, so the band is now TIGHT x0.260-0.380/y0.600-0.698
(y260 line is the cell frame, kept); (2) PINK (reserve ghost): widened cover
stands + live digits already draw on top + strip alpha-bleed from round 5;
(3) ring redesigned to a VERTICAL annulus: outer 0.10m / inner 0.05m (5cm rim,
empty middle), plane spanned by XR-up and horizontal eye-right (billboard
around Y, perpendicular to the floor), same XR projection as plates.
L1_PASS (pips-band lock, halo lock, vrpose invert). L2_PASS (PE32, manifest
commit 99b2546 clean). L3/physical PENDING: red deco restored, pink ghost
gone, vertical ring at hip with gun held + side native-match gloves. No fix
claimed.

LATEST 2026-10-15 MAIN round 6 DEPLOYED (DLL
AAABE9FE732BD4F2BE83F2212ACEF89CB260407EBF86599A5A792E2FE95D8387/source
ffb5b83 branch plates-l3-main = merge origin/feature/glove-animation
native-match (ea86587) + MAIN XR-space holster marker; x86 PE32 /O2,
built==installed via tools/deploy-singularityvr.ps1 -DllOnly; backup
20261015-174458; game closed at deploy; INI flags kept
(GloveRendererEnabled=1, holster 6/18/-36)). Changes: (1) marker rewritten
from camMat to XR projection — 17:19 log proved ammo drew while the marker
never attempted, and camMat slots are invalidated by post-scene draws before
Present, so no valid slot exists at overlay time; hip UU converts via the
exact inverse of vrpose::TrackingDeltaToWorldUU (L1 roundtrip
test-vrpose-invert.cpp) + head pos, radius via eye-right probe — only proven
pieces (holster NearHip frame, plates' VrUiProjectEye); (2) side native-match
merged (native cut/wrist, original UV/albedo, rig-tied scale, SVRG v2) and
v2 pack installed with hash verification (AC199AF2/1942A834/0F6A90BB =
side hashes.json; v1 backed to
deployment/install-backups/20261015-171643/glove_*.svrg.v1); SVRT unchanged;
(3) user screenshot analyzed: small plates beside ours have no mini-plate
code path in the wheel (only DrawAmmoPlate draws plates+digits) — pending
user identification if they persist. L1_PASS (vrpose invert, layout, probe,
holster, glove 276-check anim + state guard on merged headers). L2_PASS
(PE32, manifest commit ffb5b83 clean). L3/physical PENDING: ring at hip with
gun held unholstered + native-match gloves (scale/cuff/fist) + digits fringe
check after the bleed. No fix claimed.

LATEST 2026-10-15 MAIN round 5 DEPLOYED (DLL
65A1539F4D0529E83E08BA7004CAFAB78C887619362E0B1184EAC5900EAC71D5/source
6cd7ab2 branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-172846; game closed
at deploy; glove assets + GloveRendererEnabled=1 retained from round 4).
Root-cause fix for "burnt digits": offline SBS render of the exact
composition proved baked art is fully hidden — the burnt look is the LIVE
digits' own dark minification fringe (strip transparent pixels were
transparent BLACK; GPU linear downscale 184px->~80px bleeds black into the
glow). Fixed by 16-pass alpha-bleed of installed ammo-digits.png
(6BD0D4DA->D0872891; alpha untouched; pre-edit backed to
deployment/install-backups/20261015-171643/ammo-digits.pre-bleed.png);
offline re-render confirms the boxes are gone (residual edge in the sim is
GDI+ bicubic overstatement vs in-game bilinear). Plus: holster marker camera
slot one-shot log (gate/camMat/proj/drew chain now complete — 17:01 log
proved ammo drew while the marker never attempted, so the break is gate or
camMat, and the next session names it). L2_PASS (PE32, manifest commit
6cd7ab2 clean). L3/physical PENDING: gameplay pass holding the gun
UNholstered, eyes on the hip + digits check. No fix claimed.

LATEST 2026-10-15 MAIN round 4 DEPLOYED (DLL
C6D35DC695218CB08C3E10F05B35223DBCE6D9D73969573C7573460560A3FF75/source
6961f5d branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-171643; game closed
at deploy). Why round 4: (a) 16:41 gameplay log (BBA4BF95) contains ZERO
plate/marker lines despite STORED/DRAWN holster events — all three overlays
sat in their top gates all session, but no gate log existed, so the blocker
was unidentifiable; (b) user still sees burnt reserve digits. Changes:
(1) ReserveCover widened to x0.494-0.606/y0.512-0.720 — baked "48" ends 1px
from the old rect (right x616 vs 617, top y179 vs 179), glow bled past;
bullets icon gap rows y172-179 verified dimmest, ~3px icon-glow overlap
accepted; live reserve digits already draw on top inside the rect;
(2) ammo plate is now world-anchored (constant 0.19m wide via per-eye
0.5m-probe px-per-metre, clamped 64px-0.30H, 0.16H fallback) so it shrinks
with distance instead of growing as the gun moves away; fallback also
carries the asked ~5% reduction; health plate untouched;
(3) one-shot "gate blocked (flag/vr/dup/paused/...)" falsifier logs on all
three overlays — next session names the exact blocker if lines stay absent;
(4) ring radius clamp 400->1000px + proj-fail log (a close hip legitimately
projects ~300-500px);
(5) docs/handoffs/side-glove-visual-parity-prompt.md prepared for SIDE
(native weapon-hold look + size parity, L1/L2 only, own worktree).
L1_PASS (layout incl. halo-margin lock, probe, holster, both glove suites
re-run on merged headers). L2_PASS (PE32, manifest commit 6961f5d clean).
L3/physical PENDING: one PSVR2 gameplay pass with gun held (plates, ring at
hip, store/draw) + glove script. No fix claimed.

LATEST 2026-10-15 MAIN enabled side glove renderer for L3 (DLL unchanged
B7369140, source d060c33): installed private pack
vr-assets/hud/katorga/glove-animation/{glove_left.svrg,glove_right.svrg,glove.svrt}
from side candidate, all 3 SHA-256 verified equal to candidate hashes.json
(58A267AD/5683181F/0F6A90BB) before AND after copy; installed INI
GloveRendererEnabled 0->1 (pre-edit INI backed to
deployment/install-backups/20261015-165234/SingularityVR.ini.pre-glove).
Fail-safe: missing/bad pack logs FAILED and keeps native hands; flag reads
at XR startup, game was closed. L3/physical PENDING — side's 3-sentence
script: empty-hand pad rotation, cuff-to-eye hole check, slow grip open/fist
+ draw weapon (grip/aim unchanged) + quit. Revert = set flag 0 (assets can
stay; OFF loads nothing).

LATEST 2026-10-15 MAIN plates+gloves round 3 DEPLOYED (DLL
B7369140B76AF4D7A715F0C16A2DCE73A98500272B946A0ABFAFC42088E710DB/source
d060c33 branch plates-l3-main = merge origin/feature/glove-animation
(8518e6f, PR #30 open) + MAIN round-3 fixes; x86 PE32 /O2, built==installed
via tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-165234; game
closed at deploy; installed INI holster lines already 6/18/-36). Round 3:
(1) NEW MagPipsCover band x0.190-0.465/y0.600-0.765 (measured bright bbox
x202-469/y200-259 all 4 plates: wider than MagCover and, on shotgun/sniper,
below it — this is what stayed visible); top deco brackets untouched;
(2) holster ring: radius clamp 400->1000px (a close hip legitimately
projects ~300-500px; old cap could skip the draw) + one-shot
"not projectable" log for falsifiability; NOTE 16:41 session ran previous
DLL only to startup, so the ring NEVER drew in gameplay yet — "not visible"
is untested, not disproven; (3) glove-animation merged (default OFF,
GloveRendererEnabled=0, no asset install needed while OFF; native hands and
weapon grip unchanged). L1_PASS (ammo layout+probe, holster machine, glove
253-check anim + render-state guard re-run on merged headers). L2_PASS (PE32,
manifest commit d060c33 clean tree). L3/physical PENDING: one PSVR2 pass —
pips gone, <=2 red box, hip ring while holding gun + store/draw at new hip,
then side's 3-sentence glove test (needs GloveRendererEnabled=1 + private
assets? confirm with side docs before enabling). No fix claimed.

LATEST 2026-10-15 MAIN plates round 2 DEPLOYED (DLL
BBA4BF9590F9F7EDDA415EEB35E4A6A0EB03DBEDD02D691E9ACC1991B26EB57F/source
22dc4aa branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-160921; game closed
at deploy). Per-user package: (1) HP two-hand anchor +0.00->-0.02 and left
shift 0.60W->0.85W; (2) ammo plate 0.19H->0.17H, anchor +0.05->+0.03;
(3) MagEmpty clip<=0 -> clip<=2 (full white mag iff clip>2, red box iff <=2;
white mag cell NEVER covered, left icon always visible); (4) MagCover y1
0.615->0.710 hides baked cyan pips (measured bright y210-236 all 4 plates;
reserve bullets icon verified above its cover, y178 vs cover y179);
(5) holster moved Fwd8->6/Right12->18/Up-30->-36 (code defaults + source INI
+ installed INI lines 164-166; pre-edit INI backed to
deployment/install-backups/20261015-160921/SingularityVR.ini.pre-holster,
hash DECEB3AD); (6) NEW white 50%-alpha hip ring (DrawHolsterMarker, same
cam-matrix UU projection as the laser, radius from widest axis probe,
shown iff gun held && !holstered, full state save/restore, one-shot drew
log). L1_PASS (layout incl. new <=2 rule, probe, holster state machine).
L2_PASS (PE32, manifest commit 22dc4aa clean tree). L3/physical PENDING:
15:53 session ran previous DLL only to startup; nobody has run BBA4BF95 in
gameplay yet. Next: one PSVR2 pass (plates positions, <=2 red box, hip ring
visible while holding gun, store/draw at new hip), then read drew/unknown
lines. No fix claimed.

LATEST 2026-10-15 MAIN plates L3 candidate DEPLOYED (DLL
9EC02D511C347C488A17C960B8C6420B53926413A3C245E43B60B43576227008/source
e943f06 branch plates-l3-main, x86 PE32 /O2, built==installed via
tools/deploy-singularityvr.ps1 -DllOnly; backup 20261015-155047; INI
DECEB3AD... preserved, AmmoPlateEnabled=1 HealthPlateEnabled=1). Merges
origin/feature/ammo-plates (9bf0cab) + origin/fix/health-plate-size (a2e8a3a);
side worktrees untouched (their unpushed 7dbbd5c diagnostics NOT taken).
MAIN-authored changes only in d3d9.cpp: (1) ammo unknown log now carries
clip/bin/hud/offC/offB values; (2) one-shot ammo drew (plate/clip/bin/rect)
and proj-fail lines; (3) one-shot health drew (twoHand/rect) and proj-fail
lines; (4) HP two-hand anchor +0.02f->+0.00f and left shift 0.35W->0.60W per
user "jeszcze nizej i troche bardziej w lewo" (single-hand untouched).
L1_PASS (ammo layout 20/20, probe 10/10) + static-analysis gate in build.
L2_PASS (PE32 0x014C/0x010B, manifest commit matches, tree clean at build).
L3 runtime evidence PENDING: 15:20 log proved ammo hidden at
clip<0||bin<0 (offsets cached +1404/+1400 on RvHUD) and health "shown" logged
but user reports both plates invisible — drew-vs-seen still undistinguished
until this candidate runs in-headset. Next: one PSVR2 pass (hold gun, check
ammo plate; empty mag to 0; grip two-hand, check HP low-left), then read
view_matrix.log for ammo unknown(...)/drew and health drew lines.
Falsifier: drew present + still invisible = placement/alpha, not sampling;
unknown with clip/bin>=0 readable = offset/window wrong, not HUD-missing.
No fix claimed; no rollback (candidate healthy, game closed at deploy).

LATEST 2026-10-15 ammo PLATES FIX DEPLOYED (DLL 742645B8/source 43f1381,
built==installed; backup 20261015-151309; INI carried over). Root cause of
invisible plates: WIC size guard (<=2048) rejected the 2172px digits strip.
Need: plate behind gun with live digits. Live:
deployment/hands-dubbing/ammo-plates-fix-20261015/live.json. Prior history.

LATEST 2026-10-15 ammo PROBE ROUND-2 DEPLOYED (DLL 05F960F0/source 2a22d62):
class property dump on equip (RvWeaponShared vocabulary: no ammo data
fields, only GetAmmoInClip/Bin-style functions). Prior history.
LATEST 2026-10-15 SIDE PR #30 REOPENED by physical feedback: functional
gloves, but size/appearance rejected; user requires the SAME native hand cut,
including wrist/cuff behind glove. New candidate uses shared accepted 48 face
runs / 2918 faces, full cooked-to-ActorX index roundtrip; exact original UVs
and diffuse bytes. Scale now inherits native LocalToWorld, GunScalePct and
world UU/metre; wrist/IPD not scaled. SVRG v2 carries baked units: old caches
are rejected (update DLL AND both meshes). Removed fabricated normal-Y tint.
L1_PASS: 284 CPU checks, 9 Python tests incl exact albedo and native face IDs,
guard/native-hand regressions. L2_PASS clean 3939add, x86 PE32 /O2, DLL SHA256
2C813E40A67F85375F57639F988BBC71536BAB07F17FAEC79BFC8E2138C62F9C.
Full UE3 normal/specular/scene lighting is NOT reproduced by the diffuse-only
GLB renderer: full appearance acceptance remains OPEN. No Steam touched.
See docs/glove-native-match.md for the new private candidate and test scope.

LATEST 2026-10-15 SIDE feature/glove-animation: L1_PASS + L2_PASS, clean
source 94aab901; x86 PE32 /O2 DLL SHA 27D5DA96...82568A4. Default OFF.
User explicitly selected a new default-OFF baked-GLB renderer after source
inspection proved main uses the native UE3 skin, not the rigid HUD GLBs.
Closed-cuff + open/fist morph assets prepared locally; free noTMD hands only,
held weapon/TMD/aiming unchanged. 261 CPU checks + fake render-state guard,
native hand regressions and 7 geometry tests; zero Steam access/deployment.
MAIN L3/L4 pending. See docs/glove-animation.md and
docs/glove-animation-evidence.md (exact SHA, private candidate, 3-sentence test).
SIDE 2026-10-15 left-hip knife: IMPLEMENTED_NOT_VERIFIED, L1_PASS, L2_PASS;
isolated feature/left-hip-knife from origin/main 72ad59d, independent of glove
PR #30. Default OFF HipKnifeEnabled: local native knife mesh on physical left
hip, free-hand grip pickup/hold/release, bounded swing -> native RB pulse.
Pre-TMD only, unknown/dead/unfocused/menu/cine/transition states fail closed.
IMPORTANT: native trace is inview_camera (50 damage / 96 UU), not blade-tip
collision; offline identity is not runtime input-to-damage proof. L1 147 checks,
5 Python tests, fake-D3D guard/decoder + holster/hand regressions PASS. Clean
ec124ec source built x86 PE32 /O2, DLL SHA256
75F22354E2FF12A816BD986438518694FEBB37DD02BA8C75968066CA4D5A5BC1.
ZERO Steam; L3 native damage and L4 geometry/grip still unverified.
See docs/left-hip-knife.md for exact limits, local assets and MAIN test script.

LATEST 2026-10-15 CINE ROUND-2 L3_PASS (user: stereo back after Load).
Log: `menu-load cut ... latch=1` -> `VideoFlat -> GameplayStereo`, drift back
to `cine off`. PR to main pending (user creates, gh auth missing here).
Prior history.

LATEST 2026-10-15 health-plate ROUND-2 DEPLOYED (DLL 89CB0984/source b73e311,
built==installed; backup 20261015-110532). Gesture earlier: show 60 cm /
hide 80 cm. NOTE: Steam runs main+plate WITHOUT the wheel (wheel code lives
only on unmerged fix/hud-3d-textures) — reunion = merge wheel PR, then
rebase this branch. Need: face-raise shows plate sooner, two-hand + hide as
before. Plates atlas installed (Steam-only data): 512x256 PNG + plates.json
(3 rects, sha verified), 11 segment slots per mockup measurement. Prior history.

LATEST 2026-10-15 wheel PR PREPARED, not yet created: `gh` has no auth in
this session (push works, PR creation needs user-side login). Branch
fix/hud-3d-textures pushed (17 commits over origin/main). User creates via
web compare link with the prepared title/body (see handoff message), then
reviews/merges. Open test debt stands (multi-weapon switch).
Wheel visuals (ring, 4 gun wedges, held brighter, unowned empty, labels,
silhouette icons, hand highlight): PHYSICALLY_VERIFIED by user in headset.
Switching logic: IMPLEMENTED + L1/L2 only (commit-mirror fix in, L3 evidence
pending: select+DIRECT+LANDED+visible change on a 2+ gun save; Seeker/TMD
sectors, 6 slots, ammo also open per docs/wheel-round-layout.md). Prior history.

LATEST 2026-10-15 wheel ICONS installed (no DLL change): 6 weapon silhouettes
cut from references/95f791be-....png into the Katorga atlas reserved row
(gun-centurion/valkyrie/spikeshot/kasimov + dev-seeker/dev-tmd for later);
atlas 512x256 PNG E77799B6 + atlas.json 22 rects (sha matches file) +
catalog sectors 1-4 remapped; all 6 catalog ids resolve, bounds/unique PASS.
Backup: ww-icons-20261015/atlas-backup. Installed DLL C1974AFF loads the
atlas dynamically, so icons appear on next launch with the current lease.
Upstream SIDE hud pack should absorb the deltas. Need: R3 open shows gun
silhouettes instead of mag squares + `HUD atlas ready (512x256, 22 icons)`.

LATEST 2026-10-15 wheel ROUND-2 IMPLEMENTED, L1_PASS + L2_PASS (user feedback
on 2E2064FB: hand-select, smaller, ownership-gated labels, translucency).
Head-relative hand travel (6 cm deadzone, eye-0 basis) drives selection via
WeaponWheelPickSource=1 (default; 0 = legacy stick); FSM/commit untouched.
Wheel 20%->16% frame height; labels only for held/seen weapons; fills
translucent, text opaque. L1: 99-check layout suite (hand direction incl.
behind-eye/deadzone/fallback, ownership gate, FSM hand edges) PASS. L2: x86
PE32 /O2, static analysis clean (candidate SHA after clean-tree rebuild).
Regression verdict from the 2E2064FB session log: doubled movies + 26-38 fps
from boot predate first wheel OPEN — wheel code exonerated (early return
while closed); fixed ~30 ms/frame stall, draw-count independent, needs
separate A/B vs accepted 6F4BBEF2 on the same save. L3 redeploy pending.

LATEST 2026-10-15 wheel ROUND-2 DEPLOYED for L3 (DLL C1974AFF/source 2ecd757,
x86 PE32 /O2, clean tree; built==installed hash-verified). Backup
20261015-094655 (prior 2E2064FB). INI lease carried over (02DCE23D, backup in
ww-round2 dir): Round=1 + DirectSelect=1 + KatorgaHud=1, no PickSource key =
hand mode. Live record: deployment/hands-dubbing/ww-round2-20261015/live.json.
Need: 10 s no-R3 baseline, then R3 open (hand moves select, 6 cm deadzone),
owned names only, translucent smaller ring, release commits, normal exit.

LATEST 2026-10-15 wheel ROUND layout IMPLEMENTED, L1_PASS + L2_PASS (MAIN
takeover: previous agent gone, user authorises MAIN continuation in this
checkout). Ring + wedge sectors + catalog labels behind
[Input] WeaponWheelRound=0 (default OFF; squares path byte-identical when
OFF). Slot-0 icon/label show the held weapon; FSM/commit pulses untouched;
ammo NOT drawn (AMMO_UNKNOWN, no native hook). L1: layout round-trip 2..8
slots + sanitation + slot labels, catalog labels, atlas, wheel FSM — all
PASS. L2: x86 PE32 /O2 DLL SHA
2E2064FB1689E166162107D42CFD829E0BD9AC4A83AB08B3ECDD24E8F201DC3F
(source 39d6b38 clean tree; static analysis clean; deployed, see below). LATEST 2026-10-15 wheel ROUND DEPLOYED for L3 (DLL 2E2064FB/source 39d6b38,
x86 PE32 /O2, clean tree; built==installed hash-verified). Backup
20261015-092336 (prior EA9EFFB1). INI leased 02DCE23D (backup 4924DF4D):
WeaponWheelRound=1 + DirectSelect=1 + KatorgaHudEnabled=1; installed diff is
exactly those two added lines. Game was closed at deploy; MAIN does not
launch — user starts via Steam. Live record:
deployment/hands-dubbing/ww-round-20261015/live.json. Need: R3 open shows
ring + 4 labelled wedges (top = held weapon), stick tilt highlights orange,
release on sector>0 commits as before, normal exit. After exit: log
analysis (OPEN/COMMIT/DIRECT/LANDED + `overlay active (round`), INI restore
+ DLL rollback hash-verified. Prior history.

LATEST 2026-10-15 wheel VISUAL test (PID428, 3149A872): R3 open (atlas load +
icons?), select sector, release, normal exit. Flags leased (DirectSelect=1,
KatorgaHud=1; backup ww-visual-test/ini-backup). DLL backup 20261015-003155
(TV-accepted 6F4BBEF2). Observer ww-visual-20261015-0032. After exit: atlas
evidence + equip logs + INI restore + DLL rollback, all verified. Prior history.

LATEST 2026-10-15 wheel direct-select TEST (PID7112, 1ADD5608): R3 open
(catalog load), select another sector if 2+ weapons owned, release, normal
exit. INI leased (DirectSelect=1, backup ww-direct-test/ini-backup). DLL
backup 20261015-001750 (TV-accepted 6F4BBEF2). Observer ww-direct-20261015-0017.
After exit: LANDED/EXPIRED analysis + cycle_order verdict + INI restore
(757F414B) + DLL rollback (6F4BBEF2), all hash-verified. Prior history.

LATEST 2026-10-15 TEXTURES row VERIFIED: user cycled 2x→4K→8K in headset
("jest git"), installed texture_override.ini reads Quality=8K — save path
PASS on disk. L3 display/cycle PASS per user; injector-log proof (quality: 8K
after restart) pending next launch. Row authorized for PR (no merge by MAIN).
HUD assets stand installed (81 files, inert until code integration — user
asked why invisible: catalog/wheel/draw code not yet written). Prior history.

LATEST 2026-10-15 TEXTURES row TEST (PID37152, 9C37BD1B): panel DISPLAY →
TEXTURES cycle + save, normal exit; then INI check. Backup 20261015-000526
(TV-accepted 6F4BBEF2). Observer textures-row-20261015-0005. L3/L4 pending. Prior history.

LATEST 2026-10-14 TV PHYSICALLY_VERIFIED (user: single image OK) + PR #25
merged. PROMOTED accepted DLL 6F4BBEF2 (source cbb4509, already installed +
verified). EA9EFFB1 retained as fallback. New branch fix/hud-3d-textures from
origin/main for HUD assets + TEXTURES row. TV round closed; no rollback. Prior
history below (merged).

LATEST 2026-10-15 health-bar ART + GESTURE on feature/health-bar-glove (new
worktree from origin/main; PR branch fix/hud-3d-textures untouched). Plates
atlas installed (Steam-only data): 512x256 PNG + plates.json (3 rects, sha
verified), 11 segment slots per mockup measurement. L1: 18-check gesture
suite PASS (face-raise 40/55 cm hysteresis, 11-seg mapping, unknown->-1).
Runtime (sampling/visibility/draw + L2 + L3) is next, behind
[Render] HealthBarEnabled=0. See docs/health-bar-glove.md. Prior history.

LATEST 2026-10-14 visible-region mirror DEPLOYED (cbb4509/6F4BBEF2, 21/21
fixture, clean): eye aspect-fit into the on-monitor part of the window
(expected dst 430,0,1489,1080 for the observed placement). Backup
20261014-233228; accepted EA9EFFB1 in 20261014-224124. Observer armed
(mirror-vis-20261014-2332), game NOT launched — awaiting user start. Need:
full 1080p single-eye TV image, then rollback. Prior history.

LATEST 2026-10-14 placement probe DEPLOYED (8dd550d/68644067, clean): logs
window screen rect + monitor rect (max 2/session) to place the mirrored eye
into the visible 1080p. Contract ok=600 proven in PID39420. Backup
20261014-232827; accepted EA9EFFB1 in 20261014-224124. Observer armed
(mirror-place-20261014-2328), game NOT launched — awaiting user start. Prior history.

LATEST 2026-10-14 mirror CONTRACT DEPLOYED (9b96c17/E612C2D5, 18/18 fixture,
clean provenance): full-bb Present src + NULL dst now proceeds to mirror;
true subrects stay native + probe. Backup 20261014-232154; accepted EA9EFFB1
in 20261014-224124. Observer armed (note typo'd dir mirror-fix-20261014-2322,
evidence lands there regardless), game NOT launched — awaiting user start.
Need: ok>0 in log + single TV image + HMD no-regression, then rollback.
Prior history.

LATEST 2026-10-14 boot logo: stock-vs-candidate frames compared (90/180/300).
Stock ends "...SINGULAЯITY" (mirrored R = signature style); candidate appends
3 marks ("ЧПЧ"-like) meant as VR suffix. User says mirrored style is intended
("tak ma być"). Readability of the suffix as "VR" unverified by MAIN — user
is arbiter. Stock currently installed (FE40BA3B verified); reinstall of
D85FC268 on user order only. Prior history.

LATEST 2026-10-14 boot logo ROLLED BACK to stock (my install, my rollback):
candidate SNG_logo_vr renders garbage glyphs ("SINGULAЯITYЧПЧ" instead of
"SINGULARITY VR", confirmed by decoding frame 180 of the installed .bik —
matches their reference preview, so the defect is in the candidate, not the
install). Stock FE40BA3B restored + verified. SIDE must fix the VR-suffix
glyph cloning before any reinstall. Also: user's "no VR" check looked at the
FIRST screens (ATVI/raven/UE3 logos play before SNG_logo) — branding was only
ever targeted at the 4th card. Prior history.

LATEST 2026-10-14 rect-probe DEPLOYED (b17715e / 24B9ECC4, fixture
PASS, clean provenance), backup 20261014-224124 holds EA9EFFB1. One-shot
mirror-rect logger (max 4 distinct src/dst shapes + bb/win, read-only).
Observer armed (rect-probe-20261014-2241), game NOT launched — awaiting user
start. Need: building save ~30 s gameplay + menu, normal exit, then rollback
+ contract design. Prior history.

LATEST 2026-10-14 FULL DUBBING ON STEAM (no launch): v7 installed 102/102
(101 banks + story flags), independently re-hashed. Live Polish banked lines:
78 VI2 + 721 new = ~799 + old intro helicopter. Retained EN: 5 VI2 + ~184
new (tempo) + 3 failed banks stock (RT1_fx01, RT3_Vignette, VI6_kismet03) +
combat barks/deaths + MP + VI_intro-new (no source). DLL/INI/injector/font/
WAVs frozen. Backup asset-backups/20261014-222826-40844. Game-load/headset
gates PENDING (no test requested). v7 tooling committed (profiles, pins,
generators). Prior history.

LATEST 2026-10-14 boot logo INSTALLED (user-ordered): candidate SNG_logo_vr
independently validated (1280x720/30fps/11s, BIKi, audio, D85FC268…),
stock FE40BA3B backed up to boot-logo-backup-20261014, hash verified on disk.
Full xlingual build done: 109/112 banks OK (800 PL lines, 184 retained);
FAILED stay stock: RT1_fx01, RT3_Vignette (PCM meta), VI6_kismet03. "Pauza"
clipped to "AZA" noted (title field widths, pre-existing likely). Next: v7
story flags + single install of remaining banks (VI2-done excluded). Prior history.

LATEST 2026-10-14 previous intro bank INSTALLED (user-ordered interim):
4904F281 via new v6 profile (Preflight PASS, stock backup
asset-backups/20261014-221929-43400), hash verified on disk. Known quiet;
no xlingual source for intro exists. No launch. Prior history.

LATEST 2026-10-14 xlingual test PID18444 (short: menu→gameplay→cutscene→exit):
helicopter still EN — structural, not a regression: SP_VI_intro_Kismet bank
is stock BY DESIGN and xlingual has ZERO rows for it (11 xlingual 'intro'
rows are freight-train/RT1 scenes). Nothing to integrate for it from the new
set; old intro candidate (old voices) exists but intro stays protected unless
user orders otherwise. VI2 building content still unreached this run (user
tested game start). Xlingual banks intact on Steam (8FCAE808…), overlay still
menu-only. Prior history.

LATEST 2026-10-14 decoder UNLOCKED + candidates independently verified:
tools/uedecompress/unpack_chunks.cpp (in-tree since 18f64f4) decodes stock
sound banks BYTE-EQUAL to reference logicals (kismet02 → f6a24416…); same
binary decoded all 8 xlingual candidates → sane packages, all 78 replaced
Vorbis payloads byte-equal to fitted OGGs. Old-flow verification depth
reached offline. Remaining banks (VI3-6/RL/RT/SL/WL) now unblocked: stocks on
Steam, mapping in xlingual manifest, same pipeline. User headset test on
PID18444 doubles as first game-load of the new container format. Prior history.

LATEST 2026-10-14 xlingual banks INSTALLED on Steam (no launch): 8/8 VI2
banks built offline (78 PL lines, 5 retained EN, peaks/tempos gated,
container unpack + structural verification PASS) and deployed via new v5
profile in test-prepared-assets (Preflight PASS, backup
asset-backups/20261014-220913-43700 with v4 banks). Independently re-hashed
on disk. Story manifest v4 flags already cover all 78 banked lines — no
double-play, no manifest change. DLL/INI/injector frozen, 992 WAVs intact.
Game-load/headset gates PENDING (no test requested). PR #22 merged prior
branch state to main without MAIN authorization — noted. Prior history.

LATEST 2026-10-14 xlingual bank build (offline): old recipe reconstructed,
tools located; uedecompress sound-incompatible so logicals verified by pins
instead (5 room + 3 audit MATCH). Pipeline tools/audio/prepare_xlingual_banks.py
(8 stocks+logicals pinned, 83 mapped lines, QA recorded; 6ba2 in scope, 825b
out). Two unpack-check self-bugs fixed (tuple compare, record offset).
Checkout was found switched to foreign side/polish-deploy-profile (their
pushed 9cf84b9 + my one accidental local-only commit 8b6f6f4) — restored to
fix/recover-accepted-opening, foreign ref left untouched for SIDE to repair.
Build running in background; Steam untouched. Prior headers history.

LATEST 2026-10-14 user anchor CONFIRMED: working Polish dialogues = PID37660
live test 16:27 (DLL 2763A984, first-area L4: NPC/recordings + note glyphs).
Reinstalled v4 bank/font/manifest set is that era's accepted payload scope
(8 VI2 banks + 60E72871 + 3 manifests); notes re-confirmed PL in PID46016.
Open: menu double-flat (overlay falsifier hit), TV subrectangle mapping,
xlingual-to-bank integration, PR via user browser. No test requested.

LATEST 2026-10-14 SIDE-dubbing claims verified offline (no Steam writes):
R6 files 6ba2/825b byte-identical in C:\PL-dubbing, xlingual source AND Steam
(476c5c72/a8ae6d25, mono16/24k) — already deployed, nothing to install; 825b
middle still ear-QA pending. sol_dobry.txt searched: NO dubbing build in it
(Oct-12 pause/Bink era); the working build was Oct-14 VI2 banks (restored).
install_polish.ps1:39 already xlingual BUT script is broad (VFX+INI+admin) —
will not run; story files already on Steam. Branch
agent/polish-xlingual-dubbing-20261014 = PR20 head 1532907: 62 files
+15064/-8 incl. renderer (vr_scene_mode.h) — NOT audio-only, do not merge.
gh unauthed here; PR creation needs user browser. Prior headers history.

LATEST 2026-10-14 dubbing-check PID46016 (38-pack installed): notes POLISH
CONFIRMED by user. Start-game/radio speech still ENGLISH — consistent with
scope, not a contradiction: intro bank stock BY DESIGN (candidate never
installed), 4 short kismet01 radio lines retained EN by design (VI1_0107_CP2,
VI1_0507_NIM, VI1_0114_CP, VI1_0110_CP2), nonverbals kept; 16/20 kismet01 +
23 NPC + PA/ANN lines are Polish in installed banks but the New-Game path
never reaches VI2 building content — building save with banks UNTESTED, no
test requested. Menu double-flat on New Game CONFIRMED still BROKEN:
MenuOverlay=1 experiment hit its falsifier. SIDE edited MAIN checkout
(reviewed: kept run-trial hard-disable + verify notes, removed __pycache__);
SIDE must not edit MAIN tree again. Prior headers history.

LATEST 2026-10-14 SIDE offline verify (no launch): Steam read-only
38/38 asset v4 +992 story WAVs +3 runtime match; intro stock (protected).
Root-cause write-up docs/SIDE-AUDIO-INVESTIGATION-20261014.md;
tools/audio/verify-polish-install-state.ps1 → steam-verify-latest.json.
New xlingual still needs native bank integration (87 native_replaced).
run-recovery-trial.ps1 hard-disabled (stale quiet-revoice). L4 pending.

LATEST 2026-10-14 dubbing+notatki FIXED offline (no launch, no test asked):
root cause = restore guards had wiped the full accepted 38-asset payload to
stock; re-persist saved only DLL/INI/injector + vr-assets WAVs. vr-assets
WAVs alone never drove NPC/radio (native banks are the sole producer —
proven). Reinstalled exact38 v4 payload via test-prepared-assets (Preflight
PASS, backup asset-backups/20261014-212951-40600): 8 Polish VI2 banks
(FB03741D/D6EA4374/A1A83F2/1CAF1E3F/A2E6B73/56CDDB02/7D50BD2B/1B2522CC),
Polish font 60E72871, 3 PL manifests (6518581E/0D632BF4/22B755EB, 87
native_replaced), UI 69CABCCF unchanged, 24 DDS + override INI. Independently
re-hashed: all match; DLL EA9EFFB1/INI/injector frozen; 992 new xlingual WAVs
byte-identical to freeze (names resolve from new manifest). See
docs/native-npc-radio-producer.md. Prior headers history.

LATEST 2026-10-14 mirror-diag RESULT: root cause PROVEN. All ~26k Presents
(44×600 windows) hit skipRect — engine Presents with explicit src/dst rects
every frame, mirror `ok=0` for the whole session. The subrectangle contract
was never implemented, so the TV fix could never engage in-game; prior
"wszystko okej" likely never included a real TV check (HMD on). Accepted
EA9EFFB1 restored + verified, INI/injector/992 WAVs intact. Next probe:
bounded one-shot capture of actual presentSrc/presentDst rect values, then
subrectangle mapping. See docs/native-npc-radio-producer.md (TV section).

LATEST 2026-10-14 mirror-diag probe DEPLOYED, awaiting headset: DLL
CF96A841 (source 5dfaada, x86 PE32 /O2, fixture 12/12 PASS, analysis gate
clean for desktop_mirror.h). Bounded outcome counters + last geometry, one
`mirror-diag:` line per 600 Presents; copy logic byte-identical. Installed
via deploy-singularityvr -DllOnly -KeepIni -SkipTextures, backup
20261014-211920 holds accepted EA9EFFB1 for post-test rollback. INI/injector/
992 WAVs untouched. Steam PID43880 launched, observer-only
(mirror-diag-20261014-2119), no restore guards. Need: 1-2 min load-save at
building + radio, normal exit, then rollback + analysis.

LATEST 2026-10-14 manual headset retest PID12424 (~2 min, observer-only, no
deploy): audio hypothesis CONFIRMED — zero `polish voice:` lines, only menu
strings through Localize; NPC/radio/long dialogue all English, note English.
Gameplay window only ~25 s (MenuFlat→GameplayStereo 21:12:33→MenuFlat 21:12:58).
TV reported TWO images on the identical accepted build+INI (EA9EFFB1/757F414B)
— cause UNKNOWN: desktop_mirror.h has zero runtime logging, so logs cannot
distinguish mirror-inactive (menu/raw SBS) from mirror-failed (gameplay).
Install intact after exit (3 hashes + 992 WAVs). Next: bounded mirror
diagnostics candidate (counters only), then bank integration. See
docs/native-npc-radio-producer.md. Prior headers history.

LATEST 2026-10-14 MAIN turn: NPC/radio root cause PROVEN (not fixed), freeze
ready, no launch. PID35568 log shows zero `polish voice:` lines and Localize
saw only menu strings — NPC/radio text never reaches PolishMaybeSpeak
(chars>=40 gate + text-hook-only path); native SoundNodeWave banks are the
sole producer. 40 first-building xlingual WAVs mapped to SP_VI2_kismet01-04
assets, all present mono16. Native originals untouched, pins match.
New offline freeze deployment/hands-dubbing/current-install-freeze-20261014/
holds3 runtime +992 story +8 VI2 bank hashes; old run-recovery-trial.ps1
refused (stale A8F38EF6/quiet-voice baselines). gh CLI not authed: PR must be
opened via the logged-in compare page. Game closed, tree clean, HEAD 6bba929
in sync with origin. See docs/native-npc-radio-producer.md. Prior headers history.

LATEST 2026-10-14 PID35568 / EA9EFFB1: user reports everything OK except Polish
story speech; note NOT inspected. Clarification: loaded save at first building,
NPC and radio remained English. Scope is not opening/new-game audio acceptance.
Guard restored all4 core+38 assets+1886 WAV originals; TV subsequently reverted
with the previous renderer/config. User now requested new992 xlingual WAVs and
persistent TV correction. Exact3 runtime/INI promotion completed (EA9EFFB1 /
F5137234 / INI757F414B), backup20261014-205618; no guard or new launch.
992 source WAVs installed with verified backups;894 UI voices unchanged.
Intro, multiplayer, native banks and font untouched in this story-only action.
PR21 already merged the43 SIDE text/tool files; do not merge conflicting PR20.
Native NPC/radio playback remains BROKEN/producer investigation, not fixed by
the WAV copy. 825b middle phrase still requires listening. See the new handoff.

LATEST 2026-10-14 recovery: user rejected A8F38EF6 combined installation
(TV stretched, menus/free hands/FPS regressed, Polish text/voices/notes lost).
Audit proves original injector/font/eight native bank deltas/three manifests,
and absent accepted wrist/cinema/menu/texture-bridge/native-speech flags.
Recovery preserves UI69CABCCF ring/title, stages the accepted first-area payload,
and fixes TV aspect mapping including Present window scaling. No Steam writes
or launch. Eleven optimized x86 fixtures + scoped INI tests PASS; combined L3/L4
PENDING. See docs/recovery-after-tv-test.md. Earlier latest blocks are history,
not acceptance of this rejected configuration.

LATEST: hands accepted on 627F1111. That same session captured the main menu
and pause into the RTT (pixels are there) while MenuOverlay=0, and the scene
copy of the main menu was suppressed, so the headset had nothing to show.
DLL A8F38EF6007BCED8F3CB247E5A4A65942958CD4A1ED43F1B6601F81E342D001C keeps the
in-scene menu and notes when the overlay is off, and stretches the right SBS
half across the game window after XR submit (DesktopMirrorEye=2). Glove-only
stays forced. Backup 20261014-200828. Boot card is still stock SINGULARITY:
no binkenc. L4 is the TV picture plus the main menu, pause, and a Polish note.

LATEST: cursor ring accepted. Glove-only is now forced on: ini ShowControllerArms
is 0 (was 1, one byte) and the panel cannot turn the arm mesh back on. DLL
627F11118E72A41A5305ABD9A80975AAC587D22C5B0788305AF298678F1BCC7B. Free hands use
the measured wrist-anchor correction; a live 5799 pistol still aims through
BuildGunC. Backup 20261014-195152. UI 69CABCCF and font unchanged. No restore
guard. L4 is the next unarmed-glove look, then the pistol in the same session.
Boot card is not installed: no binkenc.dll on this machine, only the SIDE MKV.
Hand plates are not in this DLL.

LATEST: headset check of 256BF823 + E3E7C902: menu text SINGULARITY VR is good,
arms no longer stretch, free gloves still sit too far from the pads until the
pistol is picked up, then tracking and grip are good. Exit was not checked.
Glove distance is unchanged this turn. The native Mouse shape is a 27×29 px
arrow whose bounds start at the hotspot and extend down-right, so the visible
cursor sat off the laser. UI 69CABCCF13C0F13B120163331ADDCFDDE3764CE8E32FE60A15F800FDF8C87D66
keeps the same title fit and replaces that leaf with a ring centred on the
hotspot. DLL stays E3E7C902, INI 202AA13A, font 3F7BF0BE. Backup of the stock
UI is title-ui-20261014-193045. No restore guard is armed. L4 for the dot is
the next menu look. SIDE also reported texconv is absent; the boot card stays
on SNG_logo.bik, not a DXT5 swap of ui_logos.logo.

LATEST: menu title package 256BF823 stays installed. It is the LZO rebuild, not
the uncompressed package that crashed. DLL E3E7C902 from 318fb9a hides the
native arm mesh until a weapon is in hand, because the 19:02 log on 2763A984
showed ShowControllerArms=1 and a weaponless 3314/390 pass that correctly did
not latch. Backup of the previous hand DLL is 20261014-191533. No restore guard.
Boot wordmark remains the separate SIDE task.

LATEST: user still sees the boot wordmark SINGULARITY without VR, and hands
remain stretched on D8928180. Installed the previously accepted hand DLL
2763A984F01384C6FC0B289333E0283A9273665FD02B13A978DE95611F48C0B7. Backup
deployment/install-backups/20261014-185905. UI is the original 5B2EA469.
Boot card is a separate producer from the GFx title fields. SIDE handoff:
docs/handoffs/side-boot-logo.md. L4 for this reinstall is the next launch.

LATEST: title package E852A20D crashed before the menu. Log ended on the engine
string "Ran out of virtual memory" and Windows recorded 0xe06d7363. That package
stored the UI chunk uncompressed; the game decompressed those bytes and aborted.
Replacement 256BF8234CBCD959C8E7FB62D27EBC4A2D186810746326EE65C3C9DA24FE3F14 uses
LZO literal blocks, round-trips through unpack_chunks, and is installed. DLL
remains D8928180. Backup title-ui-20261014-185316. L4 still open.

LATEST: title UI E852A20D2CB613602BA3361550F44551BD7A72484CB0D834468813190E86A7CD
installed. DLL stays D8928180, INI202AA13A, font3F7BF0BE. Package only widens
iis character 10 (right edge fixed) and SPMenus character 166 (centred) and
writes SINGULARITY VR. No cursor change and no 4233E4A2 DLL. Chunk roundtrip
and fake install/restore PASS. Backup title-ui-20261014-184342. Physical read
of the boot title is the next check. L4 not claimed.

LATEST: PID33552 witness ran and was restored. Live RawData is the original movies,
not the private candidate: iis 30959 FNV669B2A3F185327C8, SPMenus 231953
FNV51B6A92D4DED91D0 on main, options and pause. The old +3 byte title edit clipped:
iis character 10 is right-aligned in a 500px box that the word already fills, and
SPMenus character 166 is centred in a 900px box. fit_title_field now grows the free
side (iis xmin -40 to -2767, right edge fixed; SPMenus both sides by 4909 twips)
and keeps the replacement exclusive. DLL4233E4A2 stays rejected. Steam remains
DLLD8928180, INI202AA13A, UI5B2EA469, font3F7BF0BE. No new Steam install.

LATEST: user rejected witness DLL4233E4A2 during PID33552. Hands stretched again
and the menu was doubled flat on exit to the menu. That DLL is not installed.
Independent restore remains DLLD8928180, INI202AA13A, UI5B2EA469, font3F7BF0BE.
Do not redeploy 4233E4A2. The physically accepted first-area DLL was 2763A984
from 34b180e, saved at deployment/hands-dubbing/post-opening/next-core-v1/d3d9.dll.
HUD corner reach was not changed by this lease.

LATEST: menu-witness session PID33552 L3, then restored. Independent hashes: DLLD8928180,
INI202AA13A, UI5B2EA469, font3F7BF0BE. Live MovieInfo.RawData matched the original
packages, not the private candidate: iis 30959 bytes FNV669B2A3F185327C8 on
RvUIGFxMovie_IIS; SPMenus 231953 bytes FNV51B6A92D4DED91D0 on SPMain, SPOptions
and SPPause. GFX magic 47465809. Saved frames show MENU GŁÓWNE and PAUZA, not a
separate title texture. No title edit. No L4 for this DLL.

LATEST: menu-witness lease ARMED, game not launched. Installed DLL4233E4A23BE50D47A017369A4F231D2DAE2A10DA068BA08D57E0EAC7EC540BAC
from build c5d3924, x86 PE32 /O2. INI8A8F52F3ADFFC9AE63ADCF8364B97F59C8651D680FEBC5D4001C9AB95D767026
adds only Diagnostic.MenuAssetWitness=1 over preserved202AA13A. UI5B2EA469 and font3F7BF0BE
untouched. Backup deployment/install-backups/20261014-180712. Restore guard READY,
1800s to launch, then restores after a normal exit. No L3 until a process runs. No L4.

LATEST: witness candidate c5d3924, DLL4233E4A23BE50D47A017369A4F231D2DAE2A10DA068BA08D57E0EAC7EC540BAC,
x86 PE32 /O2. Offline fake lease PASS: WhatIf, path refusal, corrupt-backup refusal,
user-INI preservation, owned rollback, frozen UI. Not installed, not launched, not L3/L4.
Installed Steam unchanged: DLLD8928180, INI202AA13A74BF1FF49104C8C507A1B9A6A6069D22DC43A529FCF6BFFC07430DBF,
UI5B2EA469, font3F7BF0BE. Source-only title strings: iis WFontLogo HTML is the run the
private candidate changed to SINGULARITY VR; SPMenus keeps a styled SINGULARITY run in
both packages and the candidate also writes MajorTitle text SINGULARITY VR. ui_logos.logo
is one 512x128 DXT5 beside weapon HUD textures. SPMenus token UI_Logos. sits by
getSubstitutions/SCOPE_MP_PLAYER_MSG, so it is not the proven title. Live RawData FNV
still required before another edit. Source FNV, not pixels: control iis 669B2A3F185327C8
30959 bytes, control SPMenus 51B6A92D4DED91D0 231953, candidate iis 2470EB3DBD9A78BB
31178, candidate SPMenus 56AFE359868D7429 232282. HUD SIDE assets are 5f4364c on
fix/psvr2-hud-design and stay unintegrated.

LATEST after PID37660: scoped L4 PASS cinema stability, right hand/pistol grip,
Polish first-area NPC/recordings and note glyphs. Logo and native arrow STILL
BROKEN; slight laser/cursor deviation. User defers small acceptable FPS dips.
1927 original files restored independently; INI user armsON/narratorOFF preserved
and owned flags removed via exact cleanup. Actual INI202AA13A74BF1FF49104C8C507A1B9A6A6069D22DC43A529FCF6BFFC07430DBF;
safe baseline DLLD8928180/UI5B2EA469/font3F7BF0BE. No game/test left active.
Next producer witnesses defaultOFF: live GFx RawData identity/coordinate dispatch
and native pistol template identity. No speculative cursor/logo offset fix.
Offline original logo texture decoded ui_logos.logo512x128 DXT5, earlier inventory
incorrect; contributing draw still UNKNOWN. Native weapon cue/particle assets
found; user requests BOTH sounds and VFX, no generated replacements.
HUD SIDEcd76bac reviewed, source-art prompt delivered; no HUD renderer integrated.
First-building projector mismatch recorded UNKNOWN producer. MAIN active scope:
logo/ring/laser correspondence and original gun FX/audio, preserving L4 systems.
See docs/first-area-accepted-and-ui-witness.md. Previous entries historical.

HISTORICAL LIVE:
LIVE 2026-10-14: fresh user start, Steam PID37660 launched16:32:55+02:00.
DLL2763A984F01384C6FC0B289333E0283A9273665FD02B13A978DE95611F48C0B7
loaded from Steam Binaries; injectorF5137234 loaded. Exact4+38+1886 installed
hashes and original backups PASS; actual OpenXR session3/running1/haveEyes1.
PSVR2 native SteamVR HMD evidence before launch; no xrsim/registry/UI automation.
Private deployment/hands-dubbing/first-area-font-live-20261014-1627/live.json.
Core backup20261014-162546-8912, assets20261014-162549-8912; WAV originals in
live/revoice. Guards10836/36428/39912 READY600s no-launch/post-exit; observer38296
READY1800s active/passive exit collector. No restoration claim while game runs.
Cinema world quad ON2000mm, gloves-only, accepted shadows/input unchanged.
User stays headset: NewGame/save confirmation, helipad pause/options, free glove,
pistol, first-area speech/note, title/pointer/menu, normal exit. L4 feedback PENDING.
Two pre-launch orchestration failures restored exact baseline before retry; no
game launched in them. Failed guard parameters/empty-log READY detection corrected
in private runner; candidate DLL/assets unchanged. Restore all3 leases after exit
and independently verify1928 originals/frozen107DDS. Never overwrite running files.

HISTORICAL PREPARATION:
LATEST 2026-10-14: OFFLINE PREP READY first-area/font polish on fix/post-opening-followups,
Draft PR19, NOT merged. Font SIDE219c6fa cherry-picked59da9f9; private output60E72871
matches pinned original3F7BF0BE. UI5E46B619 replaces exclusive Mouse geometry and
scoped Initialize title; three NPC banks/23 utterances retain9 nonverbal exports.
SIDE audio e285652f reviewed;3 ASR REVIEW lines, not full-game/auditory acceptance.
Free-hand target selected after wrist reconstruction; accepted pistol/modal policy
retained, COM invalidation moved to render thread. Cinema LOCAL quad defaultOFF,
same composed pixels/one scene anchor; enabled only in staged next test INI.
26 Python tests +9 optimized x86 fixtures/policy PASS. Exact38 fake installation
WhatIf/backup/corruption/conflict/restore PASS. Fresh hidden WindowsPS38 guard and
core4 no-launch timeout independently restored exact baselines PASS. x86/O2 DLL
2763A984F01384C6FC0B289333E0283A9273665FD02B13A978DE95611F48C0B7
from tracked-clean source34b180e; next-core-v1/next-assets-v4/quiet-revoice-v2 ready.
Actual1928 original identities and staged1928 candidate hashes PASS;992 legacy
WAV filenames preserved by new lookup. Original font independently backed up.
Actual Steam remains original, no real lease/game launched this preparation turn;
fresh user start required. Previous live restore independent1924 originals PASS.
Font preview is an approximate raster, not proof of native GFx fill/wrap.
Private bundle deployment/hands-dubbing/post-opening/next-candidate-index.json.
See docs/post-opening-next-candidate.md. Entries below are historical snapshots.

HISTORICAL:
PREP READY after c029abe: x86PE32/O2 DLL1454A62B0AC28EC9A9C8D3A83E16DD9CF142E6CAEE3220FC591079F015192751.
Private post-opening/core-ready +assets-ready exact34 +quiet-revoice-v2. Core4
and asset34 fake/preflight gates PASS; fresh hidden WindowsPS34 timeout guard
restored/independent hashes PASS. WAV plan894UI/992story all identities/formats,
pending146QA, no install. Actual4+34+1886 originals/107DDS verified unchanged.
Draft PR19 created/attached, not merged. New physical/runtime feedback PENDING;
fresh start only. Original Steam untouched; no active real-install guardian/test.

LATEST PR17 merged on GitHub into main8751a27 (not merged by this MAIN turn).
Dedicated fix/post-opening-followups worktree. OFFLINE preparation, no Steam
write/launch. New rooms2banks/19waves/13SIDEWAV, properties/cues/timing preserved,
exact PCM/loudness/peak/independent LZO PASS; SIDE17e2f6b reviewed full native text,
NOT_AUDITIONED.15 first-area Notes translations prepared, other57/open keys remain.
UI NARRATOR defaultOFF live DISPLAY toggle; quiet894UI candidate35% amplitude,
992story unchanged, inherited146QA review not upgraded. Source long-note bounded
capacity/nontruncation, diagnostic sparse-flush fix and two-epoch wrist probe L1
PASS. ShowControllerArms1 observed in actual INI/log; stage0 for glove-only test,
no guessed pistol grip offset. NewGame SPSlots/dialog producer/capture identity
probe only; layering fix and pistol grip remain OPEN. Exact34 v2 asset lease fake
install/restore/corruption/conflict/WhatIf PASS; source build/staging finalization
below feature note. Fresh start required; never deploy over running game.
See docs/post-opening-followups.md. Previous entries below are historical.

LATEST PID17436 closed; all3 guards restored, independent4+32+1886 hashes and
107DDS PASS (live restore-verification.json). User physically heard Polish radio,
generally accepts this version and requests PR targeting main (NO merge).
PHYSICALLY_VERIFIED scoped opening radio presence after one-wave loudness change;
not full-game dubbing or all materials/hands acceptance. New requested follow-ups:
English indoor recordings/dialogue; untranslated notes; FIRST pistol pickup leaves
visible forearms and poor right grip AFTER animation; UI/tutorial narrator too loud
and needs VR Settings ON/OFF; New Game save selection/confirmation overlap.
Prepare follow-ups separately from accepted PR snapshot. No new Steam test until
fresh user start. Audio generation remains SIDE-only when needed; MAIN installs.
Room/notepad identities being narrowed; no guessed pose offset or shader register.
Below previous live/preparation history.

HISTORICAL LIVE PID17436 after user gotowe VR. Previous no-HMD session23792 closed;
all3 guards completed and4+32+1886 originals independently verified in its
restore-verification.json. SteamVR had quit-all from vrmonitor; MAIN relaunched
app250820, actual playstation_vr2 HMD and vrcompositor confirmed BEFORE game launch.
Fresh same candidate lease deployment/hands-dubbing/three-items-live-20261014-102151:
core C1D904D0/F5137234, bankFB03741D, all candidate hashes and backups verified;
all3 guardians READY. Steam game launched, actual loaded module paths confirmed,
CURRENT OpenXR ready/session3/running1/haveEyes1. Observer26396 collects logs;
physical radio/language/loudness result and exit/restoration pending. Core backup
deployment/hands-dubbing/backups/20261014-102152-17684; asset backup
deployment/hands-dubbing/asset-backups/20261014-102406-17684; WAV revoice/backup
inside new live evidence. Only one radio waveform gain differs from earlier trial;
no hands/input/shadow/stereo/INI/render changes, remaster_apply0. User headset
script continues1-2min into first gameplay after standing up, closes without chat
switch/restart, reports radio instruction near tower. DO NOT replace running files.
Do not claim current restore until all3 guardians finish and independently checked.
Below prior startup/completed trial/preparation history.

HISTORICAL 2026-10-14 startup-only PID23792 (09:53), fresh user start authorized.
Core C1D904D0/F5137234 and native radio bankFB03741D installed; all4+32+1886
candidate hashes/backups verified, all3 guardians READY. Evidence private
deployment/hands-dubbing/three-items-live-20261014-094554; observer33028.
NO RADIO/PSVR2 TEST YET: actual log says no HMD, session0/haveEyes0. SteamVR
was absent at launch; MAIN requested app250820, vrserver23780/vrmonitor31384 now
observed. Source InitXR called once behind g_initTried; BACKSPACE changes VR flags
but cannot create a missing session. User asked to enable HMD/SteamVR, close game
normally and reply gotowe VR. No forced termination or writes while game runs.
Candidate remains leased until exit; DO NOT claim restored until all3 guardians
finish and MAIN independently hashes originals. Then authorized startup retry
must redeploy and start only after headset/runtime readiness. Core backup
deployment/hands-dubbing/backups/20261014-094555-28084; asset backup
deployment/hands-dubbing/asset-backups/20261014-094755-28084; WAV revoice/backup
inside live evidence. startup-boundary.json records exact startup failure.
Below completed prior trial/preparation history.

LATEST PID24468 completed/closed. User: menu OK; radio did not speak. This is
NOT confirmation that all voices were silent or that the radio trigger fired.
All3 guardians completed; independent4+32+1886 original hashes,107DDS and frozen
forwarder PASS (live restore-verification.json). No candidate remains installed.
Runtime single-owner bridge/accepted base substitutions and two exact glove
remaster matches confirmed, remaster_apply0 and no remaster=1 substitutions.
Native radio playback/branding typography/all24 remaster visuals remain OPEN.
Offline RE: typed Kismet RadioConversation -> SoundCue ->17 imported waves;
single-line main action points to VI1_1005_GST. Tested PL radio clips7-13.49LU
below stock; quiet audio is a hypothesis, not proof of event execution.
Next isolated candidate changes ONLY VI1_1005_GST volume (-23.16 -> -13.64LUFS,
stock-12.83, encoded peak-1.37, exact442418 mono48k frames). All other prior
voices/properties/cues/DLLs/INI/title/material settings unchanged. Prepared
deployment/hands-dubbing/radio-event-investigation-20261014/delta-single-radio-ready/
asset-manifest.json; bank SHA FB03741D672537CFC8117C8448361CFEABF819562B3955DADD7BBAD0945AECF8.
L1 synthetic guards/typed graph, FFmpeg PCM+peak, independent source/candidate
LZO and fake32 lease PASS; new runtime/audition pending. Two broad louder batches
failed guards and are NOT deployable. Fresh user start before any new launch.
See docs/radio-loudness-single-line.md. Below earlier live/preparation history.

HISTORICAL LIVE 2026-10-14 08:56+02:00, user authorized fresh start. Steam PID24468,
candidate d3d9 C1D904D0/injector F5137234; actual application module paths and
all4+32+1886 candidate hashes verified. OpenXR ready and actual MAIN slot65
callback forwarding logged, NativePolishSpeech1. All three separate guardians
READY; original core/asset/WAV backups verified. Remaster_apply0 (accepted base
textures kept), title/native radio candidates installed. Independent read-only
observer PID25180 collects session in private
deployment/hands-dubbing/three-items-live-20261014-085320. Core backup
deployment/hands-dubbing/backups/20261014-085321-22672; asset backup
deployment/hands-dubbing/asset-backups/20261014-085516-22672; WAV originals in
live folder revoice/backup. Physical trial/exit/restore verification PENDING.
DO NOT deploy or swap while PID24468/any Singularity runs. No repeat launch
after exit until all3 guards finish and MAIN independently hashes originals.
Below preparation/historical headers describe earlier state.

User physical feedback for E213B216 PID23584/36340: all requested hands/menu/
cinema pause/eyes/controls checks OK except gameplay English radio. Own-wrist
and cinema/menu presentation accepted for tested scope, not all future weapons.
Next requested work: native Polish +SINGULARITY VR branding +existing material
remaster, OFFLINE prepare only; no new Steam launch/install until fresh start.
Current installed originals/frozen files remain restored; guards completed.

Three-item prep: native48new waves +10preserved helicopter waves in4banks;
6short replacements refused/retained EN, not all-game Polish completion.
SIDE old29 +improved8 @9a5ae536; new1886-WAV lookup/format plan PASS,146QA pending.
Exact native ownership gate keeps UI/uncovered PlaySound path. New title package
UIE0F5A661 exact iis/char10 +3bytes; accepted RTT and other movies retained;
independent LZO roundtrip PASS, typography/layout physical acceptance pending.
24existing materials/91,229,592B staged privately from0dcdcd5/188c10a; no new
texture generation or game assets in Git. Separate remaster folder; base107DDS,
glyphs/FX/config groups stay frozen. DefaultOFF single-owner bridge+COM lifetime
and pool-aware Ex upload implemented. Hidden plain/Ex DXT1/DXT5 RGB/alpha/Reset/
exact-forward/refcount and actual MAIN translated-Lock/hash/forward chain tests
PASS. L3/visual remaster/overhead remain OPEN. Exact32 delta lease fake tests
PASS incl corruption/path/WhatIf/conflict. Hidden cold-process timeout guardian
restored all32 fake baselines; independent hash verification PASS. Runtime source
9989baef built x86/O2: d3d9 C1D904D0, injector F5137234. Private core-ready,
delta-log-final/delta-apply-final and voice-delivery-v3 artifacts prepared.
Installation verification 2026-10-14T01:50+02:00: original4/asset32/WAV1886,
UI5B/accepted107DDS/frozen config+forwarder unchanged. No Steam writes/launch
during prep. READY for bounded physical test after fresh start, NOT L3/L4 accepted.
See docs/prepared-audio-title-remaster.md. Below historical.

LATEST 2026-10-14 E213B216 test completed, physical feedback PENDING. PID23584:
OpenXR/loaded paths/exact four hashes PASS; cinema pause reason32 and resume,
then stereo recorded, hands blocked by existing position gate (no sample).
User restarted PID36340 on same leased candidate; second log separately saved.
Wrist execution confirmed, eight GameplayStereo records actualRide/moved0;
captured posed-constant reconstruction residual <=~0.001UU at log precision.
Not pixel/controller-origin/physical acceptance; moved1/2 still L1 only.
Both sessions closed; exact four originals and backups independently verified,
UI5B/107DDS/frozen config+forwarder PASS. No new audio/WAV deployment in this
trial. SIDE b13b8dda provides29private WAVs/34bindings,20ASRpass/9review; audit
of previous42 finds20faithful/22needs correction. Native integration remains
OFFLINE/OPEN; never deploy the old42 blindly. Next test requires fresh start.
See docs/no-tmd-wrist-anchor.md. Below headers historical.

LATEST PID21056 physical: single frontend menu, matching eyes; background not
fully rendered. Cinema pause NOT tested. Small FPS dips remain; L2/R2 unrelated
in this trial. Native gameplay English persists. Four originals D892/AAA/B3FE/
890C + UI5B/frozen files and all1886 WAV originals restored/independently hashed.
Next Steam install/launch requires fresh start. Own-wrist defaultOFF experiment
L1_PASS only: reconstructed no-ride residual49.997/40.844UU; no guessed50cm offset.
Probe corrected to gameplay-only and actual ride capture. L2_PASS sourcea23d7e9,
x86 PE32/O2 SHA E213B216F90CF5545862D8850574D5CD7F82A480983847D4F989507E3E3E58A5.
Offline wrist-anchor artifact prepared; fake four-file lease/restore gates PASS.
Physical pending; no real install changed/Steam launched.
See docs/no-tmd-wrist-anchor.md. Native audio offline three-bank vertical slice
42replacements/4too-long retainedEN; not full Polish/runtime/audio acceptance.
User delegates missing sound generation to SIDE; MAIN retains all Steam writes.
SIDE found incorrect earlier transcriptions in four oversized lines. Native42
candidate text fidelity is now being checked against SpokenText, not inferred
from ASR screen PASS. Do not deploy those native banks before this audit.
Older NEXT/LATEST headers below are historical.

LATEST physical PID34980: no FPS dips noticed in this trial. Frontend menu
duplication after gameplay resolved, but background still differs between eyes;
cinema pause remains small/doubled. Hands too far from face/controllers (~50cm
apparent near-face discrepancy), NOT accepted. Helicopter PL speech louder but
still much too quiet; gameplay native English remains. D892/AAA/B3FE/890C and
UI5B/107DDS/frozen files restored/hash verified after exit. Profile3600frames/
40313ms: cine0/menu165, idle3600/LT0/RT0 (not trigger A/B). Prior headers historical.

NEXT presentation candidate L1/L2_PASS: source7cee8ba, x86 PE32/O2 SHA
21515F87423774ED3B9896C138F0D9EDF70690AA1B9334C4EDD5F9DC64A5FDD4.
CutsceneMenuPairFix defaultOFF: focused cinema pause owns MenuFlat instead of
full-SBS cinematic shrink; frontend/cinema pause use established identical crop
for both eyes. Gameplay/native cinema flag/other UI/accepted controls unchanged.
HandAnchorProbe defaultOFF: eight same-thread copied raw constants, >=2s cadence,
Present flush, no hand correction or guessed50cm offset. Actual seated play used.
Artifact deployment/hands-dubbing/menu-pair-test-artifact, INI89E170DC (four
explicit experiment flags only). L1 scene/probe/math/camera/Bink/hotpath gates,
runtime policy and fake four-file lease/restore PASS; physical gate PENDING.

User requests new1886-WAV candidate (sourcea390ede):894UI/992story, QA1740screen
PASS/146review, all formats and installed identity mappings verified. New WAV-only
installer uses exact hashes/all backups, conflict preservation and optional
no-launch/post-exit guardian. Synthetic1886install/restore/WhatIf/corrupt backup/
user-conflict tests PASS; WindowsPS hidden timeout/restore PASS. User-authorized
temporary WAV deployment separately recorded in private revoice-live evidence;
confirm guard/hashes before testing. Native UE3 gameplay voices are NOT redirected
by installing these WAV files. Pack contains no native helicopter INTRO identities.
Helicopter +6LU/+3LU attempts failed encoded gates and were NOT staged/deployed.
Next Steam DLL install/launch requires fresh start. See docs/cinema-menu-pair-and-revoice.md.

LATEST physical PID16456: Polish helicopter voices PRESENT but too quiet;
noTMD hands almost corrected, controller alignment unaccepted (user seated on
floor/cannot stand). Buttons show no FPS difference; residual dips remain.
Return-menu double-flat regression OPEN. All four originalsD892/AAA/B3FE/890C
restored/hash independently verified; UI5B/107DDS/frozen files unchanged.
Guard cold-process Get-FileHash failure fixed before Steam launch (9dc167a).
Native palette35 execution/OpenXR confirmed; no broad hands acceptance inferred.
Profile3600frames/43.687s includes menu1080/cine309, not pure gameplay; no second
profile armed (one-capture/process invariant). Next OFFLINE candidates: native
voice loudness within1LU (crash radio1.5LU exception), package84B9A419; frontend
presentation ownership defaultOFF/L1+L2_PASS source6628fa6 DLL85CCCB60.
No hand offset/config rewrite based on seated report. Gameplay English/native
full-voice integration and residual FPS still OPEN. See docs/opening-loudness-followup.md
and docs/cutscene-frontend-handoff.md. No next install/launch until fresh start.

NEXT COMBINED trial now prepared OFFLINE: user requested hands+dubbing before
the next FPS test. No new Steam launch/install until fresh explicit `start`.
NoTmdPaletteFix defaultOFF experiment L1/L2_PASS: actual cooked chunk0 has35
ordered slots, legacy code45 disagrees fromslot18; PSK weights independently
agree and281points mix wrist/native forearm. Candidate exact noTMD map+rigid
same-side contributors, NOT TMD/gun/world/shadow changes. Source316f33c,
VR D10A6EE56F76D1F02CC00E3A55B50664D704CF2866FEF3A95724FFE816BD8EDF.
Polish helicopter native SoundNodeWave candidate10voices L1_PASS; original
scream+128other waves/property/event metadata preserved. Candidate package
C8B30A1B639FBA1A6A5CE58542A81C8687DA4DF5C5BB515EAFC685114B40974C;
original890C6172. Speech mono48k/exact native PCM sample budgets, LZO roundtrip
and uepkg layout PASS. Synthetic voices/ASR reviewed, auditory/native load NOT
verified. Not a claim of all-game Polish completion. See docs/opening-native-polish-audio.md
and docs/no-tmd-palette-repair.md. FPS residual cause still OPEN, bounded sparse
native Draw CPU probe included; accepted cache repair injectorD6EFDDAE retained
in offline artifact. Current installed accepted originals D892/AAA, INI B3FE,
UI5B and original audio890C independently verified, game CLOSED.
Exact four-file lease adds only Render.NoTmdPaletteFix=1 (INI383D5E36); retains
ShowControllerArms1/ArmIK0/GloveIsolate2/FaceFilter0. Fake installation+restore,
WhatIf/path rejection/corrupt backup/user-INI preservation PASS incl WindowsPS5.1.
No-launch guardian timeout60s PASS in fake install; all four original hashes restored.
Artifact deployment/hands-dubbing/test-artifact/test-manifest.json. No request
armed. READY_FOR_PHYSICAL_ACCEPTANCE for hand/audio candidate and diagnostics,
NOT a residual-FPS fix or L3/L4 acceptance. Older headers below historical.

CURRENT PID33140 post-cache attribution completed3600frames/40.625s with
229menu frames (NOT pure gameplay). User: dips in gameplay. Slow77FPS window
has no new discovery/replay; input-cache changes NOT justified. Three43.712ms
native scans tied to cinema/menu boundaries. Routine injector classification
~0.104us native/~0.238us noncandidate,no hash churn. Next offline bounded
diagnostic separates original Draw* CPU submission from inclusive draw hooks;
no image/quality/hand/shadow/input-policy changes. L1_PASS: sparse forwarding,
thread domain,stop/exception/HRESULT and existing input/camera/audio tests.
L2_PASS regular x86 PE32/O2 sourcebdcd330,VR99D3F8008F3BF0BD894E2AA28BF062F1A32BE354F3DB055DBD5429BB7C51660E.
Offline staged deployment/native-draw-cpu/artifact; same injectorD6EFDDAE.
READY_FOR_PHYSICAL_ACCEPTANCE for diagnostic execution only, not FPS fix.
No next candidate installed/request armed; await fresh start.
Both originalsD892/AAA restored/hash verified;INI B3FE/UI5B/107DDS unchanged.
See docs/post-cache-performance.md. Prior headers below are historical.

LATEST PID43712: texture native-cache runtime execution PASS; physical L2/R2
FPS drop no longer present. Remaining smaller dips + stretched hands OPEN.
14,145,976 binds,9,909,759 native hits,668,586 legacy hits,950 hashes (late
plateau),914 stamps,zero failures/evictions. No identical-scene speed ratio
claim and no explicit physical texture-appearance acceptance. Both original
DLLs D8928180/AAA07005 restored/hash verified; current INI B3FE/UI5B +107DDS
unchanged. Game closed. User requires prepare offline then ask `start`; NEVER
launch/install the next candidate before that reply. Current next work is
bounded post-cache CPU attribution, not a speculative hand/shadow/quality fix.
Synthetic native shape scan ~0.055us/call does not explain ms-scale dips;
no shape-cache intervention. Diagnostic-only injector timing + existing VR
cinema-exit sampler L1/L2_PASS (x86/O2); candidate remains OFFLINE pending
explicit start. No post-cache runtime attribution yet. See docs/post-cache-performance.md.
The pre-test cache status below is historical.

NOW cache-only repair authorized and L1/L2_PASS. Injector sourcef6aeb6b,
x86 PE32/O2 SHA1BC0F7DD59A46BD7E29DDEC0B17053A3CEA6726C0476A5C0782DC672F6CC3059.
655resource negative results retain native classification; no pixel/asset,
quality/group/hash-table change. Old saturation reproduced; native D3D9
pixel/reset/lifetime test PASS. Pair-only synthetic install+rollback PASS,
INI/DDS/original-forwarder untouched. Actual game perf/visual gate pending;
user authorized short Continue(catastrophe)->stereo+L2/R2 test. Reuse previous
VR64353686 unchanged; no new UAC/WPR. Steam not yet deployed at this header.
Accepted original injectorAAA07005 and VRD892 must both be restored after test.
Hands stretching still open. See docs/texture-native-cache-fix.md.

CPU ROOT-CAUSE PATH IDENTIFIED (not fixed): PID8804 native trace111319samples.
~64.78% of target-process weighted CPU samples are texture injector dinput8,
SHA AAA07005F4FF658C4A71CC30EA258E469DAF250C77BFE447011FFD650379E4F7,
not our d3d9 cinema compositor/old DirectInput enumeration. Exact top RVA24E5
is mip0 FNV hashing; RVA5670.. is its 64-bit multiply. Source/live DLL hashes
match. Injector log35.2MB:533291 candidate-hash lines,655 distinct hashes,
one texture repeated75841times. Fixed512 cache evicts first nonreplacement
slot repeatedly -> negative-cache churn is supported mechanism; runtime
occupancy/resource identities still need a bounded falsifier. No textures,
injector DLL or its INI changed. This is outside frozen cinema component;
request explicit scope to repair cache while retaining same DDS assets/lookup.
Hands stretch remains independent/open; current user INI ShowControllerArms1
was preserved, not silently changed. Full physical second-New-Game gate pending.
Game closed; accepted DLL D892/INI B3FE/UI5B exact restore PASS. No new test
launched. See docs/cutscene-second-entry-crash.md for measured path/limitations.

LATEST Steam PID8804 / camera-guard DLL64353686: OpenXR+cinema/native exit
confirmed; physical hands/performance still BROKEN. User reports stretched
hands, slowdown already at catastrophe, trigger influence unknown. Native
cinema90->4.2->2.2FPS; actualOFF->GameplayStereo, then~6FPS. After-exit
sampler45,015ms/265frames, idle227/LT26/RT12; no menu/cine contamination.
No claim that a normal second New Game or the guarded fault was exercised.
UAC explicitly authorized: WPR native CPU trace saved privately (2.5GB),
after actual cinema exit, PID/start-time qualified. Two failed recorder
attempts: sharing violation in ReadAllText; fixed live shared reader and
preserved error output, tested in Windows PowerShell5.1, source8bc527f.
ETL->private CSV export/target-PID analysis in progress; no FPS patch yet.
Game closed; guard3472 restored D892 and MAIN rehashed DLL/INI B3FE/UI5B.
No unattended candidate, no install/config/asset mutation beyond DLL lease.
The earlier CURRENT block below is historical, superseded by this paragraph.

CURRENT: CPU-profile candidate D430CE9F / source3bccd67 / Steam PID14600
failed second entry: Continue(catastrophe)->stereo gameplay with stretched
hands and worsening FPS->doubled flat menu->New Game access violation.
Exact dump+MAP+DLL prove FindCamera cached Class+0x34 read, RVA0x1F6B8,
not the earlier guarded pawn sample. Safe D892 independently restored;
user INI B3FEED43/UI5B retained/rehashed. Camera lookup guard L1_PASS:
valid/miss/nested lookup, NOACCESS/released object, unpublished fault result,
non-memory exception propagation. L2_PASS: sourcef57c3d5, x86 PE32/O2,
DLL64353686EDD457EDE84B0D8892B32B6EC9D108C78912AEF9A7F753CFA163DC15.
Six narrow test executables + runtime policy PASS; no new runtime pass.
Full existing read-only profile captured45s/3474frames, LT917/RT101; slow
CPU phase did occur90->49FPS and continued~40. Hook averages do not attribute
the native frame cost. Need native slow-window stack trace; Windows WPR
requires fresh UAC authorization. No speculative performance/hand patch.
Cinema60% native-camera head isolation and Polish intro preserved; helicopter
Polish speech still UNIMPLEMENTED. See docs/cutscene-second-entry-crash.md.
Recorder supports an opt-in actual cinema-exit marker gate, max180s wait then
90s native CPU trace, no recording during long intro/helicopter. Pending UAC
authorization/runtime execution. Candidate remains OFFLINE, safe install live.
Historical entries below are superseded by this header.

PID45372/7A0EC94D: cinema60% / native-camera head isolation and eventual
stereo return PHYSICALLY_VERIFIED. Start-of-game perf/hand tracking BROKEN:
user~20FPS crash/~40gameplay, L2~10, hands not matching pads. Native windows
6.7FPS; Present CPU saturated in read-only1s sample. Cinema composition CPU
~0.02ms, not GPU proof. INPUT_ENUM_SCAN native0/replayactive: old discovery
mechanism not established. Controller PlayerWalking occurs while CINE ON/OFF:
do not use that name alone for cinema detection. Actual second nativeOFF works.
Safe D892 and UI5B restored/rehashed. User panel saved ShowControllerArms0->1;
INI B3FEED43...2A079F preserved, NOT C59. Helicopter PL speech still unimplemented.

Next user-authorized read-only CPU profile: existing sampler + separate cached
callback replay timing, explicit owned after-cinema request, max45s/3600frames.
No request ->OFF. No image/control/hand/shader/INI/asset change. L1 in progress.
Camera/framing and Polish intro preserved. Need measured hot scope before fix;
Windows stack profiler is available but needs fresh explicit UAC authority.

READY_FOR_PHYSICAL_ACCEPTANCE (not L3/L4): native-camera candidate source
f83fd71513a7792f639acfbcf7c861dacdabc6d7, regular x86 PE32 /O2 DLL SHA
7A0EC94D22FDB1E27EFA5CA2469F28D1598126BFC5CF97004084B88AA02C1E65.
DLL-only installed/rehashed18:19:34, backup20261013-181931 contains exactly
one accepted DLL/zero directories; original DLL/INI/UI backups verified.
INI C59/UI5B unchanged. Guard29140/observer43780 READY; launch lease600s,
then post-exit restore. No game running at handoff. Await next user start.
Eight x86/O2 executables + policy/syntax PASS. Native exit/FPS are still an
OPEN physical gate: this candidate observes rather than invents exit criteria.

PID45080 / 8B34C584: Polish intro PHYSICALLY_VERIFIED. Cutscene camera/size,
exit to gameplay and late performance BROKEN (user confirms too wide,
head-dependent content, cinema/no hands after crash, several FPS). Native log
has OFF/OFF then ON/ON, NOT a never-received-OFF; ~90FPS helicopter followed by
16.3 then2.3FPS. `our work` residual includes game rendering; cause not isolated.
Guard restored D892, independent DLL/INI C59/UI5B hash verification PASS.

Next controlled experiment L1_PASS: native-camera CutsceneFlat (no HMD source
rotation/translation/roll, native projection/FOV retained), centered60% fit.
Gameplay/menu/accepted Bink50%, shadows/input and Polish intro unchanged.
Bounded composition timing and read-only actual controller state observation
are added, NOT a guessed exit override. MAIN metadata Steam boot on D892 proves
native GetStateName frame chain; menu state PlayerWalking. mCinematicData.mState
is weapon raise/lower, not generic cinema detection. Normal exit/no install
writes. Eight x86/O2 unit executables PASS; build/policy gate in progress.
Priority: cinema + start of gameplay; whole-game Polish audio next. Helicopter
speech still UNIMPLEMENTED, no global movie-track10 assumption.
See docs/CUTSCENE_RESUME_PHYSICAL.md.

PID5052/820E8263 correctly loaded new candidate (native Bink probes prove it),
but movie/loading camera/pawn transition wrongly cleared CINE: no cinematic
composition/framing/foreground execution. Intro selected IDs0/1/2/3, excluding
known Polish10. Safe D892 restored after normal exit; INI C59B1CF6/UI5B2EA469
verified unchanged. New controlled fix L1_PASS: preserve native cinematic ON
only across observed VideoFlat->world handoff, retain other level/menu clears;
select intro0/10/2/3 only for exact known basename/flags/layout, preserving
native default post-Open and failure rollback. Existing exports/camera hooks
only; bounded argument/state logs. Seven narrow executables and runtime policy
PASS, x86 /O2 build next. Actual whole-helicopter exit, frame/glove result and
Polish intro listening PENDING. Helicopter dialogue still not redirected from
native SoundNodeWave; next existing Localize diagnostics will cover CINE ON.
No movie/WAV/UI/INI file or gameplay shadow/input changes.

Handoff/intro candidate L2_PASS sourceed9b1bfacd58e520e8ad34d97b1c20f14ac68db5,
x86 PE32 /O2 SHA8B34C584B4FD9AF1774A8B2FDEDC4646746D33E1F055B68CA54E62C7E3070384.
DLL-only installed/rehashed; verified backup20261013-173236, guard7664 and
observer42544 READY with600s launch window. Intro content SHA AF53C8F8...CA9A3686
verified before applying the known-track policy; no movie rewritten. UI/INI
unchanged. Next gate: listen intro, full helicopter-end/gameplay/pause return;
no mid-headset chat. Scene handoff execution and Polish intro L3/L4 pending.

PID44632/DLL983BED0A: full-image mono copy L3_PASS and user reports good image
overall. Remaining cutscene framing/aspect/foreground polish: too wide,
edges outside view and tracked glove moves at periphery. Guard restored D892;
MAIN verified DLL/INI/UI unchanged. New candidate L1_PASS:85% centered fit,
full-viewport render tangent (actual one-draw viewport versus global half-width
dup setting), scoped tracked foreground hiding. Bink50%, menus, gameplay,
shadows/input unchanged. Synthetic GPU pixel/state/MRT rollback and narrow
regressions PASS. Runtime execution/physical framing and glove result pending.
Audio diagnosis: English Steam language versus independent PL mod flag; no
runtime package/asset voice redirect, only Localize exact text/40-char route.
Polish intro embedded track10 exists. New bounded read-only Bink exports and
voice-call observations separate selection from missing dialogue entry.
No audio track/file/config changes. See docs/CUTSCENE_RESUME_PHYSICAL.md.

Current candidate L2_PASS source22a68c7 / x86 PE32 /O2 SHA820E82639FA89FD13B6DFF7B0E6F15B15DF06FD0D66362123AFCE3A88A5F98FC.
Installed DLL-only, baseline backup20261013-164928 verified; restore guard and
read-only observer READY (600s launch window). UI/INI unchanged, all25 movie
hashes match known source inventory. Await user start; aspect/foreground and
native audio selection still require runtime evidence and PSVR2 feedback.

PID40412/DLL3242DB31: New Game no-crash PASS again; Bink50% size now
PHYSICALLY_VERIFIED / USER_ACCEPTED. Preserve size and pawn guard. Helicopter
BROKEN: different stretched eye halves and semi-transparent NPC face. Native
stage evidence proves first source->plain-scratch StretchRect fails with
D3DERR_INVALIDCALL, while zero-copy source->RT-texture succeeds. New controlled
candidate mirrors the working RT destination/null-rectangle copy primitive;
shared helper synthetic RGB/alpha + pixel rollback + viewport/scissor/RT
restoration PASS at 640x360 and actual5264x2684. L3/L4 still pending; no NPC
shader/camera/input/shadow/menu changes. Safe baseline D892 restored/rehashed,
INI C59B1CF6 and accepted UI5B2EA469 unchanged. Helicopter Polish voice is a
separate UE3 dialogue gap, not the SIDE Bink audio task.
See docs/CUTSCENE_RESUME_PHYSICAL.md.

RT candidate L2_PASS source468c74f / x86 PE32 /O2 DLL983BED0ADDFBEF64927800F0F25481D7DF35BF2E7E37E1A8BD7E1CC8E5B9F7F1.
DLL-only installed/verified; backup20261013-160811 and guard READY (600s launch
window), INI/UI unchanged. Await physical start; no game-runtime copy success
or helicopter visual acceptance claimed. All narrow regressions PASS.

PID4672/4483CF02: New Game crash reproduction PASS in Steam/PSVR2; guarded
stale-pawn read rejection logged. Helicopter still BROKEN: CINE ON but no mono
composition success. Safe D892 baseline restored, INI/UI unchanged.
Next candidate: requested Bink50% width/height + max20 mono gate/stage HRESULT
notes only. GPU reproduction falsified surface-type-alone hypothesis.
L1_PASS placement/mono/scene/pawn tests and runtime policy. L2_PASS source4ba2174,
x86 PE32 /O2 DLL3242DB3167CF0A33476CD565EA5D6A6D70303DD60E532B1D76900C14D98CED4D.
Installed DLL-only; verified guard/backup20261013-152140, 600s launch window.
Helicopter fix NOT claimed; runtime stage diagnosis and video size L4 pending.
Dubbing audit: installed PL manifests/WAVs and Localize hooks present; movie
Bink audio redirection absent. Details: docs/CUTSCENE_RESUME_PHYSICAL.md.

MAIN cutscene resume, PID41200: New Game crashed before intro; WER/dump and
exact candidate MAP/disassembly prove pawn Location.Z read in camera
stabilization (RVA0x2898C, stale pawn0x0DA0A000), not mono composition. Safe
baseline DLL D8928180 restored and rehashed; INI C59B1CF6 and UI5B2EA469 unchanged.
Small guarded pawn-sample fix L1_PASS; no camera-math/render/input changes.
L2_PASS: normal x86 PE32 /O2 source f69caf3, DLL4483CF02D86D6EB466CB9A2A924B4401329BD0345544BD5BA2254814E305A921.
Candidate installed DLL-only, INI/UI retained; verified baseline restore guard,
600s launch window. Crash regression subsequently passed in PID4672; helicopter remains broken.
See docs/CUTSCENE_RESUME_PHYSICAL.md. Prior active observer expired before the
late crash; authoritative native log/dump recovered, no live-coverage claim.

CUTSCENE MONO (branch fix/cutscene-mono-presentation, 08d7ed2):
IMPLEMENTED_NOT_VERIFIED. CutsceneFlat composes the full SBS cinematic
aspect-fit into the left half and submits that same crop to both eyes;
VideoFlat/menu paths untouched, CutsceneFlatMode switch preserved.
Offline: test-cutscene-mono PASS, test-vr-scene-mode PASS, runtime-policy
PASS, x86 PE32 /O2. L3/L4 pending one coordinated PSVR2 window against the
separately hashed merged baseline D8928180. See docs/CUTSCENE_MONO_PAIR.md.

Main PR preparation: persistent-default release build L2_PASS, regular x86 PE32
/O2 source1b4e0b6 SHA E9DC831B599734CD3EC6CB8A16C907DE26F7D9EB8CEC29E95A5DD2C104D8116E.
9 C++ unit executables, Python trace/CPU regressions and default-policy checks
pass. Same physically accepted algorithms; INI default glue is not a separate
Steam/PSVR2 runtime claim. No CI workflow configured. User approves/merges PR;
MAIN does not merge. No installed files changed during PR preparation.

SHADOW FIXES PHYSICALLY_VERIFIED / USER_ACCEPTED: PID39308, DLL5D49C7A5/source
3488084. User says everything good, shadows finished, requests main PR for
their approval/merge. World+ClearGate+near1 receiver+input cache ON. Trace1196
draws/304groups/overflow0, near1 final constants correct in both eyes, no restore
faults. Earlier black gun/background-negative and enemy-shadow passes retained.
Accepted scene only; not every weapon/level/headset/hotplug. MAIN verified safe
rollback DLL9FBA308C/currentINI C59B1CF6/UI5B2EA469; all test flags gone.
Preparing persistent INI defaults (same accepted logical feature state, missing
keys ON) and main-target PR; no main merge or new Steam deployment authorized
as part of this PR preparation. See docs/accepted-shadow-input-fixes.md.

Near1-only receiver L1/L2_PASS: source3488084/DLL5D49C7A5, x86 PE32 /O2,
captured-matrix/nested-world-scope/restoration tests and parsers passed. Separate
default-OFF FirstPerson flag requires existing WorldReceiver+ClearGate ON;
world kernel and hand/gun transform code untouched. User authorized next
physical no-tab test; runtime/eye-shadow visual result not yet established.

Combination PHYSICALLY_VERIFIED for gun material/tracking, background-negative
removal and retained enemy shadows: user's PID17212 result. Remaining BROKEN:
shadow on gun/glove differs between eyes. Preserve world inverse + ClearGate +
input cache; near1 receiver291/292 still uses identical native inverse with
different actual eye VP. Caster-origin mathematical errors61/97UU, corrected
oracle maps both eyes to same UV; depth-pixel assumption still unvalidated.
Next separate default-OFF near1-only receiver experiment; no hand transform
or material edits. See first-person receiver note for falsification/gates.

Combination L3_PASS for execution/classification (physical result above):
SteamPID17212/DLL7A7787EB, only ClearGateOFF->ON relative to successful enemy
test; WorldReceiver/input cacheON and rejectedReceiverOFF. One pause-return
frame1283draws/320groups/overflow0: same gun5799 early/late depth/color moved1,
late fgCount1/4 versus previous101/104. World receivers210/211 still corrected,
near1 receivers291/292 excluded; light chain retained, no restore faults.
Game exited, accepted DLL9FBA308C/currentINI C59B1CF6/
UI5B2EA469 independently verified; owned flags gone, gloves-only state retained.

World-only receiver PHYSICALLY_VERIFIED for dynamic enemy shadows in tested
scene: user reports enemy shadows now work and are dynamic. Steam PID37880,
DLL7A7787EB/source532cf44, WorldReceiver/input cacheON; ClearGate/rejected
ReceiverOFF. Final trace1342draws/314groups/overflow0 confirms corrected world
inverse for both eyes, preserved light chain and untouched near1 tail. Full
shadow/weapon feature remains BROKEN: black/frozen gun and controller-following
background-negative remain. Exact gun5799 early moved1, late depth fgCount101
and color104 moved0; premature foreground window persists. Next controlled
combination retains working world inverse and enables only existing ClearGate,
whose mechanism was separately demonstrated. No near1 shader/hand edits.
Game exited; MAIN verified restored DLL9FBA308C/currentINI C59B1CF6/UI5B2EA469.

Live-enemy probe L3_PASS, physical square observed: Steam PID35208/DLL54CC148F,
1000 calls/266 groups/overflow0. Dynamic6481/4307 mesh produces the map consumed
by world receivers186/187. Native light-map chain agrees within0.000114, but
actual per-eye VS camera and identical native PS inverse disagree: mathematical
caster-origin reconstruction error523/555UU; assumed-depth sample leaves map.
Pixel-depth assumption remains unvalidated. See world-shadow experiment note.
Separate default-OFF world-only inverse experiment L1_PASS (synthetic roundtrip,
depth/resource gates, COM/constant restoration/failure latch), L2_PASS x86 PE32
/O2 source532cf44/DLL7A7787EB (manifest/hash match); runtime and
physical acceptance NOT verified. First-person near1 route excluded; rejected
mixed receiver and ClearGate remainOFF. No hands/weapon transform changes.
After exit MAIN verified accepted DLL9FBA308C/UI5B2EA469. User asked about visible
arms: saved ShowControllerArms was1 from prior VR Settings. MAIN restored only
that line to0 while game closed; currentINI C59B1CF6, other settings preserved.

Producer-matrix probe L3_PASS in real Steam PID12648, DLL54CC148F/source
dbce0ed: single focused frame2304 calls/351 groups/overflow0. R32F map readback
and VS c230..240 reads succeeded. IMPORTANT ROLE CORRECTION: runtime mesh latch
identifies gun5799/arms3314 in PID12648 and18352. Previous notes calling the
3314 shadow caster "gun" were too strong. Exact first-person VB/IB linkage is
valid, but the shadow-producing3314 mesh has the arm role;5799 gun scene draws
are moved1 while3314 scene/caster draws are moved0 in this frame.
Physical user: initially holster would not toggle; later it did, controller-
following shadows disappeared, enemy squares persisted, gun was black and
scene appeared darker. User changed ShowControllerArms/RecenterChord only
AFTER black gun appeared. Captured map is the first-person arm map, not a
confirmed active-enemy map. No three-pose capture and no visual fix PASS.
Guard restored verified DLL9FBA308C and UI5B2EA469. User-modified INI preserved
and hashed B3FEED43 (ShowControllerArms1/RecenterChord1 plus saved defaults).
No experiment flags/request remain. New external PauseReturn mode schedules
one frame after actual SPPause->gameplay log transition, avoiding the holster
dependency and tab switching for next live-enemy measurement.
User clarification: captured enemy was dead; squares also appear under live
moving enemies and sometimes intersect them. Current trace's two audited
shadow receivers both sample the arm map; no other R32F sample is present.
Actual producer for enemy-square pixels remains unproven. Next capture live
enemy before firing, PauseReturn+1second, same validated read-only DLL54CC148F.

Shadow-map content probe L3_PASS for captured resource consumption: SteamPID18352,
4EADB827,1769calls/370groups/overflow0,receiver exactVS/PS match,512R32F readback
HRESULT0 with262144finite pixels,range0..~e. Encoded/filter shadow data, not raw
normalizedZ. Shadows remain BROKEN. User clarification: the VR holster action
leaves the observed shadow at its prior position; moving the head changes the
viewpoint around that shadow, not its position. Square shadows are seen around
enemies (dynamic casters). Next armed-hand-A/B/VR-holstered controlled traces;
do not re-enable rejected matrix correction. Accepted3 hashes
restored/verified DLL9FBA308C/INI61AEC56C/UI5B2EA469 after exit. The later
PID35192 A frame reidentified an exact foreground first-person VB/IB in native
512x512 R32F shadow-target draws (3314-vertex mesh, 390-vertex companion), also
present in PID18352. This is direct caster-path evidence, not artifact causality.
The no-tab-switch PID27084 sequence captured A/B at 931/919 draws, 0 overflow;
same first-person VB/IB occurs in both native R32F shadow targets. CTAB identifies
BoneMatrices starting at VS c5; its first rows differ between hand poses A/B.
Physical user: enemy squares did not change with hand motion; the gun-associated
shadow froze after VR holstering. C was not captured because the scheduler's
40-second STORED timeout elapsed just before the event and the game closed.
Accepted DLL/INI/UI hashes were separately reverified after guard rollback.
Next read-only candidate captures CTAB-backed VS c230..240 on native R32F
producer groups to separate object and projection transforms. L2_PASS:
dbce0ed x86 PE32 /O2 build SHA54CC148F, parser/trace unit checks and runtime
policy pass. Real Steam execution is now recorded above; no
shadow fix claimed. See map-probe note.
The first sampled A/B map comes from a separate 6481/4307-vertex dynamic-mesh
producer also seen in HDR scene draws; its 394x224/395x225 native viewports
and finite content are distinct from the later gun shadow route.

Input cache130dfdb / DLLF294D272: real Steam PID19520, PHYSICALLY_VERIFIED
for held-button FPS/actions/stereo/tracking and pause/options/resume. User says
everything retained and no FPS drops. Topology reconnect not tested. Profiler
ended before3600 completion; final retained partial3360 has2 timed full scans
mean75.942ms, not a complete-window claim. Zero-scan gameplay intervals replay
native callbacks with~90FPS. Game exited; guard000916 restored DLL9FBA308C;
MAIN verified actualDLL/UI5B2EA469/currentINI61AEC56C (user's change preserved).
Return to square shadows; rejected receiver correction staysOFF.

Next shadow candidate adds default-OFF one-frame R32F512 content readback at
actual volume consumption (max4 maps,1MiB staging, no render-state changes).
See docs/shadow-map-content-probe.md. Input cache retained in source; runtime
content confirmation needed before any further shader/matrix modification.

Event-invalidated input discovery cache: IMPLEMENTED_NOT_VERIFIED. Full native
callback audit led to caching device INSTANCE records and replaying original
callbacks, not suppressing all callbacks or moving DirectInput to a worker.
DefaultOFF v3 flag; Windows topology epoch/interface/context/failure invalidation;
bounded64-record snapshot, fail-open reentrancy and unavailable notifications.
See docs/input-enumeration-cache.md. Requires runtime/callback-content and
physical FPS/actions/pause/reconnect gates before production acceptance.

Input 2000ms interval: DLL F1AB2981 / commit9f1e67e, real Steam PID38864,
L3_PASS for limiter behavior and PHYSICALLY_VERIFIED for partial FPS benefit
with tested actions/stereo/weapon tracking retained. User: "tak działa, spadek
jest do85fps". Not a complete performance fix or hotplug/lifecycle acceptance.
3600-frame profile:17 full-rate timed scans, mean76.119ms. Game exited; guard
removed flag and restored DLL9FBA308C. MAIN verified DLL/UI5B2EA469/INI568E2BB5.
Next: understand native scan/callback side effects and safe invalidation before
replacing periodic scan with a production cache; never move DirectInput callbacks
to a worker speculatively. Shadow squares remain BROKEN.

Input enumeration: Steam PID38556 / DLL C24B3BB7 same-session OFF/500ms ON
is L3_PASS for scan suppression and PHYSICALLY_VERIFIED for partial FPS benefit
and retained tested actions/stereo/weapon tracking. User reports ~75–80FPS,
not a complete fix. Scans fell from13–15/s to1–2/s. Next default-OFF 2000ms
candidate is IMPLEMENTED_NOT_VERIFIED. Shadow squares remain BROKEN.
After game exit accepted DLL9FBA308C and UI5B2EA469 verified restored/retained;
user INI568E2BB5 retained (LaserSight changed), not overwritten with backup.
See docs/shadow-clear-gate-experiment.md for timing limitations and next gate.

Resumed shadow task: real Steam/OpenXR capture PID5260 on 6D147048... completed
(1329 calls / 322 groups / 11 clears / overflow0). Accepted3 hashes restored
and verified after exit. Pixel/shadow fix remains BROKEN, not visually accepted.
Next default-OFF experiment excludes only offscreen R32F depth clears from
foreground classification; see `docs/shadow-clear-gate-experiment.md`.
No new shader constant edits; native Clear and hand/weapon transform code intact.

2026-10-12 shadow session **paused at user's request**. Game closed; guard
restored accepted DLL 9FBA308C...99C4500 from backup 20261012-204256.
INI 61AEC56C...FE378B and UI 5B2EA469...673CDE verified unchanged;
experimental flag and pending trace request absent. A/B evidence retained privately.
Next candidate ba628ae / DLL 6D147048...93996A is x86 /O2, tests passed,
but NOT runtime-tested: resume with one focused HQ Decals ON capture of
depth/alpha write masks, stencil coverage, native GPU copies and depth clears.
Do not enable the rejected receiver correction. See `docs/shadow-receiver-correction.md`.

Shadow receiver experiment on `fix/stereo-shadow-rootcause`: **BROKEN**.
ON/OFF/ON final-driver traces complete; user confirms OFF removes ordinary
shadows as well. Exact receiver inverse/forward mismatch identified; bounded
default-OFF c3..10 correction implemented. Synthetic math and state restoration
tests pass, but user physically rejects ED15225F: distorted rendering and shadows
apparently following right controller. Experiment disabled live; read-only
first/last/context diagnostic being prepared. See `docs/shadow-receiver-correction.md`.
Accepted Steam restore remains DLL 9FBA308C..., INI 61AEC56C..., UI 5B2EA469...;
all hashes reverified after diagnostic PID9576 exit.

Last updated **2026-10-12**. Left glove idle: **PHYSICALLY_VERIFIED** (`010eb45`).
Boot/main placement follow-up on `fix/boot-main-menu-placement`:
**PHYSICALLY_VERIFIED** for placement: user accepted 5f05687 / DLL
9FBA308C...99C4500 ("jest dobrze"). Boot safe-area cap 0.70 -> 0.45; main plane
8% eye-height down with matching laser/native HUD fallback. Accepted pause
geometry and native RTT package unchanged. See `docs/boot-main-menu-placement.md`.
Clean-install native UI delivery remains unimplemented; tracked in `docs/BACKLOG.md`.
Pause RTT layout on `fix/pause-native-rtt-integration`: **PHYSICALLY_VERIFIED**
for appearance/placement only. User's first PSVR2 test on `eac307b` / DLL
1635A8F0...D625219: "looks perfect". Complete native RGB/full UVs, compact 50%
plane and matching pointer geometry. Navigation/Options acceptance for this
exact version remains unconfirmed. User authorizes persistent installation of
this exact tested DLL plus dedicated UI package (no restore watchdog) and PR
to main, including inherited baseline changes absent from main.
Return-to-main per-eye background difference remains UNKNOWN; no stereo changes.
See `docs/pause-rtt-integration.md`. This branch's verified safe restore is
original UI C8CDB2E1..., accepted DLL 2FE5011C..., preserved INI 61AEC56C...
Older Steam installation statements below are not this task's current state.
PSVR2 on `318b1fd`: support grip no longer hides the left glove, but it sat
**beside** the pistol because the left target was `BuildGunC` while the right
target stayed the aim controller. Pistol latch now uses `AuthoredPair`: both
regions take the aim-hand rigid delta, and `GloveLeftShift` is not applied
while that latch is held. `l_hand_IK` is the rest pose, not a grip point.
Historical pre-RTT pause evidence: see `docs/audits/hands-only-and-pause.md`;
it does not describe the physically accepted native RTT provider above.

**Steam install (MAIN):** `4F85BEAAFC7033287B900A1750203375FDEE64E9FA817EDA2E9CB6F8EA2BECBE`
(build after `94b47e8` + compile fix), backup `20261012-113123`, INI preserved.
L3 smoke: `OpenXR ready` in `view_matrix.log` (2026-10-12 11:32:54 attach); no
`support wrist snap` in this build. Support grip on headset: **READY_FOR_PHYSICAL_ACCEPTANCE**
(not re-tested on PSVR2 yet). Prior: `0eb22a6` / `2B896A7A…`, backup `20261012-111934`. Workflow:
`docs/STEAM_MAIN_DEPLOY_WORKFLOW.md`.

Previous handoff **2026-08-16** (runs 249–271: panel, splash regression, weapon swap, popup text, scope).
Read this first, then `ENGINE_NOTES.md`.

**This file is current state only.** The run-by-run log moved to `HISTORY.md` on 2026-08-04 — you
do not need it cold, but go there before building on any premise you inherited.
It also flags, with ⛔ banners, the status claims that had gone stale in place.

> ### ⭐ Run 180: UE3 yaw deltas were never wrapped, and it cost months of misattribution
>
> `camYaw - ctlYaw` on `int32_t` does **not** wrap. UE3 is 65536 units per turn, so two headings
> **0.3° apart** differ by 65590 units and read as **360.3°**. Two sites did this and both carried
> a comment saying `// wraps correctly`.
>
> Measured in one 8,476-line run: **20** false `camera handed to the GAME (360.3 deg from the
> pawn)` events, each re-anchoring `g_baseYaw` to the camera and yanking the view. Reported for
> months as phantom snap turns — there were no snap turns; the run was `TurnMode=2` and fired
> **zero**. After the fix, same conditions under heavy turning: **0 handoffs**, and the drift log
> reports `-5.0 / +89.5` instead of `±360`.
>
> **Cast the difference to `int16_t`** — 65536 units *is* 2¹⁶, so truncation gives the shortest
> signed path for free. Use `YawDelta()` / `YawDeltaDeg()`; a raw subtraction is only safe if both
> sides are provably normalised, and nothing here normalises them.
>
> ⚠️ **Several other sites still subtract yaws raw**, including one at the FRotator write path
> carrying the same `int32 wrap is correct` claim. No measured evidence yet. Audit them next.

> # ✅ The ladder is complete. Every rung, including controller input.
>
> **Full resolution parity at a locked 120 fps, inherited automatically from the headset.**
> Launch `Singularity.exe` with no arguments. That is the whole procedure — it **starts in VR**
> (`StartInVr=1`, the default since run 159). No keypress, no command line.
>
> ```
> auto-resolution: injecting -ResX=4992 -ResY=2688 -windowed (cached from the headset last run)
>     requested 4992x2688 -> got 4992x2688   *** ACCEPTED ***
>     per eye 2496x2688 against the headset's 2496x2688 -> 100% horizontal, 100% vertical
>
> fps median 119.9   our work 3.02 ms   xrWaitFrame 4.35 ms IDLE   XR submit 0.94 ms
> ```

## TL;DR — where we stand

A **working, wearable VR mod** for Singularity (2010, UE3, D3D9, x86), shipping as one `d3d9.dll`
dropped beside the game exe. No game files modified.

| rung | state |
|---|---|
| Head-tracked image, sRGB colours | ✅ |
| **6-DOF positional tracking** | ✅ camera position solved via the view matrix |
| **Head roll** | ✅ run 59 — applied in the view matrix, `NUMPAD1`/`NUMPAD2` |
| **True stereo**, real per-eye parallax | ✅ draw-call duplication; eyes align, depth correct |
| **FOV / aspect / resolution** | ✅ **100% of the headset's pixels, automatic** |
| **Performance** | ✅ **120 fps locked**, 4.35 ms idle per frame |
| **Controller input** | ✅ **working** — Touch and Valve Index mappings, menus, haptics, snap turn, off-hand movement |
| **Aim decoupling** | ✅ **SOLVED via route 2** — the bullet follows the controller, the view never moves, and it costs NOTHING in culling or LOD. Aim mode 11 |
| **Gun follows the controller** | ✅ **working** — 6-DOF, arms hidden, anchor tuned. APPS toggles it |
| **Cutscene head tracking** | ✅ **working, automatic** — run 138. `SetCinematicMode` drives it; look around freely during a cutscene, normal turning restored on exit |
| **`D3D9ExMode=1` texture corruption** | ✅ **fixed** — run 139. Zero-copy is usable again; visuals match `D3D9ExMode=0` |
| **Smooth turning** | ✅ run 140 — `TurnMode=2`, degrees per second, works during cutscenes |
| **VR settings panel** | ✅ run 142 — hold Y **1 s** anywhere (1.3 s until run 219); live changes, saved to the ini on close. ⚠️ **No page may exceed `kPanelRowsMax` (10) rows** — run 249 |
| **The game's HUD and menus in both eyes** | ✅ run 151 — health, ammo, crosshair, prompts, pause menu. `HudStereo=1` |
| **The sniper scope in both eyes** | ✅ run 213 — whole scope per eye, world once, circle round. `ScopeQuadPerEye=1` |
| **Shipping defaults = the tested configuration** | ✅ 2026-08-08 — a fresh install with no ini now behaves like the tuned dev machine. See below |

### ⭐ The code defaults ARE the tested configuration (2026-08-08)

Every validated run in this project's history ran against the dev machine's `SingularityVR.ini`.
Nothing had ever run on the **code defaults** — the configuration a new user actually gets. A second
install exposed the gap: with no ini, the gun swivelled about the wrong point, the debug readout was
drawn, and the D3D9Ex wrapper was off so the frame took the ~9.8 ms CPU copy.

All 48 keys were diffed mechanically against the source. These defaults were promoted to the tested
values, and `SingularityVR.ini.example` — which shipped `D3D9ExMode=0` and `Debug=1` **uncommented**,
overriding whatever the code said — was corrected to match:

`GunAnchorFwd/Right/Up` 30/9/−13 · `GunSignPitch` 1 · `GunSignCombo` 2 · `D3D9ExMode` 1 ·
`Debug` 0 · `PoseLeadFrames` 1 · `AimMethod` 1 · `AimFieldSet` 1 · `TurnMode` 2

**`OcclusionQueryMode=2` is now the tested value, and `SingularityVR.ini.example` was still shipping
`0`.** Mode 0 forces every query visible and costs roughly twice the draws (1500–1693 vs 737–943 per
frame). Mode 2 was carried for a long time as "never played on for a full session" — it since has
been, across many, and run 215 cleared it of the vanishing-floor report outright (that was the mesh
matchers, not culling). The example ini has been corrected to match the code.
⚠ Its residual risk is still **geometry winking out** at distance or screen edges rather than frame
rate, so `OcclusionQueryMode=0` remains the first suspect if that ever appears.

⚠️ **`AutoResX`/`AutoResY` must never be shipped or hand-edited.** They are the mod's own cache of
*this* headset's size. A shipped value injects one machine's resolution into everyone's first launch.

**Two method notes this cost.** A comment describing a defect fixed a day later was quoted as current
a week on, which is why the wrapper default stayed wrong — `HISTORY.md` now carries a ⛔ banner there.
And the `GunSignPitch`/`GunSignCombo` pair was left unmatched on the argument that `GunSigns()`
resolves −1/combo 0 and 1/combo 2 to identical signs. That reading may be correct; shipping on it was
not. Every working run was on 1/2 and every broken one on −1/0, and a code argument was used to
discount a perfect empirical correlation. **Matching a tested value costs nothing when you are right.**

### If you are picking this up cold, read these three things

1. **Almost every hard problem here was a stale premise, not a hard problem.** Parity was recorded
   as "arithmetically unreachable" for a dozen runs because a 2010-era cap was measured once on a
   device that later changed. 120 fps was blocked by a wireless link nobody had timed. The frame
   rate was wrong for thirty runs because SSW was halving it. **Re-test the premise before building
   on it**, especially one you inherited from an earlier run.
2. **"our work" in the frame budget is not a measurement.** It is the residual — everything not yet
   broken out — and it will absorb any amount of someone else's latency while looking exactly like
   your own bug. `xrEndFrame` hid a **189 ms** stall in there for an entire day.
3. **The cheap control first.** Rename `d3d9.dll` to `d3d9.dll.off` and run the game unmodded at the
   same resolution. One minute, and it settles "is this us at all" before any bisection starts.

### ⚠️ SHIPPED, NOT YET FLOWN (run 274) — Valve Index bindings, and the trigger veto was dead on non-Touch hardware

A tester's log (Pimax Dream + Index controllers) read `trigger touch gate ON: LEFT NO SENSOR
(values pass through unchanged), RIGHT NO SENSOR — vetoed 0/120` in **every one of its 289
windows**. The mod suggested bindings for `oculus/touch_controller` and nothing else, so every
non-Touch runtime got its own compatibility remap. `grasp` survived that remap because it is bound
to twelve surfaces; `trigtouch` is bound to exactly two on purpose and got neither. **The run-156
veto — the fix for the original framerate tank — had been switching itself off about 50 seconds
into every session on that hardware** (`kTrigGateGrace`), and the log line for it reads like a
benign capability report rather than a disabled safety net.

Shipped:

- **`valve/index_controller` bindings**, suggested in their own call. The suggest call is
  all-or-nothing *per profile*, so a rejected profile now costs only itself — Touch still stands
  and the runtime falls back to exactly the old behaviour. Every path checked against the Khronos
  spec source, not from memory; run 172 lost 31 bindings to one bad string.
- **`use` and `impulse` move to `squeeze/force` on Index.** On Touch a single `squeeze/value` feeds
  both `grasp` (wants to be sensitive — "is anyone holding this") and the two grip buttons (want to
  be deliberate — "did they press it"). Index splits them. This is the **suspected cause of the
  reported lag**: tightening your hand to reach a face button raises `squeeze/value`, the runtime
  thresholds it true, and X / RSHOULDER latch on — Use/reload and the impulse weapon held down for
  as long as you are gripping.
- **`x`/`y` → `a`/`b`** (Index has a/b on both hands, no x/y), **menu → `trackpad/force`** (Index
  has no menu button; `system/click` is reserved by SteamVR). The run-170 left-handed mirror runs
  first in Touch vocabulary and composes with the translation to give a plain hand swap.
- **The interaction profile is now named in the log**, per hand, on change. The tester log carried
  no headset, runtime or controller identification in 27,000 lines. ⚠️ Not the runs 178–179
  experiment — that tried to use this call to *detect* the hand-tracking hand-over and cannot.
  Nothing branches on it.
- **A per-button held-frame census.** The two triggers have had a detailed instrument since run
  156; the nine buttons had none at all, which is why the lag report could be narrowed to "input
  activity, not the stick, not the draw count" and no further.
- **`binds[]` was two entries too short** and had been since run 180 — 33 emissions into `binds[31]`,
  written past the end of the array on every successful startup. Now spelled one term per emission
  site and bounds-checked.

**What would fool this test:** the tester reporting "better" tells us nothing on its own, because
two things changed. Read the log instead — `trigger touch gate` should now name a real sensor
state, and `buttons held` should show `use`/`impulse` at low counts rather than whole windows. If
the gate still says `NO SENSOR`, the Index profile was rejected and the `xrinput:` line at startup
says so. **Nobody here can test this**; it is built, not flown.

### ⚠️ SHIPPED, NOT YET FLOWN (run 250) — the triangle window had the run-244 gate defect too

Reported as *"the triangle range selector is only the hand, not the arms"*, while wanting to check
the triangles of mesh **4818**. Two independent reasons it could not do either, and the first is a
defect this feature has now had **three times**:

- **It was gated on `g_thisDrawMoved == 2`** — draws riding the off-hand controller. That is the
  exact gate run 244 removed from the **bone collapse**, for the exact reason it is wrong here: the
  left arm is drawn by something `FgRidesGun` rejects, so the draw the window most needs never
  reached it. Both tools now share one hoisted `tmdSurgeryOn` test (`TmdOnOffHand && TmdRaised() &&
  FgMatchWindow()`), so the next widening cannot fix one and leave the other behind.
- **Its mesh came from the BONE slot** (run 246b, deliberately). Sound while the two were one page
  and one job; it meant that pointing the window at 4818 silently retargeted the bone sweep too.

The window now has **its own four slots** of `{mesh, from, to}` and its own `TRI MESH` row, mirroring
the bone slots. Panel → `TMD ALIGN` → `TRIANGLE WINDOW`: `TRI SLOT`, `TRI MESH`, `TRI FROM`,
`TRI TO`, `TRI STEP`, `HIDE MESH`. Saved per slot as `TmdArmMesh%d` / `TmdArmFrom%d` / `TmdArmTo%d`;
slot 0 inherits the old single `TmdArmFrom`/`TmdArmTo` so an existing ini keeps its parked window.

⛔ **The "one contiguous window per mesh" limit is GONE (run 251)** — see below. Run 250 shipped it
with a comment explaining why two windows would need two draw calls, and the first sweep with it
found the case that needs exactly that.

### ✅ FIXED and VERIFIED (run 258) — the mesh latch drops on the SWAP, not 240 frames later

Reported as *"switch weapons, see nothing for a while, then the gun appears with the arms and snaps
to the controller"* — and it was **270 frames, not a 2 s timer**: `kLatchMiss` 240 + `kLatchSteady`
30, so 2.25 s at 120 fps and **longer whenever the frame rate is lower**. The "nothing" phase is the
new gun being hidden as a stray, because it is outside the *old* baseline.

`RefreshArmedState` already read `Pawn.Weapon` **every frame** and collapsed it to a bool. The
pointer is the weapon's identity, and it was already measured twice: run 235b named the object as
the identity, run 235c watched it move across `ARMachineGun → ARShotgun → ARMachineGun` and **not**
move for the TMD (which is *hidden*, not swapped — exactly the false positive a draw-derived signal
would trip on). The latch now drops on the frame the pointer changes.

**Verified:** 10 × `Pawn.Weapon CHANGED` against 1 × the old frame-count path, and user-confirmed in
play. It is a **pointer compare** — no name, class or vertex count — so it is weapon-agnostic and
needs no per-weapon entry the way `GunAlign` does.

⚠️ **The one fallback was the FIRST swap of the session**, because `Pawn.Weapon`'s offset had not
resolved yet (`ResolveGameplayOffsets` is throttled to once per 2 s). `kLatchMiss` stays as the
fallback for exactly that. Fixable by resolving that offset sooner if one slow first swap matters.

⚠️ **`RvWeaponShared` is NOT shared by every weapon** — the shotgun is `RvWeaponShared_Shotgun`. That
claim was measured wrong once, was still in a comment *and in a log line*, and got repeated into the
run-258 comment before being corrected. **Classes vary and can also be shared; the object pointer is
the only key with neither failure mode.**

### ⛔ MEASURED DEAD and REVERTED (runs 272–273) — occlusion queries cannot fix cutscene culling

Forcing `DRAW ALL` for the duration of a cutscene **engaged** — the log shows two `STARTED`/`ENDED`
pairs — **and the geometry was still missing.** Reverted rather than kept as a setting that does
nothing. ⚠️ **The reverted experiment is the valuable part**, because it retires an entire lever.

**The mechanism, from the same log:**

```
worst engine VERTICAL cull this window, from mCurrentPOV: 38.3 deg vs 98.0 deg rendered -> SHORTFALL, geometry culled
```

The engine believes its vertical FOV is **38.3°** while we render **98°**, so it discards everything
outside that cone **on the CPU, before a draw call is issued**. An occlusion query cannot rescue
geometry that was never submitted — the query hook is downstream of the decision.

⚠️ **Cutscenes are the worst case, which is why it shows there.** Gameplay windows read 86–98° against
98° rendered; cutscene windows read **38.3°** and **43.5°**. The engine sets a narrow cinematic FOV
and our ask does not survive it.

**▶ If picked up again, the lever is the FOV CULLING HEADROOM, not the query hook.** `ApplyVrFov`
already asks the engine for a wider FOV than we render (`kFovHeadroom`, cycled on PAGE UP) precisely
to give its culling room — `asked the engine for 127.9 deg (culling headroom x1.00)`. The question is
**why that ask does not hold during a cutscene**, and that is a different subsystem.

⚠️ **The cutscene edge line was KEPT** (`cutscene STARTED / ENDED`). It is what proved the override
ran and still changed nothing — without it this would have been another "maybe it didn't fire" — and
it will do the same job for whatever is tried against cutscene culling next.

### ✅ FIXED and VERIFIED (run 271) — the sniper scope was OFF BY CONFIGURATION, since run 231

Reported as *"the rifle is still broken"*. It was not a regression: **`ScopeQuadPerEye=0`** had been
in the ini since run 231, and that key's own comment states the cost — *"the sniper scope goes back
to being drawn once across the whole frame."*

**Why it was turned off, and the fix.** The matcher is a pure **shape** test — prim type, prim count,
`dupDraws`, `rtIsScene` — with **no aim gate**, so it fires on ordinary post-process quads:
`scope quad matches: 0 while AIMING, 120 while NOT aiming`, 172 times in one run. Every false match
sets `g_scopeUpMs`, and `ScopeActive()` gates **four** things — the mesh latch, the laser, the ADS
pull-back and the ADS mask — which is run 231's *"my gun was not attached to the controller"*.

⭐ **The fix was already written as a comment directly above the code that ignored it:** *"the real
scope quad can only be drawn while you are AIMING. A match with the aim trigger released is a false
positive with no other possible reading."* The trigger was read, counted, and never used as the gate.
It is now, and `ScopeQuadPerEye` is back to **1**.

⚠️ **The ACTION is gated, not the shape test** — the dump and both counters still run, because run
231's note is explicit that turning the bug off must not turn off the instrument that identifies it.
**Verification is `0 while AIMING, 0 while NOT aiming` in ordinary play**, with a non-zero AIMING
count only while scoped.

⛔ **`SpriteQuadPerEye` is REJECTED and defaults 0.** Shipped in run 262 on run 192's extent theory;
its own counter proved it engaged on 104 draws/sec and the artefact it targeted did not change. It
also splits the scope overlay (a centred quad that does not span NDC) while doing **none** of the
other two things run 213 proved are required — `c1 ScreenPositionScaleBias` and the halved aspect.
⚠️ It was wrongly blamed for the rifle first; disabling it did **not** restore the scope.

### ✅ FIXED and VERIFIED (runs 253–269) — popup/note text drawn once across the seam

**`HudArmSticky=1`.** Long popup and note body text now lands correctly in both eyes.

**The bug.** `g_hudConstsWritten` is consumed by the **first** user-pointer draw after a `c5`–`c8`
write, but one UI transform serves a whole batch. A glyph run that **fits in one draw** was armed and
correct; one that **spilled into two** had its remainder go to `StereoPair` unremapped — drawn at
full-frame coordinates, each eye's scissor keeping half. It also missed the note inset, so the stray
text was *larger*. Both symptoms, one flag.

⭐ **The reported length dependence is what solved it** — *"if the menu text is short we get the whole
thing; if it's longer it looks like this."* A length threshold is a **batch boundary**, not a
transform difference, which is why every transform-keyed remedy missed.

**Two changes make it work, and the second is what four earlier attempts lacked:**

- The continuation must match the **stride** of the draw that consumed the arm.
- **The arm is narrowed by SHAPE**: `startReg >= 4 && startReg + count <= 16`. Measured — it **arms**
  on writes of 1, 3, 4, 6, 9 vec4s at `c5`/`c8` (targeted UI transforms) and **rejects** 18, 24, 33,
  45, 54, 57, 63, 69, 90, 105, 108, 183, 210, 219 (bulk scene uploads).

⚠️ **That is the whole story of the four failures.** Run 149 recorded that scene uploads overlap
`c5`–`c8`; nobody followed it through. The arm was never a UI signal, so extending it captured world
geometry, which got squashed in clip space and read as *"a lot of geometry culled out — DRAW ALL is
fine, ENGINE CULL has a lot missing."* **The occlusion setting was never involved.** The one-shot
limit had been containing the damage, not preventing it.

⚠️ **Rejected remedies, so they are not retried:** a frame-long arm (captures world draws);
content-matching `c5`–`c8` (those draws never wrote the registers, so the values still match);
matching the **texture** (a long run spills *because* it needs a second glyph atlas page, so the
continuation carries a different texture by construction); anchoring on `NoteMarker` (it recognises
**notes**, and a pop-up menu is not one — `note elements inset: 0` while a popup is on screen means
the detector did not recognise it, **not** that no popup was open).

**A latent stereo bug fell out of this and is fixed independently:** `HudStereoPair`'s four rejection
paths all did `return draw()` — one draw, **no stereo split at all**. A draw that turns out not to be
insettable UI is not exempt from stereo; they now fall back to `StereoPair`.

### ✅ FIXED and VERIFIED (runs 252–257) — the splash regression was a failing `GObjects` walk

**`RefreshArmedState` cost 1365 ms in a SINGLE call**, four times per window, on a frame with zero
draws. One call is a `FindPlayerPawn` walk over **112,565 objects**, and the walk calls `Readable()`
per object — plus more inside `NameOf` — where `Readable` is a **`VirtualQuery` syscall**. Hundreds
of thousands of kernel transitions per walk.

Two fixes, and **the log says which one worked**:

| change | result |
|---|---|
| Miss throttle **frame-based → time-based** with backoff (2 s, then 15 s) | ✅ splash **8 → 17–28 fps** |
| `Readable` region cache, **1 slot** | ❌ **no change** — 1365 → 1275 ms |
| `Readable` region cache, **8 slots** | ✅ **1275 → 351 ms**, 3.8× |

⚠️ **A frame count is not a rate limit when frames are the casualty.** `(++missTick % 30)` fired every
30 frames whatever a frame cost — at 8 fps that is every 3.7 s with each walk costing 1.36 s, so over
a third of all wall-clock went into a search that *cannot succeed* at a splash, and the slower it got
the bigger its share grew.

⚠️ **One cache slot hit approximately never, and the reasoning sounded right.** "Consecutive objects
share a region" is true and irrelevant: per object the loop reads the object, its class, `GNames`,
then the name data — three or four regions in rotation, so one slot is evicted by the next lookup.
**It measured identical and was nearly kept on the strength of the argument.**

**Why it was a staircase** (~27 → ~18 → ~13 → ~8 fps over Aug 5–15): every commit that added a
property lookup or removed a throttle added another failing walk to the one screen where nothing it
seeks can exist. Both recent steps land within minutes of such a commit.

⚠️ **`ReadableCache=0` reverts the cache without a rebuild** — it is the one change here that can
plausibly *crash* rather than slow down, since a region freed inside a frame makes a cached "yes" a
lie. Bounded to positive results only, thread-local, cleared every walk and every frame.

⚠️ **A lost pawn resets the backoff rather than advancing it.** Without that, a level transition
inherited the splash's escalated backoff — up to **15 s of `PlayerArmed()` reading UNKNOWN**, which
feeds the run-227/228 weapon check that decides the arms/gun roles. Caught before it shipped to play.

**Residual:** the walk is still 351 ms every 15 s at a splash. User-reported as "okey dokey"; not
pursued further. `player pawn: … after walking N object(s)` is the number any future attempt must beat.

<details><summary>The original run-252 investigation, kept for the method</summary>

### 📉 MEASURED (run 252) — the splash screen has degraded in FOUR STEPS since Aug 5

Reported as *"the game is lagging to all heck now, even in the splash screens"*. Taking the **first
perf window** of all 149 archived runs — the splash, before any gameplay — gives a staircase, not
noise. Each step persists until the next one:

| period | splash fps |
|---|---|
| Aug 4–5 | **~27** |
| Aug 6–13 | **~18** |
| Aug 14, from 22:36 | **~13** |
| Aug 15, from 19:08 → today | **~8** |

**It is not today's build.** Today's runs, cumulatively: 119.2 / 118.0 / 114.5 / 115.6 / **92.1** /
103.8 fps — the worst was the run *before* the panel fix, and the panel code cannot execute with the
panel closed.

**All of it is in `our work`**, which STATUS has warned for months is a *residual, not a
measurement*. Splash-window budget, oldest to newest: `our work` **35.9 → 51.0 → 51.9 → 121.2 →
121.2 ms**, while `GObjects walk` stays flat at **1.86 ms** and XR is idle throughout. Log volume
does *not* correlate (391/226/437/238/258 lines before the first perf line), so it is not the
run-24/25 logging trap.

**Both recent step boundaries land on pawn/property resolution**, within minutes:
- Aug 14 22:30–22:36 — `52c6acf` (the Weapon lookup was unreachable) and **`4b96043`, which removed
  a throttle**: `ResolveGameplayOffsets` now calls `FindPlayerPawn(force=true)`, skipping the 1-in-30
  miss throttle. A miss is a full `GObjects` walk, and **at a splash screen there is no pawn**.
- Aug 15 19:00–19:18 — the property dump and `g_wantNames` registration commits.

**▶ First suspect, and it is structural: the auto-rescan can NEVER succeed at a splash screen.**
It fires whenever `!g_haveLock && g_camPosValid && dupDraws`, and a splash satisfies all three —
there *is* a camera position, VR mode is on from startup (run 159), and there is **no view matrix to
find**, so the lock is never acquired and it re-arms every 20 frames forever. The log shows it
plainly, repeatedly: `--- scan complete: 0 candidate window(s) ... no window even had a plausible
forward vector`. The scan's own comment: *"a scan tests every window of every upload and is far too
expensive to leave running"*.

**⚠️ TIMED, NOT FIXED.** `reg scan` is now its own term on the frame-budget line, subtracted from the
residual, plus a `⚠ REGISTER SCAN ran on N frame(s)` line with the total, the peak and the candidate
count. Rate-limiting the rescan in the same build would have shipped two changes and one
measurement. **If the number comes back small the scan is exonerated** and the staircase is
elsewhere — which is the point of measuring it.

⚠️ Not gated on `TimeOurWork`: it only takes a timestamp on frames the scan is armed for, and a
diagnostic nobody switches on cannot answer a question that shows up on someone else's machine.

**Outcome: the scan measured `0.00 ms` and was innocent.** So were resolution (anti-correlated — the
fastest splash ran the *largest* frame) and log volume. Three suspects killed by measurement, and the
answer was in the block that had never been in any bucket at all. **The value of timing the wrong
thing is that it stops you fixing the wrong thing** — a rate-limited rescan would have changed
nothing and been called a fix.

</details>

### ⚠️ SHIPPED, NOT YET FLOWN (run 251) — a slot is KEEP or CUT, and the mesh draws as SEGMENTS

Reported after the first sweep: *"90-1000 cuts away a lot of what I want to get rid of but keeps
the rest. What I actually found ideal is keeping 0-29, trimming 30-89, and keeping 90-1000."* That
is two kept ranges with a hole between them — precisely what run 250 documented as out of reach.

Each slot now carries a **mode**: `KEEP` draws only its range, `CUT` removes its range from whatever
survived the keeps. The result is a list of disjoint segments and **the draw lambda emits one call
per segment**, inside `StereoPair` so each eye draws the whole set. The ordinary path is one segment
and emits exactly the call it always did. The reported case is a single `CUT 30–89` slot.

Panel → `TMD ALIGN` → `TRIANGLE WINDOW`: `TRI SLOT`, `TRI MESH`, **`TRI MODE`**, `TRI FROM`,
`TRI TO`, `TRI STEP`, `HIDE MESH`. Saved as `TmdArmMode%d`.

- **Keeps are unioned and merged**, so two overlapping keeps cannot draw their overlap twice.
- ⚠️ **Inert is mode-dependent.** `KEEP 0–1000` keeps everything; `CUT 0–1000` **deletes the mesh**.
  Same two numbers, opposite results — so switching mode resets the range to that mode's inert value,
  and the rows say `OFF (FULL MESH)` or `OFF (NO CUT)` rather than showing numbers that do nothing.
- ⚠️ **Cutting everything draws nothing, and does NOT early-return.** The bone palette still has to
  be restored and `g_thisDrawMoved` cleared; a return that skips those leaves a zeroed bone behind
  for the next draw, which this file already warns about three times.

The rows also **say `OFF (FULL MESH)`** when the window is inert (0/1000). Two rows reading `0` and
`1000` look like settings while doing nothing, which is how run 237f's measured-dead window got
re-reported as broken.

### ⚠️ SHIPPED, NOT YET FLOWN (run 249) — the TMD panel page drew no text at all

Reported twice: the TMD page is a dark plate with the selection bar visible and **every glyph
missing, the page title included**. Run 248 read that as rows being dropped past the rectangle cap
and raised the cap; the page stayed blank and **the overrun warning that fix added never fired
once**, which is how it was ruled out this time.

The panel draws its text as a **single `Clear()` with one rectangle per run of set pixels**, and a
17-row page asks for roughly 13,000 of them. That call fails as a **unit**, so nothing lands —
which is exactly why the *title* went missing along with the rows, while the plate (2 rects) and
the bar (1 rect) survived. Working pages sat at ~5,000 and drew, which is what made this look
specific to the TMD page rather than to the panel. **The HRESULT had never been checked**, so the
refusal was invisible.

Three changes, and the mechanism is *not* yet confirmed — see the risk note below:

- `ClearRects()` **batches at 1024**, so no single call can be large enough to be refused, and a
  failure now **logs the HRESULT, the batch and the total**. This is the actual fix.
- **`kPanelRowsMax` is back down to 10** and TMD ALIGN is split into three pages — `TMD ALIGN`
  (the 7 pose rows, still one press from the root), `TMD BONES`, `TMD MESH`. `px` is derived from
  that constant, so at 17 rows the glyph had hit its floor of 3, which the code's own comment calls
  unreadable through a lens; at 10 it is 5.
- The panel logs **`page N (NAME), R row(s), X text rect(s) in B batch(es), px P`** on every page
  change. That number is what the whole misdiagnosis was missing.

⚠️ **A working page will NOT by itself confirm the diagnosis.** The page is now both batched *and*
short, so either change alone could account for it. The log settles it: the per-page rect count
says how big the call would have been, and a `Clear() REFUSED` line says whether any batch was
still turned away.

**Also fixed on that page, and unrelated:** `ARM FROM`, `ARM TO` and `ARM STEP` had **no
`PanelRowText` case**, so all three fell through to the `default` arm and rendered as *"DEBUGGING
ON"*; `ARM STEP` had no `PanelAdjust` case either, so **adjusting it toggled the DEBUG setting**.
Both shipped in run 247b and were invisible only because the page was blank. `TmdArmStep` was also
adjustable-but-never-saved. ⚠️ **A `default` arm that quietly does something plausible is why a
missing case reads as a row that works.**

### Next session, in order

**A. ✅ Runs 181–190 are FLOWN and four fixes are verified.** Details at items 1, 3, 4 and 5.

| fix | state |
|---|---|
| Crosshair jump-spread no longer escapes the hide rule | ✅ **verified** — two frames at `r=0.167` / `r=0.151`, past the old `0.15` box, `8 seen 8 dropped`. The case was genuinely exercised |
| Gun stepping on slow movement (1.9 cm lattice) | ✅ **verified** — integer truncation of the hand offset; now milli-UU |
| Gun pose one full frame stale | ✅ **measured** 8.33 ms → **0.00 ms** with `PoseLeadFrames=1` |
| Muzzle flash + barrel smoke at screen centre | ✅ **suppressed** (`HideMuzzleFx=1`); ⚠️ the real cause is the stereo seam — see item 3 |

**Still owed from run 180, and none of these has had its condition occur yet:**

- **The trigger veto now fails CLOSED at startup.** It used to pass everything through until
  `trigger/touch` resolved, which is the load-in phantom melee and the gun attached to nothing.
  It is a **race**: measured at 2, 0 and 9 unprotected gate-windows across three runs of one
  build, so a clean load-in proves nothing. **Verify by finding a run whose log says
  `NO SENSOR YET (vetoing - startup grace)`.** Both runs since the fix resolved instantly and
  never exercised it.
- **The turn stick now has a narrow `thumbstick/touch` sensor and a per-window census.**
  Measuring only — nothing is vetoed. It was added to catch phantom snaps and instead proved
  they were not snaps at all. Left in because the stick had *no* instrument before.
- **The mesh-latch census** logs the whole foreground list at take and drop. It has not yet
  caught a drop. Needs a long session — every run that dropped the latch was 11,000+ lines,
  every run that did not was under 3,100.

**B. 📋 Audit the remaining raw yaw subtractions.** Same class as the run-180 fix, no evidence yet.
`YawDelta()` exists now. The one at the FRotator write path carries the same false
`int32 wrap is correct` comment and is the first to check.

**C. ✅ RESOLVED — and the "exit never arrives" claim below is STALE (measured 2026-08-17).**
`SetCinematicMode` **does** report the exit now: **92 × `arg raw 0x00000001 -> CINE ON` against
88 × `arg raw 0x00000000 -> CINE OFF`** across one session's logs — a near-balance is exactly what
enter/exit pairs look like — and every `view drift` line reads `cine off`, zero `cine ON`. The flag
is a usable per-cutscene signal, and run 272 builds the occlusion override on it.

⚠️ **Re-test a premise before building on it.** This note said the signal was one-way, which would
have latched `DRAW ALL` on for a whole level. Checking cost one grep. The historical text follows.

**C(historical). ⚠️ PARTLY FIXED (run 211) — `g_inCinematic` latched ON permanently.** `SetCinematicMode` was
only ever *observed* with `arg raw 0x00000001`: the run-95 script hook sees native→script entry
points and the cutscene **exit was not one of them**, so a `0` never arrived. Everything gated on
the flag then leaked out of cutscenes, which is why the two cutscene comfort settings were seen
affecting normal gameplay. Both settings gate on it *correctly*; the flag was the bug.

Fixed from the other end rather than by finding the exit event: `RefreshCameraPosition` clears the
flag when the camera **instance** changes, which is an unambiguous level transition. Verified —
the flag now alternates (29 OFF / 11 ON / 51 OFF / 2 ON / 28 OFF in one run) instead of sticking,
all `view drift` lines read `cine off`, and `engine-vs-asked residual` is back to 0.10–1.03° from
a 3.5° mean with a standing 21.5° drift. User-confirmed: turning behaves after a reload.

**⚠️ Still not a true fix.** A cutscene that ends *without* a level change keeps the flag set
until the next transition. The exit event remains invisible.

The tabled **menu rotation bug** is still open and was NOT this — repro: free/follows → enter game
→ pause → free/locked → exit to main menu → start game → menu is rotated. Do not confuse it with
the run-211 menu *double vision*, which was a different bug and is fixed (see D).

**D. ✅ FIXED and VERIFIED (run 211) — menu double vision after returning from gameplay.**
Readability is not liveness. A level transition leaves the previous level's `RvPlayerCamera` in
`GObjects` with its memory mapped and its class pointer intact, so both tests `FindCamera` applied
— `Readable` plus a class compare — passed on a **dead** object; the walk took the first
non-archetype match in `GObjects` order, so the stale camera could win and then be revalidated
forever.

Diagnosed entirely from `view_matrix.log`, no new instrument needed: the post-transition main menu
reported `identification: 36 of 120 frames lost the matrix entirely; best rejected dotFwd +0.6964
(gate 0.80)`. dotFwd 0.696 is 45.9°, and the camera being read reported yaw **+44.5°** while the
matrix carried ~0° — two different cameras. Every rejection invalidates the register slot, so the
menu ran at `split 0, mono-fallback 399`: every draw unsplit across the full side-by-side frame,
i.e. one image with each eye seeing a different half. The boot menu was fine only because just one
camera existed at that point, which is what made this look like a regression.

`FindCamera` now **prefers a live instance and falls back to the first match**, so it can never
return less than before — a false negative from `IsLive` costs a periodic re-walk, not the camera.
The liveness recheck is throttled to every 15 frames (~0.125 s), because the condition only arises
at a transition. After: `split 334 (334 with parallax), mono-fallback 0` at 120 fps in the same
menu, and no window in the run carries the broken `peak live in a frame 0` signature except four
splash-screen windows that read `registers tracked: none` — before any camera exists.

**Known residue:** three windows still show heavy mono-fallback (549, 1340, 4386) at 38/29/37 fps,
all landing exactly on load/transition moments while the camera is being replaced. Expected, but
recorded rather than filtered out, because a long enough transient reads as a visible flash.

0. **📋 The BACKSPACE keyboard exemption can now go** — `VR MODE` is on the panel as of run 159, so the last reason to keep it live under `Debug=0` is gone.
   Run 157's `[Render] Debug` makes every keyboard key inert except **BACKSPACE** (VR mode on/off)
   and **PAUSE** (hold to quit, which is the `WM_CLOSE` path that lets the engine save — and every
   copy of this game shares one save profile). Both are marked at their call sites as the only
   places a raw `GetAsyncKeyState` is legitimate. A panel row retires the BACKSPACE one; PAUSE
   probably has to stay, since quitting cannot depend on the thing you are quitting.

1. **✅ FIXED and VERIFIED (run 181) — the crosshair's JUMP SPREAD escaped the hide rule.**
   Confirmed by measurement, not by looking: `8 stride-8 elements, 8 dropped` throughout, with two
   frames reaching `r=0.167` and `r=0.151` — both **past** the old `0.15` box, so the spread really did
   exceed it and the case really was exercised. Not a false pass.
   Reported: with the crosshair hidden, **two** crosshair lines come back while the player is
   jumping. Spread pushes the four ticks outward and the test was a **square** box — `|cx| < 0.15`
   and `|cy| < 0.15` — in a space where x and y are not the same scale.
   - Clip x spans the frame's full **width** and clip y its full **height**, so one tick at a given
     *pixel* radius reads 16/9 = **1.8× larger in y**. That ratio was already sitting in the
     measurement and was read straight past: `0.045 / 0.025 = 1.8`. The vertical pair therefore
     leaves a square box **first and alone** — two lines, which is the report down to the count.
   - The fix uses the part of the measurement the box discarded: **each tick sits ON a centre
     axis.** `LooksLikeCrosshair` now tests "on an axis, within reach of centre" (`kXhairAxis`
     0.006, `kXhairReach` 0.60) — *tighter* across the axis than the old box, looser along it,
     which is the only direction spread can move a tick.
   - **Verify with the census, not by looking.** `crosshair census: N stride-8 element(s) this
     frame, M dropped, furthest at r=…` prints from Present on change. **Resting truth is 8 seen,
     8 dropped** (four ticks, each drawn twice). *Seen above dropped is the bug*, in whatever form
     it returns. **The false pass: not jumping hard enough to spread the reticle at all** — a run
     with no line above `r=0.045` never exercised this.
   - **⚠️ STATUS carried a stale version of this item for twenty-odd runs**, and so did the comment
     at `g_hideCrosshair`: both said mode 1 *flickered* because `g_handHeld[1]` is a capacitive
     sensor, and that `HideCrosshair=2` was "the setting that behaves". **Run 158 replaced that
     signal with AIM METHOD** — deterministic, no flicker — and neither note was updated. Modes
     0, 1 and 2 are all sound. Both texts are corrected; the code comment keeps the warning.
   - `g_handHeld` is still not to be trusted if anything else ever wants it. Run 155: `g_handHeld[0]`
     read `on desk` through a whole session including ninety seconds of deliberately holding both
     controllers, one sample moving at **1331 µm/frame**. Unreliable in *both* directions, and it
     has no per-frame consumers left.
   - **The crosshair signature, measured** (bare view, nothing else on screen): four ticks around
     centre, each drawn twice, `stride=8` at `(±0.025, 0)` and `(0, ±0.045)`. **Stride 8 is unique
     to it** — everything else is stride 4, 12 or 20, and centred *prompts* are stride 20 at
     `y = -0.138`. Deliberately NOT keyed on primitive count: 10 is what these ticks happen to be,
     and another weapon's reticle will differ. Re-measure if a later weapon's crosshair persists.
2. **📋 BETA: the HUD sits too close to the edge to see in the headset.** Health and ammo duplicate
   correctly per eye since run 151, but they are anchored to the corners of a 16:9 frame and a
   headset cannot see its own corners — the eye's usable area is a circle inside that rectangle.
   They need pulling inward toward the centre.
   - The mechanism is already in hand: the UI's stage-to-clip matrix is `c5`–`c8`, and
     `HudStereoPair` already rewrites it per eye. Insetting is a scale-about-centre on the same
     registers rather than a new system.
   - Wants to be a panel setting (HUD inset / safe area), not a constant — how far in is
     comfortable depends on the headset's optics and the player's IPD.
   - ⭐ **Run 230 built and shipped that mechanism for NOTES** (`NoteInsetPct`, **default 40**,
     DISPLAY page, `NOTE SIZE`). Confirmed in a headset: 480 draws/second = the 4 non-exempt
     elements at 120 fps, with the backing correctly left alone. Extending it to health and ammo is now a matter of choosing what to match, not
     of writing anything: the scale-about-centre and its per-eye composition are done and proven.
     ⚠️ **The inset must be applied BEFORE `ApplyEyeRemap`, never after** — the eye remap shifts x
     by ±0.5, and scaling about zero after that shift drags the eye centre, throwing the two eyes
     off in opposite directions.
   - ⚠️ Whatever gets inset needs the same **corner exemption** notes use: an element anchored at
     `|clip.x| >= 1` is a full-screen backdrop, and shrinking one leaves the world showing in a
     ring around it.
3. **⭐ The muzzle flash and barrel smoke draw on the STEREO SEAM — and it is OUR bug, not the
   engine's.** Suppressed for now: `MUZZLE FX` on the panel's DISPLAY page, `HideMuzzleFx=1`, default
   hidden. The real fix is queued and should retire that setting entirely.
   - **They are ONE particle system.** `RvGame.u` lists no smoke property at all — the barrel smoke is
     an emitter inside the muzzle-flash asset. Reported as two separate bugs for sixty runs; nulling
     `mMuzzleFlash1stPerson` plus the looping and ironsights variants kills both.
   - **The diagnosis came from the flat monitor and it is decisive.** The effect sits at the dead centre
     of the *monitor*, on the seam between the two eye images. World geometry goes through `StereoPair`
     and is drawn **twice**, once per scissored half — so a merely mis-placed effect would appear
     **twice**, once near each gun. Seeing **one**, straddling the middle, means its clip position is
     `(0,0)` in the *full side-by-side frame*: each eye's scissor keeps the half that falls in its
     region, and the halves meet at the seam.
   - **Therefore the aim method cannot matter**, because the effect never receives a per-eye position at
     all. Confirmed identical in HEAD and MOTION CONTROLLER. That is why the setting is binary — an AUTO
     that cannot differ from ALWAYS is a setting lying about having a reason to exist.
   - **📋 TABLED at run 194, with the draw identified.** Four facts are nailed down and should not be
     re-derived; the remaining step is small but my instruments kept eating the runs.
     | fact | how |
     |---|---|
     | The draw is **`DrawIndexedPrimitiveUP`, stride 72**, ~16–18 prims, ~7 per shot | run 192's baseline-vs-fire census; stride 72 appears in every fire window and never in baseline |
     | It **is** already split per eye | same census — so the split works and only the transform is unremapped |
     | Its vertices are **world-space**, not pre-transformed screen space (box `x[260..860]`) | so it is not a screen-space quad, and the Bink-quad fix does not apply |
     | It has a **vertex shader**, and **`c5..c8` is its local→world matrix** (w column `(0,0,0,1)`; translation drifts frame to frame as the particle moves) | run 193's per-draw write-mask dump |
     | Pass-throughs are **104/frame in baseline and in fire alike** | firing adds no fullscreen-quad pass-through, so the quad classifier is not involved |
   - **What is still unknown:** which register carries the draw's *clip* transform. `c5..c8` is affine,
     so something else projects it. Registers written just before the draw are
     **`c5`–`c12`, `c15`–`c18`, `c20`–`c25`**. A view matrix (unit axes + translation) is visible around
     **`c26`** in the `c24..c27` dump.
   - **🛑 The stride-72 identification is UNVALIDATED and probably wrong (run 196).** Its shader's own
     bytecode says it **reads `c0`** — and `c0` was confirmed locked, valid and remapped at that very
     draw. So that draw's transform *is* eye-remapped and it cannot be what lands on the seam. Run 192
     was right that stride 72 is *shot geometry*; the error was reading "unique to firing" as "is the
     muzzle flash". A tracer, a casing or an impact effect all appear only when firing and all render
     correctly.
   - **🛑 CONFIRMED WRONG (run 197). `MuzzleFxDropStride=72` with the flash visible: the artefact
     REMAINED.** Stride 72 was never the muzzle flash — it is some other shot geometry (tracer, casing
     or impact). **Runs 193–196 measured the wrong draw from beginning to end.** The `c0` reading, the
     write masks, the block dumps and the bytecode all describe a draw that was never the artefact.
   - **▶ NEXT STEP if resumed: identify by SHADER, not by primitive count.** The flash is on an
     **indexed** path (`DrawIdxPrim` rose ~1600 during fire — the observation dismissed in run 192 as
     confounded, and the only surviving lead). Indexed draws cannot be told apart by primitive count —
     that is exactly what saturated run 192's table with twenty rows of zeros — so the handle has to be
     something else, and the shader pointer is the natural one:
     1. Record the **set of vertex shaders** used in a baseline window and in a fire window. The ones
        unique to firing are the shot's own. `GetVertexShader` returns a comparable pointer, and run 196
        already built a bytecode reader for whatever it finds.
     2. Add an ini key to **drop draws using shader N**, then walk N until the artefact disappears.
        That is identification *by construction* rather than by inference — one build, then cheap
        relaunches, instead of a run per instrument.
   - **Do not skip step 2 for step 1's convenience.** That is precisely the mistake of runs 193–196:
     a plausible label taken as identified, then three instruments built on it.
   - **⛔ Confirm what a thing IS before measuring its properties.** Three instruments (runs 193–196)
     were built on an identification nothing had tested. The bytecode dump was the first measurement
     capable of contradicting it, and did — after three runs. The one-line drop test was available the
     whole time.
   - **⚠️ A cheaper hypothesis to check FIRST, and it costs one log line:** the flash may ride the
     scene's own `c0` and simply arrive when **no `g_camMats` slot is valid** — `g_camMatrixBound` is
     cleared whenever a shadow or post pass takes the tracked register, and `InvalidateCamMats()` fires
     on partial overlaps. A duplicated draw with nothing to remap lands identically in both halves,
     which is exactly this artefact. **Log how many slots are valid at a stride-72 draw before hunting
     any further for a register.**
   - **⛔ Four runs were lost to my own instruments, not to the problem.** Recorded because the shape
     repeats: run 183's owner cap filled with archetypes before reaching the live weapon; run 192's
     table saturated on indexed draws; run 193 `break`'d after the first constant block; run 194 added a
     4-alignment filter that **excluded `c5`, the block already confirmed**. Every one was a bound
     chosen for tidiness that silently excluded the answer. **Shader constants have no alignment
     requirement — a compiler packs them wherever they fit.** When the question is "which of these is
     it", do not cap the list at one.
   - ⛔ **Runs 182–188 chased a phantom.** They assumed the effect was correctly drawn at a wrong
     *engine* position and went after moving the weapon mesh in the engine. The engine position may have
     been right all along. What survives is in `ENGINE_NOTES.md` and is genuinely useful: the UObject
     property-offset layout, and the measured negative that writing a component `Translation` does
     nothing without a reattach.
   - ⛔ **An earlier premise named the wrong effect**, and this item chased *"find what the smoke rides
     on that the flash does not"* for sixty-odd runs. The aligned smoke was the **impact** smoke off the
     wall: spawned in world space at the trace hit point, which route 2 already rotates, so it is right
     for free. Two grey puffs appear near your aim within a second of each other — one on the wall, one
     on the gun. Easy to conflate in a headset, expensive to write down, because it then reads as a
     measurement.

4. **✅ FIXED (run 190) — the gun no longer moves in STEPS. The cause was integer truncation.**
   Reported first as jitter, then **redescribed as "moving in steps, like a deadzone — if I move the
   controller very slowly I can see it jumping."** That redescription is what solved it: latency shifts
   smooth motion in time and **cannot** turn it into stairs.
   - `g_handOffX/Y/Z` held the hand-minus-head offset in **whole unreal units**. Eye separation is a
     measured 3.32 UU = 6.3 cm, so **1 UU ≈ 1.9 cm** — the gun could only ever sit on a 1.9 cm lattice.
     `(LONG)` also **truncates toward zero**, so the step was asymmetric about the origin.
   - Now milli-UU with `lroundf`: **19 µm**, far below the controller's own noise floor. Both read sites
     divide by the same constant, including the on-screen anchor marker — run 174's trap was a readout
     in different units from the thing it described.
   - **The rotation was never involved and was deliberately left alone.** `g_handDev*UU` are engine
     rotation units at 65536/turn = 0.0055° per step. Fixing both would have hidden which one mattered.
   - **Method note, and it is the useful part.** A *confirmed measurement pointed the wrong way*.
     `pose lead` correctly read 8.33 ms of staleness; acting on that alone would have produced a small
     improvement and a false "fixed". **The redescription ruled out the entire latency class by
     itself.** When a symptom is described as *stepping*, suspect quantisation before timing, whatever
     the instruments say.

5. **✅ The gun's pose was ALSO a full frame stale — fixed separately (`PoseLeadFrames=1`).**
   Measured **8.33 ms**, exactly one display period at 120 Hz, then **0.00 ms** with the fix.
   - **Why the head hides it:** the head is submitted as the projection layer's pose and the compositor
     **reprojects** the image at scan-out, correcting late error in hardware. The gun is baked into
     **geometry** by `BuildGunC`, and reprojection cannot move a vertex. D3D9 renders *then* Presents,
     so the pose sampled at Present N is consumed by frame N+1's draws — one period late.
   - The fix is one term: locate the hand spaces at `predictedDisplayTime + predictedDisplayPeriod`. The
     period comes from `xrWaitFrame` rather than a hardcoded 8.33 ms, so it is correct at 72 and 90 Hz.
   - ⚠️ **The head's layer pose is deliberately NOT shifted.** It is already correct, and moving it would
     hand the compositor a pose for a frame it is not displaying.
   - ⚠️ **`0.00 ms` is bookkeeping, not tracking quality.** With pacing locked the metric is
     arithmetically exact and is measuring its own arithmetic. It proves the timestamp we ask for matches
     the frame the geometry lands in; it says nothing about whether the runtime's *prediction* at that
     timestamp is good. The remaining risk is **overshoot on sharp reversals**, since prediction error
     grows with lead time — judged in the headset, not in the log.
6. **✅ WORLD SCALE is adjustable (run 201).** Panel → `DISPLAY` → `WORLD SCALE`, live, 5% steps,
   50–200%, saved as `WorldScalePct`. Above 100% the world feels **bigger**.
   - **One constant governs scale and that is why one knob is correct**: `kMetresToUU = 52.5`, which is
     not a guess — UE3's documented 16 units per foot gives 52.5 UU/m exactly. Everything real-world
     flows through `MetresToUU()`: the **stereo baseline** (apparent size), the **6-DOF translation**
     (distance travelled per step), and the **hand position** the gun rides.
   - ⚠️ **All three must scale together.** Scaling the IPD alone changes how big things look but not how
     far you walk, which reads as the world resizing *as you move* — the exact complaint the slider
     exists to fix. Scaling head but not hand slides the gun around your body.
   - ⚠️ **It cannot change EYE HEIGHT** — the game owns that. A player who feels the wrong height is
     describing a different problem from one who feels the wrong scale.

7. **✅ FIXED (run 201) — the gun was anchored to the ENGINE's eye, not to yours.** Reported as *"when I
   recentered, the gun came closer to me."*
   - 6-DOF folds `g_hmdOffset` into the **camera matrix**, so you view from `engine eye + g_hmdOffset`.
     But `BuildGunC` applied `H = (hand − head)` relative to the origin of translated-world space — the
     **engine's** eye — which never received that displacement. The gun's apparent position was therefore
     wrong by however far you had drifted from the recentre point, and recentring zeroed the drift so it
     jumped. Direction depended on which way you had moved, which is why it was not obviously diagnostic.
   - Fixed by adding `g_hmdOffset` to `H`. Applied unconditionally, including with position-following
     off: the gun should stay glued to the view exactly as the flat game's does.
   - `GunFollowsHeadOffset=0` reverts without a rebuild — the *sign* of a position fix is something this
     project has had to flip more than once.
   - **Method note: no instrument in the session would have caught this.** The eye-height probe read
     `+0.0 UU` right after recentring and looked like a clean pass. A symptom that only exists when two
     things which should share a reference frame don't, needs someone wearing the headset.

8. **📋 Eye height: measured, and probably NOT a problem.** `Pawn.BaseEyeHeight = 70.0` (stable),
   `EyeHeight` 67.4–71.4 with bob and crouch. Offsets in `ENGINE_NOTES.md`, and both are writable if a
   knob is ever wanted.
   - ⚠️ **`BaseEyeHeight` is measured from the actor's Location — the CENTRE of the collision cylinder,
     not the floor.** So 70 UU is not eye height above ground and converting it to 133 cm invites the
     wrong conclusion that the character is child-sized. Log `CollisionHeight` before comparing to a
     human.
   - **Recentre is confirmed working from seated** (offset `+0.0 UU` immediately after). Sitting versus
     standing is handled by recentring, not by a setting: **hold MENU ~1 s**, which re-zeroes yaw, lean
     and height together. If the view ever feels too low or high mid-session, hold MENU before assuming
     a bug.

9. **Shadows are broken when High Quality Decals is ON, and run 139 found a new clue.** With that
   option on, **the hidden arms turn BLACK instead of staying invisible.** So whatever hides the
   first-person meshes is not reaching the pass that HQ Decals enables — the arms are still being
   drawn there, at their original position. That is a much sharper lead than "decals are broken":
   it says the mesh-hiding rides on the locked view-projection register and this pass does not.
   Connects directly to the unidentified **third scene-sized surface** and to `ExtraCamReg` (`vs
   c13`), both of which are passes we do not remap. Workaround remains free: leave High Quality
   Decals off.

> ## ✅ FIXED (run 139): the texture wrapper. `D3D9ExMode=1` is now on par with `0`.
>
> One defect, three symptoms — the soldier's colours, the black dock floor and the vanishing fire.
> `UpdateTexture` copies the source's **dirty regions**, not the source; ours did not cover what
> had been written, so it copied too little and returned `S_OK`. Marking the shadow fully dirty at
> every unlock (`AddDirtyRect(NULL)`, per-face for cubes, `AddDirtyBox` for volumes) fixes it.
>
> ⚠️ **The fix is confirmed. The mechanism is NOT, and the first explanation was wrong.** The
> diagnosis was `D3DLOCK_NO_DIRTY_UPDATE`; the counter added to prove it measured **0 of 6716
> locks**. The engine never passes that flag. Do not repeat that story — it is written up here
> only so nobody re-derives it.
>
> Still true and still worth suspecting if this resurfaces:
> - `AddDirtyRect` was never hooked and never called anywhere in the file.
> - **`e->dirty` is one flag for the whole texture while locks are per-LEVEL**, and `FlushShadow`
>   clears it after any copy — so a texture locked on several levels before any unlock copies once
>   and silently skips the rest. A real defect on inspection, whether or not it was the one biting.
> - `UpdateTexture` clears the source's dirty regions as it copies.
>
> **Cost not yet measured.** `AddDirtyRect(NULL)` marks the whole level, so streaming copies more
> than it needs to. The locked rect is already recorded if it wants narrowing.

> **Why it took five runs, kept because the reasoning error is the useful part.**
> - **Why it looked like culling.** UE3 selects texture mips by projected screen size in *pixels*,
>   so which mips get requested moves with FOV, with resolution and with distance — all three of
>   the levers that looked like rendering knobs. Every one of those uploads goes through the
>   wrapper.
> - **Why the symptoms differ by surface.** Fire is **additively blended**, so an unwritten upload
>   contributes nothing and the effect is *invisible* rather than miscoloured. The deck is opaque,
>   so the same failure renders **black**.
> - **The dismissal that cost the most.** The wrapper was ruled out in run 135 because *"the effect
>   is FOV-dependent and a dropped upload wouldn't care about FOV"*. Mip selection **is**
>   FOV-dependent, so the one fact that felt exculpatory was the one that most implicated it — and
>   the wrapper had already been proven defective on the soldier by then. **An independently
>   confirmed defect should not be dismissed on a one-line argument.**

10. **✅ FIXED (run 199) — the "reload arms" were never reload geometry.** `HideExtraArms=1`. Reload now
   looks natural.
   - ⛔ **Carried as a reload/latching problem from run 122 to run 198, and it was neither.** Reported
     correctly at last: *"they're actually always there during gameplay but hidden under where the player
     could normally see... and they move with the gun, so when I move the controller up they're pretty
     visible. And no, they don't disappear when I reload."*
   - **It is the 390-vertex mesh.** The baseline set is `[5799 gun, 3314 arms, 390]` and run 181's rider
     census reads `vertex counts seen riding: 390`. So it is **in** the baseline — which is why run 198's
     "hide what was not in the baseline" rule could not reach it — and it **rides the gun transform**,
     which is why it tracks the controller.
   - **A defect the mod created**, which is why sixty runs never saw it: the fragment sits below the
     bottom of a 16:9 frame, and VR's much taller field of view plus a gun that follows your hand brings
     it into view.
   - Hidden by **role** — "the baseline mesh that is neither the gun nor the arms" — not as the literal
     390, so another weapon's fragment is covered. ⚠️ It rides the gun, so if a weapon's third baseline
     mesh turns out to be a sight or magazine this would remove it; `HideExtraArms=0` reverts.
   - **Method note:** run 198 fixed a problem nobody had (stray animation meshes) because the label on
     the report was taken as the diagnosis. The user's description contained the answer — *always
     present*, *moves with the gun* — and both facts were already in the logs.

10b. **✅ FIXED and VERIFIED (run 212) — the run-207 role shift came back past `MinFgMeshes`.**
   `VetoQuietGun=1`, `RelatchWrongRoles=1`.
   - **Verified, and the case was genuinely exercised** — this is not a clean-run false pass. In a
     161,376-line playthrough from the beginning, `view_matrix.log` line 17471 logs the veto firing on
     `fg census (at the REFUSED take): 3 entries: [0]=3314 [1]=390 [2]=662` — **the identical signature
     to the Aug 7 failure**, including the same 662 mesh. The pistol then latched correctly at line
     35663, `[5799 3314 390]`. User-confirmed in play.
   - **Reported:** playing **from the very beginning** to the pistol, the arms are visible (low, where
     the flat game never shows them) and **the pistol is invisible**. Loading the save just before the
     pistol works. The differentiator was the diagnosis: the two runs traverse different states.
   - **Cause.** A stray 662-vertex mesh joined the weaponless pass, `n` reached 3, `MinFgMeshes` passed,
     and the arms took the gun role. `MinFgMeshes` is a **floor** — an extra mesh satisfies it as well
     as a weapon does. Named in `view_matrix_2026-08-07_23-11-44.log` line 37863.
   - ⚠️ **662 appears in exactly one census line of an 88,575-line log.** The gate is evaluated only on
     the frame the settle completes, so one frame of a stray prop poisons the session.
   - **Fix:** the gun cannot be a mesh that is drawn when you have no gun. The weaponless pass is
     already detected by the `MinFgMeshes` branch, so record it and refuse the gun role to anything in
     it. Plus a backstop that drops a latch whose gun role is contradicted — see `HISTORY.md`.
   - **Verified against data, not reasoning:** 71 takes across every log kept, **0 of 54 good latches
     affected**, and the one bad take blocked. Also confirmed a no-op on the working save-load path,
     which already records the weaponless set before latching 5799.
   - ⚠️ The run-158d **menu** latch (`[4 251 745 …]`, 8 meshes) is **not** covered and is still open.
   - **✅ Weapon swapping is now measured, and works.** The same run cycled **four** weapons — `5799`,
     `8297`, `11387`, `11389` — through **21 drop/re-take pairs**, every one re-taking the correct gun.
     The old "is 11387 a second weapon or one mesh in two states" question is settled by
     `[0]=11387 … [3]=9` and `[0]=8297 …` coexisting in one session as distinct weapons.
   - **The `RelatchWrongRoles` backstop never fired**, so the `bigger` misfire I flagged as possible did
     not occur across 161k lines and four weapons. It also cannot pre-empt a swap: the normal drop needs
     240 frames and the backstop 600, so the normal drop always wins that race. Confirmed — 21 swaps,
     zero backstop firings.
   - ⚠️ **The ~2 s of hidden gun after a swap is still neither confirmed nor refuted.** The mechanism is
     unchanged and still there in the code (the new weapon is outside the old baseline, so
     `HideStrayArms` hides it until the 240-frame drop), but the drop line marks the *end* of that
     window so the log cannot size it, and the user did not report seeing it across 21 swaps. Open.
   - **✅ CLOSED by run 216.** This recorded `gun=11389, arms=97` as a latent misassignment and named
     the cause correctly — "slot [1] is the arms" was the weak rule. It was left unfixed because it
     changed nothing visible. It later bit hard: the same positional rule put the **arms** in the gun
     role after scope use. Roles are assigned **by size** now, which fixes both. **A known-weak rule
     left alone because its current symptom is invisible is a bug waiting for a louder symptom.**

10f. **✅ FIXED and VERIFIED (runs 227-228) - the mesh latch now ASKS whether a weapon is equipped.**
   - Reported as a missing rope in a hands-tied scene. With no weapon the largest mesh in the pass is
     the ARMS, so run 216's size rule gave them the gun role, the rope inherited the arms role, and it
     was hidden. Pressing the heal button revealed it, which named `FgHidden` as the culprit outright.
   - **Four proxies for "is there a weapon" were each defeated by that one scene** - `MinFgMeshes`,
     `VetoQuietGun`, `RelatchWrongRoles`, `FgRememberArms`. Every one waits to be told what normal
     looks like, and a save can start you somewhere that never tells it. **`Pawn.Weapon +0x3D4`
     answers it directly**, resolved once by the existing property solver and read from a cached
     pointer once a frame.
   - ⚠ **Resolve it ONCE A FRAME, never per draw.** `FindPlayerPawn` walks `GObjects` on a miss, and
     the first version called it from `FgHidden`, which runs per draw. That is run 209's defect.
   - With no weapon: the arms-role mesh is not hidden (it is whatever is on your hands), it rides the
     gun transform, and its **depth-prime copy rides too** - without that it writes run 120's black
     silhouette at the engine position. The 390 fragment stays hidden either way.
   - 📝 **`VetoQuietGun`, `RelatchWrongRoles` and `FgRememberArms` are now redundant weight** -
     they exist to guess at exactly what the engine now answers. Left in place, not yet retired.

10h. **✅ FOUND (run 236) - `RvPlayerPawn.mPlayerHandsStatus` says whether the TMD is raised.**
   `+0x105C`, depth 1, right beside `mTMD` (`+0x1058`). **0 = neither, 1 = TMD RAISED, 2 = weapon.**
   - Decoded by correlating every change against the MESH CENSUS, not against anyone's memory of what
     they were holding: six deployments, no exceptions either way, correct across a weapon switch.
   - ⭐ **It LEADS the render** - the value changes a frame or two before the TMD's meshes appear, so a
     draw-time decision on it is right on the frame the geometry arrives.
   - ⛔ Two dead ends on the way, both worth not repeating: the TMD is **not** an inventory weapon
     (`Pawn.Weapon` never points at it - it stays on the gun, which is HIDDEN not swapped), and
     `IsTMD_Equipped` in the dump is a FUNCTION, not a readable property.
   - ⚠ **A new property needs registering in `g_wantNames`.** `WantedIdx` searches only that table, so
     an unregistered name fails before a single class is walked and reports as "not found at depth 13".
     The property DUMP does not share that step - it reads names off each property directly - so the
     two are not independent, and a "not found" from the resolver often means "not in the table".

10g. **📏 MEASURED (run 234) - the arms are ONE mesh, so two-handed VR is not reachable by
   transform.** 7444 verts, 4402 triangles, from a single vertex/index buffer at `base 0, startIdx 0`
   - it owns its buffer outright. Identical across **27,279 draws** with a weapon and with the TMD, so
   the one-hand/two-hand difference the wearer sees is POSE, not a second mesh.
   - ⛔ Both arms arrive in **one draw call**, which takes one world matrix. Splitting them needs
     bone-level control. `AdsFreeze` proves we can write the bone palette; the bone map and bind pose
     are what we do not have, and D3D9 does not expose them.
   - ⚠ **The mesh census cannot answer questions like this** - `FgLearnSignature` dedupes by vertex
     count, so two distinct meshes sharing a count collapse into one entry. Use the draw-call identity
     probe (`FgIdentityVerts`) instead: buffers plus range, which pose does not change.
   - ✅ The TMD gauntlet (4818) IS its own mesh, correctly posed on the left forearm, and is drawn at
     the same time as the weapon. Putting it on the left hand is cheap and does not need any of the above.

10e. **✅ FIXED and VERIFIED (run 216) — the scope reorders the pass, and the roles were positional.**
   - Reported: after using the sniper scope, the arms are on the controller and the gun is invisible,
     surviving a weapon switch. The run-207 role shift by a route neither run-212 defence covered.
   - **The scope draws the arms FIRST** — `[3314, 390, 11389, 97]` against the normal
     `[11389, 97, 3314, 390]` — so a re-latch while scoped took slot[0]=3314 as the gun.
   - **Unrecoverable by design flaw:** a latch taken from a scoped pass captures a baseline containing
     the real gun, so `FgIsExtraArm` hid it *and* run 212's recovery — which required a larger mesh
     **outside** the baseline — never saw a contradiction.
   - **Fixed by size:** the gun is the largest mesh in the pass, the arms the second largest. True of
     every census on disk, and invariant under draw order. Plus: no latch from a scoped pass, and the
     recovery's baseline requirement dropped.
   - **Verified, and the case was genuinely exercised** — two takes from the arms-first list, both
     correct: `[0]=3314 [1]=390 [2]=11387 [3]=9` gives `gun=11387 arms=3314`, and
     `[0]=3314 [1]=390 [2]=11389 [3]=97` gives `gun=11389 arms=3314`. Under the old rule both would
     have been the bug. No recovery firing was needed.

10c. **✅ FIXED and VERIFIED (run 213) — the sniper scope was drawn once across the whole frame.**
   `ScopeQuadPerEye=1`, `ScopeScenePsReg=1`, `ScopeAspectPsReg=0`, `ScopeHeadAim=1`.
   - The overlay is `DrawPrimitive, triangle strip, 2 prims` — a fullscreen quad from a bound vertex
     buffer. `StereoPair` remaps the **camera registers**, and a clip-space quad reads none, so both
     copies landed at full-frame coordinates and each eye got half a circle. Same reason the HUD was
     always fine: `HudStereoPair` transforms `c5`–`c8`, which the HUD shader does read.
   - **Three parts, all required:** per-eye **viewport** (no vertex pointer exists to rewrite);
     `c1 ScreenPositionScaleBias` **X only** (the scene read comes from the projected position, not the
     geometry); `c0 ScreenAspectRatio` halved (each half is 1280×1440, not 2560×1440).
   - Verified in the log, not just by eye: `scope overlay quads drawn per eye: 120` — one per frame
     while scoped, absent otherwise. User-confirmed in play.
   - ⚠️ **Four fixes were written on inference first, and all four were wrong.** The full autopsy is in
     `HISTORY.md`; the short version is that the pixel shader carries a **CTAB symbol table** naming
     every constant and sampler, and reading it settled in one look what three censuses could not.
   - ⚠️ The counter reads **240** in a few windows — the overlay is drawn twice a frame in some state.
     Harmless as observed, but it means "once per frame" is not universal.
   - **`ScopeHeadAim=1` shipped alongside**: while the scope overlay is on screen the shot follows the
     HEAD, not the controller, at both trace sites. Gated on the overlay being drawn rather than on the
     alt trigger, which means something different on every other weapon.
     ✅ **Confirmed in play (run 216)** — the shot follows the reticle, not the controller pose.

10d. **✅ FIXED and VERIFIED (run 215) — the mesh matchers hid WORLD geometry.** `FgPrimeWindowDraws=16`.
   - Reported as a hole in the floor with water and a basement visible through it. Bisected on ini keys:
     absent unmodded → it is us; HEAD mode fixes it → the mesh path; `HideExtraArms=0` fixes it →
     `FgIsExtraArm`.
   - **`fgDrawCount > kFgSanePass` never fires outside the first-person pass**, because the counter it
     reads is 0 there — so `0 > 64` is false and the vertex-count match proceeded **frame-wide**. With
     `9` in the baseline set, every 9-vertex draw in the frame was hidden.
   - Fixed with a window: **inside the pass, OR within the first few draws of the frame**. Gating on the
     pass alone would have restored run 120's black silhouette, since the depth prime is drawn at
     draws 2–3. Applied to the MOVE path too.
   - **Measured ~4 refusals per frame across a whole run** — this was pervasive, not room-specific. It
     had been removing small world meshes all along and only became visible on a floor.
   - ⚠️ If a hidden mesh ever leaves a **black silhouette**, the window is too narrow — raise
     `FgPrimeWindowDraws`. That is run 120's failure mode and needs no rebuild.

11b. **✅ FIXED and VERIFIED (run 214) — the melee swing was invisible.** `MeleeArmsMs=700`,
   `MeleeArmsDelayMs=0`.
   - Same cause and same remedy as the heal: the animation is performed by the arms the mod hides. The
     window shows the arms **and returns the gun to the engine's position**, because moving the gun onto
     the controller mid-animation tears it in half (run 198).
   - The heal and melee sites now share `ArmsAnimActive()` — a third animation extends one function
     rather than three call sites. Both keep the run-203 cheap-gate shape on the hot path.
   - Verified in the log: **12 windows, one per press**, none double-armed.
   - ✅ **700 ms is CONFIRMED, first flight** — reported as "the timing was perfect", no tuning needed.
     Unlike `HealArmsMs`, which took a round. Still an ini value in case another weapon's swing differs,
     not because this one is unverified.
   - ⚠️ **The impulse weapon shares this button** and that case is unmeasured. Every window logs the
     latched weapon (`gun=…`) so one ordinary run settles it once the weapon exists. `MeleeArmsMs=0`
     disables it without a rebuild.

11. **✅ The arms reappear for the health injector animation (run 198–199).** `HealArmsMs=2150`,
   `HealArmsDelayMs=100`.
   - The arms are shown **and the gun is returned to the engine's position** for the window — moving the
     gun onto the controller while the arms animate at the engine position tears one animation in half.
   - **Gated on the heal being able to do anything.** Reads `Pawn.Health` / `Pawn.HealthMax` (see
     `ENGINE_NOTES.md`) at the press, so at full health the arms never appear.
   - ⚠️ **Do NOT "verify" by waiting for health to rise.** Health is applied by the animation, possibly
     at its end, so the confirmation arrives *late* and a window opened then runs on past the animation
     it was meant to cover. Asking "could this heal do anything" *before* the press has no timing
     dependency.
   - Both edges are knobs measured from the press, because they were reported separately and move
     independently: shortening the duration to fix a late finish would also have made it start early.
   - **✅ The no-items case is closed too (run 200).** Also reads `RvGameSharedHUD.mHealthPackCount`, so
     the window is skipped when the player has no packs. Both reasons a heal does nothing are now checked.
   - ⚠️ **Chose the HUD's count over `RvGameSharedInventoryManager.mCounts` deliberately.** `mCounts` is
     an array indexed by an `ERvCountItem` enum, so using it means guessing which slot is the health pack.
     `mHealthPackCount` is a single int whose name states what it holds, and a frame of HUD lag cannot
     matter for a check made once at a button press.
   - ⚠️ **Both checks fail OPEN.** If either property cannot be read the window opens anyway and the log
     says so — a *missing* animation is a worse failure than a spurious one.
12. **The TMD**, when the game reaches it. Left-hand device, so it needs the same treatment driven
   by `g_handPoseValid[0]` instead of `[1]`. Everything generalises; it is unverifiable until that
   point in the game, and `Singularity.exe <map>` makes reaching one practical.
13. **Decals — leave alone unless it bothers you.** Five theories are dead (shadows, `G16R16`,
   `ScreenPositionScaleBias`, `vs c13`, unhooked user-pointer draws) and the workaround is free:
   turn **High Quality Decals** off in the in-game video options.
14. 📋 **OPEN — has SteamVR or OpenVR actually been tried against this mod? No.** Every run in
   this project's history has been on VDXR. "SteamVR/OpenVR has no 32-bit runtime" was inherited
   from the same assumption on the sibling Mirror's Edge mod — and a modder there has since
   reportedly gotten it running via SteamVR, mentioning OpenVR as well. Neither claim is confirmed
   here first-hand; that cuts both ways, and it does not prove either runtime works with *this*
   mod. It does mean the "no 32-bit runtime" line was never independently tested for Singularity.
   Worth an actual attempt before it gets repeated as fact again. Not started.

> **RETRACTED (run 136).** This list used to end with *"one object still culls early at distance —
> PAGE DOWN fixes it, which points at the run-21 mechanism: FOV inflation makes objects project
> 3.4× smaller than the engine expects."* The run-21 mechanism is **not** the cause. Two
> measurements kill it: sweeping the engine's asked FOV over 0.35×–6× changed nothing while the
> engine demonstrably took the request, and the draw count is identical at two resolutions that
> look completely different. PAGE DOWN "fixing" it was the coincidence that kept this alive —
> PAGE DOWN changes the forced matrix as well as the ask, and it is the forced matrix that matters.

**Watch this one:** MinHook now initialises in `DllMain` (run 59, for the command-line detour)
rather than at first `Present`. `InstallViewHook` tolerates `MH_ERROR_ALREADY_INITIALIZED`
accordingly — treating it as a failure would silently kill head tracking. It is the newest
structural change in the build and the first thing to suspect if tracking ever misbehaves.

## Measured, so it does not have to be re-derived

| Quantity | Value |
|---|---|
| View matrix | `c0`, **ROW** storage, **translated-world** space |
| Eye separation | 3.32 UU = 6.3 cm (correct IPD), stable |
| Headset wants | **2496×2688 per eye** → 4992 wide for side-by-side |
| Per eye at 2560×1440 | 1280×1440 — 51% of the panel |
| Per eye at 3840×2160 | 1920×2160 — 77% of the panel |
| Resolution cap | **RETRACTED — there isn't one at these sizes.** 4096 ✅ · 4608 ✅ · **4992 ✅** (run 58), with `D3D9ExMode=1`; the scene targets follow every time. Run 33 measured its refusal on the *plain* D3D9 device, before the Ex upgrade |
| Per eye at 4992×2688 | **2496×2688 — 100% of what the headset asks for.** Parity reached in side-by-side; no per-eye targets needed |
| Frame aspect | Keep per-eye ≈ **0.93** (2496/2688). A taller frame at fixed width narrows the horizontal field — 4096×2688 renders 82.5° vs 95° at 4096×2160 — and the shortfall shows as **black bars at the sides** |
| Frame rate | **120.0 fps LOCKED at 4992×2688 (full parity)** — median 8.34 ms, `our work` 3.02 ms, **4.35 ms idle**. Needed both zero-copy (run 39) and a **dedicated router** (run 56). SSW **off**, 120 Hz. Every figure before run 32 was SSW halving the rate and is void |
| Frame copy | **0.00 ms** under zero-copy. Was ~9.8 ms of a ~16 ms frame at 4K on the CPU path |
| XR submit (`xrBeginFrame`+`xrEndFrame`) | **0.08–1.28 ms** on a dedicated router. Was **20.94 ms mean / 189 ms worst** on shared WiFi — and it was pooled into "our work", where it looked like a game-side stall for a whole day (run 56) |
| Engine FOV | horizontal at 16:9, dips to its 65° default ~7% of samples |
| Scene targets | 2× scene-sized (A8R8G8B8 + A16B16G16R16F), shadow map 512×512 R32F |

## ⚠ The ini takes the FIRST key of a duplicated name, silently

Hand-editing `SingularityVR.ini` can leave two lines with the same key. `GetPrivateProfileInt` returns
the **first** and reports nothing about the second, so the file reads as if the change was made and the
mod behaves as if it was not. This cost a run at 219b: `ScopeDrawCensus` appeared twice, the first was
`0`, and a census that was believed armed recorded nothing.

`PanelSave` is not affected — `WritePrivateProfileString` edits in place — so this only bites
hand-edits. Check before trusting one:

```bash
awk -F= '/^[A-Za-z]/ {print $1}' SingularityVR.ini | sort | uniq -d
```

Anything printed is a key whose later copies are dead. And the log settles it either way: every setting
prints its **effective** value at startup, so `ini: ScopeDrawCensus=0` next to a file saying `3` is the
duplicate announcing itself.

## Quick start

**The live build is `spikes/view_matrix/`** (spike 9). **`spikes/vr_render/`** (spike 8) is the
known-good fallback if spike 9 misbehaves. Earlier spikes are kept deliberately — several
document approaches that did *not* work, which is most of their value.

```powershell
.\spikes\view_matrix\build.ps1 -Install     # builds x86, copies into the dev game copy
```

Connect **Virtual Desktop** first — VDXR is what every tested run here has used, and Meta's own
runtime crashes in `xrCreateSession`. ⚠️ **"SteamVR/OpenVR has no 32-bit runtime" was carried here
as settled fact and it is not.** Nobody has tried either against this mod. See item 14 below, and
re-read *"Almost every hard problem here was a stale premise"* at the top of this file before
repeating the claim again. Then launch:

```
R:\SingularityVR-Dev\Singularity\Binaries\Singularity.exe
```

**No arguments needed** since run 59 — the resolution is inherited from the headset. Pass `-ResX`
only to override it deliberately.

> ### 🛜 Virtual Desktop needs a dedicated router, and this is not optional
>
> On a shared WiFi network `xrEndFrame` blocked for up to **189 ms**, which read as seconds-long
> stalls and cost an entire session of misdiagnosis. With a dedicated router and the PC on
> **ethernet** to it, XR submit is **0.08–1.28 ms** and the frame locks to 120 fps.
>
> The `XR submit` figure in the frame budget is the check. If it is anything but small, the link is
> the problem and **nothing in the render code will help** — see run 56.

**The cold start is VR** since run 159 — `StartInVr=1` is the default, applied at the first Present
and gated on an OpenXR session existing, so a machine with no headset still comes up flat rather
than frame-split. `VR MODE` is the first panel row and it saves.

**BACKSPACE is now only a toggle**, for peeking at the flat game — it deliberately does *not*
save, because a key you press to look at something should not change how the game launches
tomorrow. Toggling off drops the forced projection with it; a mono image stretched to 98° looks
broken rather than standard, which is expected.

> Run 37 reverted run 36's default-on because it rendered the untested main menu frame-split on
> every launch. Run 159 reinstated it on a checkable change, not a change of mind: **run 151 put
> the HUD and menus into both eyes**, so the thing run 37 objected to is the thing run 151 built.
> `StartInVr=0` restores the flat cold start with no rebuild.

Individual toggles, unchanged: **F9** head tracking · **F10** 6-DOF head position · **F1** true
stereo (draw duplication) · **F12** alternate-eye stereo ·
**F6** VR-correct projection · **F8** recentre · **F5/F4** flip yaw/pitch sign ·
**F7** scan for the view matrix · **F3** constant offset probe · **F11** offset amount ·
**INSERT** blank one eye half (mapping test) · **DELETE** swap eye/half assignment ·
**PAGE UP** cycle culling headroom (now runs *down* from 1.0) · **PAGE DOWN** cycle how much of
the headset's FOV we render · **HOME** scissor off (drawn-then-clipped vs never-drawn) ·
**END** clip out everything driven by the tracked register (remapped vs unremapped) ·
**NUMPAD1** head roll on/off · **NUMPAD2** flip the roll sign ·
**NUMPAD3** gamepad emulation (on by default since run 61) · **NUMPAD4** walk-forward test ·
**NUMPAD5** stick-look test.

Controller settings live in an `[Input]` section of `Binaries\SingularityVR.ini`: `Gamepad`,
`TurnMode` (0 smooth / 1 snap), `TurnAngle`, `TurnSign`, and a 360 button mask per action
(`Jump`, `Crouch`, `Reload`, `Use`, `Menu`, `Sprint`, `LeftX`, `LeftY`, `Melee`). Raven's own
console layout is not documented anywhere, so the masks are a first guess kept editable rather
than a rebuild.

**Do not press F12 after F1.** F1 clears alternate-eye, but F12 does *not* clear duplication, so
that order leaves both stereo modes running at once. F12 before F1 is harmless and pointless.

Mouse and head tracking now **add** rather than fight — turning with the mouse no longer snaps
back, and the same fix is what stick-turning will need in the shipped mod.
Log: `C:\Users\<you>\AppData\Local\SingularityVR\view_matrix.log` — **read it directly from disk**;
it does not need pasting into chat, and it must never be pasted into this file. Truncated at each
attach and stamped with the run's date and time on line 1, so a stale file is obvious at a glance.

**Every run is kept, named for when it started.** The live run is always `view_matrix.log`; at
the next attach it is renamed to `view_matrix_YYYY-MM-DD_HH-MM-SS.log`, the stamp read back out
of its own line-1 header. Newest archive = most recent finished run. Nothing is ever evicted, so
**a reference to a specific log stays valid permanently** — cite them by name in notes.

The old `view_matrix.prevN.log` files are the previous scheme and are left alone; `prevN` named a
*position*, so those names meant something different after every launch. Pruning is manual and
deliberate — nothing deletes logs automatically, including the 24-line ones, which are the
evidence when the question is "why did it die at startup". Uninstall = delete `d3d9.dll`.

Paths, toolchain and game copies: `ENVIRONMENT.md`. Nothing is tied to the repo location.

## What works, and how

| Piece | Mechanism |
|---|---|
| Frame into the headset | backbuffer → `GetRenderTargetData` → SYSTEMMEM → D3D11 → XR swapchain |
| View **pitch** | MinHook detour on `FUN_0104e390`, the view-rotation source |
| View **yaw** | direct write to the live controller `+0x64` (the detour's seam carries pitch only) |
| FOV | write to `mCurrentPOV` FOV, `+0x0438` |
| Finding live objects | `GObjects` walk; live actor = `Outer == PersistentLevel`, name lacks `Default__` |

## ✅ Camera POSITION — solved 2026-07-29

This was the blocker for **both 6-DOF and stereo**, and it is now controllable. Not through the
engine's object model — three attempts at that failed — but by intercepting the **world→clip
matrix** on its way to the GPU and pre-multiplying a translation into it. Confirmed live: the
view moved.

| Fact | Value |
|---|---|
| Where | `SetVertexShaderConstantF`, device vtable slot 94 |
| Register | `c0`, ROW storage (registers are matrix rows) |
| Space | **translated-world** — UE3 pre-subtracts the view origin on the CPU |
| To move by `o` | `row3 -= o.x*row0 + o.y*row1 + o.z*row2` |

Findings from the same runs, all in `ENGINE_NOTES.md`:

- **The engine renders in translated-world space.** Probing the world camera position alone
  would have found nothing; the origin probe is what made the scan work.
- **The convention was settled by a direction test**, not the position test — `c0` read as ROW
  gives `dotFwd = +1.0000`, read as COL gives `+0.0023`, from identical bytes.
- **CPU frustum culling is a non-issue at stereo scale.** Measured from the backbuffer: a
  1.4–2.9% band at 300 UU, and **0 px at 100 UU and below**. The planned FOV-widening job is
  therefore **dropped** — a real IPD is a few UU.

## ✅ 6-DOF — working, confirmed 2026-07-29

The headset's real position drives the view (F10). Leaning and ducking move the camera.

## VR-correct projection — what it does

Scale could not be judged in the headset because the projection layer was claiming the
**headset's** FOV for an image the game rendered at a different one — a uniform stretch that
made head movement, world scale and object size all wrong together, so none could be measured
against another. Now the submitted frustum is *derived from the matrix* (read, not assumed) and
the game is asked for a vertical FOV matching the headset so that truth also fills the view.
Toggle with **F6**.

## Method notes that earned their keep

- **Validate the test environment before trusting the instrument.** SSW halved every frame-rate
  number this project recorded for thirty runs, and nothing in the log looked wrong — the timings
  were honest, the frame itself was synthetic. Run 31 built a correct instrument, measured
  carefully, and drew a confident conclusion that was wrong at the input. **A measurement is only
  as good as the thing being measured is real.** Check pacing, refresh, reprojection and frame
  generation *first*, and record them next to the numbers.
- **Measure the thing before building what the measurement justifies.** "The frame copy is the
  bottleneck" survived thirty runs and became the top-priority work item without anyone timing it.
  That instinct was right — but see above, because the first attempt to act on it still produced a
  wrong answer, and being right about *method* did not save it.
- **A stall that leaves your instrument has not left your frame.** Pipelining the readback took the
  copy phase from 4.2 ms to 0.89 ms and moved every one of those milliseconds into unmeasured work,
  for no net gain. Any phase timing that improves while the total does not has probably relocated a
  cost rather than removed one.
- **Rotate logs that are truncated on attach.** Every measurement here is an A/B across launches,
  so a log that truncates destroys the run being compared against — silently, since the file still
  looks complete. Run 31 lost two of three tests that way.
- **Audit the instruments before proposing another mechanism.** Six theories died on their own
  diagnostics; the seventh was found only after run 27 checked whether the diagnostics could
  report anything at all. Three could not: `remapKnown` was `const bool ... = true`, the perf
  line's `%s` was eating an integer argument, and `camera matrices tracked` is capped at 1 by
  construction. Two of those had been quoted in this document as *evidence*.
- **Build discriminators, not hypotheses.** A test that separates two whole classes
  (drawn-then-clipped vs never-drawn; remapped vs unremapped) retires more in one session than
  three rounds of argument. Every step from run 28 on was one of these, and each killed a class.
- **Narrow by causality, not by symptom.** The answer was named before it was tested: the object
  not being drawn is a CPU decision, both of our CPU-side influences were already eliminated, and
  query readback is the only route by which a GPU-side change can reach a CPU decision.
- **A test that destroys its own readability is a wasted run.** Collapsing `c0` to a point piled
  every triangle onto a few pixels and let bloom smear the result over everything. Clipping the
  geometry instead — same intent, legible frame — answered it immediately.
- **Override the answer, not the availability.** Refusing to create the occlusion query crashed
  the game; patching `GetData` to lie ran the identical experiment with the engine's control flow
  untouched. Prefer the intervention that leaves the program on its normal path.
- **Check which rows of the log you are reading.** The 51.1° culling figure that briefly revived
  the shortfall theory came from a pre-gameplay frame. The instrument was right; the row was
  wrong.

- **Verify by writing, not by reading.** A struct holding plausible live values is not proof the
  renderer uses it. `mCurrentPOV` looked exactly like the camera and steers only FOV.
- **When guessing fails twice, change tools.** A hardware write breakpoint found the rotation
  source immediately after seven failed guesses.
- **Shotgun, then narrow.** Testing candidates one at a time behind a hotkey let a working case
  get cycled past unseen.
- **Doc errors become code bugs.** `ENGINE_NOTES` recorded the backbuffer as `X8R8G8B8`; it is
  `A8R8G8B8`, and that mistake propagated straight into a `D3DERR_INVALIDCALL`.

# Headset TMD regression: provenance and isolated source correction

CPU/read-only while user tests. No launch, input, deployment, XR, registry or
user-state change. German work paused. Own branch starts at exact1b03a2f4.

## Installed versus recorded acceptance

Installed DLL remains C6C1A5041F544CCFCA23DD5D3F475E989603698067504D4D65A813AFC4552B3B,
INI D8DBA65311B426B6C633C9685F30E54B8FF8B54C85D8B5580815BF97AEF1F453.
Nov9 install report20261109-173042 replaces374A8B48 withC6, DLL-only/no INI;
producer thin-laser-dot receipt records precommit1df30254 plus dirty laser changes
later committed1b03a2f4. Its acceptance scope explicitly says laser physical
appearance pending and preserve existing baseline; it is not an all-features
physical-acceptance record. Do not infer that from the approved-profile schema.

Recorded hand/geometry commits3f1399ff,61cc6743,c3f60c0e,3c386bae,15a99557 and
gravity diagnosticfa4c3151 are all ancestors of1b03. No missing accepted fix
commit or accidental historical rollback was established by this search.
Historical tmd-sky-gravity-hands-20261107 handoff distinguishes accepted7AD356F8,
physically failed7F9FD554, and9D2C568D user-confirmed video/rotation/shooting.
21963140 wrist hold andE0E08775 decal suppression are follow-up candidates;
the handoff does not record physical acceptance of all sky/gravity defects.
This does not override the user's recollection: the exact separately accepted
sky producer, if any, has not been identified. Earlier generic C6 acceptance
wording was broader than the retrieved evidence supports.

## Fresh evidence and proved discontinuity

Ignored captures/user-headset-20261110 archives the current PID54060 log snapshot,
current INI, three supplied JPEGs and shadow-pass54060-1..3. Live log is a
timestamped read-only snapshot, not a claim that its EOF is process termination.
Original full BMPs are referenced/hashes preserved separately; small JPEG view
derivatives do not replace raw evidence. Actual15:48:41 SBS shows the black sky
strip in both eyes; gravity-input frame1735 does NOT show the reported black
rectangles and cannot establish their producer. Decal dropping is enabled and
logs actual drops before the sky capture; it is not a universal artifact fix.
No shader/material suppression change is proposed from these images alone.

The log shows successful Impulse, hands1, then support ACQUIRED forValkyrie11387.
Current profile enables DualHandActionPose and TrackedGripWrist. Production
Eligible unconditionally rejects support, causing CaptureDualActionPose to
clear the frame and BuildGunC/right glove to lose tracked action correction.
This proves a path discontinuity and a reproducible loss of correction; it is
not a measured full explanation of the visible hand's final pixels.

Default-OFF Input/TrackedGripSupportContinuity requires TrackedGripWrist. Only
the right correction continues while support uses the original same-frame
source capture. Left stays owned by existing authored/fore-end target. All
tracking/native-animation/carry/holster/identity/frame/render-thread gates stay.
No radius, timer, input, engine hook, classifier or sky behavior change.

## Verification / limit

Small x86/O2 CPU build only, no production DLL build during user's game:
initial production-header fixture26checks/5failures/exit1; candidate initial30/0,
then exhaustive flag-OFF truth table added: final542/0/exit0. The additional four
side controls specifically keep left support-owned and free/OFF unchanged.
Existing test-dual-hand-action-pose passes changing camera-local/turning ride,
four weapons/two hands/eyes/child animation/stale controls; nine existing wiring
tests pass. Exact commands/logs live in build/tracked-support-cpu/red.cmd,
red.log,green.cmd,green-final.log. Run only this CPU subset; full suite includes
OLED GPU tests and was NOT run. git diff --check passes.

Source candidate only: no new DLL/hash/runtime/headset acceptance, no install
recommendation yet. MAIN reviews this source and decides any later bounded
build/deployment. Preserve current user files. Black TMD rectangles/sky producer
and actual support-pixel result remain OPEN.

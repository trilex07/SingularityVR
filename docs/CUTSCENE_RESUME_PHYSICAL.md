# MAIN resume: round 3, no mid-session chat

Inherited candidate, no renderer changes in this resume:
`fix/cutscene-mono-presentation` @ `3df65c843cf3b4aa1b6f223d5527cf68c809ee2a`,
x86 PE32 /O2, DLL SHA256
`072ED34ECEDD29E18E7BB895A4D512C73ADEEA168DB5C2A0572B2CD2DF6F6FED`.

User explicitly says round 3 has NOT been tested. Earlier new-game report:
intro videos too large/close; helicopter lacks cinematic presentation,
~20 FPS, visible tracked gloves/gun, movement possible, head yaw reversed.
These are separate unverified mechanisms, not one presumed compositor bug.

Hypothesis inherited from round 2: Matinee camera changes clear the cinematic
latch despite pawn continuity. Candidate preserves it across camera cuts.
Falsifier: latch stays ON but CutsceneFlat/composition does not engage, or the
same bad image remains with composition engaged. Rendering success logs do
not establish correct pixels, cinema depth, or head direction.

L1 rerun: mono-fit/predicate and scene-mode unit executables PASS.
Installed DLL equals inherited artifact/manifest; INI `C59B1CF6...` and native
UI `5B2EA469...673CDE` unchanged. Independent restore guard returns only DLL
to verified merged-main baseline `D8928180262706A95972D13B728A1867F1EAB1757DCFD6C4A32D99EFBFBEC1BE`
after observed exit or no-launch timeout. No process killing, runtime/registry
change, save editing or window/input automation.

Physical route: new game -> intro videos -> helicopter -> first gameplay.
Stay in PSVR2 until closing game. Observe whole image/proportions in each eye,
head direction, gloves/gun, FPS, then stereo/control and pause/options/resume
after transition. Abort if uncomfortable. One feedback report after exit.

`tools/watch-cutscene-session.ps1` observes existing logs only for a bounded
window; tail timestamps are observation times and can miss bursts. Final
session log (max 32 MiB) supplies complete transition evidence. It cannot
measure GPU time or establish physical acceptance. No new diagnostics flags.

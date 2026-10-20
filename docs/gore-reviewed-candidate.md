# Reviewed gore + stronger R1 candidate

SIDE source reviewed: 9c75c68. Not merged verbatim.

Review found default-ON gameplay changes, unchecked property types, an unproven
`g_upOffset+4` packed-bool mask, unrestricted TakeDamage targets, unchecked write
ranges, permanent edits of shared CDO/settings, and success logs without actual
sever evidence. Resolver retry was also inside a player-offset gate that can
already be closed. These are not acceptable deployment assumptions.

Independent offline inspection of the owned decompressed RvGame package confirms
FloatProperty exports 10622/10623 (random chance/health threshold), BoolProperty
2309/2310 and Function 3074 AllowGore. Serialized properties do NOT prove a runtime
BoolProperty mask offset or that overriding the script result affects native calls.

This candidate therefore implements only the audited typed-float subset. During
TakeDamage on Soldier/Revert/Zombie class chains, after excluding player and
CombatNPC classes, type-check mLimbSettings and both FloatProperties. Require exact
RvLimbSettings class, finite plausible old values and writable target ranges. Set
chance/threshold for the synchronous damage call and restore afterwards, including
nested call behavior. No CDO, packed bool, global AllowGore or UseLowGore writes.
No object-table scan. Logs bounded to 12 successes and 12 refusals per process.

Hypothesis: native damage evaluates these settings synchronously. Falsifier:
scoped receipts occur but no native severing follows appropriate limb hits, or
the damage seam never fires. Then audit the native damage/limb consumer rather
than broadening writes or claiming the original SIDE implementation is verified.
Native censorship/unsupported enemies may still refuse gore. Alternate limb
settings, automatic death severing and TMD sever-all are NOT implemented here.

Default OFF: Gameplay/GoreDismemberment. Chance defaults to 100 but is inert OFF.
Candidate preparation explicitly enables it and Input/WeaponSteadyPrecision=2.
R1 behavior remains Kasimov + two-hand support only; HMD pose is not filtered.

Physical test, after a fresh start authorization: load an ordinary enemy scene,
test a few aimed limb hits; then Kasimov two-handed R1 aim/release and a deliberate
large reposition. Check native gore, stability, shooting, stereo and FPS, close
normally. No mid-headset chat. TMD sever-all is not part of this test.

Stable Steam installation and main are untouched during offline preparation.
L1/L2 never establish native severing or physical aim comfort.

## Preparation receipt

Source commit: `2539ee4`, branch `fix/gore-stronger-r1-integration` (pushed).
Candidate directory: `build/combined-followups/gore-r1-2539ee4` (local, ignored).
DLL SHA-256: `CF7AA4665C1C3C9D398BADC98C3B8C7A725EFC951FE1DB90FB27872B3D815D91`.
Clean-source x86 PE32 `/O2` build PASS; existing static-analysis warnings remain.
L1: 13 optimized CPU fixtures plus 24 Python regression checks PASS, including
mocked scoped settings restoration, nested calls, typed-field refusal and NPC
exclusion. These are not actual engine execution evidence.
Official deployment `-WhatIf`: accepted-profile preflight PASS, no files written.
Change plan: DLL plus only WeaponSteadyPrecision=2, GoreDismemberment=1,
GoreChancePct=100. All other promoted config/assets retained.
Installed hashes verified unchanged: DLL `EF3A1F3B…721B7B753`, INI `55A3ED4D…E3F659`.
L3/L4 pending. No deployment, game launch, main merge or stable promotion.

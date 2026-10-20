# Valkyrie visibility / stronger supported Kasimov R1

## Physical2232 feedback and safety

F7D26BEB...D1E9 ran through Steam PID10776, OpenXR confirmed. User reports
Kasimov direction improved, but R1 insufficiently steady; Valkyrie still feels
inaccurate and not every round appears visible. This is NOT aim/FX acceptance.
Private full receipt: measured-aim-13db0f0/physical-feedback-2232.log.

Game closed; guard20261020-223207 restored. Independently hashed installed
DLL7A1DE5854D1EC8635EE77200E3A39F0D4256C3A73ED6A69E99D3C26417B69495 /
INI60224CA08A8E9B01E299AD4B14A069A1E854F2B5E46ADFBB0B01A86973A74ADC.
No new Steam writes/launch or accepted-profile change in this follow-up.

## What is actually proven

Scope geometry receipt barrelDot~1 proves internal axis agreement, NOT impact
pixels. R1 held1/twoHand1/sniper1 proves the action reached the existing filter.
The previous65ms filter tested mostly alternating noise, not low-frequency
human tremor or the supported axis derived from TWO controller positions.

Owned native PlayTracerFX1694/B7DDF9B80E425AAB first gets the source and later
tests template+48, calls its random helper and compares chance+4C. Failure
skips the cosmetic particle spawn at+2EC, not the collision ray. A source-hook
counter therefore does NOT count visible tracers. Original Valkyrie.u export304
(Shared_ARMachineGun FX) has mTracerChance0.8; export1096 belongs to
SP_ARMachineGun and overrides it to0.5, archetype304. Do NOT report80% as the
live single-player value. Read-only reflection must prove the current object's
property names/offsets and value before attributing a particular missing smear.

GetWeaponSpread's complete117/A07CE9D71DC4EC2A includes both return paths.
Native AddSpread536/90E1F845D48FC976 applies random radial dispersion before
the collision hook. Shared rifle defaults include mSpreadMin0.15, max0.5,
mZeroSpreadOnFirstShot=false, and ironsight-specific values. These are native
parameters, NOT an established degree-unit calibration or live SP/perk values.
The eight prior rifle ray receipts had muzzleDelta<0.001UU and aimDot~0.99786
to0.99992. They establish translated origins and non-zero outgoing deviation;
they cannot distinguish all authored spread/recoil from a residual axis error.

Auditor: tools/fx/audit_rifle_scope_backend.py, private result
build/native-muzzle/tracer-visibility-audit.json. It exports contracts/default
values, never original code/assets. Full fingerprints refuse altered inputs.

## Controlled implementation

Only the STRONG branch changes65->200ms: R1 + two-hand Kasimov, existing
default-OFF WeaponSteady. Mild18ms, input routing, spread, damage, native FX,
scope projection, HMD pose and compositor timing remain untouched. Both hand
positions/quaternions are filtered before shared aim/render consumers. Existing
adaptive gain, >4cm/>0.20rad snap, invalid/focus/weapon/gap/release resets remain.

Synthetic5/8/12Hz opposed3mm tremor,25cm support baseline, sample rates50/90/120Hz:
supported angular RMS0.077--0.183 of raw (82--92% reduction). Quaternion tremor
also bounded. A deliberate10deg/s turn has maximum1.437deg lag; releasing R1
immediately re-seeds. These are CPU results, NOT physical comfort acceptance.

Valkyrie has NO speculative accuracy/cosmetic override in this candidate.
Eight existing exact-source receipts now also report reflected tracer chance,
template and native spread current/base/min/max. No additional hook, poll,
particle spawn or native chance/spread write. Unknown fields log valid0, not
invented defaults. Source receipts run before the original random decision.

Hypothesis: native stochastic tracers explain at least some missing streaks;
native spread contributes to perceived inaccuracy. Falsifier: actual SP chance
is1, or a zero-spread/currently stationary shot has a repeatable axis bias.
Do not label every miss native spread without that distinction. A future choice
to show every tracer or reduce VR spread changes original presentation/balance
and must be explicit, not silently bundled with an alignment repair.

## Next physical route (fresh start only)

Stay in headset for the entire session. With Kasimov supported, compare R1
released/held at a stationary target, then deliberately aim elsewhere and
release R1: no stuck aim, target/optic/hit should stay coherent. With Valkyrie,
use stationary separated single shots, then one short burst at the same wall;
compare hit locations separately from visible streaks. Exit normally; MAIN
collects bounded live chance/spread receipts and verifies restoration. No
Deadlock/heavy test claim: their native integration remains incomplete.

## Offline delivery receipt

Clean code011876d01b26156708f40c57b1662c313788ebba, x86 PE32 /O2,
no performance instrumentation. Outside-Steam directory:
build/combined-followups/strong-r1-011876d.

- DLL EB2EAA430AC465974356D73BBC19E8974E358B0A51003CB8CDA7636861D6F41D
- INI 7412EE4A43F73770A08139B999CCC80B988BB484AFC1EDD33BC2ECCB99583835
- plan 1ED2C60A6C3F69D0C60F7D39C3D5742CFA9A880F842C37F85B56E89CA4FADBF2
- preserved physical2232 log 8D71EED515D7A79374127B5EADF64DA8128866AE4E7A479D1202F2F4EC9612F9

Native FX/steady optimized fixtures and Python guard suites PASS. Full
test-centurion.ps1 with private PreparedRoot PASS, including all four weapons,
original animation/first-shot, own-process x86 ABI and both-eye GPU replay/Reset.
Preparation58 / fake lease6 / carry wiring3 PASS. Complete original native
tracer/spread auditor PASS. Accepted-profile Install -WhatIf PASS;145 pins,
no actual installation or launch. BodyEquipmentProbe remains OFF: unrelated
read-only equipment census is unnecessary in this rifle/steady round.

READY_FOR_PHYSICAL_ACCEPTANCE applies to stronger R1, not a claim of repaired
Valkyrie accuracy. Known native stochastic tracers are NOT a dropped-shot fix.
Require fresh start; verify backup/deployed hash, arm a new guard, preserve
all accepted assets. Installed accepted DLL/INI remain unchanged.

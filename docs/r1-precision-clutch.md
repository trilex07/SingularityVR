# Supported Kasimov R1 precision clutch

Physical feedback from EB2EAA43: rifle VFX and weapon handling accepted for
this test; scope-to-hit correspondence remains uncertain and R1 tremor persists.
Do not describe the optic as ballistically verified. Private session log retained
at build/combined-followups/strong-r1-011876d/physical-feedback-2305.log.
After exit, installed DLL7A1DE585 and INI60224CA0 independently verified restored.

User requests a stationary hold on R1 unless hands move substantially.
Smallest experiment: replace only the existing STRONG supported-Kasimov branch
of WeaponSteady (default OFF). Hold each controller's position and quaternion
exactly within 12mm / 0.025rad of its latched pose; crossing either threshold
reanchors to raw immediately. Comparison is against held pose, so even slow
deliberate movement eventually escapes. No time-dependent accumulation inside
the envelope. Release, support loss, identity change, invalid tracking or gap
reseed through existing gates. Mild filtering unchanged. Never filter HMD.

CPU falsifier: repeated subthreshold tremor changes either held pose; deliberate
motion never escapes; release retains a stale pose. Tests cover both translation
and quaternion escape, sample rates50/90/120Hz, 5/8/12Hz opposed hand tremor,
identity/gap/reset and intentional10deg/s motion. Exact zero tremor in these
synthetic samples; maximum intentional angular lag1.430deg. Not L4 proof.

Important limit: independent hand thresholds permit a step when either hand
escapes. Physical comfort and small aiming corrections require a headset test;
do not mistake a synthetic exact hold for world-space zero camera/recoil motion.

Scope log barrelDot near1 is input-axis agreement only, not a scope-pixel/hit
validation. Keep optic alignment open; do not add another speculative offset.
Next test after fresh start: supported Kasimov R1 hold/release/clear reposition,
then stationary-target scope and actual impact comparison. Preserve native spread.

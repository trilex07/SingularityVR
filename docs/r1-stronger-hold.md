# Optional stronger supported R1 hold

Stable baseline main 8ffa2d1 / tested runtime a53a180 DLL EF3A1F3B is preserved.
User accepts scope alignment but requests more stability. Runtime receipts prove
supported-Kasimov R1 gate was actually held, not merely an INI flag.

Candidate-only Input/WeaponSteadyPrecision=2 widens the exact held-pose envelope
from 12mm/0.025rad to 30mm/0.060rad (3.44 degrees). Mode 1 remains default, so the
accepted installation is unchanged. Strong mode still requires R1, two-hand
Kasimov, valid tracking and no reload/holster/menu conflict. Head/eyes untouched.

Large motion reanchors immediately; R1 release, identity, tracking/time gaps or
changing precision mode reseed the raw pose. No extra RTT/render work added.
CPU tests prove exact hold at 29mm/0.055rad and deliberate escape/release.
Not physically accepted: wider thresholds can create larger reposition steps.
Physical check: R1 hold at distant target, deliberate reposition, release.

# Measured aim and physical equipment follow-up

## Brief / falsifiers

Physical2016: scope draws and magnifies, but does not point where the rifle
points; Valkyrie also appears inaccurate. The scope latches a controller-local
mount while BuildGunC can follow the two-hand solution and mesh-only trims.
GunAimDir uses Euler aim without those trims. Hypothesis: these independent
axes diverge. Experiment: default-OFF measured pose/axis from the exact original
weapon palette and BuildGunC, published only after a successful draw/restore.
Preserve native spread with a rigid direction rotation, not a zero-spread ray.
Falsifier: measured axis receipts agree yet optical/impact alignment still fails.

User equipment layout: ammo left shoulder, grenade right shoulder, heavy weapon
lower back behind right hip; existing knife/ordinary-gun hips remain unchanged.
Grenade: right grab, left grip at pin then pull, right swing/release. A gesture
must not create inventory or arm a fuse before its native mechanism is proven.

Offline original package control.u confirms distinct enum slots HeavyWeapon2 /
GrenadeSlot3, native-heavy classification and projectile functions. It does NOT
establish a carried frag grenade or its pin/fuse ABI. Grenade-steerable and
Deadlock props are distinct classes; never treat TriggerActiveWeaponThrow as a
grenade launch (it can discard a weapon). Physical-equipment policies are built
independently; gameplay delivery remains gated on native ownership/fire evidence.

No Steam writes or launch in preparation. Accepted profile is frozen. No native
offset, function or FName may be guessed to make a gesture appear integrated.

## Implementation and evidence boundaries

- `WeaponMeasuredAim` (Input, default0): successful native weapon draw/restore
  publishes the exact barrel axis and origin. Fresh pawn+weapon identity and
  <=150ms lease required. Rotate the already controller-aimed collision ray
  rigidly onto that axis, preserving its native cone and distance. Native scope
  head-aim/test modes and Centurion without a measured rifle witness stay out.
  The same direction feeds existing muzzle/laser consumers; no replacement FX.
- `ScopeMeasuredPose` (Render, default0): sample the actual rendered lens and
  optical axis each frame. Do not reconstruct the view from a previously
  captured hand-local mount when two-hand aiming or mesh trims move the gun.
  Keep the existing proximity gate, calibrated4x, 45.5Hz capture cap, HUD gates,
  R1 steadiness and original L3 slow-time limits. Missing/stale pose refuses.
- `BodyEquipmentProbe` (Diagnostic, default0): <=1Hz, <=128 chain entries,
  <=32 receipts, existing engine input thread. Original native IsHeavyWeapon
  wrapper58/B06FD8D75EF187D2 dispatches virtual554 to the complete13-byte
  getter AD9AEEB598427D84, reading flags2D8 mask80. Reflection cross-checks
  owner/chain/name fields. No native calls, new hooks or writes.
- `body_equipment_policy.h` is NOT wired to gameplay. It provides exclusive
  nearest-zone resolution, yaw-only right-shoulder/lower-back anchors,
  identity/sequence-bound native acknowledgement transactions, left-grip pin
  pull in right-hand-local space, and bounded swing/release velocity. There
  is intentionally no invented fuse, ammo count, grenade spawn or safe return
  of an armed object. A future backend must own that object's real lifetime.

The original PSK bone oracle now also checks muzzle bone +X == cog +Z for
Valkyrie, Volk and Kasimov, all within1e-5. `--axes-only` does not rerun or claim
the separate magazine/palm collision fit. Original PSK SHA256s:

- Valkyrie b445a4a2c27e9eedfc487a2cd5358b6db1bd755b184cafd663c4a2454991b442
- Volk 366f599d3777c7c8158566bc71c693076ca0c8da97b0d7746221e2a815b611e8
- Kasimov 39316f6a2fdbef91292cba42f83b62d5a9d19c2255785d441681f559bdc852ef

Offline native auditor `tools/fx/audit_body_equipment.py` requires the private
owned EXE, archived complete .text, and decoded original RvGame package.
Verified EXE BD459ACD...0F51 / text070D20A0...B0E1 / package09DE450D...6C92.
Output is hashes/contracts only, not game code or assets. The package contains
DeadlockGrenade, ARGrenadeLauncher and GrenadeSteerable classes; that is NOT
proof of the requested player's frag/pin/fuse/inventory path. In particular,
the four-gun ownership mask must not be reused as grenade/heavy ownership.

## Offline gates and next physical route

Run `tools/fx/test-native-muzzle.ps1`, full `tools/reload/test-centurion.ps1`
with the existing private PreparedRoot, candidate/lease fixture tests and
the original-mesh/native auditors. Build x86 PE32 /O2 from clean source before
preparing a candidate with `prepare_combined_followups.ps1 -MeasuredAim`
(requires the preceding calibrated-optics switches). Optional
`-BodyEquipmentCensus` collects identities only; it does NOT enable gestures.

Only after fresh user start and normal verified backup/deploy: same save,
Valkyrie single shots at two distinct stationary targets; Kasimov lens at eye,
point it between the same targets with one/two hands, test R1/L3 and manual
reload; return to menu and exit. Stay in headset throughout. Read bounded
`weapon measured aim: fresh/rigid`, `weapon actual ray`, and
`scope measured pose: paired/barrelDot` receipts. Axis agreement is geometric
evidence, not optical/impact pixel or physical acceptance. No grenade/heavy
playtest claim until the actual native backend and item rendering are complete.

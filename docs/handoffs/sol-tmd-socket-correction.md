# Socket attachment correction / support parking

Base af580e6, branch fix/sol-tmd-component-support. No Steam access/deployment.

## Evidence

Original ch_inview_tmd_mesh sockets attach independent StaticMeshes to disc1,
faceplate, rail_rot, nixie and wheel bones. These bones have no skin weights in
the main mesh. UModel export of original baseplate has 729 vertices / 1096 faces;
its winding-corrected indices uniquely occur in the private decompressed package
and CRC32 is 5ffe23c5. The failed-session foreground census contains 729, 1368,
207, 251, 118 vertices, matching baseplate, faceplate_lvl_1, housing, nixie and rail.
Counts are corroboration, NOT sufficient runtime identity.

Main body uses an independent bone delta targeting BuildLiveControllerC.
Separate attachments previously used BuildTmdC, with a different anchor and
TMD-specific trims. Therefore the code does not preserve their original rigid
attachment relationship. The screenshot is consistent with this violation;
runtime CRC receipt still required to prove this correction reaches its pixels.

## Correction A

11 original attachment signatures are guarded by full index-stream CRC plus
vertex/face counts, start/min index, scene/foreground, isolation2, independent
hands, short gauntlet, no support and existing off-hand ride. Verified attachments
use ride3: the physical-left BuildLiveControllerC in BOTH stereo world-matrix
applications. Native materials and socket animations are retained. No changes
to BuildTmdC/native aim publication, right gun or shadow path.

Cache retains at most16 IB references until device reset, one read-only lock per
resource, no repeated retries on failure; failures retain old rendering. Reset
releases references. No geometry or proprietary textures committed.

Limitation: shared/suballocated IBs or changed topology fail closed; unsupported
variants require measured identities, not adding vertex counts. Shadow draws
are not changed by this candidate. Physical stereo/shadow acceptance is open.

## Correction B

During HANDS_WEAPON support the log confirms the fore-end target route. Code then
applied GloveLeftShift=12 AFTER that target, displacing the glove away from its
calculated support position. Suppress that idle parking shift only for TMD glove
support with a successfully selected weapon delta. Authored pair and fallback
behavior unchanged. This does not claim to lengthen the native short closed cuff.
No new caps, hardware visibility during support, IK or support-offset edits.

## Validation / MAIN gate

- x86 PE32 /O2 build PASS.
- CRC fixture and private 11-mesh cooked roundtrip PASS.
- Short-gauntlet dispatch, seven-bone sleeve and routing regressions PASS.
- Runtime CPU suites: TMD, holsters, cuffs, reload, measured aim, WeaponLocker PASS.
- D3D9/D3D9Ex isolated fixture PASS outside sandbox (HAL creation blocked inside):
  two7110 draws and3304 pixels per eye, restored IB. Not attachment pixel proof.
- Dual-TMD/OLED suite results recorded with delivery; no physical PASS.

MAIN: raise TMD, rotate wrist, look for socket parts staying on the device;
require `tmd-attachment: cooked socket mesh CRC verified` receipts. Grip/release
shotgun in both hands states, check glove and wrist, then fire/use Impulse. Keep
685e42dc sleeve correction and existing SVTG unchanged. If receipts are absent,
do not declare fix applied or loosen identity gates.

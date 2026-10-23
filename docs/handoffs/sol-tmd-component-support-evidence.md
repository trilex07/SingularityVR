# Remaining TMD component / support evidence

Base: af580e6. Investigation branch: fix/sol-tmd-component-support.
No rendering changes, deployment or game launch.

Latest local gameplay log establishes:
- frame 27254, HANDS_TMD/support=0: material 0 binds SVNC 3994,
  IB 655F9600; material 1 binds SVTG 7110, IB 655F8160.
  Both eyes report identical skin matrices for each material separately.
- frame 27448, HANDS_TMD/support=1: material 0 binds SVNC 3994 in both
  eyes, IB 655F9600. Both eyes report identical wrist matrices.
- frame 27930, HANDS_WEAPON/support=1: likewise SVNC 3994 in both eyes.
- Before frame 27448: authored-pair gun=8297 leftDelta=primary.
- Before frame 27930: TMD-pose glove uses fore-end target gun=8297.

These receipts are pre-draw observations, not successful pixel submission proof.
They rule out complete early visibility rejection for these particular draws.
They do NOT establish full finger palette correctness, material visibility,
clip-space placement or identity of the detached circular object.

The two physical screenshots referenced in the request are not attached to this
turn. The newest local codex-clipboard file is from October 21, older than this
test. Require MAIN's exact screenshots/session handoff before identifying the
detached part; do not substitute an older unrelated screenshot.

Next discriminating step: correlate the pictured component to original material
and skin influence topology, then compare its source/final palette against the
body in the same frame. For support, reconstruct all left finger/forearm matrices
and local-to-clip transform for both hands states, not only the wrist matrix.
Do not change support offsets on the basis of the two different target receipts
alone: their differing modes may be intentional. No additional caps: original
left SVNC remains topologically closed.

No new root cause is proven yet. No new candidate DLL or physical PASS claimed.
Preserve 685e42dc sleeve correction and af580e6 attachment shift correction.

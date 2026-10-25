# Clean candidate receipt — 2026-10-25

- Code commit: `4a1050d5d5fd70385bd386c9f2ed2e581bd5e1e2`.
- Branch: `fix/dual-hand-action-pose`.
- Build input working tree: clean (`workingTreeStatus: []`).
- x86 PE32 `/O2`, machine `0x014C`, optional header `0x010B`.
- DLL: `spikes/view_matrix/d3d9.dll`.
- SHA256: `3D58F60C29D9E429DCA2294C8F8C224C156A5D4945A6D0DF87110F360D2455A5`.
- MSVC static analysis and production build: PASS with inherited warnings;
  not a warning-free claim.
- Actual emitted WeaponLocker factory/FName stack regression: PASS;
  literal `mov eax,[eax+20Ch]`, followed by `call eax`, preserved.
- Offline regressions and precise limits: [implementation handoff](sol-dual-hand-action-pose.md).
- L1 / L2 PASS. Steam/OpenXR/PSVR2 validation NOT RUN by SIDE.

This receipt is a documentation-only follow-up; it does not change the built code.
MAIN must preserve the accepted installation and review the integration before
enabling the new default-OFF `DualHandActionPose` flag. See the handoff's explicit
draw-order and relative-IK gates; offline evidence does not prove their PSVR2 result.

# L2 support, carried magazine and Square FX — controlled candidate

Source baseline: `b20c6bb608bf6dd1e7a0d2ee0fb2637589f27909`.
Physical baseline: 2026-10-25 PSVR2 test reported all three symptoms unchanged.

## Evidence and changes

- The archived session accepted both support grips (`support-acquire ... ACQUIRED`). For L2 the native state was `HANDS_TMD`, with the left controller below/behind the gun; for R2 it was `HANDS_WEAPON`. The existing code rotated the barrel toward the inter-controller vector in both states even though the TMD-pose glove is placed on the fore-end by a separate presentation transform. In the L2 state alone, barrel aim now remains on the measured right-controller orientation. The accepted R2 two-hand direction remains unchanged. The `DualHandPoseProbe` also releases its source stage when that draw is omitted, so a later color draw can record its final bound palette. Physical confirmation remains pending.
- The carried Volk magazine's depth and color draws in one frame used different wrist witness ages (e.g. frame 3002: 16 ms and 0 ms). The previous log compared `magSkinT` only, which is insufficient to prove a visible world-space jump because the native local/controller matrices may differ. The candidate holds the first *valid* physical wrist for both draws of a frame and logs `magTrackingT`, `magSkinT` and reuse. If the carried magazine still blinks with identical `magTrackingT`, investigate draw state/occlusion instead of adding another pose offset.
- During Square, the reflected `mAgingBeam` and `mOwnerMesh` reads reported null. The game's `RvTMDPower_Age::CreateFX` actually uses the RvTMD argument at `[ebp+8]`, stores the beam at `[esi+530h]` and loads the owner component at `[esi+204h]`. The beam attachment and native `TMD_Muzzle` socket gate now use these audited reads, retaining ownership/class/thread checks and forwarding original calls. The offline disassembly and compiled DLL audit passes. The next session must show a live beam and a corrected native attachment receipt before declaring this visually fixed.

## One physical session

In the same save, perform Square once with the left hand clearly visible; perform L2 then left Grip on Valkyrie or Volk, release, perform R2 then left Grip in a similar pose; finally take a fresh Volk magazine from the left shoulder and hold it for several seconds. Close normally. Report Square effect origin, gun/support position after both trigger sequences, and magazine flicker. Do not switch to chat mid-session.

Offline tests and x86 `/O2` build are not physical acceptance. Keep the installed accepted DLL/INI as the rollback source.

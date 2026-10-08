# XR simulator status — 2026-10-08

SingularityVR uses **OpenXR**. The production DLL includes the OpenXR loader and
does not use an OpenVR runtime path. A useful reference simulator is present in
`references/Dishonored-VR/src/tools/xrsim`: it is a 32-bit mock OpenXR runtime
selected per process with `XR_RUNTIME_JSON`, with scripted poses and captures in
`.xrs` files. Its PowerShell harness is under
`references/Dishonored-VR/tools/xrsim-*.ps1`.

That reference runtime is not built in this checkout, and it is Dishonored's
runtime contract rather than a drop-in Singularity test. The local desktop
coverage therefore uses `spikes/view_matrix/tests/test-vr-motion-sim.exe` and
`test-vr-transition-sim.exe` for deterministic hand, muzzle, grip, menu-edge,
camera-loss and reload invariants. Those tests prove state and math only; they
cannot prove that a physical headset displays a shadow or that a muzzle flash is
visually at the barrel. One consolidated PSVR2 run is still required for those
observations.

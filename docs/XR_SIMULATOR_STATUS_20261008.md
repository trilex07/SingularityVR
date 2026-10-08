# XR simulator status — 2026-10-08

SingularityVR uses **OpenXR**. The production DLL includes the OpenXR loader and
does not use an OpenVR runtime path. A useful reference simulator is present in
`references/Dishonored-VR/src/tools/xrsim`: it is a 32-bit mock OpenXR runtime
selected per process with `XR_RUNTIME_JSON`, with scripted poses and captures in
`.xrs` files. Its PowerShell harness is under
`references/Dishonored-VR/tools/xrsim-*.ps1`.

The runtime was built locally with the installed Visual Studio 2026 toolchain at
`references/Dishonored-VR/build-xrsim/src/RelWithDebInfo/dvr_xrsim32.dll`.
Its standalone OpenXR probe completed 60 frames in FOCUSED state with zero
runtime errors. The reference runtime can drive actions, controller poses,
session transitions, and compositor submissions, but it does not emulate PSVR2
optics or prove that a D3D9 shadow is visually correct in the headset.

The local desktop coverage also uses
`spikes/view_matrix/tests/test-vr-motion-sim.exe` and
`test-vr-transition-sim.exe` for deterministic hand, muzzle, grip, menu-edge,
camera-loss and reload invariants. Those tests prove state and math only; they
cannot prove that a physical headset displays a shadow or that a muzzle flash is
visually at the barrel. One consolidated PSVR2 run is still required for those
observations.

Running Singularity under xrsim from an elevated launcher is not a valid visual
result: the Windows OpenXR loader ignores `XR_RUNTIME_JSON` for an elevated
process, so the production DLL falls back to its normal no-HMD path. The
simulator build and self-test remain reproducible without changing the Steam
installation.

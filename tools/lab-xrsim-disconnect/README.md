# Replaying a controller disconnect

`xrsim-touch-disconnect.patch` modifies the Dishonored VR simulator at commit `7c1cb8a32`. It includes the Touch binding baseline used for the Singularity lab and adds `hand l connected off`, `hand l connected on` and the corresponding right-hand commands. Use `hand l valid off` to lose tracking while keeping the controller connected.

Apply the patch in a separate Dishonored VR checkout. Build `src/tools/xrsim/*.cpp` with the x86 MSVC toolchain and OpenXR SDK headers. Link the DLL with `src/tools/xrsim/xrsim.def` and the D3D11, DXGI, D3DCompiler, WindowsCodecs, Ole32, User32 and Shell32 libraries.

Compile `disconnect-smoke.cpp` with the same toolchain and OpenXR headers, and link with D3D11. Run the executable with the simulator DLL path and a fresh writable control directory. It uses D3D11 WARP and validates both controllers through the runtime API. The test covers loss of pose alone, disconnect, unavailable pose while disconnected, reconnect and profile change events. Bound-source enumeration is still a stub.

This runtime is for an isolated test process. Use it with the diagnostic Singularity build to capture depth and color before, during and after a disconnect. The experiment reproduced the left profile and action state from the controller report. At the Workers' District wreck, all three phases retained matching per-eye depth and color projections and showed one wreck.

# Optical capture isolation / Valkyrie tracer source

MAIN, 2026-10-20, branch `fix/rifle-vfx-sniper-grip-scope`.
Offline repairs after physical DLL `51BF3212...78DC` failure, NOT accepted.

## Actual result, preserved evidence

User: Valkyrie flash visible, smoke absent/uncertain, tracer starts beside barrel.
Kasimov becomes a displaced black silhouette with another textured rifle beside
it; scope is not usable. Screenshot supplied in chat. Private runtime log:
`build/combined-followups/pose-repair-acc2f2b/physical-feedback.log`.

Unlike the previous session, native scope registry/ABI preflight now succeeds.
The exact owned 512 RTT/component is constructed, SetView executes, completion
epochs occur, RGB samples contain 16384/16384 nonblack pixels and nonzero alpha.
There are NO aperture-draw receipts. These are content/engine receipts, not a
working scope. Foreground riding then reports 648 bailed draws; FPS reaches 73.
Do not tune the eye-distance gate or claim that RGB proves correct presentation.
Valkyrie mode3 executes native pose updates and the user sees a flash; this does
NOT establish smoke rendering. Only accepted particle family0 routes in this log.

Accepted DLL/INI were restored by the existing guard and independently hashed:
DLL `7A1DE5854D1EC8635EE77200E3A39F0D4256C3A73ED6A69E99D3C26417B69495`,
INI `60224CA08A8E9B01E299AD4B14A069A1E854F2B5E46ADFBB0B01A86973A74ADC`.
No deployment/launch/package/asset change in this repair turn.

## Proven mechanisms and smallest changes

1. The final submitted source is the SBS backbuffer. The previous aperture code
   used the current arbitrary UE3 RT and cached matrices from the last draw.
   Draw now explicitly binds that backbuffer, projects the measured tracking-space
   lens through VrUiProjectEye into each half, and uses the existing tested
   custom-hand StateGuard for RT/MRT/DS/viewport plus GPU state and COM lifetime.
   Raw native setters/draws avoid contaminating observation caches. Restore failure
   disables the experiment; engine PlayerTick detaches the owned capture.

2. Owned SceneCapture can use full-scene-sized intermediate surfaces. Dimensions
   do not identify the player's main view. Offline disassembly proves component
   vtable factory+164h -> constructor -> capture-info vtable render+4; its target
   is capture-info+10h. Exact whole-code contracts guard a native render callback:
   factory205/C87668CD09D29B85, constructor195/F2ECDE1DE85EA63A,
   render737/9CCA1CFB4E9B6123. Only the exact nonzero owned target gets a nested
   thread-local lease. Its VS constant uploads and four draw APIs forward unchanged,
   scene-size classification is suppressed, and Clear bypasses foreground gates.
   Main-eye matrices are invalidated afterwards. Never apply a state block across
   UE3's capture itself: UE3 legitimately changes its own RHI state/cache.
   Three bounded callback receipts include native draw/upload counts and duration.
   Zero enclosed calls disproves synchronous isolation; do not call entry a fix.

3. PlayTracerFX accepts an ENDPOINT, not a muzzle-origin argument. Its native
   method calls the weapon source-interface getter. Default-OFF
   `Diagnostic/NativeRifleTracerOrigin=1` corrects only that returned start vector
   for the exact local Valkyrie, exact callsite and fresh measured muzzle witness.
   Endpoint, impact, spread, collision, inventory and other weapons remain native.
   Wrapper141/F637C8C8C78F8557, method1694/B7DDF9B80E425AAB, getter
   FULL156/DC2F29F0817B2902 (both return branches, not an early-return prefix).
   Interface+2D4h derives from the actual native constructor. No hardcoded callable
   executable addresses, borrowed game RTT or packaged proprietary assets.

`tools/fx/audit_rifle_scope_backend.py` independently derives these chains from
owned PE/archived code. It also confirms the original Valkyrie loop includes
`swirlingSmoke_02c_add_weapons` with local-space modules. Missing smoke is STILL
OPEN: original authored presence is not GPU draw identity/visibility. No widened
shader match, replacement smoke or guessed register is introduced.

## Validation / next physical gate

L1: nine x86 /O2 /W4 /WX native FX/scope/math tests; parser/transform/backend tests;
custom-hand GPU-state/COM fakes; 39 fake candidate-preparation checks. Full owned
backend contract audit PASS. This is not L3/L4. Clean L2 artifact and exact hashes
are recorded in the followup delivery note after the code commit.

Preparation: `tools/reload/prepare_combined_followups.ps1` with
`-RifleSniperFollowups -OpticalScopeAndRifleOrigin -RifleTracerOrigin` preserves the
accepted INI otherwise, seven explicit key deltas, no large pass dumps. Deployer
`Install -WhatIf` must pass before an actual guarded DLL/INI deployment. Wait for
fresh user start; never write while Singularity runs, never promote from CPU tests.

No-chat PSVR2 route after MAIN launch: Valkyrie three spaced bursts into open
space, observe barrel flash/smoke/tracer; Kasimov raise/lower lens to one eye,
check no black duplicate and whether magnification stays inside the lens; pistol,
reload and pause regression, normal exit. Stop immediately on wrong stereo,
black silhouettes or discomfort. MAIN reads bounded execution, RGB/aperture and
source receipts, verifies restore hashes, then compares with physical feedback.
Neither source-call success nor aperture HRESULT proves alignment or smoke.

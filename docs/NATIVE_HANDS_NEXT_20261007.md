# Native hands and mouseless menus — 7 October 2026

This continues the deployed D402291D native-hands build. No game executable,
localization asset, Windows driver, or registry configuration was changed.

## Mouse compatibility

The real DirectInput CreateDevice(GUID_SysMouse) still returns 0x80040154 with a
null device. Four exact loaded-code fingerprints protect conditional guards:
initial mouse setup, mouse polling, cooperative-level changes, and the shared
acquire helper. The real-device path uses the original trampolines. Keyboard
and controller processing continues. No success HRESULT is fabricated and no
fake COM object exists. Shutdown already checks null. The ineffective mouse
creation retry delay has been removed.

## Native UI and visible pointer

Singularity uses Scaleform GFx 3.x movies. Main, pause, options, dialog and IIS
movies share a focused-movie interface. The mod hooks the original input tick
and viewport cursor getters, and dispatches native InputAxis/InputKey calls on
the game's input thread. Windows cursor movement and SendInput are not used.

The right OpenXR aim pose supplies a ray in the same XR space as the headset.
It intersects a head-relative plane 1.2 metres away, sized from the rendered
FOV and menu inset. Normalized coordinates map to the actual viewport SizeX/Y,
including 5264x2684 SBS. A small outlined cross is rendered in both eyes with
R2 SELECT / O BACK hints. A fresh verified offscreen GFx texture can be blitted
to the same plane; that capture path passed a real D3D9Ex test but was NOT
observed activating in the user's first run. The existing screen UI remains
the fallback; its visual alignment needs headset confirmation.

R2 emits paired native mouse down/up edges. Menu entry immediately masks
virtual gameplay triggers/shoulders. Holding a trigger through a mode or focus
transition blocks it until release. O uses native gamepad back; sticks retain
native navigation. The pointer disappears outside allowlisted focused movies.

The user confirmed the first menu worked, but the second required pointing
higher. The renderer previously used SetPause history while pointer mapping
used focused GFx state. The new build also insets the focused GFx menu after
dialogs, keeping the rules consistent. This is an evidence-backed correction,
not a claim that visual alignment is already verified.

## Left input and independent hands

The same run reported no walking and both hands following the right hand.
OpenXR never reported a left interaction profile, left trigger was inactive,
and left tracked motion was zero. SteamVR logged the right Sense connecting
but no left Sense connection in this session. Its generated default binding
does contain the left movement stick and both hand poses. Whether the left
controller was connected during the test is uncertain.

The new build logs physical stick action activity, pose/position validity,
and bound sources on interaction-profile changes only (fixed bounded storage).
It does not manufacture a left pose or silently swap controller roles.
Independent transforms remain restricted to verified singleplayer sections
with correct c5 bone palettes, preserving finger animation and restoring
constants between draws. A valid left pose is needed to evaluate this path.

## IK and two-hand work

Implemented and tested a constrained two-bone solver, stable bend transport,
finite/singular rejection, actual PSK bind reconstruction, transactional arm
chain palette application and support-point calibration. Live IK is disabled:
the shoulder and wrist occur in different draw sections and require an earlier
complete palette from the same animation pose. Combining stale sections is
not safe. Existing two-hand controller orientation is preserved. The common
l_hand_IK asset offset is not a verified weapon-specific fore-end point.

## Rendering, camera, health and performance

Shader layouts are cached once; shadow/light-space passes bypass eye and HMD
modification. Bone edits use the original constant setter and restore exact
original values. Render-target changes invalidate camera binding. No global
shadow disable was introduced. Ghost/square-shadow visuals remain unverified.

Camera work adds bounded passive actor/camera/HMD translation evidence, enabled
in this candidate. It changes no translation or forced-animation aiming.
The view-facing left-controller health band is preserved.

Paired hand deltas now share matrix inversions: 100000 x86/O2 iterations measured
0.608 microseconds original versus 0.446 shared per section, with 5000
byte-identical outputs. This is a small measured mod-side saving. The user's
run reached about 90 Hz in some windows but also had hitches; stable gameplay
90 Hz is not established. Existing WMI caching, incremental object search,
async logs/snapshots, optimized build and 2632x2684 per-eye swapchains remain.

Original Coalesced_INT.bin and Singularity.exe remain untouched. Runtime
resource/hash paths were investigated; original prompt replacement is not
deployed. Contextual mod-rendered menu hints are implemented.

## Validation and candidate

Desktop tests pass: shader/CTAB, 5000 palettes, native D3D9Ex restoration,
arm solver and bind hierarchy, two-hand grip, 16200 pointer samples over five
resolutions/aspects and 360 head rotations, trigger/focus transitions, native
GFx ABI/click edges, real D3D9Ex menu overlay/state, 2000 camera matrices,
classification cache, asynchronous thread snapshots and overlay restoration.
Mouse guard and real MinHook synthetic trampoline tests also pass. Unit tests
do not prove headset visuals or actual independent-arm behavior.

Installed candidate: PE32, machine 0x014c, x86 /O2.

SHA-256: `044EC8637A7B321DB6AFF83590D4C178F71AFD37E2B667E72B860DA5A7B1F1B2`

DLL: `C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\d3d9.dll`

Staging/manifest/rollback: `C:\Users\Trilex\vr-mods\deployment\native-hands-next`

Rollback restores the exact D402291D DLL and its matching INI after normal
game closure. Startup logs and detailed mathematical/RE reports are in
`C:\Users\Trilex\vr-mods\research\native-hands-next`.

Three consecutive launches of this exact deployed hash passed with real
0x80040154/null DirectInput mouse, graphics viewport, OpenXR swapchains and
focused GFx IIS menu initialization. Evidence: startup-final-1/2/3.log and
startup-validation-final.json. The third process remained responsive. A
debugger-controlled native click at the user's recorded first-menu coordinate
produced paired down/up and dismissed IIS, without desktop cursor movement;
all temporary mod probe values were restored.

Next consolidated headset check: first confirm BOTH Sense controllers are
connected/tracking in SteamVR. Operate first/main/settings menus with pointer,
R2 and O; start gameplay and walk with the left stick; pause/resume and verify
R2 selects only in menus and fires only in gameplay. Then inspect independently
moving hands, support grip/release, both-eye shadows, walking camera stability,
health band, aiming away from gaze and performance. Live IK is not part of
the visual claim for this candidate. Keep the complete diagnostic log.

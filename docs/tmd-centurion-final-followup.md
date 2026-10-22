# TMD visibility and Centurion readiness (offline)

## Original mesh evidence

Private input `ch_inview_tmd_mesh.psk` SHA-256 `F64F5EE7ABD6A2DFDA0A686B3FF16C4EAD13325D4CEB352D987C8895EF269A5B` has material sections 4402, 7704 and 17 faces. Its complete 12123-face index array (0/1 winding exchanged) occurs exactly once in the privately decompressed `RvGame.xxx` (SHA-256 `09DE450D9F620952C3701D874BEB90A8BD9235263975157A641B9E3441336C92`). No original game data is installed or committed by this change.

Material 0 (`tmdGloves`) contains hands and some forearm weights; material 1 (`tmdBase`) is mixed: 6527 faces weighted exclusively to `tmd_base` and its animated hardware children, 1177 arm/sleeve faces. Material 2 (`tmdExtra`) has 17 small right-forearm faces, not TMD hardware. Rejecting all of `tmdBase` therefore hid most of the device. The original native material, vertex buffer, bone palette and index buffer remain in use. In `GloveIsolate>=2`, only the 49 measured hardware face runs of material 1 are submitted. The previous full-section suppression and legacy TMD range surgery cannot bypass that cut. All draws remain inside the existing per-eye `StereoPair`, followed by normal palette restoration.

The device belongs to the mesh's physical left wrist (side 0). It consumes the same controller delta and same side-validity gate as that wrist: off-hand delta in right-handed mode, aim-hand delta in left-handed mode. Each original hardware skin matrix is transformed, retaining native hinge/disc animation. This does **not** mirror the device to the other wrist in left-handed mode; physical placement in that mode remains for headset validation.

## Centurion path

With `TraceFromGun=1`, `WeaponTraceRigidOrigin=1` and `WeaponMeasuredAim=1`, the original `muzzle_flash` bone witness is staged at the Centurion draw (`5799` vertices), then published after successful stereo draw. `GunAimDir` accepts only current pawn/weapon/time and returns its measured direction. `Hook_SingleLineCheck` consumes it through either `NativeScopeAlignShot` when `WeaponNativeAxis=1` and the native shot receipt matches, or the measured `weapontrace::Align` fallback. Both rotate the original native shot cone, preserving range, spread and recoil. `WeaponNativeAxis` therefore does not bypass the witness when it is valid. The existing read-only `CenturionAimProbe` remains available for a bounded runtime receipt.

## Validation boundary

L1: original PSK/cooked index roundtrip and face-weight census; optimized `test-hands-only` and `test-measured-aim`. L2: x86 PE32 `/O2` DLL and OLED/WeaponLocker/TMD regressions. No Steam deployment or PSVR2 test was performed on this branch. The 49 material-1 draw runs add draw-call overhead per eye; MAIN should check frame time during the physical TMD pass before accepting.

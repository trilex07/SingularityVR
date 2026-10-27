PR60 physical back retrieval failed before the gesture gate: CameraOwnedPawn returned zero while the reciprocal native resolver identified the live player. Independently, Seeker required Instigator/mWeaponName reflection but neither name was registered, making ownership validation impossible even with a pawn.

Reuse the existing reciprocal native identity and register the two required names. Preserve strict live/ownership/layout/native method checks, exact two SwitchWeapon caller scopes, native selection, ammunition and guided rocket behavior. Do not move the back zone.

Add bounded first-gate receipts for ownership/hook/Grip/selection and projectile setup. Observe actual equipped identity separately from a selector request. Existing archive contains no Seeker launch receipt; no claim of corrected physical firing or retained support/save-load acceptance.

Tests PASS: real adapter with production name registry, camera-zero reproduction, 100 native-owned selector ABI transactions with byte-exact ammo conservation, selection timeout/confirmation, Grip reasons, projectile adapter, D3D9/Ex IB lifecycle (standalone GPU retry), critical gameplay/TMD/reload/accuracy/OLED, actual x86 Seeker and WeaponLocker ABI. PE32 /O2 source 5e908879; DLL SHA256 8112FB08142B380C6B8A6421D1522203CE0864017A95A8765A2A7379F303C60F.

Handoff and exact evidence: docs/handoffs/seeker-back-holster-followup.md and .json. No Steam access/deploy/launch. MAIN physical retest pending; support and save/load NOT TESTED. Valkyrie accuracy and Valkyrie/Volk 20 cm accepted behavior unchanged.

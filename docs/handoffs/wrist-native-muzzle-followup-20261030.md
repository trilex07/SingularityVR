# Inner wrist visibility and native muzzle follow-up

## Physical baseline and scope

Installed source 190c1127, DLL 57E1BFAD4886E7F4EAA6148326C1D6119D9854BD599B567D2DA456201ECA72B7: user reports weapons/grips OK, wrist HUD invisible, muzzle VFX slightly beside barrel. This supersedes the previous candidate receipt's PENDING for that limited physical scope. No blanket performance, crash, or all-weapon certification is inferred.

## Wrist: source-proven longitudinal-axis violation

Final-palette wrist publication and depth submission execute: frame 3470, world-visible samples 33823/39205 versus foreground-surviving samples 2760/2939. This proves submission and substantial foreground occlusion, not a visible readable HUD. Original named PSK bind, transformed by the actual PSK-to-native row-matrix conversion, places elbow at wrist Y +21.485884 UU and index at Y -8.743004 UU. Previous inner panel extended along negative Y, into the palm/fingers. Its clearance report selected that incorrect longitudinal interval too.

Inner-only SurfacePoint now uses {ox+z, oy+x, oz+y}. After the existing quarter-turn glyph right is +Z, top -Y (toward fingers), surface normal +X. Outer presentation is unchanged. The panel interval is +2.5..+157.5 mm behind wrist, centre +80 mm. Re-running source geometry measurement over this corrected interval gives max +X 6.050211 UU, hence +94 mm including the existing 1 mm thickness. The previous X clamp at 80 mm would refuse that measured clearance; inner-only maximum is now 120 mm. No controller or glove transform changes. Inner defaults match these measured values; legacy outer defaults remain unchanged.

See wrist-panel-inner-clearance-20261030.json for original PSK/SVNC/SVTG hashes, bone-reference positions, and selected geometry bounds. Bind clearance is not final animated headset proof. Existing world/foreground depth routing, state restoration and legacy-health suppression remain unchanged; suppression stays OFF.

## VFX: missing scope and bounded correction

The existing native PlayMuzzleFX hook deliberately accepted only Valkyrie. Thus the new HD Kasimov/Centurion muzzle witnesses reached native firing but could never replace the cosmetic spawn location through this hook. Add default-OFF [Render] NativeWeaponMuzzleOrigin=1 for these two exact measured profiles (Kasimov 11389 plus Kind::Kasimov; Centurion 5799 plus Kind::None). It still requires first-person gameplay, matching current local pawn/weapon, fresh witness, finite vectors and the existing origin-epoch bound.

The existing runtime-verified native wrapper/property/full-method fingerprints remain mandatory (197-byte wrapper 1153B073558EDE80, 846-byte method DEA566CA454CAFEB, mWeapon+90). No new native ABI, emitter spawn, asset, guessed yaw or offset. Native vector argument now receives camera + the same final rendered muzzle witness already used by accepted shot alignment. Loop maintenance/lease and tracer hook remain Valkyrie-only. Native spread, recoil, range, damage and endpoints are untouched. Eight receipts per profile record exact mesh, fresh/changed and source/destination; no per-frame logging.

Evidence limit: the old decoded text archive referenced by previous RE is no longer present at research/etw/capture-20261006-205508/Singularity.text-memory.bin. The owned Steam EXE on disk remains packed; this follow-up does not pretend its encrypted bytes are fresh disassembly. It reuses the established complete-body, fail-closed runtime contract and adds a narrow policy extension. Correct spawn receipt is not particle-content proof; an authored emitter-local offset or later particle update would falsify sufficiency of this correction.

## Validation and acceptance

Focused native muzzle/aim/scope/holster policy and wiring tests PASS. New cases explicitly reject stale, foreign, third-person, unknown-mesh and flag-OFF requests; non-rifle loop leases and tracer paths remain rejected. Source reference regression now reconstructs BOTH actual 90/89-bone hierarchies and verifies elbow/index signs. HUD D3D9/D3D9Ex wall/glove/magazine pixels, stereo, OFF/failure/reset/state tests PASS. Native reload/carry and mixed-skinning regressions PASS. Remaining final build/critical checks are recorded in the candidate receipt.

One physical check only: raise the INNER left wrist and rotate for upright text; cover it with weapon or carried magazine in each eye. Fire Kasimov/Centurion and check smoke/flash begins at the visible muzzle. Grips/aim/reload are protected regressions; no need to repeat all prior baseline tests. Close normally and report each remaining failure. No game launch or physical PASS claimed by this source change.

Archived actual failed-session SHA256: CC72032A973B5D9A662BD0737873C6659DC12C300B2DE4F72300916618E6863B

Regression maintenance: dual-TMD wiring had a stale exact text match predating the accepted kasimovLensFault fail-closed prefix. The test now explicitly checks BOTH lens failure and sleeve-palette failure before StereoPair. No production TMD/lens routing changed. Native backend receipt test now verifies three eight-entry profile caps instead of the previous shared 24-entry cap.

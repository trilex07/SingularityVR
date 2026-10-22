# WeaponLocker VR LOADOUT: visibility review

Baseline: `fix/gemini-locker-rootcause` at `defe0a7`.
No Steam files were modified by this branch.

## Runtime evidence

The latest relevant `view_matrix_2026-10-22_21-55-00.log` shows
`Menu_WeaponsLocker` focus and `locker vr loadout ready: state=1 ...
available=0xF owned=0x1 pending=0xE` three times (at lines 17778, 22425,
24444). The HUD atlas also reaches `ready (512x256, 22 icons)` at line
20519. The data and atlas gates therefore passed in that session. This does
not itself prove that any pixels were visible in the headset.

## Cause and correction

`ClampEyePanel` projected the loadout from the menu plane and then forced its
centre to the far-right backbuffer edge. At 5264x2684 SBS, the previous
READY rectangle was approximately [2368,2590] in the left eye and
[5000,5222] in the right eye (both about 90-98% of eye width). The controller
ray was bounded to the original GFx canvas and could not reach that region.

The revised anchor comes from each eye's projected right edge of the *actual*
WeaponLocker menu plane plus a 28 px gap. The panel is accepted only if it
fits beside the native menu and within the central 10-90% of that eye.
Otherwise the left side is tried; if neither side fits, no overlapping panel
is produced. The same layout builder is used by rendering and hit-testing.
Only a ray hitting a loadout target can extend beyond the native GFx canvas;
native GFx clicks are suppressed on those targets.

For the offline station model (inset .42, 64 mm IPD, 1.4 m plane distance,
5264x2684 SBS), the projected native right edges are x=1898.8 and 4470.6.
The new 2x2 panel rectangles are [1926.8,2148.8] and [4498.6,4720.6];
EQUIP ALL is [1952.8,2122.8] and [4524.6,4694.6]. The loading label is
[1926.8,2196.8] and [4498.6,4768.6]. Y bounds, all four tile bounds,
and the EQUIP ALL bounds also pass. Repeating at +/-25 degrees HMD yaw
produces identical eye-relative placement because the station plane follows
the head. The geometric test projects an extended controller ray to each
tile centre and EQUIP ALL, then verifies the draw and hit coordinates match
within 0.1 px.

## Evidence level

L1 geometry and hit tests PASS. x86 `/O2` PE32 build PASS. No Steam runtime,
OpenXR, pixel capture or physical PSVR2 test has been performed for this patch.
MAIN should review the bounded extended-ray interaction before deployment.

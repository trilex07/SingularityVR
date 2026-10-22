# WeaponLocker VR loadout rendering — L1/L2 handoff

Base: `387c90a8fcd94ee1300a654990b4bc59cb8ea38c`. Scope: rendering and shared screen-space geometry only. No Steam deployment, acquisition, native ABI, READY or native GFx changes.

## Evidence and falsifiers

- Physical report: panel and EQUIP ALL visible, four icons absent, heading reads `VR LOAD`. Runtime log: HUD atlas ready (512×256, 22 icons), loadout READY with `available=0xF`, but no wheel catalog-ready event. Source shows the locker icon lookup returned null until the wheel catalog was loaded, while that catalog is loaded only when the wheel opens. Falsifier: if the new bounded log shows four successful icon submissions but pixels remain missing, the remaining issue is draw state/composition, not lookup.
- `VR LOADOUT` needs exactly 100 merged pixel-run rectangles in the current 5×7 font. Its old 64-rectangle array silently truncated the final glyphs. New capacity is 128, and glyph-count-based centering replaces the erroneous 11-character width.
- The native menu's projected edge is the panel placement authority. Gap changed from 28 to 12 screen pixels, moving both panels 16 pixels left while preserving a positive native-GFx separation. Drawing and laser hit-test use the same `ComputeLayout`/`ProjectAdjacentAnchor` path.

## Offline validation

- The installed atlas was read, not modified. It contains four distinct 64×64 weapon sprites with nontransparent content (559, 700, 530 and 493 visible pixels). Their atlas UV spans are `[0,.125]`, `[.125,.250]`, `[.250,.375]`, `[.375,.500]` horizontally and `[.500,.750]` vertically.
- At 5264×2684 with yaw 0/±25°, native right edges project to x=1898.8 and x=4470.6. Before: left panel `[1926.8,2148.8]`, right `[4498.6,4720.6]`. After: left `[1910.8,2132.8]`, right `[4482.6,4704.6]`. Both retain a 12 px gap. Tile, EQUIP ALL and laser ray tests pass in both eyes.
- State-block-scoped draw now explicitly disables inherited GFx scissor and enables RGBA writes. This is defensive state isolation, **not** a proven cause of the missing icons. First two READY frames log per-eye panel, title rectangle count, sprite UV and `DrawPrimitiveUP` HRESULT. If submitted draws still do not appear in PSVR2, MAIN can use that bounded evidence to locate the next rendering stage.

## Acceptance boundary

This is L1/L2 only. MAIN should review before any deployment, then verify all four icons, full title, modest left shift, laser targeting and native WeaponLocker interaction in PSVR2. EQUIP ALL must not be exercised as part of this UI-only review until its crash fix is separately accepted.

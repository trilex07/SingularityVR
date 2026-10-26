# Native main/pause buttons — V2.1 integration

MAIN, 2026-10-26. Offline candidate; Steam and physical acceptance PENDING.

## Proven missing connection

`ui_menu_skin.h` supplies the backdrop, not native buttons. The current private
`UI_SinglePlayer_SF.xxx` has GFx exports `iis` and `SPMenus`. Native AVM1
`AddButtonCenter -> AddButtonBase -> attachMovie("Button")` creates main-menu
buttons; `AddButtonLeft` creates pause buttons from `ButtonLeft`. AddButtonBase
uses each instance's `_height` to advance the next button. Replacing a complete
screen or changing button bounds would break this existing layout contract.

The patch embeds the four existing **neutral** V2.1 PNG plates in those actual
symbols: iis Button(8), SPMenus Button(104), ButtonLeft(155). No baked PL/EN
labels, second menu, duplicate text, new input route or coordinate offset.

## Exact changes and preservation

- Four native states: up/over/down/disabled. Original 40-frame timelines,
  frame labels, Stop actions, text definitions, instance names, clip actions,
  filters, matrices, root AVM1 and Mouse sprite remain unchanged.
- A plate occupies depth1; original child depths shift by one. No external
  script in these symbols addresses those internal numeric child depths.
- Native animated hover artwork remains in its original display-list role
  with alpha zero, preserving bounds/pulse layout; it no longer covers the
  new plate. Its shared definition is NOT changed for other widgets.
- Plate bounds are contained inside the native text bounds in **every frame**:
  iis `[-3946,5400,-19,597]`; main `[0,6400,0,617]`; pause
  `[640,6640,0,692]` twips. Thus no added width/height for hit/layout calculations.
- Existing per-eye projection, pointer/laser mapping, Settings, wheel, 3D HUD,
  localization and RTT_VRPause are untouched. Native behavior remains in GFx.
- New SPMenus FNV64 `27D40BFD1480CE52` is admitted to the existing exact
  native-cursor gate because root/Mouse actions are byte-identical. Existing
  accepted fingerprints and native-method ABI guards stay in place.
- GFx exporter/FileAttributes metadata precedes new definitions. IDs include
  imported fonts and native GFx external images to prevent collisions.
- Embedded DefineBitsLossless2 uses premultiplied ARGB and bitmap-filled
  DefineShape3; transparent corners are retained. No added D3D9 hooks/state.

## Reproduce (private user-owned input only)

1. Decode an offline copy of the accepted UI package with the existing
   `tools/uedecompress/unpack_chunks.exe`; whole-package decoder is not correct
   for this summary container. Never overwrite the accepted file.
2. `python tools/title/prepare_menu_remaster.py --original <private.xxx>
   --logical <decoded.u> --decoder <unpack_chunks.exe> --out <new-private-dir>`
3. Input is pinned to physical SHA
   `51CDAB5C892A7B682D89319721330AA3E78C7013D1C60E1476A0115431F01A1A`
   and decoded SHA in the companion JSON. Unknown input refuses. Both accepted
   movie fingerprints are also pinned; the patch cannot silently run twice.
4. Preserve the private package; extracted game data is not committed. The
   metadata JSON records exact output/asset SHA values and inline relocations.

Current private output:
`build/release-evidence/menu-remaster/candidate-v2/UI_SinglePlayer_SF.xxx`.

## Validation

- Six focused tests PASS, including actual private iis/SPMenus timelines:
  all120 frames preserve native child matrices/geometry/text/action payloads;
  every new plate remains inside native text bounds; exact embedded bitmap
  pixels/alpha checked against all four original PNGs.
- Existing title/cursor Python suite: 20 executed PASS, two private-fixture
  cases separately executed above. Initial sandbox-temp permission failure was
  resolved using a worktree-local TEMP; not a code/test failure hidden as PASS.
- Native cursor compiled x86 policy/ABI regression PASS.
- All45 UE3 export identities/content verified; only the two movie arrays and
  proven inline bulk offsets change. Existing LZO decoder independently reads
  the repacked candidate back to byte-exact expected logical data.
- This is structural/decoded-bitmap evidence, NOT Scaleform or headset pixel
  acceptance. No game was launched and no installed package was changed.

## Grouped physical acceptance / rollback

After MAIN's guarded deployment of the exact DLL plus UI package, open main,
pause and Options in PL/EN; hover/click native buttons with laser and controller,
including disabled states. Check intact title/cursor, no duplicate labels,
unchanged scroll/spacing and readable plates in both eyes. Stop on crash,
misaligned hit targets, clipping or lost localization and restore both package
and DLL from verified backups. The OFF path is the exact backed-up UI package;
this native package patch is not controlled by a misleading runtime-only flag.

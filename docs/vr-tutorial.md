# In-VR tutorial

## Design

The tutorial is an optional set of short lessons inside `VR SETTINGS`. Opening
`TUTORIAL / SAMOUCZEK` first presents `POLSKI` and `ENGLISH`; the selected
language applies to the lesson pages. Users can reopen it, change language,
move backward or forward, and finish back at VR Settings. It does not interrupt
gameplay or trigger automatically during a level transition.

The current implementation uses the mod's existing stereo bitmap panel and
controller ray. Polish diacritics are decoded as UTF-8 by the existing 5x7
glyph renderer. The tutorial does not add input hooks or change gameplay
bindings.

## Lessons

1. Left-stick movement, right-stick-up jump and right-stick-down crouch.
2. Pickup with right grip and medkit on X.
3. Hold R3, move the right hand to a weapon-wheel slot, release R3 to equip;
   L3 cancels.
4. Magazine weapons: O releases the magazine; reach behind the left shoulder,
   grab a fresh one and insert it.
5. Centurion: O opens the cylinder; raise the muzzle to dump cases; take a new
   drum from behind the left shoulder, insert and flick to close.
6. Triangle opens the menu; laser plus R2 selects, O returns.

The PSVR2-specific actions above follow the current MAIN input layout and the
user-confirmed O reload behavior. The weapon-wheel instruction follows the
default hand-selection mode in `d3d9.cpp`; it should be revisited if a future
controller profile changes the action mapping.

## Evidence gate

Offline review confirms that the tutorial pages reuse the existing panel
navigation and ray selection; they do not write settings or send native game
actions. Before calling the feature physically verified, a PSVR2 pass must
confirm: both language choices open readable lessons, all six pages advance
and return correctly, Polish diacritics render, and closing the panel returns
to the underlying menu/game without changing the lesson target or gameplay.

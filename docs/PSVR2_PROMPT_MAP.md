# PSVR2 prompt map: native actions → current bindings → icons

Gameplay branch: `fix/vr-menu-input-grip-performance`. Status: mapping table +
placement helper ready; NO token replacement built (target unconfirmed).

## PAD2 / PAD3 finding (verified negative)

`PAD2`/`PAD3` (and `PAD0`/`PAD1`) appear in NONE of the shipped game text:
`RvGame\CookedPC\Coalesced_INT.bin` (+`_ESN/_FRA/_ITA`), `Binaries\
Singularity.exe` (ASCII+UTF-16), decompressed `RvGame.u` / `Engine.u` /
`Core.u` / `GFxUI.u`, raw SF movies (`UI_SinglePlayer_SF`,
`ui_icons_360_SF`, `ui_controls_360_SF`), and prior prompt research JSON —
zero hits everywhere (`PAD` hits are `GBA_DPad_*` / `GAMEPAD_*` substrings;
repo `PAD2|PAD3` hits are `NUMPAD2/3` debug hotkeys, unrelated).

What native gamepad prompts actually are:
1. Tutorial/HUD strings are `[token]` localized strings (`Press [impulse]
   to Impulse…`, `[use] Continue`, tokens: `use fire impulse age crouch
   grab flashlight aim dash switchwpn jump reload cancel health`). Token →
   button resolution is exe-native (no resolver in script/config).
2. Menu prompts use `RavenShared.RUIDataStore_StringAliasMap` →
   `MenuInputMapArray` with `PC` / `360` / `PS3` sets; the 360 set renders
   single font-glyph chars (`w/x/y/z/L/C…`) and word labels (`X BUTTON`,
   `CLICK LEFT STICK`, …). The 360 set has NO D-Pad entries.
3. Bindings: `XboxTypeS_* → GBA_* → PressChainButton CB_*` (16 + 24 rows).

Most likely explanation for the in-headset report: Xbox button-font glyphs
misread at headset resolution — NOT a PAD alias. Decisive next step is a
headset screenshot + the `[token]`/FieldName context. Do NOT build
replacement on the PAD assumption.

## Verified native semantics (Coalesced_INT.bin)

| 360 pad | GBA alias | ChainButton / action |
|---|---|---|
| RT / LT | `GBA_Fire` / `GBA_Aim` | `CB_FIRE` / `CB_AIM` |
| A / B / X / Y | Jump / Crouch / Use / CycleWeapon | `CB_JUMP/CROUCH/USE/CYCLEWEAPON` |
| LB / RB | AgeYoung / Impulse | `CB_AGEYOUNG` / `CB_IMPULSE` (TMD powers) |
| L-click / R-click | Dash / AntiGrav | `CB_DASH` / `CB_ANTIGRAVITY` |
| D-Up / D-Down | Health / Flashlight | `CB_HEALTH` / `CB_FLASHLIGHT` |
| Start / Back | ShowMenu / Scoreboard | `ShowMenu` / `ToggleScoreboard` |
| D-Left / D-Right | `GBA_DPad_Left/Right` → bare `CB_DPAD_*` | no gameplay command |
| L-stick X/Y, R-stick X/Y | `aStrafe/aBaseY/aTurn/aLookup` | deadzones .25/.25/.25/.2 |

Matches `d3d9.cpp:5890-5916` (run 61b) and `HISTORY.md:2966-2978`.

## Current OpenXR/XInput mapping (working-tree source, right-handed default)

| Game action (native alias) | OpenXR action ← path | PSVR2 control | Status |
|---|---|---|---|
| Fire (`GBA_Fire`, RT) | `g_actFire` ← `…/right/input/trigger/value` | Right trigger (R2) | verified |
| Aim (`GBA_Aim`, LT) | `g_actAltFire` ← `…/left/input/trigger/value` | Left trigger (L2) | verified |
| Jump (`A`) | `jump` ← `…/right/input/a/click` | × | verified binding; face equivalence UNVERIFIED from source |
| Health (`D-Up`) | `health` ← `…/right/input/b/click` | ○ | remapped; emits native `CB_HEALTH` and arms the bandage animation |
| Use/reload (`X`) | `use` ← `…/right/input/squeeze/value` | Right grip | verified; CONFLICTS with overlay legend `USE=R1` |
| AntiGrav (`R-click`) | `antigrav` ← `…/right/input/thumbstick/click` | R3 | verified |
| Impulse/melee (`RB`) | `impulse` ← `…/left/input/trigger/value` (thresholded) | Left trigger — SAME pull as Aim | code-verified; dual-role needs design decision |
| Age/renovate (`LB`) | `ageyoung` ← `…/left/input/x/click` | □ | verified |
| Flashlight (`D-Down`) | legacy row retained but unpublished; left Y is Triangle | △ → game menu | replaced by the requested Triangle menu edge |
| Dash (`L-click`) | `dash` ← `…/left/input/thumbstick/click` | L3 | verified |
| Pause (`Start`) | `triangle_menu` ← `…/left/input/y/click` → rising edge `XI_START` | △ | implemented; held Triangle does not repeat |
| CycleWeapon (`Y`) | stick Y>0.7 → `XI_Y` 8-frame hold | Right stick up | verified |
| Crouch (`B`) | stick Y<−0.7 → `XI_B` | Right stick down | remapped; 8-frame native crouch pulse |
| Turn / Move | XR `turn`/`move` → snap/smooth + `g_padLStick` | Right/left stick | verified |
| Menu select/back | trigger-gate → native mouse; `ui_select/ui_back` → `XI_A/XI_B` | R2 / ○ | verified |

Remaining verified conflict: the native `impulse` action and left-trigger melee/aim roles
share the left trigger in the current profile. The overlay labels are contextual and do not
claim that R1 is the native reload binding; the production `use` action remains right grip.
Mask overrides: `[Input]` ini keys.

## Prompt placement (prepared helper)

Root cause of "too low": blind backbuffer fractions (`.83H`/`.90H`, now
`.72H`) with no FOV/crop/safe-area awareness — the bottom rows fall outside
the comfortable frustum on the 2684px SBS target and collide with the menu
quad. Also: `gameplayHints` gating changed to `Rd(g_camPosValid)`, which is
typically false during level loads (legend may no longer draw there at
all); `size=max(22,…)` floor breaks resolution-independence at low H.

Prepared: `spikes/view_matrix/prompt_placement.h` — `SafeAreaFromFov()`
(centered visible fraction from rendered vs visible half-FOV tangents),
`LegendRowY()` (bottom-anchored rows in safe-area fractions, ordered,
contained, oversize rejected), `EyeBaseX()` (SBS per-eye origins).
Tests: `tests/test-prompt-placement.cpp`. The main agent connects these to
`DrawVrPromptLegend` (`vr_menu_overlay.h:42-65`) — ideally anchoring above
the menu-plane bottom edge via the existing `HeadPlane`/`VrUiProjectEye`
machinery. Gameplay HUD prompts only; menu capture untouched.

## KHARVOX prompts

Verified negative: no native-prompt replacement, no controller-icon
overlay (`HudHook.cpp:3057-3058` explicitly EXCLUDES the drone proximity
prompt from repositioning; `updateVirtualButtons` is input pass-through).
Nothing transferable beyond the trigger policy already abstracted.

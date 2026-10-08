# PlayStation prompt icons (per-element mapping)

Branch `feature/ps-prompt-icons` (worktree `SingularityVR-prompts`). Extends the
mod's prompt legend from a 5-icon subset to the **full PlayStation button set**,
with a per-action mapping so each element shows the right icon (Enter → **Cross**,
interact → **R1**, fire → **R2**, aim → **L2**, …).

## Assets (Kenney Input Prompts 1.5, CC0-1.0)

`tools/ui/build-prompt-atlas.ps1` packs a 64px row into
`assets/processed/ui/runtime/vr_prompt_atlas.bmp` (**1280×64**, 20 icons) and
emits:

* `assets/processed/ui/psvr2-input-icons.json` — icon name → UV rect + action map,
* `spikes/view_matrix/prompt_atlas_generated.h` — the same UV table as C++.

Icons: `R2 Circle Cross R1 Options L1 L2 Square Triangle L3 R3 DpadUp DpadDown
DpadLeft DpadRight DpadAll StickL StickR Create Touchpad`.

Re-run after editing the icon list:

```powershell
powershell -File tools/ui/build-prompt-atlas.ps1
```

## Action → icon map (`prompt_roles.h`)

| action | icon | native alias (verified, `docs/PSVR2_PROMPT_MAP.md`) |
| --- | --- | --- |
| Confirm (Enter / menu select) | **Cross** | A |
| Back / cancel | Circle | B |
| Fire | R2 | RT (`CB_FIRE`) |
| Aim | L2 | LT (`CB_AIM`) |
| Interact | R1 | use/interact |
| Jump | Cross | A (`CB_JUMP`) |
| Crouch | Circle | B (`CB_CROUCH`) |
| Age | Square | X (`CB_AGEYOUNG`) |
| CycleWeapon | Triangle | Y (`CB_CYCLEWEAPON`) |
| Impulse | L1 | RB (`CB_IMPULSE`) |
| Dash | L3 | L-click (`CB_DASH`) |
| AntiGrav | R3 | R-click (`CB_ANTIGRAVITY`) |
| Flashlight | DpadDown | D-Down (`CB_FLASHLIGHT`) |
| Health | DpadUp | D-Up (`CB_HEALTH`) |
| Pause | Options | Start (`ShowMenu`) |
| Create / scoreboard | Create | Back (`ToggleScoreboard`) |
| Move / Look | StickL / StickR | left / right stick |
| Touchpad | Touchpad | — |

## Integration (done in this branch)

`spikes/view_matrix/vr_menu_overlay.h`:

* `LoadVrPromptAtlas` now validates the geometry **from the BMP header**
  (`height == kVrPromptAtlasHeight`, `width` a multiple of `kVrPromptIconPx`,
  `<= kVrPromptAtlasWidth`) and uploads that size — so the icon set can grow
  without another code change;
* `DrawVrPromptLegend` selects icons by **role** (`promptroles::Action`) via the
  generated UV table instead of hard-coded UV pairs; gameplay shows
  Interact/Fire/Aim/Confirm, menu shows Confirm/Back/Pause.

Test: `tools/reference-tests/ui/prompt_roles_test.cpp` →
`PASS: PS prompt role mapping + atlas UV table` (every action resolves to an
atlas icon; UVs in range and non-overlapping).

## Scope / honest limit

This replaces the icons the **mod draws** (the legend) and gives a complete
per-element mapping. The game's **native** interaction prompts ("Pad 3 /
Open Chest") are Xbox button-font glyphs and `[token]` strings resolved
engine-side; `PAD2/PAD3` do not exist in the shipped data (verified negative in
`docs/PSVR2_PROMPT_MAP.md`). Replacing those glyphs *in place* needs the
Scaleform/font path and is **not** part of this branch — the per-element icon map
here is exactly the input that work will consume.

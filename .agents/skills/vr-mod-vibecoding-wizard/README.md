# VR Mod Vibecoding Wizard

A Claude skill that walks you — beginner or expert — through converting a flatscreen game into a full 6DoF, motion-controlled VR mod.

It's a setup wizard, a build plan, and a bug-tracking process in one file. It interviews you, researches your game, checks what VR frameworks already exist for it, and then builds the conversion in ten gated stages: camera, stereo, controllers, locomotion, physics hands, weapons, full-body IK, HUD and menus, an in-VR settings menu, and packaging. You put the headset on; it does the rest.

## What it produces

A "Phase 1" VR mod with all of these wired up and switchable in an in-VR menu:

- True stereo rendering and 6DoF head tracking, with a fallback ladder that never downgrades silently
- Motion controllers for every major family (Meta Touch, Index, Vive, Pico, WMR, PSVR2 Sense, Steam Frame), full left-handed support
- Smooth, snap, and teleport locomotion; physical crouch, lean, prone; comfort options
- Physics hands, grabbing, throwing, doors and levers by touch
- Holsters, manual and arcade reload, two-handed weapons, melee and bow gestures, abilities
- Full-body IK retargeted to the game's own skeleton, with in-VR calibration
- HUD converted by element type, cutscenes on a virtual screen, button prompts that match your controller
- An in-VR settings menu for every tunable, plus a layout mode to grab and move UI by hand
- A bug pipeline: in-game logger, one-button bug capture with screenshots, structured playtest scripts

## Install

**Claude Code:** copy the `vr-mod-vibecoding-wizard/` folder into your skills directory (usually `~/.claude/skills/`), or add this repo as a plugin source.

**claude.ai / Claude Desktop:** open `vr-mod-vibecoding-wizard.skill` from the [latest release](../../releases) and click **Save skill**.

Then start a new conversation and say something like *"I want to play [game] in VR"* — the wizard takes it from there.

## Requirements

- A PCVR-capable headset connected to the same PC the skill runs on
- A single-player game (the skill warns about anti-cheat and won't try to bypass it)
- For the full experience, an agentic environment like Claude Code, so the skill can build and test the mod itself. It works in chat too, just slower.

## How it works

1. **Setup wizard** — four short screens; it detects what it can on your PC (with your OK) and asks the rest
2. **Research** — your game's mechanics, and whether a VR framework or prior mod already exists; you choose build-on or from-scratch
3. **Stages 0–10** — each stage is built, tested without a headset, then handed to you as a short playtest script
4. **Package** — installer, README, uninstall

Every project keeps five living documents (target profile, phase plan, decisions, bugs, questions) so you can resume in any session and see a status board of what's built, what's next, and what won't make it.

## Contributing

The controller tables, connection methods, and framework landscape drift monthly. If the skill told you something outdated, open an issue with the section and what's changed. If you used it on a real game, the most useful thing you can share is your project's `docs/DECISIONS.md` and `docs/BUGS.md` — they show where the workflow held and where it didn't.

## License

MIT — see [LICENSE](LICENSE).

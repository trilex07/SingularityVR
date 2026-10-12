# Hands-only gloves and pause-menu text

Status: IMPLEMENTED_NOT_VERIFIED after PSVR2 re-test candidate. L1 covers rigid-glove policy, diegetic menu composition, and clip math. L2 build/tests pass on the branch. L3 smoke (Steam, no restore) and L4 headset are separate gates.

## PSVR2 visual fail on `27ffc32` / `F769CD7E...`

1. **Gloves** still stretched from an anchored point (not rigid 6DoF).
2. **Pause menu** still clipped upper-left; backbuffer `c5` eye-copy in `27ffc32` did not affect the path that actually drew pause GFx.

## Root cause — rigid gloves (priority 1)

Two coupled bugs in production `GloveIsolate=2` / `ShowControllerArms=0`:

1. **`ApplyGloveIsolation` still ran palette stump collapse for mode ≥ 2.** Mode 2 was documented as “hide arm draw, no palette collapse,” but `CollapseDelta` still scaled forearm/cuff bones toward the wrist. Blended cuff vertices stretched between a moving wrist (controller delta) and a collapsed forearm anchor.

2. **`PrepareIndependentLeft` skipped `BoneRole::Collapse` bones** in the transform loop (`continue` before `TransformBone`). Cuff/forearm palette entries stayed at native bind pose while wrist and fingers received the controller delta → visible stretch from the original arm anchor.

**Fix:** `UsesPaletteStump()` limits stump collapse to mode 3+. In hands-only mode, collapse-role bones get the same controller `TransformBone` as the wrist chain, then are skipped for the descendant loop (no IK, no extra offsets).

## Root cause — pause menu (priority 2)

Pause GFx for class 2/3 on the **scene-sized offscreen target** (`menu mono`, `g_rtIsBackbuffer=0`) took a **single full-width draw** in `HudStereoPair`. The cockpit monitor and SBS path show one combined texture; canvas ortho coordinates meant for a flat monitor were not remapped per eye. The `27ffc32` **`PauseCanvasNeedsEyeCopy`** path only ran on the **backbuffer** HUD route, which logged runs showed pause text rarely used for the failing visual.

**Fix:** Detect **diegetic menu canvas** (canvas ortho + viewport width ≥ scene width). For that path, draw twice with half viewports, class inset scale, and `ApplyEyeRemap` on `c5` — same composition idea as backbuffer HUD, without moving `DrawPauseVrSettingsRow`. Bounded `menu-mono trace:` logging records backbuffer vs offscreen and per-eye vs single draw.

## Weapon / holster (unchanged)

Holster, aim-hand weapon, support grip, and independent off-hand behavior use the existing latch and delta gating; rigid cuff bones follow the same delta rules as other hand bones.

## Evidence gates

| Check | Result |
|-------|--------|
| L1 `test-hands-only.exe` | PASS (`UsesPaletteStump`, hands-only policy) |
| L1 `test-pause-menu-space.exe` | PASS (`DiegeticMenuCanvas`, eye inset math) |
| L2 x86 PE32 `/O2` + SHA in `d3d9.build.json` | Record per deploy commit |
| L3 Steam smoke | OpenXR, menu class, input — not visual pause/glove PASS |
| L4 PSVR2 | User-owned; do not mark PHYSICALLY_VERIFIED without it |

## Steam session notes

Earlier `27ffc32` deploy: INI preserved (`ShowControllerArms=0`, `GloveIsolate=2`, `IndependentHandsPrototype=1`, `ArmIKMode=0`). Post-fix candidate SHA and backup path are recorded in `STATUS.md` after MAIN deploy.

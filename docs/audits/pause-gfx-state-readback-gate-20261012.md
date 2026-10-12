# Final gate: pause GFx state readback (2026-10-12)

**Branch context:** `research/pause-vr-overlay-feasibility` @ `0935a0c`  
**Steam baseline (unchanged):** DLL `2FE5011C20DFEAAEDFFFFE58CD39E92A9570867F344649B00C0038AD97B39A47` @ `726e1a7`

**Question:** Can we safely read pause selection, Objectives tab, and Objectives text from the live pause movie (`NativeVrUi::UpdateMenu` → `RvUIGFxMovie_Pause`)?

**Method:** Static only — decompressed Steam packages (`tools/uedecompress`), compressed SWF ASCII scan, existing mod hooks. No runtime deploy, no `Invoke`, no render/D3D changes.

---

## Executive decision: **NO-GO**

Stop pause-menu **implementation research** (native-backed overlay and GFx readback). Preserve accepted Steam build.

**GO bar (not met):** Validated native readback with evidence for **all three** of selection, active tab, and Objectives content.

**Why NO-GO in one line:** Package and SWF evidence does **not** supply callable **`GetVariable` path strings** or a **verified UFunction + `this` object** chain from the live **`movie`** pointer; alternative menu APIs exist as **names only**, not as validated pause readback.

---

## 1. Starting point (verified in mod)

| Item | Source |
|------|--------|
| Live pause instance | `movie = *(*(engine+0x148)+0x50)`, class **`RvUIGFxMovie_Pause`** via `NameOf` (`native_ui_input.h`) |
| No readback today | Zero `GetVariable` / `GetVariableString` / menu `ProcessEvent` calls in `spikes/view_matrix/` |

---

## 2. `GetVariable` API (`GFxUI.u`, decompressed)

| Finding | Detail |
|---------|--------|
| **Where defined** | Export name cluster on **`GFxInteraction`** (same block as **`GetFocusMovie`**, **`GetGameViewportClient`**) |
| **Variants present** | `GetVariable`, `GetVariableString`, `GetVariableBool`, `GetVariableNumber`, `GetVariableArray`, … (8 variants) |
| **Related** | `GFxAction_GetVariable` (Kismet), `GFxMovie`, `GFxDataStoreBinding` |
| **On `RvUIGFxMovie_Pause`?** | **Not shown.** `RvGame.u` has **no** `GetVariable` strings. Inheritance `RvUIGFxMovie` → `GFxMovie` is plausible but **not export-verified** in this pass. |
| **Callable from mod?** | **No.** Mod has no UFunction index, no native stub, no `ProcessEvent` wrapper for `GFxInteraction::GetVariable*` on the live movie or interaction object. |

**Important mismatch:** Focus uses the **`RvUIGFxMovie_Pause`** pointer; Epic’s **`GetFocusMovie`** API names sit on **`GFxInteraction`**. Even if `GetVariable` is native on `GFxMovie`/`GFxInteraction`, the mod has **not** identified which **`this`** pointer to use alongside `movie`.

---

## 3. Variable / property names (evidence only — no guessing)

| Source | Names found | Usable as `GetVariable` path? |
|--------|-------------|-------------------------------|
| **`UI_SinglePlayer_SF.xxx`** (chunk-compressed; **not** fully decompressed) | `_root`, `_selectedIndex`, fragments `Tab`, `SELECT`, `PauseGameWhileActiveI`, `setFocus`, `pathToFocus` | **No full path.** No `_level0.foo.bar` strings in `RvGame.u` (0 hits). **`_selectedIndex` alone is insufficient** to call `GetVariable` without SWF decompilation. |
| **`RvGame.u`** | `RvUIDataStore_SingularityObjectives`, `RvUIDataProvider_SingularityObjectives` | Data-store types; **no** binding field names or GFx paths in cleartext. |
| **`RvGame.u`** | `ShowInMenuObjectives`, `GetSelectedListItemName`, `SetSelectedMenuItemByName`, `RefreshDefaultListItem*` | **UnrealScript/native function names** in the package heap; **class owner not resolved** statically (not tied to `movie` pointer). |
| **`Coalesced_INT.bin`** | No hits for above APIs or `_selectedIndex` | — |

Per project Scaleform spike (`HISTORY.md`): **`UI_SinglePlayer_SF.xxx` chunk format** still blocks SWF/ActionScript decompile; that is exactly where menu **AS paths** would live.

---

## 4. Assessment vs required data

| Required | Static result | Verdict |
|----------|---------------|---------|
| **Selected pause row** | SWF fragment `_selectedIndex`; UScript names `GetSelectedListItemName` / `SetSelectedMenuItemByName` (context: list/objective/menu helpers). **No** validated call target from `movie`. | **Not demonstrated** |
| **Active tab (Objectives)** | SWF strings `Tab` / `tabEn` fragments only; UScript name `ShowInMenuObjectives` (purpose unknown). **No** tab index/string read API bound to pause movie. | **Not demonstrated** |
| **Objectives text** | `RvUIDataProvider_SingularityObjectives` / `RvUIDataStore_SingularityObjectives`; `GetObjectives` in global native name table (likely **game/pawn**, not GFx movie — alphabetical heap with `GetNumCollectibles`, etc.). | **Not demonstrated** |
| **Submenu state** | **`g_vrMenuClass` / focus change** only (Options* → class 4). No row/tab readback. | **Partial (movie swap only)** |

---

## 5. `Invoke` and safety

- Task forbids **arbitrary `Invoke`** for diagnostics — correct; `Invoke` runs ActionScript entry points and is not read-only.
- **`GetVariable` / `GetVariableString`** are the appropriate read-class APIs on paper, but calling them without a **proven path** and **`this`** is still an **unvalidated native experiment**, not a gate pass.

### If MAIN ever runs one runtime experiment (not approved by this gate)

Only worthwhile **after** SWF decompile yields an exact path string:

| Step | Field | Why safe-ish |
|------|-------|----------------|
| Resolve `UFunction` for `GetVariableString` on the class of `movie` (or linked `GFxInteraction`) via existing `g_scriptFns`-style name→index machinery | Output `FString` | Read-only GFx API; log return value once on pause open / tab change |
| **Do not** call `Invoke`, `SetVariable*`, or guessed paths | — | Avoid side effects |

Without SWF paths, a runtime test **cannot** satisfy the GO bar and is **out of scope** for this final static gate.

---

## 6. Consequences

| Action | Status |
|--------|--------|
| Native-backed pause VR overlay | **Abandon** (see `pause-native-vr-overlay-feasibility-20261012.md`) |
| Further pause-menu implementation research | **Stop** |
| Accepted Steam DLL | **Preserve** — no research deploy required for this gate |
| In-game pause visuals | Remain on current **`HudStereoPair`** / accepted build track; no new pause modes from this line of work |

---

## 7. Evidence artifacts (local, not committed)

Decompressed packages under `deployment/_pkg-out/` (`GFxUI.u`, `RvGame.u`) from Steam `CookedPC` via `tools/uedecompress/uedecompress.exe`. Reproducible from any agent with Steam install + build script.

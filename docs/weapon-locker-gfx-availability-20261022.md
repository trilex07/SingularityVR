# WeaponLocker offered-weapon list — GFx / UnrealScript investigation (2026-10-22)

**Scope:** Offline asset + export evidence for **which list the Equip tab cycles** (fourth bucket:
not catalog-only, not owned, not merely equipped). **No deploy, no hooks, no headset.**

**Related:** page state **PROVEN** (`docs/weapon-locker-vr-loadout-audit-20261022.md` §1);
engine catalog **`GGameSystem → RvGameSystem.mWeapons`** (`docs/weapon-locker-availability-contract-20261022.md`).

---

## Ranked conclusion

### (A) PROVEN `availableOrdinaryMask` — **No**

We did **not** recover decompiled ActionScript that builds the locker cycle list, nor a native
function that returns a 4-bit ordinary mask. **Do not** use `ownedMask` or `0xF`.

### (B) STRONG CANDIDATE — **Yes**

**Offered names** while cycling are produced by the **`RvUIGFxMovie_WeaponLocker` / `UseStation`**
Scaleform stack, calling native **`equip(StrProperty Weapon, Int slotNum)`** with **no ownership check**
in `0xF00F50`. That matches PSVR2: `ownedMask = 0x1` but slot 1 accepted **`ARMachineGun`** /
 **`ARShotgun`**.

The **row source** for those names is **very likely** `RvGameSystem.mWeapons` (`WeaponInfo`, stride
`0x190`) filtered in **UnrealScript** (Initialize / inherited list helpers), then displayed in GFx.
**One short runtime correlation** remains: log list rows vs `equip` FNames in the same session.

### (C) NOT FOUND as isolated SWF — partial

The in-game locker movie is **not** a separate `GFxMovieInfo` export named `Menu_WeaponsLocker` in
scanned packages (see below). ActionScript for the station UI is **not** available as plain text in
committed assets; **next offline step** is UE3 bytecode decompile of `RvUIGFxMovie_WeaponLocker` /
 `RvUIGFxMovie_UseStation` from `RvGame.decompressed.xxx` (export indices 15318 / 15724).

---

## Packages scanned

| Package | Path | GFx `RawData` exports |
|---------|------|------------------------|
| Single-player UI | `pause-re-lab/UI_SinglePlayer_SF.u` (same content family as Steam `UI_SinglePlayer_SF.xxx`) | **`iis`**, **`SPMenus`** only |
| Game scripts + flash | `research/native-hands-20261007/RvGame.decompressed.xxx` | **`menu_start`**, **`HUD`**, **`Dialogs`**, **`Credits`** |

Tool: `python tools/reload/inspect_weapon_locker_gfx.py`

**Not in UI package exports:** any `RvUIGFxMovie_WeaponLocker` or extra `GFxMovieInfo` for the station.

**Not in plaintext SWF bytes (all six movies above):** `LoadEquippedWeapons`, `GetGameSystem`, `ARPistol`,
`ARMachineGun`, `WeaponLocker`, `Menu_WeaponsLocker`.

Runtime mod label **`Menu_WeaponsLocker`** = **`RvUIGFxMovie_WeaponLocker`** focus class (1280×720 RTT);
that string does **not** appear in scanned on-disk assets (likely runtime / `MovieInfo` binding only).

---

## Unreal class graph (export inventory)

```
RvUIGFxMovie
  └── RvUIGFxMovie_UseStation
        ├── RvUIGFxMovie_BarisovStation
        └── RvUIGFxMovie_WeaponLocker
```

**`RvUIGFxMovie_UseStation`** (station list pattern — Barisov + WeaponLocker):

| Native exec | Parameters (export) | Inferred role |
|-------------|---------------------|---------------|
| `Initialize` | `responseID` | Open station / seed GFx |
| `GetItemAt` | `Index`, `responseID` | **Indexed list row** for GFx provider |
| `TransferData` | `Start`, `End`, … | Bulk list transfer |
| `equip` | `slotNum`, `perk`, … | Barisov equip (WeaponLocker overrides — see below) |
| `Upgrade` | `perkIndex`, … | Station upgrade |
| `GetTMDEquipment` / perks | … | TMD station data |

**`RvUIGFxMovie_WeaponLocker`** (adds ordinary weapon locker):

| Native exec | Parameters | Role |
|-------------|------------|------|
| `Initialize` | `responseID` | Same family as `LoadEquipped` setup (`0xF237C0`) |
| `LoadEquippedWeapons` | `responseID` | Equip tab load (`0xF23840`) |
| `LoadUpgrades` | `forWeapon`, `responseID` | Upgrade tab load (`0xF87C80`) |
| `equip` | **`Weapon` (Str), `slotNum`, `responseID`** | **Cycle/commit selection** → `0xF00F50` |
| `Upgrade` | `perk`, … | Upgrade tab commit |
| `GetPoints`, `BuyClipForWeapon`, … | … | Upgrade economy |

**Package correction (2026-10-22 offline):** **`GetItemAt` / `TransferData` UFunction exports
live on `RvUIGFxMovie_BarisovStation` only**, not on `WeaponLocker`. WeaponLocker still inherits
C++ vtable slots **`+0x200` / `+0x204`** (same impl as Barisov). See
`docs/weapon-locker-availability-producer-20261022.md`.

**Inheritance implication (Barisov):** GFx likely fills a scrolling list via **`GetItemAt` / `TransferData`**
(from `UseStation`) with **weapon rows**, then calls **`equip(Weapon, slot)`** when the user cycles —
not `NativeOwnedInventory::Mask()`.

---

## ActionScript evidence (extracted SWF only)

### HUD — shared weapon identity map (not the locker UI)

In **`RvGame/HUD`** (`GFxMovieInfo` `RawData`), compiled class  
`__Packages.constants.WeaponNames` maps UI tokens to **lowercase package aliases**:

| AS constant | Alias (engine path key) |
|-------------|-------------------------|
| `W_PISTOL` | `arpistol` |
| `W_MACHINEGUN` | `armachinegun` |
| `W_SHOTGUN` | `arshotgun` |
| `W_SNIPER_RIFLE` | `arsniperrifle` |
| `W_RAILGUN` | `arrailgun` |
| … | heavy / special entries … |

Locker cycling uses **FNames** like **`ARMachineGun`** in native logs; HUD map shows the project’s
**alias convention** (case / `AR` prefix) between GFx and packages — useful when correlating
`WeaponInfo.mName` / `mPath` rows.

### SPMenus — MP loadout only

**`UI_SinglePlayer_SF/SPMenus`** contains `LoadoutDropdown`, `SetupLoadoutSelect`, `RavenDataProvider`,
`clrEquipped` — **multiplayer / frontend loadout**, not the in-world **`RvUIGFxMovie_WeaponLocker`**
station.

### menu_start

Frontend / MP frames (`WMultiPlayerLoadoutFrame`, `equip`, `clrEquipped`). **No** locker exec strings.

---

## Filters (hypothesis — require script decompile or witness)

Expected **UnrealScript-side** filters on `RvGameSystem.mWeapons[]` / `WeaponInfo` before rows reach GFx
(not proven line-by-line):

| Filter | Export / evidence |
|--------|-------------------|
| Exclude heavy / special | `WeaponInfo.mbIsHeavyWeapon`; ordinary cycle uses AR* line |
| Progression / station unlock | Unknown bool on `WeaponInfo`; possibly `mFeaturesUnlocked` / story — **sniper must fail here when locked** |
| **Not** owned inventory | PSVR2: MG/Shotgun offered with `ownedMask=0x1` |
| **Not** `mWeaponsAvailableToLocker` on `RvInventoryItem` | No locker native xref; deprioritized |

**`EnableAllWeaponsForLocker`** (`RvPlayerPawn`, export) is a **cheat/progression unlock**, not the
station cycle list.

---

## Trace backwards (target shape)

```text
User cycles slot on Equip tab (GFx)
  → ActionScript selection handler (SWF not isolated; likely UseStation list UI)
  → fscommand / ExternalInterface → native RvUIGFxMovie_WeaponLocker.equip(Weapon, slotNum)
  → 0xF00F50 writes mEquippedWeapons[slot] FName (no owned check)

Parallel load path:
  → LoadEquippedWeapons / Initialize
  → GGameSystem @ 0x01CF394C, GFx invoke vtable+0x130
  → (rows likely from GetItemAt / mWeapons — needs decompile or witness)
```

---

## Minimal read-only witness (one headset session)

Prefer **list contents** over mask guessing:

1. **`GetItemAt` hook or script log** (if native body resolved):  
   `locker gfx witness: GetItemAt i=N weapon=<str>`

2. **Engine catalog** (already designed):  
   `locker catalog witness: [%d] mName=%s heavy=%d` on `Initialize` / `LoadEquippedWeapons`.

3. **Existing trace**:  
   `equip ENTER slot=%d fname=%s` — correlate with (1) or (2).

**Pass:** For each cyclable ordinary weapon, one stable **`WeaponInfo` row** (or `GetItemAt` index)
matches the `equip` FName, and a **pre-unlock sniper** row is absent from the list.

---

## Bitmask mapping (still **not proven**)

If rows map 1:1 to ordinary slots in **fixed catalog order**, mod convention remains:

| Bit | FName (typical) |
|-----|-----------------|
| `0x1` | `ARPistol` |
| `0x2` | `ARMachineGun` |
| `0x4` | `ARShotgun` |
| `0x8` | `ARSniperRifle` |

**Derive `availableOrdinaryMask` only after** witness proves which rows appear at the station.

---

## Next offline work (before another headset pass)

1. Decompile **`RvUIGFxMovie_WeaponLocker::Initialize` / `GetItemAt`** script from `RvGame.decompressed.xxx`.
2. Locate **`GetItemAt` native body** (registration name in package) and disassemble for **`mWeapons`**
   TArray walk (`stride 0x190`).
3. Optional: extract **Scaleform** for the station clip if `MovieInfo` points at a subfile not listed as
   top-level `GFxMovieInfo` (runtime `MovieInfo` on live movie).

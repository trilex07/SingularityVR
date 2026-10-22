# WeaponLocker offered-weapon list — offline producer audit (2026-10-22)

**Branch:** `feat/locker-vr-loadout-audit-20261022`  
**Basis:** Steam `Singularity.exe` + archived `.text` (`research/etw/capture-20261006-205508`),  
`RvGame.decompressed.xxx`, export inventory `rvgame-assets.txt`.  
**No** runtime hooks, **no** Steam deploy, **no** PSVR2 in this pass.

**Verdict: (B) STRONGER CANDIDATE — not (A) PROVEN.**

The cyclable **Equip** tab names (`ARMachineGun`, `ARShotgun`, …) are **not** explained by
`GetItemAt(Index)` indexing `RvGameSystem.mWeapons[Index]` or by `ownedMask`. The inherited
**GetItemAt / TransferData** natives do **not** walk `mWeapons` with stride `0x190`. The
**WeaponLocker** class has **no** `GetItemAt` / `TransferData` script exports. Progression
filtering for the ordinary four (including sniper exclusion) remains **opaque** at the
**GFx / subobject vtable+0x484** boundary.

---

## 1. Registration and ABI (pinned)

| Symbol | VA | Exit | Role |
|--------|-----|------|------|
| `intURvUIGFxMovie_BarisovStationexecGetItemAt` | `0x00F6E5A0` | script → vtable | FFrame: read `Index`, `responseID`; call movie **`vtable+0x204`** |
| `intURvUIGFxMovie_BarisovStationexecTransferData` | `0x00F6E4F0` | script → vtable | FFrame: read `Start`, `End`, `responseID`; call **`vtable+0x200`** |
| **`RvUIGFxMovie_WeaponLocker` vtable `0x019A0948` +0x200** | `0x00F23A50` | **ret 4** | Shared **TransferData** impl (C++) |
| **+0x204** | `0x00F23B60` | **ret 4** | Shared **GetItemAt** impl |
| **+0x208** | `0x00F23CB0` | **ret 4** | GFx row: `Cost` / `WeaponTech` wchar paths |
| **+0x20C** | `0x00F23DF0` | **ret 8** | GFx lookup via `GGameSystem` + `+0x214` on game system |

Witness: `tools/reload/inspect_use_station_methods.py --strict`, `test_use_station_methods.py`.

**Important:** Package registration strings exist for **BarisovStation** exec names only.
**WeaponLocker** reuses the **same vtable slots** (+0x200/+0x204) but does **not** export
`GetItemAt` / `TransferData` UFunctions.

---

## 2. GetItemAt / TransferData native bodies (disassembly)

Both impls follow the same **GGameSystem** chain (not `GEngine`):

```text
mov eax, [GGameSystem @ 0x01CF394C]
call 0x00EAE080          ; global → URvGameSystem*
vtable+0x194 on game system → call 0x00EADF60
[player + 0x1E0]         ; call 0x00EADA30 → pawn/controller
[player + 0x3D0]         ; call 0x00EAE020 → subobject (inventory/UI helper)
```

**GetItemAt (`0xF23B60`):**

- Optional prologue: movie **`vtable+0x1C4`** (guard); if zero, skip data fill.
- On subobject: **`vtable+0x484`** twice with stack keys **`0x19`** and **`0x37`** (and float
  constants `@ 0x17C3644` / `0x17C3634`).
- Build GFx payload (`0x00D9E600`), then **`vtable+0x130`** invoke with **`[ebp+8]`** =
  `responseID` only on the GFx path.
- **No** `imul 0x190`, **no** load of `Index` from `[ebp+8]` for catalog indexing (Index is
  consumed only by the **script wrapper** when dispatching to this impl; the impl body ignores
  index for TArray).

**TransferData (`0xF23A50`):**

- Same GGameSystem → subobject chain.
- **`vtable+0x44C`** with argument **`2`**, then GFx **`vtable+0x130`**.

**Conclusion:** These are **station UI data providers** (Barisov-style list / bulk transfer),
not **`mWeapons[Index]`** row fetchers.

---

## 3. UnrealScript / export matrix

| Class | List / load API (exports) |
|-------|---------------------------|
| `RvUIGFxMovie_UseStation` | `PopMenu` only (no `GetItemAt` export) |
| `RvUIGFxMovie_BarisovStation` | **`GetItemAt`**, **`TransferData`**, `Initialize`, `equip` |
| `RvUIGFxMovie_WeaponLocker` | `Start`, `Initialize`, `LoadEquippedWeapons`, `LoadUpgrades`, `equip`, `PopMenu` |

**WeaponLocker script** (export tails, `dump_ue3_script_bytecode.py`):

| Function | Script size | Notes |
|----------|-------------|--------|
| `Initialize` | 7 bytes | Native stub → `0xF237C0` impl (GFx invoke only; no catalog loop) |
| `LoadEquippedWeapons` | 7 bytes | Native stub → `0xF23840` (iterates **movie `mEquippedWeapons`**, not full catalog) |
| `LoadUpgrades` | 12 bytes | Native stub → upgrade tab |
| `equip` | 17 bytes | Parms **`Weapon`**, **`slotNum`**, **`responseID`** (export refs `0x3D59`–`0x3D5B`) → `0xF00F50` (**no ownership gate**) |

**Barisov `GetItemAt`:** 12-byte native stub; parms **`Index`**, **`responseID`**.

Flow for **Barisov** (proven dispatch, not row semantics):

```text
GFx → UScript GetItemAt(Index, responseID)
    → exec @ 0xF6E5A0
    → vtable+0x204 @ 0xF23B60
    → subobject vtable+0x484 (keys 0x19 / 0x37)
    → GFx invoke vtable+0x130
```

**WeaponLocker Equip tab** (physical evidence: `ownedMask = 0x1` but slot cycles
`None → ARMachineGun → ARShotgun`):

```text
GFx → equip(Weapon, slotNum)  [script export on WeaponLocker]
    → 0xF00F50 writes FName into movie+0xB0[slot]
LoadEquippedWeapons → syncs equipped[] to GFx / spawn path
```

That path does **not** require `GetItemAt` or `mWeapons` filtering in the natives reviewed.

---

## 4. `RvGameSystem.mWeapons` / `WeaponInfo`

| Item | Evidence |
|------|----------|
| Catalog | `ArrayProperty mWeapons` on **`RvGameSystem`** (export index 6337), **not** checkpoint `StructProperty mWeapons` on session data |
| Stride | `ScriptStruct WeaponInfo` size **`0x190`** (400 bytes) |
| Fields (export names only) | `mbIsHeavyWeapon`, `mNumTechPointsToBuyClip`, `mAmmoType`, floats, `mPerk`, `mType`, `mPath`, `mName`, `Mid` |
| **Instance offsets** | **Not proven** — UProperty **`Offset`** dwords are **0** in package serial for all `WeaponInfo` members (reflection must come from engine layout or native field access, not export order alone) |

No function in the **GetItemAt / TransferData / Initialize / LoadEquippedWeapons** chain
contains **`GGameSystem` + `imul 0x190`** in the same routine (whole-.text scan).

---

## 5. Correlation target (`ownedMask = 0x1`)

| Observation | Offline explanation |
|-------------|---------------------|
| `ownedMask = 0x1` (pistol only) | Consistent with **`NativeOwnedInventory`** |
| Equip slot cycles MG / Shotgun | **`equip`** writes **GFx-supplied** `Weapon` FName **without** owned check; list likely **GFx-side** (see `constants.WeaponNames` in `RvGame/HUD` SWF scan) or **subobject `+0x484`** keys — **not** `ownedMask` |
| Sniper excluded early | **Not proven** — candidate gates: **`mFeaturesUnlocked`** on game/profile exports, **`EnableAllWeaponsForLocker`** on pawn (`0xF6B100` wrapper), or GFx branch — **no** sniper-specific branch found in GetItemAt/TransferData |

**Do not** derive `availableOrdinaryMask = 0xF` or equate it to `ownedMask`.

---

## 6. Outcome (B) — one minimal read-only witness

If runtime confirmation is still needed, prefer **one** of:

1. **Log `equip(Weapon, slotNum)`** on WeaponLocker when the user cycles the replacement row
   (proves GFx producer string per step; compare to `ownedMask` and to sniper lock station).

2. **Log subobject `vtable+0x484`** inside **`0xF23B60`** only when **`LockerNativeTrace`**-style
   hook is authorized — keys **`0x19` / `0x37`** and returned wide strings (bounded).

3. **Optional:** Barisov station **`GetItemAt(Index)`** at **`0xF6E5A0`** — validates list API
   for **Barisov**, not necessarily WeaponLocker Equip tab.

Avoid logging the entire `mWeapons` catalog unless (2) fails to correlate.

---

## 7. Tools

| Tool | Purpose |
|------|---------|
| `tools/reload/inspect_use_station_methods.py` | Addresses, vtable slots, disassembly markers |
| `tools/reload/dump_ue3_script_bytecode.py` | UFunction script tails |
| `tools/reload/inspect_weapon_locker_script.py` | Combined JSON witness |
| `tools/reload/test_use_station_methods.py` | Drift gate |

---

## 8. Next implementation gate (unchanged priority)

1. **Prove** GFx or `+0x484` progression filter (sniper off / MG+Shotgun on) with witness (§6).  
2. Then design `availableOrdinaryMask` / VR Loadout against **proven** producer — not `mWeapons` guess.

See **`docs/weapon-locker-vr-loadout-audit-20261022.md` §7** (page state **done**; availability **blocker**).

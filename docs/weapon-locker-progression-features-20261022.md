# WeaponLocker progression / feature subsystem (2026-10-22)

**Branch:** `feat/locker-vr-loadout-audit-20261022`  
**Prior:** `docs/weapon-locker-availability-producer-20261022.md` (GetItemAt ≠ `mWeapons` indexer)  
**Offline only** — no runtime hooks, no Steam deploy.

---

## Verdict summary

| Track | Verdict | Meaning |
|-------|---------|---------|
| **`vtable +0x484` / `+0x44C` on `[pawn+0x3D0]`** | **(C) unrelated to WeaponLocker availability** | Barisov-style **GFx list UI metrics** (counters at `+0x364`, floats at `+0x374`). Not used on `equip` / `LoadEquippedWeapons` / `Initialize`. |
| **`RvInventoryManager+0x3E4` bitmask** | **(B) progression bitmask on manager — not proven WeaponLocker consumer** | **`FD5AC0` / `FD5BA0`** prove catalog row → **`1<<i`** on manager. **`0x91B221` retired** (wrong vtable — see **`docs/weapon-locker-mask-3e4-contract-20261022.md`**). **Ordinary AR* bit map not proven** offline. |
| **`RvGameSystem.mFeaturesUnlocked`** | **(B) parallel story/progression byte** | `GetFeaturesUnlocked` / `SetFeaturesUnlocked` → vtable **`+0x1F0` / `+0x1F4`**. Not the same storage as manager **`+0x3E4`**. |

**Do not** use `GetItemAt` keys **`0x19` / `0x37`** as weapon-unlock IDs — they are **third stack argument deltas** in **`0xFC4C10`**, not FName ordinals.

---

## 1. Concrete type at `player + 0x3D0`

**`RvInventoryManager`**

| Evidence | Detail |
|----------|--------|
| Mod runtime contract | `native_owned_inventory.h`: **`kPawnInvManager = 0x3D0`**, class name must contain **`InventoryManager`** |
| `EnableAllWeaponsForLocker` | **`0xFFA950`**: `mov eax, [ecx+0x3D0]` then **`0xEAEFB0`** (IsA helper, cache **`0x1D18AB8`**) |
| Barisov / GetItemAt chain | `[pawn+0x3D0]` → **`0xEAE020`** (IsA cache **`0x1D18AB0`**) → vtable calls |
| Export | `Class RvInventoryManager` @ export inventory index 10594 |

**Vtable (pinned):** **`0x019CB480`**

---

## 2. `vtable + 0x44C` → **`0x00FAEAC0`**

**ABI:** `__thiscall`, **`ret 4`**, one **`byte`** argument at `[ebp+8]`.

```text
if (arg >= 4) return 0;
return *(uint32_t*)(this + 0x364 + arg*4);
```

**`TransferData`** (`0xF23A50`) calls with **`arg = 2`** → returns **`manager+0x36C`**, a **dword counter** for GFx bulk transfer (list category **2**), not a weapon id.

---

## 3. `vtable + 0x484` → **`0x00FC4C10`** (+ helper **`0x00FC4AA0`**)

**ABI:** `__thiscall`, **`ret 0xC`**. Stack args (cdecl order on stack after `call`):

| `[ebp+8]` | `[ebp+0xC]` | `[ebp+0x10]` | Role |
|-----------|-------------|--------------|------|
| **2** | **1** | **0x19** | GetItemAt 1st call |
| **3** | **1** | **0x37** | GetItemAt 2nd call |

**Behavior (not unlock logic):**

- Uses **`manager+0xCC`** → sub-object for GFx **`0xD9E600`** payload build.
- **`0xFC4AA0`**: for category index **`bl < 4`**, reads/writes **`[this+0x364 + index*4]`**, calls **`vtable+0x450`** for baseline float, **adds** third arg to counter (scroll/page math).
- **`vtable+0x450` (`0xFFF3B0`)** reads **`[this+0x374 + index*4]`** and issues GFx **`+0x190`** calls — **UI layout**, not inventory.

**Call-site cluster (`.text` scan):** 15 `mov edx, [eax+0x484]` sites; **WeaponLocker-relevant** hits are only **`0xF23C1C` / `0xF23C3B`** inside **`GetItemAt`**. **`0xF00F50` `equip`**, **`0xF237C0` `Initialize`**, **`0xF23840` `LoadEquippedWeapons`** — **no** `+0x484` / `+0x3E4` / `+0x364`.

---

## 4. `EnableAllWeaponsForLocker`

| Item | Value |
|------|--------|
| Script exec | **`0x00F67E40`** (`intARvPlayerPawnexecEnableAllWeaponsForLocker`) |
| Native | **`RvPlayerPawnSP` vtable `+0xDDC` → `0x00FFA950`**, **`ret 4`**, bool at `[ebp+8]` |
| Manager | **`[pawn+0x3D0]`** via **`0xEAEFB0`** |
| Catalog | **`GGameSystem @ 0x01CF394C`**: data **`+0x160`**, count **`+0x164`**, element stride **`0x50`** |
| Match helper | **`0xFD5AC0`** — finds catalog row by weapon path/name |
| State write | **`or` / `and not` on `[manager+0x3E4]`** with **`1 << index`** (`0xFFA9DC` / `0xFFA9E6`; also **`0xFD5BC0` / `0xFD5BCD`**) |

**Does not:** walk `mWeapons` with **`0x190`** stride; does not patch **`RvInventoryItem.mWeaponsAvailableToLocker`** (separate **`IntProperty`** on **`RvInventoryItem`**, export 10573).

**Interpretation:** cheat/progression hook sets a **manager-local bitmask at `+0x3E4`** aligned with catalog rows — plausible **`availableOrdinaryMask`** storage, but **per-weapon bit indices** must be correlated at runtime (catalog order vs AR*).

---

## 5. Related progression (not `+0x484`)

| Mechanism | Location |
|-----------|----------|
| **`GetFeaturesUnlocked` / `SetFeaturesUnlocked`** | **`RvGameSystem`**, exec **`0xF55B30` / `0xF55B70`**, impl vtable **`+0x1F0` / `+0x1F4`** |
| Export | **`ByteProperty mFeaturesUnlocked`**, enum **`EFeaturesUnlocked`** |
| ~~GFx gate using **`+0x3E4`~~ | **Retired:** **`0x91B221`** is **`vtable 0x019D4488` (+578)**, not InvMgr / WeaponLocker; treats **`+0x3E4` as `UObject*`** → **`0x40BEF0`**, not bitmask. |

**Why MG + Shotgun can show with `ownedMask = 0x1`:** GFx **`equip(Weapon)`** still has **no owned check**; cyclable names are **not** explained by **`+0x484`**. If progression gates exist in native UI, **`+0x3E4`** (or GFx-only) is the **next** boundary — **not** GetItemAt keys.

**Sniper exclusion:** not tied to **`0x19`/`0x37`**. Candidate: **catalog row filter** in **`0xFFA990`** loop (`byte+0x24`, **`test [eax+0x4c], 1`**) before a bit is set; or GFx-only branch — **needs witness**.

---

## 6. Minimal witness (if authorized later)

1. On WeaponLocker **`equip`**: log **`Weapon` FName** + **`*(uint32_t*)(manager+0x3E4)`** (same manager as `pawn+0x3D0`).
2. Optional: when **`EnableAllWeapons`** is not used, log **`FD5AC0` index** for that FName once (bounded).

No full catalog dump.

---

## 7. Tooling

```text
tools/reload/inspect_locker_progression_features.py --strict
tools/reload/test_locker_progression_features.py
```

Update **`docs/weapon-locker-availability-producer-20261022.md`** §6 to prefer **`+0x3E4`** over **`+0x484`** hooks.

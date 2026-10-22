# `0xFE2500` — `+0x3E4` → WeaponLocker Initialize offered list (**A1**)

**Branch:** `feat/locker-vr-loadout-audit-20261022`  
**No** runtime witness, **no** Steam deploy, **no** PSVR2.

## Verdict **A1**

**`RvInventoryManager+0x3E4` → WeaponLocker Initialize offered-list source: PROVEN offline.**

The progression bitmask on the inventory manager is **directly** read by **`0xFE2500`**, which emits the **`TArray<FName>`** that **`0xF11570`** intersects with the live **`GGameSystem+0x160`** catalog before **`Initialize`** pushes data to GFx via **`vtable+0x130`**.

No init-time availability witness is required for the **`+0x3E4` contract** (catalog **string content** at runtime may still matter for VR path mapping, but **not** for “which weapons are offered”).

---

## 1. `FE2500` ABI (pinned)

| Item | Value |
|------|--------|
| **Range** | **`0x00FE2500` … `0x00FE25B2`** (`ret 4`) |
| **Convention** | **`__thiscall`** — **`ecx` = `this`** |
| **Arg** | **`[ebp+8]`** → **`TArray<FName>*`** (UE3: `Data`, `Count`, `Max`) |
| **Cleanup** | **`ret 4`** — caller pops **one** stack argument |
| **Output** | Resets **`Count`/`Max`**, frees old **`Data`** if needed, **appends** `(Index, Number)` FName pairs via **`0x44B0F0`** |
| **Callers (E8)** | **`0xF11627`** (WeaponLocker init helper), **`0x10057EA`**, **`0xF419FB`**, … — all use **`EAEFB0(pawn+0x3D0)`** pattern for **`this`** |

**Does not** call **`FD5AC0`** / **`FD5BA0`**.

---

## 2. Input object from `F11570` (**PROVEN**)

Immediately before **`call 0xFE2500`** @ **`0xF11627`**:

```text
GGameSystem* gs = *(GGameSystem @ 0x01CF394C);     // kept in esi for later loops
player = gs->vtable+0x194();
pawn   = EADA30(player + 0x1E0);
manager = EAEFB0(pawn + 0x3D0);   // RvInventoryManager IsA path (same as EnableAll)
mov ecx, manager
push &TArray<FName>   // [ebp-0x4C]
call FE2500
```

**`EAEFB0`** validates **`pawn+0x3D0`** against cached **`RvInventoryManager`** class (**`0x1D18AB8`**), same as **`EnableAllWeaponsForLocker`** in `docs/weapon-locker-mask-3e4-contract-20261022.md`.

**Vtable pin:** **`RvInventoryManager` = `0x019CB480`**.

---

## 3. `+0x3E4` participation (**PROVEN**)

Disassembly @ **`0xFE2539`–`0xFE2576`**:

```text
if (this->+0x3E4 == 0) return empty array;

gs = GGameSystem();  // 0xEFA070
for (i = 0, rowOff = 0; i < gs->+0x164; ++i, rowOff += 0x50) {
  row = gs->+0x160 + rowOff;
  if (row[+0x24] != 0) continue;
  if (row[+0x4C] & 1) continue;
  if ((this->+0x3E4 & (1u << i)) == 0) continue;

  // FName from catalog primary FString at row+0 (empty -> 0x179E4D0)
  FName fn = 4AFFD0(row FString);
  TArray_Add(out, fn);  // 44B0F0
}
```

This is the **inverse** of **`EnableAllWeaponsForLocker`**: there **`FD5AC0(path)` → `1<<i` → OR/AND `+0x3E4`**; here **`+0x3E4` bit `i` → FName from row `i`**.

**Bit index `i`** is **catalog row index**, matching **`FD5AC0`** return **`1u << i`**.

---

## 4. FName source

| Source | Used in `FE2500` |
|--------|-------------------|
| **`catalog row+0`** (`FString` → **`0x4AFFD0`**) | **Yes** — only identity written to output |
| **`row+0x0C`** | **No** in `FE2500` |
| Hard-coded ordinals | **No** |
| Inventory item fields | **No** |

**`0x1783598`** (called from **`F11570`**) compares each **output FName** to **`row+0` / `row+0x0C`** (as FNames) while scanning the full catalog — **match gate** for GFx row build, not a second progression source.

---

## 5. `FE2500` → `F11570` → GFx (**end-to-end**)

```text
RvInventoryManager+0x3E4                    [PROVEN]
    -> FE2500: set bits -> TArray<FName>    [PROVEN]
    -> F11570: outer idx 0..Count-1        [PROVEN]
        inner: foreach catalog row         [PROVEN]
          1783598(outFName, row+0/+0xC)    [PROVEN call site]
          skip row+0x24, row+0x4C&1        [PROVEN]
          build GFx property bag           [PROVEN: 4AE4C0/4498C0/D9E600]
Initialize @ F237C0                        [PROVEN caller]
    -> F11570(1, payload*)                  [PROVEN]
    -> movie vtable+0x130(payload, GFx)     [PROVEN invoke]
    -> GFx selectable list                  [inferred — AS/UI not disassembled]
```

---

## 6. VR `availableOrdinaryMask` translation

Use **native catalog index → bit in `+0x3E4`**, then map ordinary weapons to VR bits (from offline default order + PSVR2 **`0x27`** consistency):

| Catalog index | Native bit in `+0x3E4` | VR ordinary bit | Weapon |
|---------------|------------------------|-----------------|--------|
| 0 | `0x1` | `0x1` | Pistol |
| 1 | `0x2` | `0x2` | MG |
| 2 | `0x4` | `0x4` | Shotgun |
| 5 | `0x20` | `0x8` | Sniper |

**Implementation note:** VR should read **`*(uint32_t*)(manager+0x3E4)`** and map set native bits through this table — same bits **`FE2500`** uses for Initialize.

---

## 7. Witness v2 status

**Not justified** for **`+0x3E4` / offered-list** after this bridge.

Optional future work (separate from availability): one-time catalog **FString content** log if VR path strings still fail to match **`wp_SP_*`** literals — that is **identity encoding**, not **eligibility**.

If implementing anything later, prefer **`vtable+0x130`** with **`Initialize`** return-address gate (see re-audit §9) — **do not** hook **`Initialize`** whole function without full ABI pin.

---

## Tooling

- `tools/reload/inspect_fe2500_locker_progression.py` — JSON + marker checks  
- `tools/reload/test_locker_availability_postmortem.py` — includes **`test_fe2500_a1_bridge`**

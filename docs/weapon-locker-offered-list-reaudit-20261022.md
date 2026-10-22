# WeaponLocker offered list — offline re-audit (2026-10-22)

**Branch:** `feat/locker-vr-loadout-audit-20261022`  
**Basis:** Steam `Singularity.exe` + archived `.text` (`research/etw/capture-20261006-205508`),  
`RvGame.decompressed.xxx`, export inventory `rvgame-assets.txt`.  
**No** runtime hooks, **no** Steam deploy, **no** PSVR2.

**Verdict: (A1)** — see **`docs/weapon-locker-fe2500-3e4-bridge-20261022.md`**: **`RvInventoryManager+0x3E4` → `0xFE2500` → `F11570` → Initialize GFx** is **PROVEN offline**. Init-time **availability** witness is **not required** for the **`+0x3E4` offered-list contract**.

Earlier **(B)** sections below remain as historical RE context (F00B60, LoadEquipped **A**, etc.).

---

## 1. Catalog witness — corrected interpretation

**Wrong postmortem framing:** “`row+0` is not `FString`.”  
**Correct:** disassembly at **`0x00FD5AC0`** proves **`row+0` is an `FString`** compared via **`0x4409F0`** after **`0x4AC340`** copy; filters **`row+0x24`** (byte) and **`row[0x4C] & 1`**.

**What failed on PSVR2:** **content/layout semantics** of live `FString` data (empty pool, full package path, casing, prefix/suffix, non-ASCII), plus witness **exact `wp_SP_*` ASCII match** on **`ReadFStringAscii(row+0)`** only.

### Runtime catalog reader contract (witness v2 — design only)

Once per locker session (no L/R navigation required), for each live row `i` in `[0, count)` with `count = *(GGameSystem+0x164)`,  
`row = *(GGameSystem+0x160) + i * 0x50`:

| Field | Log |
|--------|-----|
| `primary.Data` | `*(uintptr_t*)(row+0)` |
| `primary.Num` | `*(int32_t*)(row+4)` |
| `primary.Max` | `*(int32_t*)(row+8)` if UE3 `FString` triplet applies |
| `primary` text | bounded `wchar_t` dump, **full value**, cap e.g. 256 chars — **no** `wp_SP_*` pre-filter |
| `secondary` | same at **`row+0x0C`** |
| filters | `filter24 = *(uint8_t*)(row+0x24)`, `filter4C = *(uint32_t*)(row+0x4C)` |

**Line format (example):**

```text
catalog[i] primary="<full bounded string>" secondary="<...>" num=N filter24=X filter4C=Y
```

Then (separately) map rows to **`+0x3E4`** bits **only after** identity strings are known.

---

## 2. Native `0xF00B60` — name, ABI, role

| Item | Finding |
|------|---------|
| **Export name** | **Not** in `RvUIGFxMovie_WeaponLocker` UFunction export list (`Initialize`, `LoadEquippedWeapons`, `LoadUpgrades`, `equip`, … only). |
| **Registration** | Engine native cluster **`0x0199D730`**: `0xEF70D0`, **`0xF00C50`**, `0xEF7080`, `0xEDBB80`, **`0xF00B60`** (pointer @ **`0x0199D758`**). |
| **Vtable confusion (corrected)** | **`WeaponLocker` vtable `+0x1DC` = `0xEF7370`**, **not** `0xF00B60`. Both **`0xF00B60`** and **`0xF00C50`** **call** `vtable+0x1DC` first (guard / subobject fetch). |
| **Best-supported label** | **`Native_SetRequestedPackageState`** (behavioral; C++-only, no script export name recovered). |
| **ABI** | `__thiscall` (`ecx` = movie), **`ret 0x14`** → **5 DWORD stack args** after `this`. |
| **`[ebp+0x8]`** | Not consumed in the hot path shown (may be unused/reserved for UE3 stub). |
| **`[ebp+0xC]`** | **float** — if ≠ `mActiveWeaponSlot` (`movie+0xB8`), writes **`+0xB8`** and sets dirty **`+0xA4` bit 0**. |
| **`[ebp+0x10]`** | **0/1** — desired **bit 0** of **`mRequestedPackages` (`+0xBC`)**. |
| **`[ebp+0x14]`** | **0/1** — desired **bit 2** (toggle mask **`4`**). |
| **`[ebp+0x18]`** | **0/1** — desired **bit 1** (toggle mask **`2`**). |
| **Bits meaning** | **Package-request bitfield**, not weapon identity. **Not** `offeredOrdinaryMask`. |
| **Preview callback?** | **Unproven.** No `.text` `E8` call sites; not in WeaponLocker vtable; likely GFx/external or native-table dispatch. **Do not** call it L/R preview until runtime shows it on navigation. |
| **Every L/R move?** | **Unlikely required** for offered list; only justified if init/load paths show repeated calls (not observed offline). |

**`vtable+0x1DC` (`0xEF7370`)** — refreshes **`movie+0xC0`** from **`GGameSystem` vtable `+0x73C`** after **`+0x1E8` / `+0x1E4`** hooks; **`F00B60`** bails if this returns zero.

---

## 3. Native `0xF00C50` — follow-up after `mRequestedPackages`

| Item | Finding |
|------|---------|
| **Label** | **`Native_ApplyRequestedPackages`** (behavioral). |
| **Relation to `F00B60`** | **`F00B60`** updates **`+0xBC` / `+0xB8` / dirty `+0xA4`**; **`F00C50`** **consumes** **`+0xBC`** and pushes state into game/player objects, then calls **`0xF00AE0`** (weapon mesh / package instance path). |
| **`mActiveWeaponSlot (+0xB8)`** | **Float** slot/scalar mirrored into profile / helper objects and fed to **`GGameSystem` subobject `vtable+0x160`** as **`float`** with constant **`0xD` (13)** — **package/streaming channel**, not an FName. |
| **`GGameSystem` subobject `vtable+0x160`** | **`0xD80EF0`** — builds load request via **`0x52F560`**, pushes **two `FString` args** + type tag **`5`** into streaming subobject **`vtable+0x2C`**. **Generic package-by-name load**, not catalog row index. |
| **`vtable+0x15C`** | **`0xD80E30`** — **`float`** parameter block (type **`3`**) on same streaming object — **scalar package param**, not weapon list. |
| **Weapon identity passed?** | **No FName / catalog index** in this path — only **bit mirrors** of **`+0xBC`** into **`player+0x620`**, **`game+0x130`**, profile **`+0x4C8`**, etc. |
| **Conclusion** | **`preview selection → package request → weapon identity`** is **not** proven here; this is **package-load / dirty-flag synchronization**. |

---

## 4. `LoadEquippedWeapons @ 0xF23840` — classification

| Check | Result |
|--------|--------|
| Loop bound | **`movie+0xB4`** (= **`mEquippedWeapons`** array **Count**) |
| Data source | **`movie+0xB0`** — **`FName` pairs** per equipped slot |
| GFx path | **`0x4AE4C0` / `0x4498C0` / `vtable+0x130`** per slot (empty-slot literals @ **`0x1955898`** region; non-empty via **`0x4AC340`**) |
| Full catalog / `+0x3E4` / `GGameSystem+0x160` iteration | **Absent** |

**Classification: (A) current equipped slots only** — **retire** as **full offered-list** witness.

---

## 5. `Initialize @ 0xF237C0` + `0xF11570` — payload audit

**`Initialize` (7-byte script stub → `0xF237C0`):**

```text
push 1
lea payload, [ebp-0x18]
call 0xF11570          ; build GFx data buffer
mov ecx, [script parm: GFx target / responseID @ ebp+8]
movie->vtable+0x130(payload, ecx)
```

**`0xF11570` (catalog × progression builder):**

1. `GGameSystem` → player → subobject **`+0x3D0`**.
2. **`0xFE2500`** fills **`TArray<FName>`** progression/unlock list (`ebp-0x4C`, count `ebp-0x48`).
3. **Outer loop:** `0 .. progressionCount-1`.
4. **Inner loop:** `0 .. catalogCount-1` on **`GGameSystem+0x160/+0x164`**, stride **`0x50`**:
   - skip **`row+0x24 != 0`**, **`row[0x4C]&1`**
   - **`0x1783598`**: match progression **`FName`** to catalog **`row+0` / `row+0x0C`**
   - on match: push **`row+0x28/0x2C`**, **`row+0xC/0x10`**, floats **`+0x34..+0x48`**, upgrade counts via subobject **`vtable+0x444/0x448`**
   - pack into GFx invoke via **`0x4AE4C0` / `0x4498C0` / `0xD9E600`**

**Does not read `manager+0x3E4` directly** in this function; filtering is **catalog row flags + progression FName list + `0x1783598`**.

**Initialize sends:** **progression-filtered weapon row payloads** (stats/names for GFx), **not** generic movie setup alone.

---

## 6. `mRequestedPackages (+0xBC)` — xref classification

| VA | Access | Class |
|----|--------|--------|
| **`0xF11497`** | write **0** | **initialization** (`Start @ 0xF11430`) |
| **`0xF00BF0` / `0xF00C1B` / `0xF00C45`** | read/write bit toggles | **package flag update** (`F00B60`) |
| **`0xF00BD0` / `0xF00BF6` / `0xF00C21`** | read | same |
| **`0xF00CE2` … `0xF00E17`** | read + XOR into profile/game | **async apply** (`F00C50`) |
| **`0xEDBBD4` / `0xEDBC0B`** | R/W XOR | **package sync variant** (`EDBB80`) |
| **`0xEFC0C5`** | read → copy to buffer | **serialization / GFx export snapshot** |
| **`0xF04740`** | read → push | **3D preview / material** (uses **`+0xBC`** as int arg), **not** list seed |

**Explicit:** tracks **per-slot package load/request state** and dirty propagation — **not** offered ordinary mask.

---

## 7. GFx list initialization — architecture hypothesis

**Supported offline shape:**

```text
native Start → clears +0xBC, sizes mEquippedWeapons
native Initialize → F11570 builds progression∩catalog rows → GFx vtable+0x130 (seed)
LoadEquippedWeapons → (A) syncs equipped FNames only
GFx ActionScript → builds selectable array / index (L/R likely here)
native equip @ F00F50 → commit FName to mEquippedWeapons[slot]
```

**Falsifiers to watch in v2 witness:**

- Init **`vtable+0x130`** payload contains **≤4** ordinary names → supports offered-four list seed.
- Init payload **only** metadata with **no** name strings → push producer back to pure AS.
- **`F00B60`** never fires during L/R → confirms preview is **not** this native.

---

## 8. Live `0x27` (conservative)

If index→bit map **0→0x1, 1→0x2, 2→0x4, 5→0x20** holds, **`0x27 = 0x1|0x2|0x4|0x20`** matches **four ordinary bits** in this save.  
**Not** offered-list proof; **no** sniper-negative on this save; **no** early-save PSVR2 until producer witness runs.

---

## 9. Witness v2 (design only — no build/deploy)

**Priority 1 (best):** one-shot hook **`Initialize @ 0xF237C0`** after **`F11570`**, log **`[ebp-0x18]`** GFx buffer: bounded wchar **method names** and **string args** from **`0x4498C0`/`0x4AE4C0`** chain — **offered list seed**.

**Priority 2:** same for **`vtable+0x130`** entry (movie, payload, responseID).

**Priority 3:** session-open **catalog contract** (§1) once.

**Include once/session:** optional full **17-row** catalog log.

**Avoid:** `equip @ 0xF00F50`, full `LockerNativeTrace`, `LoadUpgrades`, per-frame polling, **`F00B60`** unless offline+runtime prove preview dispatches it.

---

## Tooling

- `tools/reload/inspect_locker_offered_list_reaudit.py` — machine-readable summary of this pass  
- `tools/reload/inspect_locker_preview_navigation.py` — corrected `+0x1DC` / preview notes  
- `tools/reload/test_locker_availability_postmortem.py` — offline gates

---

## Verdict summary

| Grade | Meaning |
|-------|---------|
| **A** | Full offered-list producer **proven offline** — **not claimed** |
| **B** | **Strongest candidate** = **`F11570` via `Initialize`**; **`+0x3E4`** still consistent with catalog bits; **one tiny init witness** remains |
| **C** | **`+0x3E4`** disconnected from offered list — **not supported** |

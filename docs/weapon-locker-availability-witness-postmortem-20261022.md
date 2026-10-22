# WeaponLocker availability witness — postmortem (PSVR2 run)

**Run verdict:** **INCONCLUSIVE — witness assumptions wrong** (not **C** / not falsified **`+0x3E4`**).

**Steam:** restored stable `d3d9.dll` `9E7CEFF9…`; `LockerAvailabilityTrace=0`, `LockerNativeTrace=0`.

---

## What the run proved

| Fact | Evidence |
|------|----------|
| Valid `RvInventoryManager` | Session open `mgr=…`, vtable gate would pass |
| `manager+0x3E4 = 0x27` | Open + summary |
| Live catalog `count = 17` | Matches `Default__RvGameSystem.mWeapons` default count |
| Session gates OK | Summary on close; hook installed |
| **Zero** `equip @ 0xF00F50` | Full L-end → R-end sweep, no `locker avail equip:` lines |
| **Zero** `wp_SP_*` path hits | `ReadFStringAscii(row+0)` exact match failed for all four ordinals |

---

## Objective A — Real runtime `0x50` row layout (FD5AC0)

**Disassembly conclusion:** identity for bitmask lookup is **`FString` at `row+0`**, compared via **`0x4409F0`**, after copying the query path with **`0x4AC340`**. It is **not** a raw pointer scan and **not** “skip row+0”.

```text
row = GGameSystem->catalogData_160 + i * 0x50
if (row[0x24] != 0) continue
if (row[0x4C] & 1) continue
if (FStringEquals(row, queryPath)) return 1u << i
```

**Secondary field:** `FString` at **`row+0x0C`** used in **`0xF033F0`** unlock (`GGameSystem` vtable **`+0x20C`**), **not** in `FD5AC0` equality.

**Why the witness failed:** `LockerAvailabilityTrace` used **`ReadFStringAscii(row+0)`** and **exact `wp_SP_*` substring** matching. That ignores:

- UE3 **`FString`** empty/default pool (`0x179E4D0`) semantics
- Possible non-ASCII or non-literal path text at `+0`
- Need to log **all 17** row strings (and **`+0x0C`**) — not only four ordinals

Tool: `tools/reload/analyze_catalog_row_fd5ac0.py`

---

## Objective B — Static `Default__RvGameSystem` → live rows

- Property **`mWeapons`**: **`ArrayProperty`**, **`count = 17`** in default export blob.
- Path order in blob matches regex catalog order (pistol, MG, shotgun, gattling, railgun, sniper, …).
- **Offline index check (binding hypothesis, not PSVR2-proven row bytes):**

| runtime/catalog index | path | native mask |
|---:|---|---:|
| 0 | wp_SP_ARPistol | 0x1 |
| 1 | wp_SP_ARMachineGun | 0x2 |
| 2 | wp_SP_ARShotgun | 0x4 |
| 3 | wp_SP_ARGattlingGun | 0x8 |
| 4 | wp_SP_Railgun | 0x10 |
| 5 | wp_SP_ARSniperRifle | 0x20 |
| … | … | … |

Serialized default rows ≈ **420 bytes** each; **runtime `+0x24` / `+0x4C` offsets apply only to hydrated `0x50` memory**, not to raw default-property bytes.

Tool: `tools/reload/decode_gamesystem_mweapons_default.py`

---

## Objective C/D — Preview vs commit; GFx navigation

| Native | VA | Role |
|--------|-----|------|
| **`equip` hot** | **`0xF00F50`** | **COMMIT** — writes **`mEquippedWeapons[slot]`** FName from script **`Weapon`** parm; **no** `.text` `call` sites |
| **`equip` full** | **`0xF00FA0`** | Ownership / **`vtable+0x208`** (`0xF23CB0`) before inventory add |
| **`LoadEquippedWeapons`** | **`0xF23840`** | **Classification A** — syncs **equipped slots only** (`+0xB4` count, `+0xB0` FNames); **not** full offered catalog |
| **`Start`** | **`0xF11430`** | Clears **`mRequestedPackages` (+0xBC)**, resizes equipped arrays |
| **`Initialize`** | **`0xF237C0`** | **`0xF11570`** (catalog × progression → GFx buffer) + **`vtable+0x130`** — **strongest offered-list seed candidate (offline)** |

**Classification:** Left/right preview **did not** hit **`equip @ 0xF00F50`** → treat preview as **GFx-side list index** until disproven; native **`equip`** is **commit**.

Tool: `tools/reload/inspect_locker_preview_navigation.py`

---

## Objective E — `mRequestedPackages` (+0xBC)

- **`IntProperty`** bitfield on **`RvUIGFxMovie_WeaponLocker`**
- **`Start`** zeroes it (`mov [ebx+0xBC], ecx` @ **`0xF11497`**)
- Native **`0xF00B60`** (**not** `vtable+0x1DC`; slot **`+0x1DC` = `0xEF7370`**) — **`ret 0x14`**, toggles **bits 0 / 1 / 2** of **`+0xBC`** from **`[ebp+0x10..0x18]`**, float **`[ebp+0xC]` → **`mActiveWeaponSlot (+0xB8)`**. **Unexported** UFunction name; **not proven** L/R preview callback.
- Follow-on **`0xF00C50`** mirrors **`+0xBC`** into profile/game and calls **`GGameSystem` subobject `vtable+0x160/0x15C`** (package streaming — **no weapon FName**).

**Not** `offeredOrdinaryMask`. Full xref table: `docs/weapon-locker-offered-list-reaudit-20261022.md`.

**Witness v2 (design only):** prefer **`Initialize` / `F11570` GFx seed** log; **`F00B60`** only if runtime proves dispatch.

---

## Objective F — Live mask `0x27`

**Not proof** of WeaponLocker offered list.

If catalog index → bit mapping holds:

`0x27 = 0x1 | 0x2 | 0x4 | 0x20` → bits **0, 1, 2, 5** only → **all four ordinary progression bits set** in this save.

**No sniper-clear negative case** on this save (would need earlier progression or different save).

Does **not** include gattling (**bit 3 / 0x8**) or railgun (**bit 4 / 0x10**).

---

## Next witness (design only — do not implement until reviewed)

See **`docs/weapon-locker-offered-list-reaudit-20261022.md`** (§1 catalog contract, §9 witness v2).

1. **Best:** one-shot log after **`F11570`** inside **`Initialize`** (GFx offered-list seed).
2. **Second:** once/session **catalog v2** lines (`primary`/`secondary` full bounded wchar — **no** `wp_SP_*` pre-filter).
3. **`F00B60` / `mRequestedPackages`:** only if runtime proves preview dispatch (**unproven**).

**Do not** use **`equip @ 0xF00F50`** for L/R preview or **`LoadEquippedWeapons`** as full-list witness.

---

## Deliverable verdict: **A1** (supersedes interim **B**)

**`RvInventoryManager+0x3E4` → `0xFE2500` → `F11570` → Initialize GFx offered-list source: PROVEN offline** — see **`docs/weapon-locker-fe2500-3e4-bridge-20261022.md`**.

PSVR2 catalog **FString content** miss remains a **string-encoding** issue, not eligibility. **No init-time availability witness** required for **`+0x3E4`**.

**Not C** — **`+0x3E4`** is the direct progression input to Initialize’s weapon list.

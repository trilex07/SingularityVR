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
| **`LoadEquippedWeapons`** | **`0xF23840`** | **Seed GFx** via **`0x4AE4C0` / `0x4498C0`** (invoke payloads) |
| **`Start`** | **`0xF11430`** | Clears **`mRequestedPackages` (+0xBC)**, resizes equipped arrays |
| **`Initialize`** | **`0xF237C0`** | **`0xF11570`** + script **`vtable+0x130`** |

**Classification:** Left/right preview **did not** hit **`equip @ 0xF00F50`** → treat preview as **GFx-side list index** until disproven; native **`equip`** is **commit**.

Tool: `tools/reload/inspect_locker_preview_navigation.py`

---

## Objective E — `mRequestedPackages` (+0xBC)

- **`IntProperty`** bitfield on **`RvUIGFxMovie_WeaponLocker`**
- **`Start`** zeroes it (`mov [ebx+0xBC], ecx` @ **`0xF11497`**)
- Native **`0xF00B60`** (`vtable+0x1DC`, **`ret 0x14`**) toggles bits **0 / 2 / 4** from script args **`[ebp+0x10..0x18]`** when GFx updates package flags
- Follow-on **`0xF00C50`** may call **`GGameSystem` vtable+0x160`** with **`mActiveWeaponSlot`** scalar (**`+0xB8`**)

**Cheapest future read-only witness (design only):** log **`0xF00B60`** entry or **`mRequestedPackages`** deltas — **not** hooked in this pass.

---

## Objective F — Live mask `0x27`

**Not proof** of WeaponLocker offered list.

If catalog index → bit mapping holds:

`0x27 = 0x1 | 0x2 | 0x4 | 0x20` → bits **0, 1, 2, 5** only → **all four ordinary progression bits set** in this save.

**No sniper-clear negative case** on this save (would need earlier progression or different save).

Does **not** include gattling (**bit 3 / 0x8**) or railgun (**bit 4 / 0x10**).

---

## Next witness (design only — do not implement until reviewed)

1. **Session open:** bounded **`wchar` log** for **every** catalog row at **`+0`** and **`+0x0C`** (17 max), plus filters **`+0x24`**, **`+0x4C`**.
2. **Session open (seed):** capture **`LoadEquippedWeapons`** GFx invoke string table once.
3. **Preview (optional):** **`vtable+0x1DC` (`0xF00B60`)** or **`mRequestedPackages`** read-only trace.

**Do not** use **`equip @ 0xF00F50`** for L/R preview.

---

## Deliverable verdict: **B**

**Exact producer boundary known; one tiny runtime witness remains** after correcting catalog string capture and moving offered-list observation to **Initialize/LoadEquipped GFx seed** and/or **`mRequestedPackages` / vtable+0x1DC**.

**Not A** — offered list not observed in PSVR2 run.

**Not C** — **`+0x3E4`** still consistent with catalog progression bits; **`0x27`** matches ordinary OR of inferred indices; no offered-vs-bit contradiction captured.

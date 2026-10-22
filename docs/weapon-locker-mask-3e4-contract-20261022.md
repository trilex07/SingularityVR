# `RvInventoryManager+0x3E4` — offline contract (2026-10-22)

**Branch:** `feat/locker-vr-loadout-audit-20261022`  
**Scope:** Close the `+0x3E4` bitmask / catalog mapping offline. No Steam deploy, no runtime hooks.

---

## Executive verdict

| Question | Verdict |
|----------|---------|
| What is `0xFD5AC0`? | **PROVEN** — weapon path → catalog row index → **`1 << index`** (or **`0xFFFFFFFF`**) |
| `GGameSystem+0x160` row layout | **PARTIAL** — identity + filters **`+0x24`**, **`+0x4C`** documented; full **`0x50`** bytes not all classified |
| `EnableAllWeaponsForLocker` filters | **PROVEN** — same skip rules as `FD5AC0`; bool OR/AND-NOT on manager **`+0x3E4`** |
| Does **`0x91B221`** prove WeaponLocker reads the mask? | **NO — (C) wrong subsystem** — not `RvInventoryManager`, not `RvUIGFxMovie_WeaponLocker` |
| Ordinary AR* → bit index | **NOT PROVEN** offline (catalog order / strings not in `.text`) |
| Is **`+0x3E4`** still a progression bitmask on the manager? | **YES (B)** — written only via **`0xFD5BA0`**; normal game sets bits at pickup / mission paths |

**Overall for VR loadout:** **(B) STRONG CANDIDATE** — native mask semantics and row→bit math are proven; **WeaponLocker GFx consumer of this dword is not proven**; retire **`0x91B221`** as locker evidence. Next boundary remains **GFx `equip(Weapon)` / `LoadEquippedWeapons`** unless a read-only witness ties catalog rows to AR* names.

---

## 1. `0x00FD5AC0` — full reconstruction

**Symbol role:** `CatalogWeaponPathToLockerBitMask` (name recovered from behavior only).

| Item | Value |
|------|--------|
| **VA** | **`0x00FD5AC0`** |
| **ABI** | **`__stdcall`**, **`ret 8`** |
| **Arg0 `[ebp+8]`** | **`FString*`** — weapon path / export name to match (caller builds via **`0x4AFFD0`**) |
| **Implicit `this`** | **`RvInventoryManager*`** in **`ecx`** on entry from direct callers (wrapper passes manager as `this`) |
| **GameSystem** | **`call 0xEFA070`** → **`GGameSystem*`** in **`esi`** |
| **Catalog** | **`data = [gsi+0x160]`**, **`count = [gsi+0x164]`**, **`stride = 0x50`**, loop index **`i = [ebp-8]`** |

**Per-row skip (row = `[gsi+0x160] + i*0x50`):**

```text
if (row[+0x24] != 0) continue;
if (row[+0x4C] & 1) continue;
```

**Match key:** `0x4AC340` copies row identity FString; `0x4409F0` compares to arg0 FString (UE3 **`FString::operator==`** pattern).

**Success return:** **`1u << i`** (single-bit mask, **not** bare index).

**Failure return:** **`0xFFFFFFFF`** (epilogue at **`0xFD5B6E`**: `or eax, -1`).

**Direct callers (`.text`):**

| Site | Role |
|------|------|
| **`0xFD5BB0`** | Inside **`0xFD5BA0`** wrapper |
| **`0xFFA9CC`** | **`EnableAllWeaponsForLocker`** loop |

---

## 2. `0x00FD5BA0` — set/clear one catalog bit on manager

| Item | Value |
|------|--------|
| **ABI** | **`__thiscall`**, **`ret 0xC`** |
| **`this` (`ecx`)** | **`RvInventoryManager*`** |
| **`[ebp+8]`**, **`[ebp+0xC]`** | Weapon path pieces (passed through to **`FD5AC0`**) |
| **`[ebp+0x10]`** | **`bool enable`** — non-zero → **`or [this+0x3E4], mask`**; zero → **`and [this+0x3E4], ~mask`** |

**Call sites (progression, not cheat-only):**

| VA | Context |
|----|---------|
| **`0xF03431`** | Mission / game-system unlock loop (**`0xF03350`**) — same catalog filters as EnableAll, then **`vtable+0x434`** add weapon if missing |
| **`0xF41675`** | Pickup / inventory item bundle — iterates **`[item+0xC8]`** weapon pairs |
| **`0x10419F7`** | Pickup handler — **`[pickup+0x2E8/0x2EC]`** path → enable bit on **`[pawn+0x3D0]`** manager |

**Only bit-mask writes on manager `+0x3E4` in `.text`:** **`0xFD5BC0` / `0xFD5BCD`**, **`0xFFA9DC` / `0xFFA9E6`**.

---

## 3. `GGameSystem+0x160` catalog row (`stride 0x50`)

**TArray header (inferred):** pointer **`+0x160`**, count **`+0x164`**.

**Fields used by `FD5AC0` / `0xFFA950` / `0xF033F0`:**

| Offset | Size | Used as | Semantics (evidence-based) |
|--------|------|---------|----------------------------|
| **`+0x00` / `+0x04`** | FString | Match key vs equip path | Primary weapon identity string (package path / export name) |
| **`+0x0C` / `+0x10`** | FString | **`0xF0343C`** secondary push to **`GGameSystem` vtable `+0x20C`** | Secondary label / display / alternate path (exact role unknown) |
| **`+0x24`** | **`byte`** | Skip if **`!= 0`** | **Exclude from locker bitmask catalog** (heavy/special/non-locker — sniper gating candidate) |
| **`+0x4C`** | **`dword`** | Skip if **`& 1`** | **Second exclusion flag** (bit0 = skip EnableAll / FD5AC0 row) |

**Default empty FString:** literal pool **`0x179E4D0`** when **`[row+4]==0`**.

**Not used by EnableAll path:** `WeaponInfo` **`0x190`** stride / `mWeapons` TArray on manager — confirmed separate.

**Other `+0x164` iterators (sample):** `0x40C088`, `0x726733`, `0xDBA94E`, … — same count field; full row semantics require per-function audit (out of scope except filters above).

---

## 4. `0x00FFA950` — `EnableAllWeaponsForLocker` loop

**Entry:** `RvPlayerPawnSP` vtable **`+0xDDC`**, **`ret 4`**, bool **`[ebp+8]`**.

```text
manager = EAEFB0([pawn+0x3D0]);
gsi = *GGameSystem;
for (i = 0; i < gsi.count_164; ++i) {
  row = gsi.data_160 + i*0x50;
  if (row[+0x24] != 0) continue;
  if (row[+0x4C] & 1) continue;
  path = FStringFrom(row[+0]);  // 4AFFD0, default 179E4D0 if empty
  mask = FD5AC0(manager, path);
  if (mask == -1) continue;
  if ([ebp+8]) manager->mask3E4 |= mask;
  else         manager->mask3E4 &= ~mask;
}
```

**Conditions still respected when “enable all” is true:** rows with **`+0x24 != 0`** or **`+0x4C & 1`** are **never** OR’d — cheat does **not** bypass those catalog exclusions.

**Extra vs `FD5AC0` alone:** EnableAll does **not** call **`vtable+0x434`** add-weapon (**`0xF03484`** path in normal unlock loop).

---

## 5. `0x91B220` consumer — hard gate **FAILED**

| Item | Value |
|------|--------|
| **Function** | **`0x91B220`–`0x91B2E5`**, **`ret 0x20`** |
| **Hot read** | **`0x91B241`**: `mov ecx, [esi+0x3E4]` → **`call 0x40BEF0`** |
| **`0x40BEF0`** | **UObject outer/package helper** (`this=ecx`), **not** a bitmask scan |
| **Follow-up** | **`0x987060`** — class/object lookup from **`[ebp+8]`** index |
| **GFx** | **`vtable+0x120`** on resolved object; also **`vtable+0x578`**, **`+0x54C`** on **`this`** |

**Vtable ownership:**

| Class vtable | **`+0x578`** | **`+0x54C`** |
|--------------|-------------|-------------|
| **`RvInventoryManager` `0x019CB480`** | **`0x4A74F0`** | **`0x8CDBC0`** |
| **`RvUIGFxMovie_WeaponLocker` `0x019A0948`** | **`0x5B93C0`** (stub **`ret`**) | **`0x5B9320`** |
| **Consumer class `0x019D4488`** | **`0x91B220`** | **`0x907AE0`** (shared GFx base slot) |

**Conclusion:** **`0x91B221` does not read `RvInventoryManager`’s bitmask.** Same offset **`0x3E4`** on another type is treated as a **UObject\*** (then **`0x40BEF0`**). **Do not** cite this path for WeaponLocker availability.

**WeaponLocker natives (`equip` / `LoadEquippedWeapons` / `Initialize`):** **no** **`+0x3E4`**, **`FD5AC0`**, or **`FD5BA0`** references in disassembly scan.

---

## 6. All `.text` xrefs to **`[reg+0x3E4]`** (manager vs other types)

| VA | Op | Likely `this` | Role |
|----|-----|---------------|------|
| **`0xFFA9DC` / `0xFFA9E6`** | or/and | **InvMgr** | EnableAll bitmask |
| **`0xFD5BC0` / `0xFD5BCD`** | or/and | **InvMgr** | **`FD5BA0`** |
| **`0xB40D56`** | mov | **InvMgr ctor** | Zero init |
| **`0xB40E78`** | mov/free | **InvMgr dtor** | Treats field as **heap FString** (lifetime); coexists with integer OR — field may stay **0** until first bit set without allocation |
| **`0x6D626C` / `0x6DAAA2`** | mov | UI/state object | Init parallel **`+0x3D8..+0x3EC`** block |
| **`0x6D94A2` / `0xE6A997`** | mov | UI object | **`+0x3E4` = pointer to int array**, indexed |
| **`0x91B241`** | mov | **vtable `0x019D4488`** | UObject\* for **`0x40BEF0`** |
| **`0x102DEE0`** | mov eax | Embedded subobject | Getter returning **`+0x3E4`** |
| **`0x103CE9E`** | mov | GFx bridge | Store from **`0x1051D80`** |
| **`0x1046251` / `0x10462E4` / `0x11AE74A`** | mov | Non-manager UI | Render / layout (verify per function) |
| **`0x13E8D0A` / `0x13E8D4F`** | movzx word | Different layout | **16-bit** at **`+0x3E2/0x3E4`** — not InvMgr bitmask |

---

## 7. Ordinary weapon bit mapping

**Rule (proven):** bit **`N`** = **`1 << catalogIndex`** where **`catalogIndex`** is the **`i`** in the **`+0x160`** loop when **`FD5AC0`** succeeds.

**Not proven:** which **`i`** corresponds to **`ARPistol` / `ARMachineGun` / `ARShotgun` / `ARSniperRifle`** (no AR* strings in `.text`; catalog lives in cooked data).

**Do not assume** VR wheel bits **`1/2/4/8`** match native indices.

---

## 8. Minimal runtime witness (only if authorized later)

Prefer **read-only** correlation using proven offline math:

```text
locker avail:
Weapon=<FName/path>
catalogIndex=<i from read-only catalog scan matching FD5AC0>
nativeBitIndex=<i>
nativeMask=<1<<i>
bitSet=<((manager+0x3E4) & nativeMask) != 0>
```

Avoid logging **`mask=` alone** while cycling weapons — mask may be constant across MG/Shotgun if both bits already set.

---

## 9. Tooling

```text
python tools/reload/inspect_locker_progression_features.py --strict
python tools/reload/test_locker_progression_features.py
```

Pins: **`FD5AC0`**, **`FD5BA0`**, **`FFA950`**, **`+0x3E4` xrefs**, **`0x91B220` vtable gate**, **`FD5BA0` call sites**.

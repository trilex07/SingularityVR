# WeaponLocker `availableOrdinaryMask` — offline availability contract (2026-10-22)

**Page state:** **PROVEN** (session-scoped `LoadEquippedWeapons` / `LoadUpgrades` — see PSVR2 pass `33A84173…`).  
**This document:** availability producer only. **No implementation, no Steam deploy.**

**Steam runtime restored:** `d3d9.dll` `9E7CEFF9…`, `LockerNativeTrace=0`, overlay off.

---

## Ranked conclusion

### (A) PROVEN producer for `availableOrdinaryMask`

**Not found offline.** No bitmask with Centurion/Valkyrie/Shotgun/Sniper bit map was identified. Do **not** use `available = 0xF` or `ownedMask` as availability.

### (B) Strong runtime-witness candidate — `RvGameSystem.mWeapons` catalog

**Owner:** `RvGameSystem` (not `UEngine`).

| Export | Value |
|--------|--------|
| `ArrayProperty mWeapons` | `TArray<WeaponInfo>` on **RvGameSystem** |
| `ScriptStruct WeaponInfo` | size **`0x190`** bytes |
| `ObjectProperty SessionManager` | `RvSessionManager` on same class (separate from weapon **catalog** array) |

**WeaponInfo export field set** (declaration order; **instance offsets not proven** until one reflection pass on `GGameSystem`):

| Field | Type |
|-------|------|
| `mbIsHeavyWeapon` | Bool |
| `mNumTechPointsToBuyClip` | Int |
| `mAmmoType` | Byte |
| `mRateOfFire`, `mRange`, `mDamage`, `mAccuracy` | Float |
| `mPerk` | Str |
| `mType` | Byte |
| `mPath`, `mName`, `Mid` | Str |

**How to reach the table (corrected from failed witness):**

```
GGameSystem*  := *(void**)0x01CF394C   // NOT GEngine @ 0x01CF8778
RvGameSystem* := GGameSystem
TArray        := RvGameSystem + off(mWeapons)   // reflect once; export names only
WeaponInfo[i] := TArray.data + i * 0x190
// primary identity strings: WeaponInfo.mName / mPath (FString — use engine FString layout)
```

**Export disambiguation (same symbol name, different owners):**

| Property | Owner | Type | Role |
|----------|--------|------|------|
| `mWeapons` | `RvSessionManager` checkpoint block | `StructProperty` | Death-checkpoint snapshot — **not** the progression catalog |
| `mWeapons` | **`RvGameSystem`** | `ArrayProperty` → **`WeaponInfo`** | **Catalog table under investigation** |
| `SessionManager` | **`RvGameSystem`** | `ObjectProperty` → `RvSessionManager` | Separate object; prior witness wrongly hung `mWeapons` off **Engine** |

**Native evidence (capture `20261006-205508`):**

| Function | VA | Uses `0x01CF394C`? | Reads `mWeapons` TArray? |
|----------|-----|--------------------|---------------------------|
| `LoadEquippedWeapons` | `0x00F23840` | **Yes** @ `0x00F23886` → vtable **`+0x194`** on `GGameSystem` | **No** (iterates movie `+0xB0/+0xB4` FName array for GFx) |
| `Initialize` | `0x00F237C0` | **Yes** (shared tail with LoadEquipped) | **No** direct TArray walk in scanned prologue |
| `equip` (slot write) | `0x00F00F50` | **Yes** @ `0x00F00FC8` (longer script path) | **No** in short write — stores script FName only |

So **`mWeapons` is the progression/catalog table**, but **locker tab cycling does not prove it is read on every equip** in the short native path. It remains the best **engine-side list** to correlate with GFx-offered names.

**Secondary native (pawn progression, not station UI):**

- `EnableAllWeaponsForLocker` — `intARvPlayerPawnexecEnableAllWeaponsForLocker` @ **`0x00F67E40`** → pawn vtable **`+0xDDC`** (bool). Cheat/progression unlock, not the ordinary station cycle list.

### (B) Strong candidate — GFx / script catalog (primary for **offered** names)

Runtime PSVR2 evidence (`ownedMask = 0x1` only):

- Slot 1 cycled **`None → ARMachineGun → ARShotgun`** while native **`equip @ 0x00F00F50`** only copies FNames into `mEquippedWeapons` (**no ownership check** in disassembly).

Therefore **names offered while cycling are chosen in GFx/script** (or a script `equip` call path), not from `ownedMask`.

| Component | Role |
|-----------|------|
| `Initialize` wrapper | `0x00F6E810` → vtable **`+0x1EC`** → **`0x00F237C0`** |
| `LoadEquippedWeapons` wrapper | `0x00F6E870` → vtable **`+0x1F4`** → **`0x00F23840`** (log `callerRet=0x00F6E8C8`) |
| `equip` registration | `0x00F87D60` (full script path); hot write **`0x00F00F50`** |
| GFx batch invoke | vtable **`+0x130`** → **`0x00D83500`** |

**GFx / AS follow-up (2026-10-22):** `docs/weapon-locker-gfx-availability-20261022.md` — scanned UI + RvGame SWF exports; **strong candidate** = `UseStation` list API + `WeaponLocker.equip`; isolated locker SWF **not found**; HUD `constants.WeaponNames` alias map documented.

### (C) Deprioritized — `RvInventoryItem.mWeaponsAvailableToLocker`

- Export: **`IntProperty`** on **`RvInventoryItem`** only.
- **No** property name string in `Singularity.exe`; resolve only via **`RvGame.xxx` reflection**.
- **No** use in disassembled `LoadEquippedWeapons` / `Initialize` / short `equip` bodies.
- Prior docs: **not** station owned inventory (`docs/combined-acceptance-followups.md`).

**Status:** drop as mask producer until a **WeaponLocker** or **Initialize** xref appears.

### Other movie fields (witness only)

| Field | Reflect offset (2026-10-22 session) | Notes |
|-------|--------------------------------------|--------|
| `mEquippedWeapons` | **+0xB0** (data), count **+0xB4** | 8-byte FName slots; **display/candidate**, not catalog authority |
| `mRequestedPackages` | **+0xBC** | Not read in `equip @ 0xF00F50`; semantics **unknown** |

---

## Minimal read-only witness (next build — design only)

One short session should log **names**, not counts:

```
locker catalog witness: GGameSystem=%p mWeapons count=N
locker catalog witness: [%d] mName=%s mPath=%s heavy=%d type=%d
locker gf x witness: equip ENTER slot=%d fname=%s owned=0x%X
```

**Rules:**

1. Resolve **`GGameSystem @ 0x01CF394C`**, class **`RvGameSystem`**, property **`mWeapons`** (ArrayProperty struct **`WeaponInfo`**).
2. Log **every element** on **`Initialize`** and **`LoadEquippedWeapons` ENTER** (cap e.g. 32 lines).
3. On each **`equip` ENTER**, log **slot + FName** from script frame (existing trace) **and** nearest catalog index match on `mName`/`mPath`.
4. **Do not** call `Poll()` from these hooks; **do not** treat `mWeaponsAvailableToLocker` as station mask without WeaponLocker xref.

**Pass criterion for `availableOrdinaryMask`:** for each weapon the user can cycle on WYPOSAŻ, identify a **stable row or bit** in **`mWeapons`** (or a single GFx list) that is **true** for that FName and **false** for a known locked weapon (e.g. sniper before unlock) — **same session**, same station.

---

## Tooling

```bash
python tools/reload/inspect_locker_availability_offline.py \
  --exe "%Steam%/Singularity/Binaries/Singularity.exe" \
  --text research/etw/capture-20261006-205508/Singularity.text-memory.bin \
  --export research/etw/controller-features-20261006/rvgame-assets.txt
```

Tests: `python tools/reload/test_locker_availability_offline.py`

---

## Summary table

| Question | Answer |
|----------|--------|
| `availableOrdinaryMask` PROVEN? | **No** |
| Best engine table | **`RvGameSystem.mWeapons` / `WeaponInfo`** via **`GGameSystem @ 0x01CF394C`** |
| Best offered-name source | **GFx + unguarded native `equip` write** |
| `mWeaponsAvailableToLocker` | **Not proven**; not locker consumer |
| Failed witness root cause | **`Engine.SessionManager`** path; SessionManager lives on **`RvGameSystem`**, different global |

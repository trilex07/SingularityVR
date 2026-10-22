# Native WeaponLocker contract and VR wheel gap — investigation report (2026-10-22)

**Scope:** Evidence-only audit. **No Steam deploy, no runtime hooks, no INI changes** in this work.

**Tools re-run (offline PASS):**

- `python tools/reload/test_locker_runtime_contract.py` — 6/6
- `python tools/reload/inspect_locker_contract.py` — owned `Singularity.exe` + capture text
- `python tools/reload/inspect_locker_runtime_contract.py` — capture `research/etw/capture-20261006-205508/Singularity.text-memory.bin`, base `0x402000`

**Runtime context (Arm A accepted, not re-tested here):** `%LOCALAPPDATA%\SingularityVR\view_matrix.log` attach `2026-10-22 11:20:03`, `LockerStationGrantOrdinary=0`, `WeaponWheelOwnedInventory=1`, `KeepAcquiredWeapons=1`. Locker focus: `ownedMask=0x1`, Phase B plan `uiOrdinary=1`; wheel later `ownedMask=0x3` after commits — **no crash, no hitch (user confirmed)**.

---

## 1. Confirmed native identities (Singularity 2010, owned EXE)

| Artifact | Value |
|----------|--------|
| `RvUIGFxMovie_WeaponLocker` vtable | `0x019A0948` |
| `Start` | vtable `+0x11C` → `0x00F11430` |
| `PopMenu` | vtable `+0x190` → `0x00F17F60` |
| `LoadEquippedWeapons` | vtable `+0x1F4` → `0x00F23840` |
| `equip` | vtable `+0x1F8` → `0x00F00F50` |
| Weapon controller getters | vtable `+0x740` GetWeaponSlotName, `+0x744` SetWeaponSlotName |
| GFx movie active slot byte | `+0xC0` (PopMenu re-hand) |
| GFx equipped TArray (script object) | data `+0xB0`, count `+0xB4` (LoadEquippedWeapons witness) |
| GFx reflection (runtime) | `mEquippedWeapons` / `mOriginalWeapons` on movie instance (witness logs offset 176 / 164) |

Helper callees referenced in locker bodies (capture witness): `TArray::Find` `0x00447BA0`, `TArray::Append` `0x0044B0F0`, `destroyItem` `0x00A05790`, `spawnByName` `0x00F00FA0`, script callback push `0x00F0B470` → dispatcher `0x0041F380`.

---

## 2. Native call sequence (locker open → close)

```mermaid
sequenceDiagram
    participant GFx as GFx ActionScript
    participant Start as WeaponLocker.Start
    participant Ctrl as Weapon controller
    participant Arr as mEquipped/mOriginal TArrays
    participant Equip as WeaponLocker.equip
    participant Load as LoadEquippedWeapons
    participant Pop as WeaponLocker.PopMenu
    participant Inv as Inventory / pawn

    Note over GFx,Inv: Menu open (GFx invokes Start)
    Start->>Arr: clear original + equipped
    Start->>Ctrl: GetWeaponSlotName(0)
    Start->>Arr: append FName slot 0
    Start->>Ctrl: GetWeaponSlotName(1)
    Start->>Arr: append FName slot 1
    Start->>Arr: copy equipped → original

    Note over GFx,Inv: User toggles weapons in UI
    GFx->>Equip: equip(slotNum, weaponFName)
    Equip->>Arr: equipped.data[slotNum] = FName (no bounds check)

    Note over GFx,Inv: Optional load / station grant path
    GFx->>Load: LoadEquippedWeapons()
    Load->>Arr: read count +0xB4, data +0xB0
    Load->>Inv: grant/spawn per equipped entry (not hardcoded 0/1 loop)

    Note over GFx,Inv: Menu close (GFx invokes PopMenu)
    Pop->>Arr: compare original vs equipped
    Pop->>Inv: destroy items in original not in equipped
    loop for each equipped index i
        Pop->>Ctrl: SetWeaponSlotName(i, equipped[i])
    end
    Pop->>Pop: read active slot byte +0xC0
    Pop->>GFx: script callback 0xF0B470 via 0x41F380 (re-equip hands)
```

**Start (confirmed):** Rebuilds **only two** ordinary entries from controller slots **0 and 1** (`push 0`, `push 1`, two `+0x740` dispatches, two `TArray::Append`). Does **not** read equipped count from `+0xB4` for its producer loop.

**equip (confirmed):** Script `slotNum` → `[ebp+0xC]` → store at `equipped.data[slotNum]` with **no capacity check** (heap corruption if GFx index ≥ capacity).

**LoadEquippedWeapons (confirmed):** Iterates **whatever count** is in the equipped TArray (`+0xB4` / `+0xB0`). It is the **load-time grant producer** for names already in that array — not a substitute for Start’s two-slot controller read.

**PopMenu (confirmed):** Inventory diff + **`SetWeaponSlotName(i, …)` for every equipped index `i`**. If `equipped.Count > 2`, indices **2+ overwrite controller slots reserved for heavy/grenade/special** (offline conclusion in `inspect_locker_runtime_contract.py`). Destroys originals absent from equipped; restores active hand via `+0xC0` and scripted callback.

---

## 3. Three layers: availability vs ownership vs equipped slots

| Layer | What it is | Producer in vanilla | VR mod reads |
|--------|------------|---------------------|--------------|
| **Station / GFx availability** | Which weapons the **UI may offer** at this station (often four ordinary archetypes in design docs) | Scaleform + script calls into `equip` / `LoadEquippedWeapons`; **not** fully reverse-engineered in this pass (no ActionScript extract in repo) | `MenuAssetWitness` → locker slot **counts** only (Arm A: `count=2`, `cap=6` / `cap=2`) |
| **Inventory ownership** | Which weapons exist on **`RvInventoryManager` chain** | Pickups (`RvPlayerPawnSP` vtable **`+0xD70`**, 4-arg, keep-existing at `+1A4`), `SwitchWeaponByArchetype` script path, locker **`PopMenu` destroy** / **`LoadEquippedWeapons` grant** | `NativeOwnedInventory` snapshot: pawn `+0x3D0` → chain `+0x1E0`, `mWeaponName` `+0x2E8` → **mask bits 0x1/0x2/0x4/0x8** |
| **Equipped controller slots** | Which FNames are bound to **weapon controller indices 0…N−1** (Start uses **0 and 1** only) | `Start` ← GetWeaponSlotName(0,1); **`PopMenu` → SetWeaponSlotName(i, …)** for all equipped indices | Not mirrored into VR wheel directly |

**Key separation:** The **VR weapon wheel** (`WeaponWheelOwnedInventory=1`) gates sectors on **`NativeOwnedInventory` mask**, not on GFx TArray contents or controller slot names. The **native locker UI** is seeded by **two controller slots** at Start, regardless of how many icons GFx draws.

---

## 4. Why the VR wheel does not show four ordinary weapons (root cause)

### 4.1 Primary cause (confirmed in Arm A log)

**The wheel only enables sectors for weapons actually present on the inventory chain.**

- At locker focus: `locker virtual: plan … mask=0x1` → one ordinary owned (pistol mesh 5799).
- Wheel draw plan: `ownedMask=1` → **one named sector** (later `ownedMask=3` after gameplay commits).
- This matches `DrawWeaponWheel` logic: with `ownedReady`, `slotNamed[i] = (ownedMask & WeaponBit(verts)) != 0`.

The station locker **did not** add three missing ordinaries to ownership during focus. That is expected with **`LockerStationGrantOrdinary=0`** and **no** working grant path.

### 4.2 Failed grant path (historical evidence — do not repeat)

Focus-time **`mEquippedWeapons` 2→4 + `LoadEquippedWeapons`** (deploy `7EFFFD02…`):

- Log: grant line then **`mask 0x1 -> 0x1 count=1`** — **inventory unchanged**.
- User: hitch/crash.

So **`LoadEquippedWeapons` after TArray rewrite on focus is not a correct “give me four guns” integration**; it either does not touch `NativeOwnedInventory`’s chain or runs in an unsafe re-entrancy window.

### 4.3 Secondary factors (mod policy, not engine bugs)

| Factor | Effect |
|--------|--------|
| `KeepAcquiredWeapons` pickup adapter | Rewrites **keep=0→1** only on **vtable +0xD70** pickup when scoped (`inventory_vm_hook.h`). Arm A log once showed `keep=0->0` — pickup path may not run at station at all. |
| `SwitchWeaponByArchetype` hook | **Does not** force retention (bytecode write withdrawn); only logs / opcode throw guard. |
| Wheel fallback | If `ownedMask==0`, wheel falls back to **mesh-observed** set — not station catalog. With `ownedReady=1`, fallback is **disabled** for naming. |
| Archetype grant experiments | GObjects / unloaded archetype scans **excluded** by policy; earlier attempts failed when classes not loaded. |

**Hypothesis (falsifiable, not yet runtime-proven at station close):** Vanilla gives station ordinaries via **`LoadEquippedWeapons` + PopMenu** when GFx has populated **`mEquippedWeapons`** through **`equip`** and/or level setup — **not** by expanding the wheel mask. VR never observes four owned items because **that grant never ran successfully** for this save/state, not because the wheel renderer is capped at two sectors.

---

## 5. Can four VR wheel weapons coexist with the native two-slot locker?

**Yes, with separation of concerns — but not by “making the locker GFx array four wide” alone.**

| Approach | Coexistence | Risk |
|----------|-------------|------|
| **A. Wheel-only (current Arm A)** | Wheel shows **owned** count (1–4); locker stays native **two-slot Start** | User expectation mismatch at station (“four icons in design” vs two native slots) — **UX**, not crash |
| **B. Phase C TArray rewrite on focus** | UI arrays show four FNames | **PopMenu still writes SetWeaponSlotName(i)** for i≥2 unless **Phase D** filter; **`mOriginalWeapons cap=2`** in witness → partial apply / inconsistency |
| **C. Inventory grant via audited pickup (+0xD70, keep=1)** | Four **owned** entries; wheel OK; locker can stay 2-slot UI | Must run on **input/script thread**, correct **archetype** pointers, **not** during GFx re-entrancy; verify **PopMenu destroy** does not strip grants |
| **D. Fake wheel mask (`0xF`) without ownership** | Shows four sectors | **Selection fails** — `NativeOwnedInventory::Queue` + pawn selector `+0xB14` only equips **owned** FNames |

**Conclusion:** Four **selectable** VR weapons require **inventory ownership** alignment, not GFx TArray surgery alone. Native two-slot locker can remain for **controller slots 0/1** if **Phase D** prevents ordinary names from being written to slots **≥ 2** and grants go through **inventory**, not SetWeaponSlotName(2/3).

---

## 6. Existing retention / pickup path (mod, audited)

| Mechanism | Thread | ABI / identity | Purpose |
|-----------|--------|----------------|---------|
| Native pickup adapter | Engine input poll thread (`NativeOwnedInventory::thread`) | `RvPlayerPawnSP` vtable **`+0xD70`**, `__fastcall`, 748-byte body hash **`CC78CCB4EE3651A2`**, neighbour **`+0xB18` → HasWeapon `0x0102F700`** | **`keep` 4th arg 0→1** when `Retain()` true (`KeepAcquiredWeapons=1`, local pawn, linked manager, supported ordinary archetype, !pause/!cinematic) |
| `RunAuditedPickupRestore` | Same (calls `PickupCall` with keep=1) | Same | Game-authored restore shape (`verify_native_pickup.py`) — **intended Phase F primitive** |
| VirtualFunction opcode adapter | Script thread | Opcode `0x1B` table slot | Suppress exact **`ThrowActiveWeapon`** tail on `SwitchWeaponByArchetype` drop path |
| Pawn weapon select | Input poll | Vtable **`+0xB14`**, FName arg | Wheel commit → existing owned weapon |
| Inventory snapshot | Input poll / locker focus | Chain walk | **Authoritative wheel mask** |

**Not evidence of station grant:** `InvokeRetained` on `SwitchWeaponByArchetype` **forwards** without mutation (log-only).

---

## 7. Minimal Phase F proposal (implementation later — not in this branch)

**Goal:** Up to **four ordinary weapons owned** for the VR wheel at/after weapon **station**, without GFx TArray expansion on focus, without GObjects scans, without fake wheel masks.

### 7.1 Required prerequisites (order matters)

1. **Phase D — PopMenu guard (mandatory before any four-name UI):** Wrap `PopMenu` (`0xF17F60`); intercept **`SetWeaponSlotName`** loop so **`DecidePopMenuControllerWrite(i, count)`** from `locker_virtual_contract.h` — **only `i ∈ {0,1}`** for ordinary FNames. *Regression risk:* wrong filter → special slots blank or wrong re-hand at `+0xC0`.

2. **Phase E — equip guard (recommended):** Wrap `equip` (`0xF00F50`); refuse `slotNum >= equipped.Count` / `>= 4` per `DecideEquipIndex`. *Regression risk:* GFx script assumes silent write — UI desync vs safety.

### 7.2 Smallest grant integration point (recommended)

**When:** **`PopMenu` return** (menu close), on the **same thread** as GFx native call (typically script/game thread — **must match pickup adapter thread evidence**; refuse if `thread != GetCurrentThreadId()`).

**What:**

1. If focus class was **`RvUIGFxMovie_WeaponLocker`** and station menu (class 4) — bounded one-shot flag.
2. `NativeOwnedInventory::SnapshotNow()` → `maskBefore`.
3. For each ordinary archetype **authorized for this station** (source TBD — see §7.3) whose bit is **missing** from `maskBefore`:
   - Resolve **`RvWeaponShared` archetype object** via **existing possessed item template** or **offline name→archetype table** (no full GObjects walk).
   - Call **`InventoryVmHook::RunAuditedPickupRestore(pawn, archetype)`** (keep=1).
4. `Refresh()` + snapshot; log **`maskBefore -> maskAfter`, count** (same receipt style as pickup adapter, cap 12).
5. **Do not** call **`LoadEquippedWeapons`** from mod code until a read-only probe proves it is safe on close (failed on focus).

**Why close, not open:** Start resets arrays from **two controller slots**; open grant fought Start/LoadEquipped re-entrancy. Close follows native **commit** ordering (PopMenu already reconciled equipped vs original).

### 7.3 Open evidence gaps (must close before coding)

| Gap | Smallest experiment (read-only / default OFF) |
|-----|-----------------------------------------------|
| Station **authorized four-pack** vs player **choice of two** | Log **FName list** in `mEquippedWeapons` at **PopMenu entry** (reflection read only, `MenuAssetWitness`-style) across one station visit |
| **`LoadEquippedWeapons` caller** | One-shot return-address log inside existing vtable wrap (no body patch) |
| **Archetype pointer source** | Prove one station ordinary can be resolved from **inventory chain template** or **cached archetype from prior pickup** without scan |
| **PopMenu destroy vs new grants** | Compare chain **before/after PopMenu** when pickup restore runs **after** native PopMenu |

### 7.4 Regression risks (explicit)

- **Special weapons:** Unfiltered PopMenu with four equipped entries → **SetWeaponSlotName(2/3)** clobber (offline proven).
- **Inventory strip:** PopMenu **destroyItem** may remove weapons player deselected in GFx — grants after PopMenu must respect **final** equipped/original diff.
- **Thread / lifetime:** Pickup from render or XR thread → refuse (existing policy).
- **Save pollution:** User asked not to persist experimental inventory during A/B — Phase F tests need **non-save** or rollback discipline.
- **Stereo / menu / hands:** Any hook on Start/PopMenu/equip must stay scoped to **WeaponLocker** vtable identity check (`0x019A0948`).

### 7.5 Explicit non-goals for Phase F v1

- No **`LockerStationGrantOrdinary` on-focus** TArray + LoadEquipped (withdrawn).
- No wheel **`ownedMask | 0xF`** latch.
- No **Phase C `LockerVirtualApply` on focus** without Phase D.
- No **catalog unlock** or save edits.

---

## 8. Summary table (per ordinary weapon — Arm A + design intent)

| Weapon | Bit | Station “available” (design) | Owned at locker (log) | Native pickup at station | Wheel sector |
|--------|-----|------------------------------|------------------------|---------------------------|--------------|
| Pistol / Centurion | 0x1 | Likely | **Yes** (`mask=0x1`) | Not observed | **Yes** |
| MG / Valkyrie | 0x2 | Likely | No | No | No |
| Shotgun | 0x4 | Likely | No | No | No |
| Sniper | 0x8 | Likely | No | No | No |

---

## 9. References in repo

- Offline locker: `tools/reload/inspect_locker_contract.py`, `inspect_locker_runtime_contract.py`, `docs/station-locker-followup-20261022.md`
- Virtual policy: `spikes/view_matrix/locker_virtual_contract.h`, `docs/locker-virtual-contract-sketch.md`
- Ownership / wheel: `spikes/view_matrix/native_owned_inventory.h`, `DrawWeaponWheel` in `d3d9.cpp`
- Pickup ABI: `inventory_pickup_policy.h`, `tools/reload/verify_native_pickup.py`, `inventory_vm_hook.h`
- Arm A: `build/locker-grant-investigation-20261022/TEST-ARM-A.md`
- Historical acceptance (four ordinaries via retention): `docs/owned-wheel-cinema-followup.md` (mask `0xF` session — **different path than locker grant**)

---

**Investigation status:** Native **two-slot Start + PopMenu commit** and **wheel owned-mask gating** are **confirmed**. Four wheel sectors require **inventory ownership** produced outside fake UI masks; **smallest correct grant locus** is **post-PopMenu audited pickup (keep=1)** with **mandatory PopMenu slot filter**, not focus-time LoadEquipped/TArray hacks.

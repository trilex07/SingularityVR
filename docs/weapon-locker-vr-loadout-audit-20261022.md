# WeaponLocker VR Loadout — evidence audit (2026-10-22)

**User direction:** Retire `LockerOwnedDisplay` bottom-row overlay (PSVR2 rejected: oversized
`OWNED - VR WHEEL`, overlap with native Equip/Upgrade/Esc). Target: compact **VR LOADOUT**
2×2 panel + one-visit acquisition — **after** Equip-page and station-availability contracts
are proven.

**Status:** Evidence and design audit only. **No** new runtime UI, **no** Steam deploy in this
deliverable.

**Repo:** `SingularityVR-title-menu-fix`  
**Basis:** UE3 export inventory (`research/etw/controller-features-20261006/rvgame-assets.txt`),
offline locker runtime contract (`tools/reload/inspect_locker_runtime_contract.py`), grant
investigation (`build/locker-grant-investigation-20261022/REPORT.md`), live trace plan
(`docs/locker-native-trace-instrumentation-20261022.md`).

---

## 1. Equip vs Upgrade page state — what we know vs what we must witness

### 1.1 Proven from package export (not yet runtime-correlated)

`RvUIGFxMovie_WeaponLocker` exposes **separate native exec entry points** for equip vs upgrade
workflows (export block immediately preceding the class token at index 15724):

| Native exec | Export symbol | Role (inferred from names + disassembly tier) |
|-------------|---------------|-----------------------------------------------|
| Equip load path | `LoadEquippedWeapons` | Enumerates **`mEquippedWeapons` TArray** and drives load/grant (`0xF23840`; not a fixed 0/1 loop) |
| Upgrade load path | `LoadUpgrades` | Distinct callback; **not** analyzed in runtime contract script yet |
| Per-slot GFx commit | `equip` | Writes `mEquippedWeapons[slotNum]` (unbounded index — contract hazard) |
| Perk/upgrade commit | `Upgrade` | Separate from `equip` |
| Session setup | `Start` | Rebuilds equipped from **controller slots 0/1 only** |
| Close | `PopMenu` | Compare original/equipped, destroy/spawn, `SetWeaponSlotName(i)` for **every** equipped index |

**UObject fields on the movie (same export block):**

- `mEquippedWeapons` / `mOriginalWeapons` — `ArrayProperty` (+ duplicate `NameProperty` entries in export)
- `mActiveWeaponSlot` — `IntProperty`
- `mRequestedPackages` — `IntProperty`

**There is no exported `BoolProperty` or `ByteProperty` named `EquipPage`, `UpgradePage`, `mCurrentTab`,
or similar on `RvUIGFxMovie_WeaponLocker`.** Equip vs Upgrade is therefore **not** something we can
gate today by a single reflected “page id” without runtime proof.

**Falsifier:** If toggling WYPOSAŻ ↔ ULEPSZENIA in-headset **never** calls `LoadUpgrades` / `Upgrade`
and **always** leaves `LoadEquippedWeapons` traffic unchanged, the page is **GFx-only** and we must
witness GFx state (ActionScript) or a side effect (e.g. only `mRequestedPackages` changes).

### 1.2 Recommended runtime witness (next instrumented pass, default OFF)

Extend **`LockerNativeTrace`** (or a sibling `LockerPageWitness`) — **read-only**, same budgets:

1. Hook/log **`LoadUpgrades`** and **`Upgrade`** at their vtable slots (resolve from export → live
   vtable `0x019A0948` neighbour scan, same as existing four methods).
2. On every bounded ENTER for **any** locker exec, log:
   - `mActiveWeaponSlot`, `mRequestedPackages` (reflected offsets, once per session)
   - `mEquippedWeapons` / `mOriginalWeapons` FName lists (already partially logged)
3. User protocol on PSVR2: open locker → stay on **Equip** 10 s → switch to **Upgrade** 10 s → back
   → close. Grep log for method order.

**Acceptance for “Equip page only” UI gate:**

- **PASS:** VR Loadout draw allowed only when **`LoadUpgrades` / `Upgrade` have not run** since last
  Equip entry **or** when a **proven** field transition matches Equip (document exact rule from log).
- **FAIL (do not ship UI):** Upgrade tab triggers the same exec set as Equip, or page state lives only
  in GFx with no stable native discriminator.

**Do not** use pointer Y/U coordinates or “bottom of panel” heuristics as the page gate.

---

## 2. Station `availableOrdinaryMask` — not proven

### 2.1 What existing evidence **does** prove

| Source | Proves | Does **not** prove |
|--------|--------|---------------------|
| `mEquippedWeapons` / `Start` | **Two** native GFx slots; FNames from controller 0/1 at open | Which weapons the **station offers** to pick |
| `NativeOwnedInventory::Mask()` | **Owned** ordinary bits after pickup/snapshot | Station catalog or “can pick up now” |
| `LoadEquippedWeapons` | Iterates **current equipped list** to spawn/load | Full station weapon list |
| `PopMenu` + `spawnByName @ 0xF00FA0` | **Commit-time** spawn for names **in equipped[]** | Pre-commit availability |
| Phase B/C logs | Plan from **owned** mask at focus (`locker virtual: plan …`) | `availableOrdinaryMask` |
| `LockerStationGrantOrdinaryTry` (withdrawn) | TArray rewrite + `LoadEquipped` **did not** raise owned mask | Safe multi-grant |

`locker_native_trace.h` explicitly tags FName TArray logs as **“not station catalog”**.

### 2.2 Export-level **candidates** (hypothesis rank — require runtime correlation)

| Candidate | Location (export) | Hypothesis | Risk if misused |
|-----------|-------------------|------------|-----------------|
| **A. Session weapon table** | **`RvGameSystem`** (`GGameSystem @ 0x01CF394C`) → `ArrayProperty mWeapons` of **`WeaponInfo`** (stride **0x190**) | Progression-wide catalog; filter `mbIsHeavyWeapon` + ordinary FNames → bitmask | **Not** `UEngine.SessionManager`; separate `StructProperty mWeapons` on checkpoint data is unrelated. See **`docs/weapon-locker-availability-contract-20261022.md`**. |
| **B. `mWeaponsAvailableToLocker`** | **`RvInventoryItem`** `IntProperty` (export index 10573) | Bitmask or enum of locker-eligible weapons for an item/station context | Documented warning: **not** owned inventory (`docs/combined-acceptance-followups.md`); semantics **unknown** (bit width? per-weapon? per-station?) |
| **C. GFx data providers** | e.g. `RvUIDataProvider_*` with `ArrayProperty mWeapon` | UI list driving cyclable replacements | May be **upgrade** or narrative-gated; needs Equip-page correlation |
| **D. `mRequestedPackages`** | On `RvUIGFxMovie_WeaponLocker` | Package load flags when switching weapon packages in UI | Unknown mapping to ordinary four |
| **E. In-world `RvComponent_WeaponLocker`** | Small component class (`0x398A60`) | Station actor binding | No weapon list fields in export snippet — likely points at movie/session |

**No runtime log in repo yet ties any of A–E to “player sees Centurion + Valkyrie in cyclable list at
this station.”**

### 2.3 Required evidence before `EQUIP ALL`

1. **Producer function:** Log when the player **cycles** the replacement weapon on Equip page — capture
   which native exec runs (`equip` only vs `LoadEquippedWeapons`) and whether **`SessionManager.mWeapons`**
   or **`mWeaponsAvailableToLocker`** is read (breakpoint / trace / read-only probe on engine thread).
2. **Bitmask semantics:** For a station known to offer **N** ordinary weapons, log candidate field value(s)
   and map bits → `{ARPistol, ARMachineGun, ARShotgun, ARSniperRifle}`.
3. **Progression gate:** At a station where Sniper is **not** yet unlocked, prove **`availableOrdinaryMask`**
   lacks `0x8` while **`ownedMask`** may still be lower — distinct fields.

**Definition:**

```text
availableOrdinaryMask  — station/progression filtered (candidate fields above)
ownedMask                — NativeOwnedInventory::Mask() after Refresh
pendingAcquireMask       — availableOrdinaryMask & ~ownedMask   (only after A–C proven)
```

---

## 3. Audit: `LockerStationGrantOrdinaryTry` and reuse

### 3.1 Current code (`f12b7a8` / HEAD `903444f`)

```text
LockerStationGrantOrdinaryTry  →  read-only log + NativeOwnedInventory::Refresh/Snapshot
LockerStationOrdinaryWheelMask → always 0
LockerVirtualRuntime           → Phase C TArray rewrite only if LockerVirtualApply=1 (OFF)
```

Withdrawn behavior (documented in `build/locker-grant-investigation-20261022/REPORT.md`):

- On focus: **`mEquippedWeapons` count 2→4** + call **`LoadEquippedWeapons`**
- **`owned mask` unchanged** (`0x1` → `0x1`); wheel latch could lie; **crash/hitch** reported

### 3.2 Reusable vs retired

| Piece | Reuse? | Notes |
|-------|--------|-------|
| Focus hook site (`OnWeaponLockerFocus`) | **Yes** | For **read-only** witnesses and future **engine-thread queue** |
| `WriteFNameArray` / 4-wide equipped | **No** | Violates PopMenu contract; does not grant inventory |
| `LoadEquippedWeapons` on focus | **No** | Disproven for ownership; sync re-entrancy risk |
| GObjects archetype scan grant | **No** | Hitch; classes not loaded |
| `NativeOwnedInventory::Refresh` after mutations | **Yes** | After **proven** pickup path |
| Native pickup **`+0xD70`** adapter (`inventory_pickup_policy.h`) | **Yes** | `keep: 0→1` for retention; thread/engine guards |
| PopMenu **`spawnByName`** path | **Partial** | Proven for **equipped** names on close — not multi-weapon alone |
| Phase F **`destroyItem` suppress** | **Yes** | Retain ordinary on close; independent of multi-acquire |

---

## 4. Proposed one-visit acquisition sequence (conditional on §2 proof)

**Goal:** One locker visit, one close, all **station-approved** ordinary weapons in real inventory +
VR wheel; native slots **0/1** unchanged as compatibility layer.

### 4.1 Preferred (if mid-session pickup is thread-safe)

On **engine/input thread** (same as `UpdateMenu` / exec hooks — **never** D3D9 overlay thread):

1. User toggles VR Loadout **EQUIP ALL** or per-tile **EQUIP** (mod hit targets only on Equip page).
2. Compute `pending = availableOrdinaryMask & ~ownedMask` (fail-closed if availability unproven).
3. For each bit in `pending`, enqueue **archetype pickup** using the **same ABI** as game restore:
   `RvPlayerPawnSP` vtable **`+0xD70`**, **`keep=1`**, valid **`RvWeaponShared`** archetype pointer
   (source must be logged once — not GObjects scan).
4. `NativeOwnedInventory::Refresh()` after batch.
5. User finishes native 2-slot equip UI as today; **PopMenu** runs once.

**Open question (trace must answer):** Does pickup during open movie conflict with GFx `equip` / TArray?

### 4.2 Fallback (if open-movie pickup unsafe)

1. While locker open: VR Loadout only sets **`pendingAcquireMask`** (no UObject writes).
2. User configures native two slots; closes locker.
3. On **`PopMenu` EXIT** (proven hook point, after native commit, same thread):
   - For each bit in `pendingAcquireMask` still not in `ownedMask`, run **`+0xD70` pickup** or
     **`CreateInventoryFromArchetype`** chain matching PopMenu’s **`spawnByName`** success path.
4. Refresh inventory; Phase F retention still suppresses erroneous **`destroyItem`** for retained ordinals.

This matches user-acceptable “one visit, one close” even if native PopMenu still only commits two GFx slots.

### 4.3 Explicitly forbidden (unchanged)

- `mEquippedWeapons.Count` / `mOriginalWeapons.Count` → 4
- `SetWeaponSlotName(2|3)` / controller slots ≥ 2
- `equip(slotNum)` with slotNum ≥ 2 for ordinary grant
- Grant all four FNames when `availableOrdinaryMask` lacks bits
- `LockerVirtualApply` TArray rewrite on production path

---

## 5. Safety gates (progression + specials)

| Gate | Mechanism |
|------|-----------|
| Station scope | `pendingAcquireMask` ⊆ **`availableOrdinaryMask`** (proven field), never hard-coded `0xF` |
| Owned-only UI state | **OWNED** = `ownedMask`; **AVAILABLE** = bit in available not owned; **LOCKED** = not in available |
| Heavy / grenade / special | PopMenu **`DecidePopMenuControllerWrite`**: refuse equipped index ≥ 2 (`locker_virtual_contract.h`) |
| Retention | `LockerRetainOrdinary` + `KeepAcquiredWeapons` unchanged |
| Thread | Pickup / inventory mutation only on **engine/input** thread; overlay **display-only** |
| Fail-closed | If page witness ≠ Equip, or availability unreadable → **hide** VR Loadout and disable EQUIP ALL |
| Cheat / debug | Reject `cheat` exec path (`WeaponLocker` export) — do not hook for mod features |

---

## 6. Compact VR Loadout layout (replaces `LockerOwnedDisplay`)

Reference: GFx **`Menu_WeaponsLocker`** private RTT **1280×720**, station plane scaling
(`StationMenuScalePct=50`, `g_vrMenuPanel` projection — same as `LockerProjectPanelUv`).

**Retire:** bottom-row label at **`v≈0.56`**, tiles at **`v≈0.66`**, `ClearRects` caption
(`DrawLockerOwnedDisplay` in `d3d9.cpp`).

**Proposed anchors (stereo panel UV, tune on device):**

| Element | UV (u, v) | Size note |
|---------|-----------|-----------|
| 2×2 grid centre | **(0.78, 0.48)** | Right of native equip preview; **above** Upgrade/Esc band |
| Tile spacing | Δu **0.11**, Δv **0.11** | Four icons: Centurion TL, Valkyrie TR, Shotgun BL, Sniper BR |
| Optional title | **(0.78, 0.36)** | Small atlas text **“VR LOADOUT”** or **omit** — prefer icon-only |
| Mod hit targets | Same quads as icons | Only **AVAILABLE** tiles; **OWNED** = checkmark overlay, no native GFx poke |

**Keep clear (no draw / no hits):**

- Native slot labels and cyclable equip row (~**u 0.35–0.65, v 0.38–0.55** — approximate from prior tune)
- Upgrade panel lower third (**v > 0.62** — where user saw overlap)
- Esc / Enter hint strip (**v > 0.85**)

**Draw gates (all required):** `RvUIGFxMovie_WeaponLocker` focus, **Equip page witness true**, `!g_weaponWheelOpen`, `KeepAcquiredWeapons`, new flag (e.g. `LockerVrLoadout=0` default OFF).

**Recommendation for installed Steam test build:** set **`LockerOwnedDisplay=0`** until VR Loadout ships.

---

## 7. Next implementation gate

| Step | Owner | Deliverable |
|------|-------|-------------|
| 1 | MAIN + PSVR2 | Trace log: Equip vs Upgrade exec + `mActiveWeaponSlot` / `mRequestedPackages` |
| 2 | MAIN | Identify **`availableOrdinaryMask` producer** with line citations |
| 3 | MAIN | Pick **4.1 vs 4.2** from thread-safe pickup evidence |
| 4 | MAIN | Implement VR Loadout **display-only** with Equip gate + layout §6 |
| 5 | User | PSVR2 PASS one-visit multi-acquire + no Upgrade overlap |

---

## Appendix: ordinary weapon bit map (unchanged)

| Bit | Mesh | Typical FName |
|-----|------|----------------|
| 0x1 | 5799 | ARPistol / Centurion |
| 0x2 | 11387 | ARMachineGun / Valkyrie |
| 0x4 | 8297 | ARShotgun |
| 0x8 | 11389 | ARSniperRifle |

# Technical Handoff: Seeker PR #60 PSVR2 Runtime Failure Analysis

**Audience:** Astra (Seeker Feature Owner)  
**Date:** 2026-10-25  
**Candidate Tested:** Commit `9ea16074` (`feature/sol-seeker-back-slot` / PR #60)  
**Deployed DLL SHA256:** `98560D7413CBA7298321DA7EED85AC63ABCC457010C372295615780AAEBE0809`  
**Archived Session Evidence:** `deployment/deploy-backups/20261025-pr60-psvr2-session`  
**Archived Session Log:** `deployment/deploy-backups/20261025-pr60-psvr2-session/view_matrix.log`  
(SHA256: `AD0A4DD62445BF6C3B0F287D6A3573D21A2FD1619CD6952F4B4A77622CF11AD5`)

---

## 1. Physical Verification Results (PSVR2)

* **Valkyrie Accuracy:** **PHYSICAL PASS**  
  (Candidate settings `VrValkyrieSpreadPercent=70`, `VrValkyrieTwoHandPercent=50` verified; noticeably tighter and consistent burst grouping in VR confirmed).
* **Valkyrie & Volk Reload Insertion Radius (20 cm):** **PHYSICAL PASS**  
  (`ValkyrieInsertRadiusCm=20`, `VolkInsertRadiusCm=20` confirmed comfortable and smooth).
* **Seeker Back Holster:** **FAIL**  
  (Right Grip gesture behind right shoulder does not stow or retrieve the Seeker).
* **Seeker Firing:** **PARTIAL**  
  (Native rocket firing works, but VR muzzle launch alignment hooks did not arm).
* **Seeker Left Support Grip:** **NOT TESTED**  
  (Support math and witness ran in engine, but physical feel was not tested by user).
* **Seeker Save/Load Persistence:** **NOT TESTED**.

---

## 2. Evidence Analysis from Gameplay Log (`view_matrix.log`)

### A. What Succeeded (Proven In-Engine):
1. **Seeker Possession Proven:**  
   `NativeOwnedInventory` repeatedly detected `ARSteerable` in the player's possession chain:
   ```text
   owned inventory: unmapped weapon item name=ARSteerable mWeaponName=ARSteerable
   ```
   The pawn legitimately owns the weapon and its native class is `ARSteerable`.
2. **Seeker D3D9 Mesh & Witness Published:**  
   When the user equipped the Seeker, its 14,990-triangle body and index buffer matched the offline geometry fixture:
   ```text
   Line 21061: Seeker pose: source IB matched, body=14990 eyes=2 rootSlot=1; restored, witness published
   Line 21322: Seeker support: original idle wrist relative to rendered cog; shared glove/device target
   ```

---

### B. What Failed & Missing Runtime Witnesses:
1. **Missing Hook Installation Witness:**  
   The log line:
   ```text
   Seeker back slot: exact native switch/getter contracts ready; no grants, slot/ammo writes or save edits
   ```
   **NEVER appeared** in `view_matrix.log`. `NativeSeekerSlot::Bind(s)` was never called or never reached the MinHook installation.
2. **Missing Selection / Back Slot Activation Witness:**  
   Neither `retained owned item=...` nor `draw/store existing item=...` appeared.
3. **Missing Projectile Launch Witness:**  
   Neither `Seeker launch: audited RocketLauncher...` nor `Seeker launch: native archetype=...` appeared.

---

## 3. Root Cause Diagnosis for Astra

In `native_seeker_slot.h`:
```cpp
static void Poll(){
    ...
    seekerslot::Snapshot s;
    const bool valid=Read(s);state.Reconcile(s);
    const bool ready=valid&&state.seeker&&Bind(s);
    InterlockedExchange(&g_seekerBackAvailable,ready?(LONG)s.pawn:0);
    ...
}
```

1. **`NativeSeekerSlot::Read(s)` failed closed:**  
   Because `Read(s)` returned `false`:
   * `valid` was `false`.
   * `Bind(s)` was never called to install the `heavy` hook.
   * `g_seekerBackAvailable` remained `0`.
2. **Back gesture was completely inert:**  
   In `d3d9.cpp` (line 11527):
   ```cpp
   const bool backGrip = seekerslot::BackGrip(..., Rd(g_seekerBackSlot) && Rd(g_seekerBackAvailable), ...);
   ```
   Because `g_seekerBackAvailable` was `0`, `backGrip` could never evaluate to true regardless of the user's hand position and Grip press.
3. **Projectile launch was disabled:**  
   In `native_seeker_projectile.inl`:
   ```cpp
   static bool Local(uintptr_t self){
       ...
       if(!NativeSeekerSlot::Read(s)||s.pawn!=g_playerPawn||s.current!=self)return false;
       ...
   }
   ```
   Because `NativeSeekerSlot::Read(s)` failed, `Local()` failed, and `SeekerProjectileTick()` never installed the projectile hooks. Firing fell back to unhooked native engine execution.

---

## 4. Key Failure Points in `NativeSeekerSlot::Read(s)` to Investigate

Review these strict checks in `native_seeker_slot.h`:
* `ownerField.Get(cls, "Instigator") != 0x98 || P(it + 0x98) != s.pawn`:  
  Does the Seeker item object in the chain actually have `P(it + 0x98) == s.pawn` when stowed or switching? Notice that `NativeOwnedInventory.h` does not require `it + 0x98 == pawn` and successfully walks the chain. If ANY item in the inventory chain fails this condition, `Read(s)` returns `false` immediately for the whole inventory.
* `MemberCache` timeout:  
  If reflection property lookup takes more than 1 frame or returns -1 initially, `MemberCache` sets a 2000 ms cooldown.
* `attempts++ >= 2` in `Bind(s)`:  
  Ensure `attempts` does not latch and permanently disable binding if called before the player controller is fully initialized.

---

## 5. Preserved Accepted Configuration

In the next iteration, keep the accepted and passed settings:
```ini
[Input]
SeekerBackSlot=1
SeekerSupportGrip=1
SeekerProjectileAim=1
VrValkyrieSpreadPercent=70
VrValkyrieTwoHandPercent=50
ValkyrieInsertRadiusCm=20
VolkInsertRadiusCm=20
```
Do not touch the accepted HD textures (`priority_material_mode=2`), 3D HUD, TMD, or UI.

# ASTRA HANDOFF: Seeker Candidate 8112FB08 PSVR2 Physical Failure Analysis

**Date:** 2026-10-25  
**Candidate SHA256:** `8112FB08142B380C6B8A6421D1522203CE0864017A95A8765A2A7379F303C60F`  
**Candidate Source:** commit `5e908879` / `19bd45da` (branch `fix/seeker-back-holster-evidence`)  
**Archived Session Evidence:** `deployment/deploy-backups/20261025-seeker-8112fb08-session/`  
**Archived Session Log:** `view_matrix.log` (SHA256: `1EB237B8ED442C55A9E8CE41564C4B4FDCBA2163DB117290D21B3818C6D8B465`, size 2,125,865 bytes, session 08:51:35 - 08:55:06)  

---

## 1. Executive Summary & Physical PSVR2 Feedback

The user tested candidate `8112FB08` in PSVR2 with a legitimately acquired Seeker rifle:

1. **Seeker Back Holster: PHYSICAL FAIL** — The player still cannot retrieve or stow the Seeker from the back-mounted VR holster.
2. **Seeker Left Support Grip: PHYSICAL FAIL** — When gripping the Seeker with both hands, the **left hand mesh completely disappears**.
3. **Weapon Accuracy: UNSATISFACTORY** — The user reported that weapon accuracy remains unsatisfactory across weapons; aim error needs dedicated investigation.
4. **Preserved Accepted Systems**:
   - Valkyrie accuracy: `VrValkyrieSpreadPercent=70`, `VrValkyrieTwoHandPercent=50` (PHYSICAL PASS preserved).
   - Reload insertion comfort: `ValkyrieInsertRadiusCm=20`, `VolkInsertRadiusCm=20` (PHYSICAL PASS preserved).
   - HD textures: `priority_material_mode=2` with 4 verified DDS files (PHYSICAL PASS preserved).
   - 3D Diegetic HUD, TMD haptics, and Polish localization preserved.

---

## 2. Evidence-Driven Root Cause Analysis

### Issue A: Back Holster Retrieval Failure (`stage=weapon-owner-or-name-field`)

#### Runtime Witness Evidence:
- **Actual Seeker Ownership:** During the exact test session, `NativeOwnedInventory::Snapshot()` repeatedly confirmed that `ARSteerable` was present in the player's possession chain:
  ```
  L12256: owned inventory: unmapped weapon item name=ARSteerable mWeaponName=ARSteerable
  L13372: owned inventory: unmapped weapon item name=ARSteerable mWeaponName=ARSteerable
  ... [logged across hundreds of frames through L18097]
  ```
- **Seeker Back Gate Receipts (all 6 in session):**
  ```
  L193:   Seeker back gate: stage=local-pawn pawn=00000000 current=00000000 ownedSeeker=00000000 count=0 valid=0 hook=00000000 available=00000000 thread=11604
  L637:   Seeker back gate: stage=weapon-owner-or-name-field pawn=0E17A000 current=0F518400 ownedSeeker=00000000 count=0 valid=0 hook=00000000 available=00000000 thread=11604
  L4939:  Seeker back gate: stage=local-pawn pawn=00000000 current=00000000 ownedSeeker=00000000 count=0 valid=0 hook=00000000 available=00000000 thread=11604
  L6105:  Seeker back gate: stage=weapon-owner-or-name-field pawn=1BD46400 current=3CDC3000 ownedSeeker=00000000 count=0 valid=0 hook=00000000 available=00000000 thread=11604
  L13454: Seeker back gate: stage=weapon-owner-or-name-field pawn=1BD46400 current=3CDC2400 ownedSeeker=00000000 count=0 valid=0 hook=00000000 available=00000000 thread=11604
  L16689: Seeker back gate: stage=weapon-owner-or-name-field pawn=1BD46400 current=69604200 ownedSeeker=00000000 count=0 valid=0 hook=00000000 available=00000000 thread=11604
  ```
- **Seeker Back Grip Receipts (all 39 rising edges):**
  - 38 receipts: `gate=unavailable hand=1 tracked=1 availablePawn=00000000 ... busy[hip=0 support=0 next=0 centurion=0 fire=0 knife=-1 wheel=0]`
  - 1 receipt: `gate=busy hand=1 tracked=1 availablePawn=00000000 ... busy[hip=1 support=0 next=0 centurion=0 fire=0 knife=-1 wheel=0]` (proximity overlap)
  - In all 39 cases, `availablePawn=00000000` because `g_seekerBackAvailable` was never set.
- **Selection Receipts:** `selection-requested` and `selection-equipped` never ran (0 matches) because the gate was never accepted.

#### Code Defect in `native_seeker_slot.h`:
In `NativeSeekerSlot::Read()`:
```cpp
// Instigator is the authoritative owner used by native weapon helpers.
if(ownerField.Get(cls,"Instigator")!=0x98||P(it+0x98)!=s.pawn||
   nameField.Get(cls,"mWeaponName")!=0x2e8||!PauseRttProbe::SafeRange(it+0x2e8,8))return Fail("weapon-owner-or-name-field");
```
1. `nameField.Get(cls, "mWeaponName")` calls `PropOffsetInChain(cls, "mWeaponName")`.
2. `mWeaponName` is **not** an exported `UProperty` reachable via `PropOffsetUnder` through the reflection property chain of `RvWeaponShared`. It is an audited native C++ field / raw struct at offset `0x2E8` (`kItemWeaponName = 0x2e8`), as verified by `NativeOwnedInventory`:
   ```cpp
   // NativeOwnedInventory (working, tested):
   if(!PauseRttProbe::SafeRange(it+kItemWeaponName,8))return false;
   const Name weapon=*(const Name*)(it+kItemWeaponName);
   char wn[96]{};NameFromIndex(weapon.index,wn,sizeof(wn));
   ```
3. Because `PropOffsetInChain(cls, "mWeaponName")` returns `-1`, `nameField.Get(cls, "mWeaponName") != 0x2e8` evaluates to `true` on the very first item in the chain.
4. `Read(s)` returns `false` with `stage="weapon-owner-or-name-field"`, leaving `s.count = 0`, `state.seeker = 0`, and `g_seekerBackAvailable = 0`.
5. **Why Synthetic Tests Passed:** In `spikes/view_matrix/tests/test-seeker-native-adapter.cpp`, Astra mocked `MemberCache::Get`:
   ```cpp
   if(!strcmp(n,"Instigator"))return 0x98;if(!strcmp(n,"mWeaponName"))return 0x2e8;
   ```
   The unit test simulated an artificial environment where reflection returned `0x2E8`, concealing the defect from offline CI.

---

### Issue B: Left Support Grip Causing Left Hand Disappearance

#### Runtime Witness Evidence:
- User engaged two-hand support grip while holding the Seeker.
- Log lines 13912-13916:
  ```
  L13912: Seeker support: original idle wrist relative to rendered cog; shared glove/device target
  L13913: Seeker support: original idle wrist relative to rendered cog; shared glove/device target
  L13915: Seeker support: original idle wrist relative to rendered cog; shared glove/device target
  L13916: Seeker support: original idle wrist relative to rendered cog; shared glove/device target
  ```
- **Physical Result:** The left glove mesh completely disappeared from view in the headset.

#### Mathematical Defect in `seeker_geometry.h` & `d3d9.cpp`:
1. In `seeker_geometry.h`:
   ```cpp
   inline bool SupportTarget(const Witness& w,uint32_t pawn,uint32_t weapon,uint64_t now,
           const Mat& wristBind,const float skin[3][4],const Mat& local,const Mat& gun,Mat& out){
       ...
       const auto source=Product(wrist,local);
       if(!skeletal::Inverse(inverse.m,source.m))return false;
       const auto desired=Product(Product(LeftGrip,w.rootLocal),gun);
       const auto result=Product(inverse,desired);
       out=result;return true;
   }
   ```
   Here `result` is computed as `inv(wrist * local) * desired`.
2. In `d3d9.cpp` (lines 18057-18095):
   ```cpp
   if(ready){
       memcpy(offWeapon,target.m,sizeof(offWeapon));
       leftTarget=offWeapon;
       weaponDeltaUsed=true;
   }
   ...
   bool paired = skeletal::LocalDeltaPair(delta, rightDelta, local, base, leftTarget,
       handanchor::AimTarget(weaponSingleHand, aimTarget, primary));
   ```
3. In `skeletal::LocalDeltaPair`:
   ```cpp
   Multiply(a, local, left);
   Multiply(b, a, invCurrent);
   Multiply(leftResult, b, invLocal);
   ```
   `LocalDeltaPair` expects `leftTarget` to be a **World ride transformation matrix `C`** (in the same space as `primary`, `off`, or `base`). It performs the inverse ride transformation `local * leftTarget * inv(base) * inv(local)`.
4. Passing `result = inv(wrist * local) * desired` into `LocalDeltaPair` double-inverts and applies invalid coordinate frames, resulting in extreme or divergent delta values applied to all left wrist and finger bones in `skeletal::TransformBone`.
5. The transformed vertices end up outside the view frustum or clipped against near/far planes, causing the entire left hand mesh to vanish.

---

### Issue C: Projectile Launch & Weapon Accuracy

#### Runtime Witness Evidence:
- `Seeker launch gate` failed at line 13544:
  ```
  L13544: Seeker launch gate: stage=weapon-owner-or-name-field weapon=3CDC2400 pawn=1BD46400 ready=0 spread=1
  ```
  Because `native_seeker_projectile.inl` called `NativeSeekerSlot::Read(s)` to determine `Local()`, it also failed closed on `stage=weapon-owner-or-name-field`.
- The VR muzzle alignment was never applied to fired projectiles.
- Across other weapons, the user noted unsatisfactory accuracy. The accepted Valkyrie spread settings (`VrValkyrieSpreadPercent=70`, `VrValkyrieTwoHandPercent=50`) remain preserved, but weapon aim and sight alignment require systematic verification.

---

## 3. Required Fixes for Astra (No More Speculative Diagnostics)

Under the **Evidence-Driven Debugging & Reverse Engineering Policy**:
- **Escalation Rule is in effect**: Speculative patching and diagnostic-only iterations are barred. Astra must provide working, verified implementations addressing the proven mechanisms.
- **Do NOT deploy PR #61**: Its action-pose path explicitly excludes Seeker and does not solve the root cause.

### Direct Action Items for Astra:

1. **Fix Seeker Ownership Resolution in `native_seeker_slot.h`**:
   - Align ownership and weapon name checks with the proven, working implementation in `NativeOwnedInventory::Snapshot()`.
   - Remove the failing `nameField.Get(cls, "mWeaponName") != 0x2e8` reflection requirement. Validate `PauseRttProbe::SafeRange(it + 0x2E8, 8)` and extract the `FName` directly from `it + 0x2E8`.
   - Validate `Instigator` using direct offset `P(it + 0x98) == s.pawn`.
   - Ensure `NativeSeekerSlot::Read(s)` returns `true` and populates `s.items` and `state.seeker`.

2. **Fix Seeker Support Grip Coordinate Mathematics**:
   - Revisit `SupportTarget` and its interaction with `LocalDeltaPair`.
   - Ensure the matrix passed as `leftTarget` is a valid target coordinate frame that matches the contract of `LocalDeltaPair` without double inversion or collapsing bone transforms.
   - Verify that the left hand vertices remain visible and correctly positioned relative to the weapon mesh when support grip is active.

3. **Verify Projectile Launch Hook Arming**:
   - Verify that once `Read(s)` succeeds, `native_seeker_projectile.inl` arms and correctly aligns launch orientation to the physical weapon muzzle.

4. **Investigate Weapon Accuracy & Aim Error**:
   - Profile the relationship between physical controller pose, weapon sight axis, and bullet trace origin/direction.
   - Maintain the accepted Valkyrie 70%/50% spread and 20 cm reload comfort.

---

## 4. Current Safe Steam Profile (Applied by MAIN)

To ensure game stability and a clean baseline while Astra resolves these issues, MAIN has applied a safe profile in Steam (`SingularityVR.ini`):
```ini
[Input]
SeekerBackSlot=0
SeekerSupportGrip=0
SeekerProjectileAim=0
```
All accepted systems remain fully active:
- `DiegeticWeaponHud=1` (PHYSICAL PASS)
- `priority_material_mode=2` (HD Textures PHYSICAL PASS)
- `VrValkyrieSpreadPercent=70`, `VrValkyrieTwoHandPercent=50` (PHYSICAL PASS)
- `ValkyrieInsertRadiusCm=20`, `VolkInsertRadiusCm=20` (PHYSICAL PASS)
- TMD haptics and Polish dubbing (PHYSICAL PASS)

No further Steam test sessions will be launched until Astra delivers a code fix resolving the proven mechanisms.

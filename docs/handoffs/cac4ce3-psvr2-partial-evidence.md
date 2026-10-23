# SOL handoff — `cac4ce3` PSVR2 PARTIAL + dual-weapon regression

**User verdict:** PARTIAL; **critical dual-weapon regression remains** (weapon disappearance / latch).

Full bundle: `deployment/crash-evidence/20261023-cac4ce3-psvr2-partial/`  
Summary: `PSVR2_PARTIAL_SUMMARY.md` in that folder.

## Installed (unchanged — deployment hold)

- Commit `cac4ce3701c08cca28bfdcb6cbbdf189bcc5076e`
- DLL `BEA5A5623E2995C5DEE85AC30D1736F0B3E3CEAF68AF20A84B86B7DA3E83D581`
- INI `00200C0572C99D1656E294C1F312D104F27A9B4CB44BD5CD0890AEAB22387A79`

## Key log finding (weapon disappearance)

At weapon swap to ARMachineGun (L127322–127342):

```text
fg census (at the SWAP drop): 9 entries: [0]=1368 [1]=8297 [2]=7444 [3]=4818 [4]=37 [5]=207 [6]=251 [7]=118 [8]=729
mesh latch: learned that 7444 verts is the ARMS
mesh latch: gun=11387 verts, arms=7444 verts
```

TMD glove (7444) and socket draws present in fg pass while `DualWeaponTmdReady=1`; latch permanently assigns 7444 to **ARMS**, likely causing visible gun loss. Socket CRC receipts at raised TMD (L55814+) are **not** contradictory — both can be true.

## Positive receipts (same session)

- `tmd-attachment: cooked socket mesh CRC verified` (multiple socket topologies)
- `stereo receipt eyes=2 tris=7110 sleevePalette=7`
- Grip: mat1 `route=zero` during `support=1` (L56676)

## Screenshots

Not archived on host — user to add `psvr2-partial-*.png` to evidence folder.

## MAIN constraints

- No renderer/input changes
- No deploy until SOL pushes candidate
- Do not integrate `9bb0e47` Volk magazine — SOL merges after geometry acceptance
- No merge to `main`

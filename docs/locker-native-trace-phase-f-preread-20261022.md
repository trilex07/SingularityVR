# Phase F integration point — pre-trace (hypothesis only)

**Until `LockerNativeTrace=1` logs exist, this is not implementation guidance.**

## Smallest valid direction (existing evidence)

1. **Product-proven path:** `KeepAcquiredWeapons=1` + audited pawn pickup **`+0xD70`** with **`keep→1`** already produced **`mask=0xF`** and four wheel sectors (session 663a49a). Phase F at the station should **extend that mechanism**, not GFx TArray surgery.

2. **Inventory-only default:** With native locker UI left at **two equipped entries**, **Phase D PopMenu filtering is not required** unless Phase C expands `equipped.Count` beyond two.

3. **Station-specific grant locus:** **Unknown** — candidates ranked by risk:
   - **A (lowest risk if thread matches):** Scoped pickup when the game already passes a **valid `RvWeaponShared` archetype`** (log pointer + name via trace).
   - **B (unverified):** After **`PopMenu`** on locker close — thread and `destroyItem` ordering must be proven by trace.
   - **C (withdrawn):** Focus **`LoadEquippedWeapons`** + TArray rewrite — **disproven** for inventory (`mask` unchanged) and crash/hitch.

## Blockers trace must clear

| Blocker | Trace fields |
|---------|----------------|
| Native event order | `#N` sequence across Start/equip/Load/PopMenu |
| Four-offered vs two-equipped | FName list length on ENTER/EXIT equip/Load |
| Ownership delta | `owned mask/count` on ENTER/EXIT each method |
| Pickup thread safety | `hookTid` vs `pollTid` on PopMenu vs pickup lines |
| Archetype source | `archName`, `archClass`, `archetype=%p` on pickup |

## Post-trace deliverable

One-page update: **single recommended hook point** with log line citations; otherwise **remain on retention-only wheel** (Arm A).

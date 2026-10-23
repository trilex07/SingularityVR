# MAIN deployment hold (2026-10-23)

**Status:** HOLD — awaiting SOL pushed candidate after `cac4ce3` PSVR2 **PARTIAL**.

**Active diagnostic (2026-10-23):** DLL `A0A3365A…` (`067e1b1` runtime-alignment probes). Rollback folder: `20261023-pre-runtime-alignment-067e1b1/` → `BEA5A562…` (`cac4ce3`).

**After diagnostic session:** restore `cac4ce3` unless user keeps probe build.

**Do not deploy** further experimental fixes until SOL provides a implementation candidate from diagnostic results.

**Preserved builds (backups only):**
- Volk magazine `9bb0e47`: `deployment/deploy-backups/20261023-pre-volk-magazine-9bb0e47/`
- `af580e6`: `deployment/deploy-backups/20261023-pre-assembly-continuity-af580e6/`

**Integration rule:** MAIN does **not** merge `9bb0e47` into TMD branch; SOL integrates after geometry acceptance.

**Latest evidence:** `deployment/crash-evidence/20261023-cac4ce3-psvr2-partial/`

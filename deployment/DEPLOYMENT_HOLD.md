# Deployment hold / active build

**Active diagnostic (2026-10-23):** DLL `6B085CBA…` (`8fbb9f4` Volk five-bone skin probe).

**Rollback:** `deployment/deploy-backups/20261023-pre-volk-skin-8fbb9f4/` → `BEA5A562…` (`cac4ce3`).

**INI:** `[Diagnostic] VolkImpulseSkinProbe=1` (+ existing settings).

**After capture:** restore `cac4ce3`; push evidence branch; no merge to `main` without authorization.

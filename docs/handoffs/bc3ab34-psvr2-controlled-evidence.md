# bc3ab34 controlled PSVR2 evidence (MAIN)

**Status:** Deployed — **PSVR2 segments A/B/C pending user execution.**

| Item | Value |
|------|--------|
| Commit | `bc3ab3455d980ae4440d554da908b0e9fc2ba080` |
| Installed DLL | `081B278E8B5ECB267EB42F1AF0C5C92227270AED77C08CB0B37519B72C125DF4` |
| Prior rollback | `19DFDFB7…` (`514bb03`) in `deployment/deploy-backups/20261023-pre-bc3ab34-segment-test/` |
| Regression gate | gun-scale ABI, short-gauntlet, dual-TMD C++ suite, D3D9/D3D9Ex 7110, OLED — PASS at deploy |
| Protocol | `docs/handoffs/bc3ab34-controlled-psvr2-protocol.md` |

## Segment results (fill after PSVR2)

| Segment | Physical | Log archived | Key receipts |
|---------|----------|--------------|--------------|
| **A** Gun scale | _pending_ | _pending_ | `scale=0 -> 1` or named refusal |
| **B** Right wrist | _pending_ | _pending_ | `dual-tmd right wrist:` |
| **C1** Knife | _pending_ | _pending_ | `knife-hand:`, `hip knife:` |
| **C2** Volk reload | _pending_ | _pending_ | `Volk carry wrist`, draw gate 10–12 |
| **Regression** Two-hand TMD | _pending_ | _pending_ | `tmd-support-gauntlet: support=1 …7110` |

## Post-session

1. Archive segments A/B/C via `deployment/bc3ab34-segment-playbook.ps1`.
2. Update this file with PASS/PARTIAL/FAIL per rubric.
3. Force-add logs; push branch updates.
4. Restore `514bb03` from rollback folder unless user retains build.

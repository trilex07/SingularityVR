# MAIN → SOL: M01 physical FAIL (`1c358e2` / DLL `2218BCA8…`)

**Verdict:** **FAIL** (PSVR2, 2026-10-23). **M03 not deployed.** Steam restored to pre-M01 **`702f0c3`** baseline.

## User report

- Valkyrie magazine still not grabbable from behind the left shoulder with TMD acquired.
- Volk S4: wants magazine pickup volume extended **farther left** of the shoulder (not only a small sphere directly behind).
- Suspected TMD glove/support input ownership (unconfirmed by logs).

## Artifacts

| Item | SHA256 / path |
| --- | --- |
| Candidate DLL (rolled back) | `2218BCA833577B13B45B28E82DE9B54295EF02339DD52ED56937B8152D63035B` |
| Session INI (unchanged) | `4EA3857A0E78033C162280AB1A8FCB8A5D6CA09B72547CE75C8D55688BC904EA` |
| Full log | `docs/evidence/m01-1c358e2-20261023/view_matrix.log` — `8D9BC6F80E9C7A786DD0767CC429BB38DFDD960082E52CA9BA92C94C655326E1` |
| Gate extract | `docs/evidence/m01-1c358e2-20261023/valkyrie-shoulder-gates.txt` (31 lines) |
| FSM extract | `docs/evidence/m01-1c358e2-20261023/next-reload-transcript.txt` |
| Rollback verified | DLL `A61C3B534F5C4C40DC878EC68CC52A77E8599DB57C6272D85254E58B795F6E1C`, INI `4EA3857A…` |

## NeedAmmo after Circle?

**Yes.** Valkyrie (`kind=1`, 11387 verts) logged **`magazine-ejected`** four times with **`phase=1`** (NeedAmmo), e.g. log `t=23110156` clip 13→13 reserve 12, `t=23135984` clip 9, `t=23171968` clip 7, plus later eject at `t=23173812` sequence. Circle witness lines show `circleEdge=1`, `context=1`, `ammoFresh=1`, `event=1` on successful ejects.

Three Circle attempts hit **`next reload gate: kind=1 … context=0`** (lines ~14732, ~33581, ~46029) with stale ammo witness (`ammoFresh=0`, `clip=-1`) — separate **input-context / ammo-stale** failure, not shoulder radius.

## Shoulder gate receipts (`valkyrie shoulder gate`)

All logged with **`tmdOwned=1`**, **`support=0`**, **`knifeBusy=0`**, **`knifeNear=0`**, **`knifeEnabled=0`**, **`tracked=1`**, **`ammoFresh=1`**, **`identity=1`**, **`native=1`**, **`grip` edge=1**, **`window=0`**.

| Outcome | Count | Notes |
| --- | ---: | --- |
| `event=grab-fresh` | 20 | Hand within **`radius=0.170`** (distances 0.052–0.156 m) |
| `event=none` | 11 | Hand **outside** sphere (distances 0.184–0.343 m > 0.170) |

**First logged grip during NeedAmmo:** `grab-fresh` at **0.052 m** — **not** blocked by knife/TMD/support or by radius.

**First logged `event=none`:** distance **0.184 m** (> 0.170) — classic **pouch sphere miss**, but this was **not** the first Grip attempt in the session.

## FSM transitions (Valkyrie `kind=1`)

| Transition | Count |
| --- | ---: |
| `magazine-ejected` | 4 |
| `grab-fresh` → `phase=2` (Carrying) | 29 |
| `discarded` → back toward `phase=1` | 29 |
| `inserted` / native **`permit=1`** | **0** |
| `cancelled` | 2 |

Pattern: **`grab-fresh` always paired with `permit=0`**; every carry attempt logged **`discarded`** within ~1 s (`nativeMagazineDraws` rises — mesh draw path active, e.g. 948→1122 on first cycle). **No Valkyrie insertion or native ammo permit** in the session.

Contrast: Volk (`kind=2`) once reached **`inserted/request … permit=1`** (`t=23232640`) — shotgun reload path can complete; user still wants **larger left-offset pickup volume** for S4.

## First actual rejection (evidence-based)

Do **not** attribute total failure to pouch radius alone:

1. **Primary mismatch:** Policy FSM accepts **`GrabFresh`** many times while the player reports no usable grab; carries **`Discarded`** immediately with **no insert** and **no native permit**. Likely **carry attachment / palm–wrist visual / discard policy** or **perceived vs rendered magazine**, not the d897dc4 knife gate.
2. **Secondary gate:** When `Update` returns **`Event::None`**, bounded receipt shows **`distance > radius`** (0.17 m sphere behind shoulder) — 11 grips.
3. **Tertiary gate:** Circle with **`context=0`** on three frames — ammo/context not ready; unrelated to shoulder offset.

**Not supported by logs:** `supportLatched=1` or `handBusy=1` on any shoulder receipt; M01 knife bypass appears effective.

## SOL actions requested

1. Reproduce with archived log; trace **`Discarded`** after **`GrabFresh`** for Valkyrie + TMD (`g_reloadPalmGrip`, wrist matrix, `NextReloadVisual`, discard thresholds).
2. Separate Volk **shoulder calibration** change (extend pouch **left** along body yaw) — user feature request, not part of M01 acceptance.
3. Ship a **revised candidate**; MAIN will not deploy until then.

## MAIN state

- **M03:** blocked.
- **Steam:** `A61C3B53…` / `4EA3857A…` verified after rollback.

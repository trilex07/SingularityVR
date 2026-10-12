# Narrow Gate A — pause backbuffer canvas (2026-10-12)

DLL: `D4EFDBE49AED3FA713F9604C80FEA6FE02D4C8271AD91E148473A6D94036E6CD`  
Log: `%LOCALAPPDATA%\SingularityVR\view_matrix.log` (attach 2026-10-12 13:09:14)

## Capture boundary (proved)

| Item | Evidence |
|------|----------|
| Scope | `PauseBackbufferCanvasRoute` only (`pause_canvas_backbuffer_trace.h`) |
| RT | `surf=0AB75A00`, `5264×2684`, `fmt=21` (A8R8G8B8), full viewport |
| Token | `500000003` (class 3), `panel=0` |
| Txn close | `Present` (`pausecanvas::EndFrame` before `g_vfxFrame++`) |
| World/offscreen | **Excluded** by construction (no draws logged) |

## Main vs Objectives (bands, not c5 ownership)

Observed scale **bands** (diagnostic labels only):

| Band | Typical `sx` | Textures (examples) | Stride/prim | Caller |
|------|--------------|---------------------|-------------|--------|
| **A** | ~7.812e-05 | `32C47D80` | 20, 36–84 prims | `d3d9.dll+0x23194` |
| **B** | ~1.249e-05 / 1.335e-05 | `72ADF120`, `72ADCFA0`, `72ADB500` | 8/12 | `d3d9.dll+0x223D3` |

**Settled frame** = one txn with `bandA>0` and `bandB>0` on the **same** `surf`.  
Log: **162** settled txns of **360** frame txns; **359/360** share `0AB75A00`.

Txn **1** (first pause frame): band A only (`settled=0`) — expected.

From txn **2** onward (example):

```
bandA=8 bandB=14..15 draws=22..23 settled=1
boundsA_tx=[-0.9236,0.4310] ty=[-0.5443,0.8587]
boundsB_tx=[-0.6408,0.3639] ty=[-0.1274,0.5557]
```

## Phases vs one transaction

- **Not** separate RTs: A and B share the backbuffer surface.
- **Within one settled frame**: both bands appear in **one** closed `pause-canvas` txn (A batches then B batches).
- **Across frames**: repeating pattern (8 A-draws + ~15 B-draws per frame while pause is held).

## Caller / GFx ownership

- Return addresses resolve to **`d3d9.dll+0x23194`** and **`d3d9.dll+0x223D3`** (HudStereoPair call sites on the user-pointer draw hook path).
- **No** `Singularity.exe` GFx render entry observed; **`RvUIGFxMovie_Pause` begin/end is still unknown**.

## SBS / mono coordinate evidence

- Logged draws use **full** `vp=0,0,5264,2684`; scissor logged as full width **before** the per-eye split inside `HudStereoPair`.
- Band A `tx` spans negative and positive clip (`-0.92…+0.43`) on one canvas — layout is **not** “one eye half only” at draw time.
- Band B clustered around `tx≈-0.64` with varying `ty` (Objectives scroll).
- Eye halving is applied **after** this trace point (per-eye scissor + c5 remap), matching the LEGACY split at submit.

## Validator (`tools/analyze-pause-canvas-log.ps1`)

- **162** settled txns pass band/surface/notOrtho/mid checks.
- Automated aggregate flag `why=0x100` on full log — treat **settled-pass count** as the Gate A metric until the aggregate rule is tightened.

## GO / NO-GO

| Question | Verdict |
|----------|---------|
| Narrow backbuffer-canvas **per-frame** txn for mono capture scope | **GO** |
| Whole **`RvUIGFxMovie_Pause`** render-call redirect | **NO-GO** (call boundary not found) |
| Gate B redirect implementation | **Stop** until MAIN implements capture using this **proved** scope only |

## Smallest implementation supported (Gate B, later)

1. On each **settled** pause frame, redirect only `PauseBackbufferCanvasRoute` batches (bands A+B) into one `5264×2684` A8 RT (same token).
2. Present via existing head-plane full-UV path; suppress **backbuffer** canvas draws for that token after verified capture.
3. Do **not** hook `0x23194`/`0x223D3` as “GFx” — they are mod batch interceptors; use them only as trace landmarks.

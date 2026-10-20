# Scope: actual weapon projection and capture-view consumer

Physical E5557636 test: rifle trial accepted; scope visible beside lens, view
fixed despite controller motion. User reports drops to 40 FPS. Accepted Steam
DLL/INI restored and independently rehashed before any changes here.

Evidence (completed log, no new headset run): before scope ~12–14ms/frame;
active scope ~26–30ms, sometimes35ms. Own native capture draws571–676 objects
with1756–2100 constant uploads. Three512px readbacks alone do not explain a
38-second slowdown. GPU wait numbers do NOT measure the whole GPU workload.
90Hz requires11.11ms: neither the baseline interval nor scope interval proves
that target. No promise of stable90 from L1/L2.

## Falsifiable changes

1. Native lens centre/radius already measured from original97-point material.
Previously its overlay used an independent XR/UI projection, while the weapon
used native foreground, controller and per-eye matrices. Now its pre-controller
points are transformed by the exact CTAB-named ViewProjectionMatrix during
the fingerprinted Kasimov body draw. No register guess, separate calibration,
double controller transform, or fullscreen crop. Missing/stale projection refuses
presentation. Falsifier: overlay still beside the native lens after movement.

2. Archived complete factory205/ctor195/render737 and SetView246 bodies show
component+A0 view copied to info+50, which native Render consumes. SetView dirties
the component, not the existing copy. Publish only our component's finite view
through a synchronized mailbox; exact owned-target RenderHook updates its own
info+50 from that snapshot, age<=150ms. Never write info+14 (clear colour),
never touch borrowed captures. First8 nonzero matrix deltas distinguish a stale
proxy from another cause. Falsifier: deltas zero or view still fixed despite
changing component view. This is a controlled experimental fix, not L4 proof.

3. Replace coarse first3 GetTickCount duration receipts with QPC CPU callback
mean/max, <=30 one-second windows. Draw/upload counts alongside. This measures
the incremental native scene submission, not GPU execution. No per-frame pixel
readback, new global renderer hooks, quality/resolution reductions or change
to accepted rifle/reload/input/shadow/localization. Main-frame hitch analysis
remains separate; do not claim all drops are scope-related.

Steam remains accepted. Next candidate must pass CPU tests, x86 PE32 /O2,
hash/provenance and145-pin deploy preflight before a fresh user start. Physical
test without chat switches: rifle regression; scope to eye, move only gun,
then move head and lower gun; normal exit and feedback on placement, direction,
scope-on/off FPS. No claim of physical verification before that run.

# Held knife versus TMD glove: route gate

The current physical observation is a knife floating away from the Sense
controller/hand after TMD acquisition. The inspected `514bb03` log records
held knife pickups with `held=0`, while installed initialization says
`HipKnifeIgnoreTMD=1`. That flag explicitly forces the pre-TMD two-hand rule;
the right-hand-only post-TMD policy was therefore not exercised in this log.

`KnifeHandGripEnabled=1` does not by itself prove the original glove/depth
joint rendered. The native joint publication requires the exact noTMD glove
palette; after TMD acquisition, material 0 can instead be the TMD glove.
When no joint is submitted, the separate OpenXR-pose knife renderer draws.
The current log contains `draw=OK`, but lacks a same-pickup route receipt.
This is a concrete ownership ambiguity, not yet a proven model-offset error.

New bounded receipts record the pickup's TMD/IgnoreTMD status and whether a
held knife uses the standalone route. No pose, mesh or input behavior changes.
Falsifier: if the knife is visibly detached while the native joint route is
confirmed in both eyes, investigate its wrist bind/mount; if only the
standalone route executes, measure that route against the final TMD glove
palette before changing the grip. A guessed offset or exposing native arm
faces would regress the accepted two-hand assembly.

Offline compile `/O2` passed. Knife alignment remains **NOT FIXED / L4 OPEN**;
MAIN must keep this as a diagnostic candidate rather than an accepted visual
repair. The installed INI is untouched.

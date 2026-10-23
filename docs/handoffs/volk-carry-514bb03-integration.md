# Volk carried magazine on the accepted short-gauntlet base

Integrated only commit `9bb0e47` onto `514bb03` after the independent L2
owner-gate change. Git applied without conflict; the newer two-hand support,
SVTG sleeve, hardware fallback and gun-scale code remain present. This adds a
Volk-only TMD material-0 wrist witness for the already existing manual-reload
FSM. It publishes the witness after successful stereo draw and restoration;
missing data still refuses the carried-magazine draw rather than placing it
at an unmeasured AIM-space location.

The original private `ch_inview_tmd_mesh.psk` and
`ch_inview_shotgun_mesh.psk` were run through the existing bounded geometric
fit. The fitted Volk mount is **identical** to the accepted noTMD mount:
translation `(5.99999568, -8.88449250, 3.80813382)` UU, 0 triangle
intersections, no sampled containment, nearest surface `0.11063` UU.
TMD source has 1578 glove triangles and the magazine has 198. Therefore an
arbitrary mount offset is not justified by the available mesh evidence.
If the carried magazine still floats in PSVR2 after this witness route,
measure the actual material-0 final palette, wrist publication and magazine
replay in the same frame before changing placement.

L1: `test-weapon-carry-ray` x86 `/O2` and `test_carry_ray_wiring.py` pass;
dual-TMD and short-gauntlet suites pass. The broader Centurion script ran
through the knife and next-reload policy tests but stopped at its explicit
private `-PreparedRoot` requirement; that fixture was not falsely marked
passing. L2 build follows this integration. No Steam/PSVR2 validation here.

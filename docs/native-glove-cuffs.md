# Full original glove cuffs and closed native weapon hands

SIDE, 2026-10-16; origin/main 644be25. User screenshot/feedback: free hands
truncate near wrist instead of full native cuff; weapon hands expose an open
back where the gun is visible. Original assets only, no Steam writes.

Evidence before runtime edit: old SVNI selects >=-3.5UU from wrist (source
rim -3.45..-0.89UU), but the actual original noTMD glove reaches -10.786UU.
Both noTMD/TMD full ActorX index arrays uniquely round-trip byte-for-byte to
private decompressed RvGame, preserving original winding. Finger-connected
surfaces identify hands separately from a disconnected 436-face right forearm
strip. noTMD: 2197 original faces per hand, left boundary 23/6/4 and right23;
TMD:1786 left faces (already closed),2180 right (holes3/28/3).
Native renderer only substitutes capped IB when NativeGlovesEligible or knife
off-hand pose executes: normal held gun keeps the uncapped face-range filter.

Hypothesis: replace ONLY audited original hand index selections with complete
finger-connected original cuffs plus caps in BOTH free and held native paths.
Reuse original wedges/skin/UV/material/normal/specular/light/scale. Never pose
held-gun fingers or change anchoring. Falsifiers: hollow axial view persists,
new cuff tears during grip/weapon animation, visible hand size/material changes,
weapon grip/aim/shadows regress, other section is modified.

Prepared geometry: noTMD4394 exterior+48 cap=4442 faces; TMD3966+28=3994.
The source has a pinched left cuff seam; edge-disjoint boundary cycles keep
all holes, not an arbitrary wrist-depth cut. Projected ear clipping refuses
diagonals already used by the source (an unconstrained ear gave a non-manifold
edge and was REJECTED offline). No generated exterior geometry or UV rescale.
All welded aliases have identical original weights; every welded edge closes.

Private `native_cuffs.svnc` v1 contains exactly the two audited index profiles,
not copied textures or an alternate rig. Existing native_hands.svni and thumb/
knife caches remain unchanged. Runtime experiment `NativeGloveCuffClosure=0`
default OFF; requires accepted native hand mode and exact independent-hand
scene palette/context. The GPU IB is DEFAULT-pool, bounded/cached, released on
Reset; failures retain the old native route. The held-gun skin palette is not
rewritten by this experiment. Updated joint knife depth draw must consume the
same selected triangle count as the colour draw.

Do not distribute extracted PSK, package or SVNC in Git; rebuild locally with
`tools/gloves/prepare_cuffs.py` against owned offline sources. MAIN must verify
installed material/profile identity, preserve its current integration, deploy
the new cache + flag via reviewed accepted-profile manifest, not replace INI.

Offline tests/build and physical three-sentence route follow after verification.

## Reproduce offline

Use `tools/gloves/prepare_cuffs.py --no-tmd <offline ch_inview_noTMD.psk>
--tmd <offline ch_inview_tmd_mesh.psk> --cooked <offline decompressed RvGame>
--output build/gloves/cuff-repair`. Input hashes and all source face IDs are
recorded in the private cuff-report.json. New SVNC SHA-256:
`1C9E5F618B8C3688A619BC866F47F2C7F853CEBCA7C6F8DDA425C3F13942259B`.
No texture, geometry scale, gun calibration, animation curve or palette is
stored/replaced by SVNC. Full noTMD cuff is about15.4cm behind wrist at the
existing52.5UU/m and75% mesh setting; actual user settings remain authoritative.
TMD left hand retains its original shorter, already closed gauntlet geometry;
do not lengthen the device or clone noTMD vertices into that different rig.

`tools/gloves/test-cuffs.ps1 -Cache <generated native_cuffs.svnc>` exercises2048
context combinations, exact mesh signatures, decoder corruption/truncation and
source IB min-size/refcounts. `tools/gloves/test_cuffs.py` accepts the same
three source arguments and `--output` for independent source closure, unchanged
finger geometry,21 blended-skin seam probes per hand/profile, axial rays and
CPU geometry previews (not native material screenshots).

MAIN target: `Binaries/vr-assets/hud/katorga/glove-animation/native_cuffs.svnc`;
one new Render key `NativeGloveCuffClosure=1`, with accepted native gloves and
GloveIsolate=2 retained. Existing SVNI/thumb/knife files stay byte-identical.
The deployment guard now requires a pin for SVNC when this flag is enabled.
The current narrow DLL-only runner does not copy extra assets: MAIN must add a
reviewed exact-file map/backup adapter for this cache, not bypass the guard or
use a broad install. The standalone SIDE DLL is an older-base build, not a
replacement for MAIN's newer camera/reload integration.

Physical route (MAIN only, no mid-test chat): With an empty hand, turn both
wrists and look from the forearm end: the full cuff must remain attached with
no see-through hole. Draw Centurion, repeat the axial view, then holster/draw
the knife and verify unchanged grip/occlusion/scale in both eyes. Check the
TMD-equipped save if available, pause/resume, close normally and report holes,
material seams, tracking, gun aim and FPS; absence of a TMD save leaves that
physical scope UNKNOWN, not implicitly accepted.

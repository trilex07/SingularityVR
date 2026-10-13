# Texture classification cache — controlled repair

Scope explicitly authorized by user2026-10-13: repair injector cache only,
preserve DDS assets, target hashes, dimensions, groups, quality, pixel data,
and existing positive substitution policy. MAIN's own worktree/branch only;
do not edit SIDE's asset checkout or run its installer.

Baseline installed injector AAA07005F4FF658C4A71CC30EA258E469DAF250C77BFE447011FFD650379E4F7
matches the SIDE-built DLL. Audited source SHA738C930FA3F0F8DB6AFA840B413C219A80ED187A6F765C3D2D838A39571AF19F;
target metadata B0ACE1E91A9A099932B9D08E752F2E68F441AF35BF0E52DE55E6AB010360B319;
group policy CDA05170C1A642B222E36C9161BB2D46BBE5F73F9686FFC4CFE2AA80A8FEC060.
Import only mod source/lookup metadata into this branch, no original pixels.
Original source snapshot stays recoverable in Git. See second-entry crash note
for actual native FNV hot instruction and target-only CPU evidence.

Hypothesis: persistent old negative entries fill the512-pointer table. Every
new unmatched texture then evicts the same first nonreplacement slot, causing
alternating resources to hash again on each bind. Existing log contains655
distinct content hashes,533291 classification lines. Falsifier: after an
executed resource-negative cache gate, repeated hashing and slow phase remain.
Runtime counters must distinguish native metadata hits, hashes, legacy hits,
metadata failure/fallback and cache saturation. No claim from compilation alone.

Small repair: store ONLY the already-established native/no-substitution
classification in a private resource-owned D3D9 byte record. This bypasses
the512-table for negative results; retain positive replacement/loading path.
No texture-content write, original-resource AddRef, lifetime pin, per-bind
allocation, arbitrary UE3 offsets, or extra invasive hook. A resource's own
private record cannot survive destruction/address reuse like a global raw
pointer negative entry. Metadata failure uses legacy cache with bounded
rotating nonreplacement eviction; never evict a held replacement. Optional
general.resource_native_cache=0 keeps the old cache policy as a control;
installed INI is not rewritten. Diagnostic output is capped and aggregated
on the existing initialization worker, not per-draw filesystem writes.

The immutable-content assumption is unchanged from the old resolved cache;
this is not a dynamic-texture content tracker. The old positive raw-pointer
cache/lifetime policy remains outside this narrow negative-cache repair.
No claim that hands stretching is repaired by this performance intervention.

L1: reproduce old saturation churn, >512 resource results remain cached,
null/error/wrong-size/wrong-marker gates, new lifetime at reused address,
fallback victim cycling with pinned positives. L2: x86 PE32/O2 DLL, SHA and
source manifest. Native D3D9 synthetic resource test: metadata reads, all mip0
pixels identical, texture release/new lifetime and device reset. MAIN physical
gate: same catastrophe -> stereo gameplay, FPS/triggers, hands observation,
texture appearance unchanged. Exact backup/restore of injector + leased VR
DLL, no asset/INI overwrite; ask for availability before launch.

Official API semantics: resource-private byte records are copied with flags0,
and GetPrivateData writes only the supplied caller buffer:
[SetPrivateData](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3dresource9-setprivatedata),
[GetPrivateData](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3dresource9-getprivatedata).

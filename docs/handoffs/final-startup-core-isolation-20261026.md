# Startup isolation: protect original core, retain integrated payload

Current decision: **GO for controlled V7 PSVR2 test**, not release PASS.
Installed: verified AB348E06, no active game/restore guard. Exact evidence in
`final-startup-core-isolation-20261026.json`.

## Actual comparisons

- New runtime pair with accepted assets/config: Steam PID 38504, session
  `startup-control-20261026/sessions/20261026-140236`, OpenXR ready and scene
  draws. Normal closure and exact baseline restoration verified.
- Full V4 except original core: PID 32108, session
  `startup-all-except-corebank-20261026/sessions/20261026-140714`, OpenXR ready,
  scene draws and split submissions. Headset asleep (`shouldRender=0`): not
  physical/pixel or performance PASS. All 134 assets + four runtime files
  restored; full 164-pin postflight PASS.
- Padding-only modified core: PID 39540, session
  `final-complete-v5-aligned-20261026/sessions/20261026-141341`, same startup
  c0000005 at EXE+ACA50. Appended 42750 zero bytes align to 64 KiB without
  changing compressed/logical content; decode PASS. **Alignment alone is not
  the fix.** Full restoration verified.
- Native-block-preserving modified core: PID 33644, session
  `final-complete-v6-native-blocks-20261026/sessions/20261026-141926`, same AV.
  213 native blocks retained verbatim, only block 140 recompressed, every block
  within uncompressed capacity. Standalone and independent safe LZO decoders
  reproduce exact logical bytes for all 214 blocks. **Not load-safe despite
  decoder equality.** All files restored and 164-pin postflight PASS.

Private paths are relative to `build/next-weapon-reload/`. Sessions retain exact
logs, guards and install receipts; failures also retain dumps. The full-minus-core
control's raw `live.json` inherited runtime-only labels. Manifests/production
backup inventories establish actual scope; that raw record stays unchanged.
The JSON companion corrects its interpretation.

## Proven boundary and remaining unknown

The full set boots with accepted core retained. Modified core reproduces the
V4 packaging failure. This establishes a usable deployment boundary, **not the
first corrupting operation**. The allocator array is already invalid at 004ACA50;
its original writer, an integrity check or a particular loader bug is unproven.
Do not patch the allocator or guess more compression variants.

Historical `combined-session-candidate.md`, `combined-acceptance-followups.md`
and `contextual-cursor.md` contain earlier modified-core failures at this RVA.
V3's report incorrectly said the core was unchanged; that statement is corrected.
Failed compressor changes are removed from production, archived privately.

## Minimal production change

`session-asset-scope.ps1` rejects `RvGame/CookedPC/RvGame.xxx`. V7 protects
accepted SHA `4153c4889c90635069ed226d918cdb76be618f46b1baea152ac10cffdf59e9ef`.
Real parser/deployer fixtures verify refusal before backup/write even with
correct candidate hashes. Full-session fixtures verify protected core drift
refuses before restoring another file. Other scopes stay unchanged.

V7 payload/runtime hashes equal the booted 134-file control. Only the TMD impulse
sound remaster is withheld; its native cue/wave stays original. The **Age visual
attachment correction is included**, independent of this sound. Seven other
sound sets, all V4 campaign banks, native menu and HD assets are retained.

Scope/escape, production asset deploy/restore, four-file + assets fake-Steam guard,
10 fixed-allocation/independent decode and eight timing-delivery tests PASS.
Whole-package tests use `uedecompress.exe`; an initial misconfigured run passed
the internal chunk decoder and failed, then passed with the correct executable.
No C++ changed; existing clean-source x86 PE32 `/O2` DLL hashes checked.
Real read-only preflight validates 138 sources and 164 baseline pins.

Use `../release/final-v7-safe-session-20261026.md` for physical testing. Further
core remaster work needs the actual loader/mutation path, separately from that
session. Old failed payloads/guards must not be relaunched.

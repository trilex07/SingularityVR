from pathlib import Path
import hashlib,json,subprocess
r=Path(__file__).resolve().parents[2]; out=r/'docs/handoffs'; out.mkdir(exist_ok=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
maps=json.loads((r/'docs/textures/hires-first-person-manifest.json').read_text())
tests={}
for p in Path(__file__).parent.glob('*.log'):
    tests[p.stem]={'log':str(p.relative_to(r)),'sha256':sha(p),'tail':p.read_text(errors='replace')[-1400:]}
m={'schema':1,'branch':'integration/astra-final-side-candidate','source_head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip(),
   'sources':{'hud':'d79ad975','gameplay_bridge_icons':'09887b18','reload':'54bd946','hires':'d837d68','audio_remaster':'ba8774c','old_voice_qa':'602a5c','bink_research':'47b4ef7','xtts':'c3c911d'},
   'binaries':{f:sha(r/f) for f in ['spikes/view_matrix/d3d9.dll','deployment/texture-cache/artifact/dinput8.dll']},
   'installed':False,'physical_acceptance':'PENDING; HUD mirror/left-hand reports remain open',
   'private_textures':maps,'first_gate_stage':'C:/Users/Trilex/vr-mods/SingularityVR-assets-local/astra-first-hires-gate-20261024',
   'normal_settings':{'priority_material_mode':0,'remaster_apply':0,'spread_percent':100,'preserve_installed_ini':True},
   'accuracy_ab':{'TwoHandSpreadPercent':100,'VrValkyrieSpreadPercent':70,'VrValkyrieTwoHandPercent':50,'VrVolkSpreadPercent':85,'VrVolkTwoHandPercent':50},
   'tests':tests,'blockers':['MAIN accepted HUD reconciliation','font package differs from original patcher pin; no repatch','Valkyrie/Kasimov lack isolated priority scope gates','Centurion/Kasimov diffuse source top mips unproven','native audio packaging needs accepted bank/profile pins and explicit package list; no bank enabled']}
(out/'ASTRA_FINAL_CANDIDATE.json').write_text(json.dumps(m,indent=2),encoding='utf-8')
(out/'ASTRA_FINAL_INTEGRATION.md').write_text('''# Final SIDE integration — 2026-10-24

Branch: `integration/astra-final-side-candidate`. No Steam launch, deployment or configuration writes. This is an offline integration candidate, not an accepted release.

## Actual integration

Started at HUD d79ad975. Merged 09887b18 without conflicts (ad4f5b15), preserving detailed icons, native spread policy and the single-owner slot-65 bridge. Cherry-picked reload comfort 54bd946 as 56a157ab; imported hires tools/manifests 132d24d1 and d837d68 as 35ab3b33/c936f7f1. HUD draw/projection headers remain unchanged. No input, aiming or TMD redesign.

Valkyrie/Volk insertion radius is 18 cm; Kasimov remains 16 cm and Centurion separate. Native Grip release and ammo transactions remain authoritative. Accuracy defaults remain 100%; optional balance settings are separate from the candidate's normal configuration.

Added fail-closed private staging for only four first-gate DDS maps. All seven private candidate hashes, source hashes/FNV, layouts and complete mip chains verified. First four copied to the private path in the JSON manifest. No proprietary DDS/WAV/package content is committed.

NIS source-wiring regression still expected pre-OLED g_frameSrc11. Updated it to assert actual displaySrc consumption; renderer unchanged. NIS pixel tests already passed. OLED processing precedes NIS; do not bypass displaySrc or combine independently allocated targets.

## Validation

Unified x86 PE32 /O2 build PASS. Actual DLL disassembly retains literal [eax+20Ch] and FName stack setup. Dual TMD/reload suite PASS, including 30,000 reload transactions and 4,000 carry samples. HUD actual D3D9/Ex atlas tests: 108 cases each; 24 projection cases. Texture bridge D3D9/Ex, UpdateTexture invalidation, Reset, COM ownership tests PASS. TMD material replacement submits 2x7110 triangles with restored IB in offline D3D9/Ex test. OLED CPU/GPU comparison PASS. Haptics 7 tests, voice coverage 4, native-bank tools 4, voice staging 3, note manifest and icon tests PASS. See hashed log receipts in JSON; historical failing NIS log is retained and superseded only by the passing wiring rerun.

Font synthetic checks PASS; the optional existing-package check fails the patcher's original-package pin. This does not prove missing Polish glyphs: the package may already be patched. Do not replace it with the old patcher's input. MAIN must reconcile its accepted hash before any font work.

## Audio and remaining scope

ba8774c audio tools/inventory are already byte-identical on this base. Eight private targets / 24 audition WAV variants passed PCM rate/channel/duration/metadata checks. This is NOT SoundCue playback, true-peak or listening acceptance. Existing prepare_xtts_native_banks.py and stage_xtts_native_delta.py preserve cue properties and require accepted profile hashes and explicit package selection. No new native bank produced: reviewed current profile/bank pins and selected take manifest are not established for this candidate. Follow-up: MAIN supplies those inputs, tool stages outside Steam, review package diffs, then separate audio test.

602a5c Pocket-TTS QA is historical, superseded by XTTS tooling; do not overwrite current voices. c3c911d note data is already present; corpus generation claims are not current installed-playback proof. Bink original Polish track routing is retained; old intro-only pilot is not reintegrated. Preserve working language INI. Accepted UI/location behavior is preserved; no new location patch justified by this task. Do not reopen accepted VFX/shadows/cutscenes.

## Review outcome

GO for source review and staged offline candidate; NO-GO for promotion as a stable release until MAIN reconciles its physically accepted HUD tip. User-reported mirrored counter and left-hand offset are unresolved, owned by MAIN. No physical PASS claimed. Valkyrie/Kasimov hires remain staged, not enabled: their old broad remaster scope is not an independent safe gate. Centurion and Kasimov diffuse remain blocked by original top-mip/identity evidence.

Binary hashes, source commits, assets, configuration deltas and test receipts: ASTRA_FINAL_CANDIDATE.json.
''',encoding='utf-8')
(out/'ASTRA_FEATURE_STATUS.md').write_text('''# Feature status — combined candidate

Installed means this exact combined artifact. No row is installed by SIDE.

| Feature | SIDE source | In final branch | Built | Installed | PSVR2 grade | Missing work |
|---|---|---|---|---|---|---|
| HUD 3D | d79ad975 | INTEGRATED unchanged | OFFLINE PASS | No | PHYSICAL FAIL: orientation report; hand offset open | MAIN accepted-tip reconciliation |
| Reload comfort | 54bd946 | INTEGRATED | OFFLINE PASS | No | Pending new radii | 18cm Valk/Volk physical test |
| Accuracy | eade95a via 09887b1 | INTEGRATED | OFFLINE PASS | No | Proposed balance unaccepted | Separate 100% vs A/B |
| Single-owner texture bridge | PR57 via 09887b1 | INTEGRATED | OFFLINE PASS D3D9/Ex | No | Pending | Log-only then four-map replacement |
| Volk + gloves 2K | d837d68 | PREPARED private stage | OFFLINE PASS hashes/mips | No | Pending | MAIN controlled gate |
| Valkyrie 4K / Kasimov normal | d837d68 | PREPARED assets | OFFLINE PASS assets only | No | Pending | BLOCKED independent runtime scope extension/review |
| Centurion / Kasimov diffuse | d837d68 | BLOCKED | No | No | Not tested | Verified top mip/source identity |
| Detailed icons | 3223b24 | INTEGRATED | OFFLINE PASS | No | Prior user PHYSICAL PASS for source; combined pending | Preserve PL/EN atlas |
| TMD/gloves/haptics | inherited baseline | INTEGRATED unchanged | OFFLINE PASS | No | Left-hand report open | MAIN physical validation |
| WeaponLocker | inherited ABI fix | INTEGRATED unchanged | OFFLINE PASS compiled ABI | No | Combined pending | Preserve acquisition/settings |
| NIS/OLED | inherited | INTEGRATED unchanged | OFFLINE PASS pixels | No | Prior reports only | Combined physical check |
| Notes/font | existing Polish assets | INTEGRATED | Synthetic PASS; package pin BLOCKED | No | Current unverified | Reconcile accepted font hash; no repatch |
| Eight audio remasters | ba8774c | Tools INTEGRATED, WAV PREPARED | OFFLINE PASS PCM | No | Not tested | Native cue packaging/listening separate |
| XTTS | c3c911d; 602a5c historical | Existing tooling retained | OFFLINE PASS tool regressions | No | Playback unverified | Accepted bank pins and selected take manifest |
| Official Polish Bink | existing expanded routing; 47b4ef7 research | INTEGRATED unchanged | Compiled | No | Current unverified | Preserve original tracks/language config |
''',encoding='utf-8')
(out/'ASTRA_MAIN_DEPLOYMENT_PLAN.md').write_text('''# MAIN-only deployment plan

1. Confirm latest physically accepted HUD commit. This branch preserves d79ad975; do not promote until MAIN's later fixes are reconciled without discarding this integration. Review JSON source/binary hashes and keep the accepted install/profile backup.
2. Build/reverify if reconciliation changes source. Install only through existing MAIN deployment tooling, with game closed. Preserve installed INI, Polish assets, texture quality and accepted features. No full-INI replacement. First session gameplay integration only; remaster_apply=0 and priority_material_mode=0. Verify four reloads, HUD, TMD, wheel, NIS/OLED and locker.
3. Accuracy independent A/B: keep TwoHandSpreadPercent=100; compare defaults (100) with VrValkyrieSpreadPercent=70, VrValkyrieTwoHandPercent=50, VrVolkSpreadPercent=85, VrVolkTwoHandPercent=50 under [Input]. Not accepted balance yet.
4. Texture gate 1: verified dinput8 texture bridge + first four staged DDS only. In texture_override.ini [general], remaster_apply=0, priority_material_mode=1, remasterdir=<MAIN-reviewed staged path>. Log-only leaves existing base override behavior intact; it does not disable unrelated accepted base overrides. Verify exact four target hashes and no UI/RT/shadow matches. Next separate session priority_material_mode=2; test materials, alpha, stereo, reset and pacing. Return mode0 to accepted base behavior.
5. Do NOT set broad remaster_apply=1 for subsequent gates. Valkyrie pair and Kasimov normal are staged with hashes but need independent scoped gate extension before deployment. Blocked mip sources stay blocked.
6. Audio is separate: inspect tools/audio/prepare_xtts_native_banks.py and stage_xtts_native_delta.py inputs. Require accepted profile SHA, bank pins, selected manifest and exact package list. Generate outside Steam, review retained/replaced waves and cue linkage. No blanket WAV copy or mixed audio/gameplay deployment. Preserve original Polish Bink tracks.
7. Font gate is BLOCKED on current package identity. Old patcher expects original package; never replace accepted fonts just to make that test pass. Notes manifest synthetic tests pass independently.

Private first-gate staging is reproducible:
`python tools/textures/stage_first_hires_gate.py --assets-root C:/Users/Trilex/vr-mods/SingularityVR-assets-local --name <new-unique-directory>`

The script verifies seven sources before writing four reviewed maps, refuses existing destination and traversal, and records hashes. Review the candidate JSON for exact paths/SHA. SIDE has not changed Steam or launched gameplay. Physical verification remains MAIN-owned.
''',encoding='utf-8')
print('handoff written')

from pathlib import Path
import json,hashlib,subprocess
r=Path.cwd(); p=r/'docs/handoffs/ASTRA_FINAL_CANDIDATE.json'
m=json.loads(p.read_text()); sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
head=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()
m['source_head']=head
m['sources']['hud']='f307ad32c2a52288534fcf196f70ac9739c1b59f'
m['sources']['hud_orientation']='5181cdc4f4a2c41ed7186b119737065283c17305'
m['sources']['left_wrist_diagnostic']='36081c3dff1529580ad98e8ec9fb2a5e41c25152'
m['binaries']['spikes/view_matrix/d3d9.dll']=sha(r/'spikes/view_matrix/d3d9.dll')
m['hud_sync']={'source_commit':head,'offline':'PASS','physical':'PENDING','orientation':'readerRight=-modelRight; body-owned camera/projection preserved','left_hand_diagnostic':{'section':'Render','key':'HandAnchorProbe','default':0,'measurement_value':1,'records_per_eye':8,'interval_seconds':2,'receipt':'tmd-hand-anchor final-bound','pose_writes':False},'left_hand_position_fix':False,'build':'x86 PE32 /O2','source_commit_note':'Merge commit used for compiled DLL; subsequent commit contains only reports/manifest.'}
m['physical_acceptance']='PENDING for combined candidate; left-hand offset not corrected'
for f in sorted((r/'build/integration-results').glob('hud-sync-*.log')):
 m['tests'][f.stem]={'status':'PASS','log':str(f.relative_to(r)),'sha256':sha(f),'tail':f.read_text(errors='replace')[-1800:]}
for key in ['hud','production','dual-tmd','locker-abi']:
 m['tests'][key]['historical']=True
m['blockers']=[x for x in m['blockers'] if x!='MAIN accepted HUD reconciliation']
m['blockers'].insert(0,'Combined HUD physical acceptance pending; no further source reconciliation required unless MAIN advances its HUD branch')
p.write_text(json.dumps(m,indent=2)+'\n',encoding='utf-8')
p=r/'docs/handoffs/ASTRA_FEATURE_STATUS.md'; s=p.read_text(); s=s.replace('| HUD 3D | d79ad975 | INTEGRATED unchanged | OFFLINE PASS | No | PHYSICAL FAIL: orientation report; hand offset open | MAIN accepted-tip reconciliation |','| HUD 3D | f307ad32 (5181cdc4 + 36081c3d) | INTEGRATED reader reflection + wrist probe | OFFLINE PASS 240 pixel cases | No | PENDING; old orientation failure addressed in code, hand offset open | MAIN physical acceptance |'); p.write_text(s)
p=r/'docs/handoffs/ASTRA_FINAL_INTEGRATION.md'; s=p.read_text(); s=s.replace('HUD draw/projection headers remain unchanged.','Initial integration preserved HUD draw/projection headers; the follow-up below synchronizes the reviewed HUD fixes.'); s=s.replace('GO for source review and staged offline candidate; NO-GO for promotion as a stable release until MAIN reconciles its physically accepted HUD tip. User-reported mirrored counter and left-hand offset are unresolved, owned by MAIN.','GO for source review and controlled MAIN testing; NO-GO for stable release until physical acceptance. The latest HUD source is now integrated: reflection is corrected in code, physical acceptance pending. Left-hand offset remains unresolved; bounded measurement is provided.'); s+='''
## Latest HUD synchronization

Merged f307ad32c2a52288534fcf196f70ac9739c1b59f at d933715b733e58acec271e213d5f9705ff110f6a. Only conflict was STATUS.md; both entries retained. Preserves all prior integration changes and body-owned projection. Includes 5181cdc4 reader-axis reflection and 36081c3d final-bound wrist measurement; no guessed wrist offsets or tracking writes.

Focused offline PASS: 6 recorded pose replays, 24 stereo projections, 120 D3D9 + 120 D3D9Ex pixel cases; wrist probe/math plus 7 parser cases; full critical gameplay/TMD/reload/OLED suite; actual compiled WeaponLocker ABI. Build x86 PE32 /O2 PASS. See manifest for exact DLL SHA256 and hashed logs. Physical acceptance PENDING.

Opt-in diagnostic: [Render] HandAnchorProbe=1; default 0. Eight records per eye at >=2 seconds, final-bound receipt. No configuration change or Steam access performed. Texture replacements and experimental accuracy settings remain disabled/default100.
''';p.write_text(s)
p=r/'docs/handoffs/ASTRA_MAIN_DEPLOYMENT_PLAN.md';s=p.read_text().replace('This branch preserves d79ad975; do not promote until MAIN\'s later fixes are reconciled without discarding this integration.','This branch now contains f307ad32 including orientation correction and final-bound wrist diagnostics. Physical acceptance is still pending; reconcile only if MAIN advances again.');p.write_text(s)

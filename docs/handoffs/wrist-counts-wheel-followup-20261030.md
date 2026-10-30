# Wrist live counts / wheel follow-up — 2026-10-30

User feedback on 57840426: E99 absent, legacy health also visible; requests health-pack count; no perceived menu change; Seeker selectable but expected shotgun absent.

## Actual session evidence
Archived log and native menu content: build/wrist-main/session-57840426/.
- First pawn 31ADA000 mask=6 (Valkyrie + Volk). After loading pawn 1C1A6400 mask=3 (Centurion + Valkyrie), then native test grant adds Kasimov: mask=B. Seeker native chain exists and selection-equipped confirmed. No Volk is present in this loaded pawn's possession chain. User clarified they expected it from an earlier save, not that they held it in this session. No artificial ownership/icon or acquisition added.
- Native menu package installed SHA CDA63308 matches SIDE. Actual 14:50:26 class-2 RGB capture shows rectangular button plates and cyan active border. This is button polish, not a replacement of the whole menu. Imported visual appearance is not inferred from hashes.
- Wrist live receipts still show HUD=0, all optional fields invalid. No equipment-source success receipt. Replacing sharedHUD with myHUD alone was insufficient; missing field, owner mismatch and null native link remain distinguishable hypotheses, not established causes.
- INI explicitly kept EquipmentWristPreviewNoDepth=1, EquipmentWristSuppressHealth=0 and HealthPlateEnabled=1. Old display was deliberately retained during preview; follow-up production configuration will disable both HealthPlateEnabled and WristHealth once requested replacement is prepared. Wrist geometry/VFX remain accepted.

## Bounded diagnostic
c1ecf92d adds only 12 maximum input-thread source gate receipts: controller/class/pawn, actual controller Pawn, named indices, resolved offsets, raw myHUD and cached HUD, optional offsets and solved property layout. Existing reads/ownership/freshness are unchanged. This distinguishes the FIRST rejecting gate; no speculative offset, ownership bypass or fake counts.
Build x86 PE32 /O2 PASS; DLL 52B883791C4BE7E694814DB909314E3C2C9ADDFFC63643354CD337AC6F3C1A51.
User authorized short load/close without headset. Guarded Steam deployment report deployment/install-reports/20261030-150307.json; backup deployment/install-backups/20261030-150306. INI/assets untouched. Await live gate before implementing source correction. No new physical PASS claimed.

## Diagnostic result and minimal correction
52B88379 live receipt reason=4: myHUD resolved at +992 and is non-null (1CB64000, then32656000 after load), but generic controller Pawn property lookup is -1. In the same session the existing typed native local-player resolver confirmed reciprocal controller 1C4C3200 -> pawn3186A000, then1C1A6400. First proven violation: the new HUD rejected an already validated local identity through an independent failing generic lookup. Why that older generic walk fails for this property is not established; no shared property walker was changed.
HUD now reuses ReadNativeLocalPlayer(), including its native object type, camera/controller and pawn/controller reciprocal checks. It additionally requires equality with the published controller/pawn. myHUD lookup and field count/freshness checks remain; no guessed offsets or global object scan.
Requested production profile: EquipmentWristPreviewNoDepth=0, EquipmentWristSuppressHealth=1, HealthPlateEnabled=0, WristHealth=0. E99Tech and health-pack rows already exist and remain hidden until valid actual values are read. Geometry/VFX/menu/wheel unchanged. Live counter acceptance remains pending.

## Corrected candidate deployment
Source b239bf30; x86 PE32 /O2 DLL SHA256 147C3297211CFA6CA6B964E29FBBD79538B7EE69D7BC155861B66F67B24BDC96.
L1/L2 PASS: startup-readiness native reciprocal ownership tests; HUD source freshness/identity; D3D9/D3D9Ex both-eye wall/glove/magazine pixels and resource restoration/reset. D3D fixture rerun outside sandbox passed after sandbox device creation failure.
Guarded Steam deployment completed with game closed: deployment/install-reports/20261030-151338.json; backup deployment/install-backups/20261030-151337. Installed INI SHA256 81E9F33BE4D6A5A968DE0D96A45C59A262F2FDD3DB8ECF7DED89D40CF12A87E2. Only DLL/INI deployed. No launch or physical validation performed for this build; real optional counter values PENDING.
Acceptance: load save, inspect inward left wrist for actual E99 Tech and health-pack count, confirm legacy health displays absent. Preserve accepted wrist occlusion and VFX. Candidate retained for user test.

## Physical acceptance and main promotion
2026-10-30: user confirmed the requested E99 / health-pack / legacy-health test with "tak jest mozesz dac na main" and authorized main promotion. HUD counter presentation and legacy-health replacement are PHYSICALLY_VERIFIED for this session. No broader new weapon/menu/performance acceptance is inferred.
Before promotion, installed DLL remained 147C3297211CFA6CA6B964E29FBBD79538B7EE69D7BC155861B66F67B24BDC96 and INI remained 81E9F33BE4D6A5A968DE0D96A45C59A262F2FDD3DB8ECF7DED89D40CF12A87E2. Accepted runtime source is b239bf30; later commits update evidence only. Steam remains untouched during promotion. Backup 20261030-151337 is preserved.
Fetched origin/main is an ancestor of the complete tested candidate; promotion uses fast-forward only, without force or source conflict resolution.

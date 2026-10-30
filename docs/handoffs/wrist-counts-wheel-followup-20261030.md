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

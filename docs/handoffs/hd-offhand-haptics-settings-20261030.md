# MAIN — one HD / independent-hand / haptics session

Base: combined HD candidate `68d0a642` (PR72), plus PR70 reviewed at
`83e0075570812a867a0afea41b8dc409d96c5e4c` (seven files, two commits).
Branch: `integration/hd-offhand-haptics-settings`. Do not merge PR70 before
the user's physical acceptance. All prior HD cache identities remain unchanged.

## Review and scoped changes

- Free-hand correction is scoped to the exact original noTMD glove section,
  committed independent palette and existing native cuff draw. Only physical
  off-hand fingers are posed. Support, native animated hands, magazine/cylinder
  ownership and knife paths remain authoritative; no wrist/IK offsets changed.
  The separate original TMD-material geometry is not claimed as newly retargeted.
- Squeeze smoothing now retains previous valid analog state; inactive or lost
  tracking clears it. Six bounded pose receipts identify actual committed use.
- Optional `WeaponShotPulses=1` observes only the current local weapon's native
  `FireAmmunition` return. `mShotCount` must advance exactly once. The reflected
  property offset is cached per class/thread, not searched on every automatic
  shot or unrelated ProcessEvent. Existing dispatch already resolves function
  indices for accepted inventory/gore gates. Added work is local-fire-only.
- Native call forwarding, damage/ammo and adaptive-trigger handling are retained.
  Counter comparison uses widened arithmetic to avoid signed-overflow UB.
  One SRW-protected bounded pulse is sampled by the existing OpenXR submission
  site; existing native rumble has priority. Focus/pause/menu clears pulses.
- TMD uses the accepted confirmed native-power path, not a button-only vibration.
  A successful OpenXR return proves API acceptance, not physical vibration.

## Image preference diagnosis

Installed accepted INI had `Diagnostic/VrSettingsSessionOnly=1`, NIS `Off`,
sharpness `0.20`, and no OLED preset. `PanelSaveRow` already writes NIS mode,
strength and OLED; `PanelSave` deliberately refuses INI writes for a test lease.
Restore then returned those defaults. This is configuration/lease behavior,
not evidence that the save switch cases are missing.

User-authorized persistent baseline preferences: `Mode=NISSharpen`,
`Sharpness=1.00`, `OLED/Preset=2` (Cinematic), `VrSettingsSessionOnly=0`.
Use an exact four-key profile transaction and keep the old profile/backup.
The controlled candidate retains session-only mode to prevent unapproved test
INI drift, but starts with the requested image values and restores to the new
preferred baseline. Normal baseline settings can save when closed normally.
No global project defaults are changed for other users.

## Offline verification

- PR70 optimized x86 policy tests: pulse confirmation/dry-fire/native priority,
  independent fingers, ownership/cuff routes and invalid tracking PASS.
- Actual native glove cache and CPU/palette/cuff tests PASS; two optional custom
  asset tests SKIPPED without their input assets, not fabricated as PASS.
- Dual-TMD/support/reload/aim/Locker suites PASS. DISPLAY wiring/persistence PASS;
  its stale assertion about unrelated old page counts was made page-specific.
- Final x86 PE32 `/O2`, emitted Locker `[eax+20Ch]`, both-eye HD pixel/state,
  OLED/NIS and staged profile gates are recorded with final DLL identity.
- Real `mShotCount` progression, OpenXR submission and hand appearance/haptic
  perception require this one Steam/PSVR2 session: PENDING, not physical PASS.

## Controlled session configuration and acceptance

Keep all three HD models, scoped native test grant for Kasimov/Centurion, native
gloves/cuffs and previously accepted runtime dependencies enabled. Add:

```ini
[Haptics]
Enabled=1
Level=1
TmdPulses=1
WeaponShotPulses=1
```

In one session: right-hand weapon / left relaxed; Grip support and release;
single and automatic shots; empty trigger; TMD Impulse and Age; carried magazine
and release; switch weapon and pause/resume. Distinguish regular rumble from
adaptive-trigger resistance. Also inspect Kasimov HD/lens/reload and Centurion
HD/cylinder/reload; checkpoint after native test grant to verify persistence.

MAIN checks `free offhand pose`, `weapon haptics witness`, TMD power receipts,
`[Haptics Trace] SUBMIT/STOP`, native ammo and actual HD eye receipts. If vibration
is absent, classify witness → pulse → gates → XR result → physical hardware;
do not raise amplitude blindly. Verify restored baseline hashes after exit.

Final build source: ba43ab7dd203950b4de6067e1782ae64f91bd23d. DLL: FB1DE1BC421904A17C13F3A3C582104D466D71DA8037268168EADE072C3F40B8. Candidate INI: 45C0EC22A07747D47282EBDD53A70698CD097EE2172E7F9FE677FDAF0CF3B02A. Mirrored deployment PRE/POST PASS. Physical acceptance PENDING. Preferred restored baseline INI: 3E27A4FD56A3DAE7E84EB370814658CBE2844BEB6A05A7E2D6FA4BFA889DE8F9.

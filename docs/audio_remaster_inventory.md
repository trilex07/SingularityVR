# Singularity (UE3) Original Audio Inventory & Remaster Registry

## Overview & Extraction Provenance

- **Engine:** Unreal Engine 3 (Version 584, Licensee 126, 2010)
- **Audio Codec:** Ogg Vorbis in Native Bulk Platform Slot 1 (PC)
- **Decoding Chain:** UE3 Package Parser -> Bulk Vorbis Extraction -> FFmpeg PCM16
- **Verification Policy:** Level 3 (exact SHA-256 source payload, declared PCM frame budget)
- **Total Identified Assets:** 1079

## Classification Summary

| Category | Count | Primary Storage Packages |
|----------|------:|--------------------------|
| MUSIC | 10 | MUSIC_combatBossGrom01_XSA, MUSIC_combatHorror01_XSA, MUSIC_combatMixed01_XSA, MUSIC_combatMixed02_XSA (+6 more) |
| WEAPON_FIRE | 29 | RvGame, wp_SP_2007Pistol_SF, wp_SP_ARGattlingGun_SF, wp_SP_ARGrenadeLauncher_SF (+7 more) |
| WEAPON_MECHANICAL | 213 | RvGame, wp_SP_2007Pistol_SF, wp_SP_ARGattlingGun_SF, wp_SP_ARGrenadeLauncher_SF (+7 more) |
| E99_EFFECT | 131 | RvGame, SP_SL1_audio01_XSA, wp_SP_2007Pistol_SF, wp_SP_ARGattlingGun_SF (+8 more) |
| EXPLOSION | 32 | RvGame, SP_SL1_audio01_XSA, wp_SP_ARGrenadeLauncher_SF, wp_SP_ARSteerable_SF (+1 more) |
| AMBIENCE | 209 | RvGame, SP_Freight_Audio_2010_XSA, SP_RL1_audio01_XSA, SP_SL1_audio01_XSA |
| DIALOGUE | 0 |  |
| UNKNOWN | 455 | RvGame, SP_Freight_Audio_2010_XSA, SP_RL1_audio01_XSA, SP_SL1_audio01_XSA (+10 more) |

---

## Priority Audio Assets Detail

### MUSIC (10 assets)

| Package | Asset Name | Ch | Rate (Hz) | Duration | RMS | Confidence | SoundCue Identity | SHA-256 (Source Vorbis) | Status |
|---------|------------|---:|----------:|---------:|----:|-----------:|-------------------|--------------------------|--------|
| MUSIC_exploreDrone01_XSA | `CONCEPTexploreDrone01` | 2 | 32000 | 132.0s | 0.0893 | **UNRESOLVED** | None | `5c0777acd4c5...` | DECODED_PCM16 |
| MUSIC_exploreDrone02_XSA | `CONCEPTexploreDrone02a` | 2 | 24000 | 135.0s | 0.0807 | **UNRESOLVED** | None | `cf5ba16cef45...` | DECODED_PCM16 |
| MUSIC_exploreMystery01_XSA | `CONCEPTexploreMystery01` | 2 | 32000 | 132.0s | 0.1148 | **UNRESOLVED** | None | `d16bf0da53a8...` | DECODED_PCM16 |
| MUSIC_exploreSuspense01_XSA | `exploreSuspense01` | 6 | 44100 | 128.0s | - | **UNRESOLVED** | None | `e058e2d295f8...` | DECODED_PCM16 |
| MUSIC_combatBossGrom01_XSA | `combatBossGrom01` | 6 | 44100 | 77.8378s | - | **UNRESOLVED** | None | `2d28c4706d36...` | DECODED_PCM16 |
| MUSIC_combatHorror01_XSA | `combatHorror01` | 6 | 44100 | 124.2353s | - | **UNRESOLVED** | None | `964de0de6397...` | DECODED_PCM16 |
| MUSIC_combatMixed01_XSA | `combatMixed01` | 6 | 44100 | 123.0769s | - | **UNRESOLVED** | None | `7db150839f01...` | DECODED_PCM16 |
| MUSIC_combatMixed02_XSA | `combatMixed02` | 6 | 44100 | 125.2174s | - | **UNRESOLVED** | None | `51565d59578c...` | DECODED_PCM16 |
| MUSIC_combatTactical01_XSA | `combatTactical01` | 6 | 44100 | 120.0s | - | **UNRESOLVED** | None | `e5143b23afba...` | DECODED_PCM16 |
| MUSIC_combatTactical02_XSA | `combatTactical02` | 6 | 44100 | 128.0s | - | **UNRESOLVED** | None | `d244c61cc520...` | DECODED_PCM16 |

### WEAPON_FIRE (29 assets)

| Package | Asset Name | Ch | Rate (Hz) | Duration | RMS | Confidence | SoundCue Identity | SHA-256 (Source Vorbis) | Status |
|---------|------------|---:|----------:|---------:|----:|-----------:|-------------------|--------------------------|--------|
| wp_SP_ARPistol_SF | `arPistolFire03` | 2 | 48000 | 1.25s | 0.2475 | **VERIFIED_REFERENCE** | mARPistolFire, arPistolFireNEW | `d3e22e6dc528...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `lateReflectionsPistolGlock01` | 4 | 24000 | 2.0s | 0.0410 | **VERIFIED_REFERENCE** | arPistolFireNEW | `ddf8abbb7376...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `pistolFireDistant05` | 1 | 24000 | 1.0s | 0.0617 | **VERIFIED_REFERENCE** | mPistolFireDistant, arPistolFireNEW | `414f54dfe7f8...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `lateReflectionsPistolGlock01` | 4 | 24000 | 2.0s | 0.0410 | **VERIFIED_REFERENCE** | arMachinegunFire | `ddf8abbb7376...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `machinegunFire57` | 2 | 48000 | 1.3876s | 0.1563 | **VERIFIED_REFERENCE** | mMachinegunFire, arMachinegunFire | `c7c8f8927296...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `machinegunFireDistant25` | 1 | 24000 | 1.0s | 0.0678 | **VERIFIED_REFERENCE** | arMachinegunFire, mMachinegunFireDistant | `d344e87d3f9a...` | DECODED_PCM16 |
| wp_SP_ARShotgun_SF | `lateReflectionsMGFamas01` | 4 | 24000 | 3.0s | 0.0333 | **VERIFIED_REFERENCE** | arShotgunFire | `24fde9e6fc1f...` | DECODED_PCM16 |
| wp_SP_ARShotgun_SF | `shotgunFire35` | 2 | 48000 | 1.5155s | 0.2476 | **UNRESOLVED** | None | `6bdfcc1908ee...` | DECODED_PCM16 |
| wp_SP_ARShotgun_SF | `shotgunFire36` | 2 | 48000 | 1.5156s | 0.2493 | **UNRESOLVED** | None | `d675801da4b7...` | DECODED_PCM16 |
| wp_SP_ARShotgun_SF | `shotgunFireDistant05` | 1 | 24000 | 1.0s | 0.0950 | **VERIFIED_REFERENCE** | mShotgunFireDistant, arShotgunFire | `ce555bfb7be9...` | DECODED_PCM16 |
| wp_SP_ARSniperRifle_SF | `lateReflectionsRifleFamas02` | 4 | 24000 | 4.0s | 0.0404 | **VERIFIED_REFERENCE** | arSniperFire | `fee85ef8bc89...` | DECODED_PCM16 |
| wp_SP_ARSniperRifle_SF | `shotgunFireDistant05` | 1 | 24000 | 1.0s | 0.0950 | **VERIFIED_REFERENCE** | arSniperFire, mShotgunFireDistant | `ce555bfb7be9...` | DECODED_PCM16 |
| wp_SP_ARSniperRifle_SF | `sniperFire08` | 2 | 48000 | 2.25s | 0.2587 | **VERIFIED_REFERENCE** | arSniperFire, mSniperFire | `9198b66d51c2...` | DECODED_PCM16 |
| wp_SP_ARGattlingGun_SF | `arGatlingGunFire02` | 2 | 48000 | 1.025s | 0.2306 | **VERIFIED_REFERENCE** | mGatlingGunFire, arGatlingGunFire | `50c8a7aae4cf...` | DECODED_PCM16 |
| wp_SP_ARGattlingGun_SF | `lateReflectionsMGFamas01` | 4 | 24000 | 3.0s | 0.0333 | **VERIFIED_REFERENCE** | arGatlingGunFire | `24fde9e6fc1f...` | DECODED_PCM16 |
| wp_SP_ARGattlingGun_SF | `pistolFireDistant05` | 1 | 24000 | 1.0s | 0.0617 | **VERIFIED_REFERENCE** | mPistolFireDistant, arGatlingGunFire | `414f54dfe7f8...` | DECODED_PCM16 |
| wp_SP_ARGrenadeLauncher_SF | `shotgunFireDistant05` | 1 | 24000 | 1.0s | 0.0950 | **VERIFIED_REFERENCE** | arGrenadeLauncherFire, mShotgunFireDistant | `ce555bfb7be9...` | DECODED_PCM16 |
| wp_SP_Railgun_SF | `arRailGunFire02` | 2 | 48000 | 2.5s | 0.2578 | **VERIFIED_REFERENCE** | mRailGunFire, arRailgunFire | `0d032452931b...` | DECODED_PCM16 |
| wp_SP_Railgun_SF | `lateReflectionsRifleFamas02` | 4 | 24000 | 4.0s | 0.0404 | **VERIFIED_REFERENCE** | arRailgunFire | `fee85ef8bc89...` | DECODED_PCM16 |
| wp_SP_Railgun_SF | `shotgunFireDistant05` | 1 | 24000 | 1.0s | 0.0950 | **VERIFIED_REFERENCE** | mShotgunFireDistant, arRailgunFire | `ce555bfb7be9...` | DECODED_PCM16 |
| wp_SP_RocketLauncher_SF | `projectileRocketFire02` | 2 | 48000 | 3.5s | 0.4479 | **HEURISTIC_MATCH** | projectileRocket | `f669152415a9...` | DECODED_PCM16 |
| wp_SP_RocketLauncher_SF | `lateReflectionsMGFamas01` | 4 | 24000 | 3.0s | 0.0333 | **VERIFIED_REFERENCE** | arRocketLauncherFire | `24fde9e6fc1f...` | DECODED_PCM16 |
| wp_SP_RocketLauncher_SF | `rocketLauncherFire07` | 2 | 48000 | 2.0s | 0.2327 | **VERIFIED_REFERENCE** | mRocketLauncherFire, arRocketLauncherFire | `ba52cddec319...` | DECODED_PCM16 |
| wp_SP_RocketLauncher_SF | `shotgunFireDistant05` | 1 | 24000 | 1.0s | 0.0950 | **VERIFIED_REFERENCE** | mShotgunFireDistant, arRocketLauncherFire | `ce555bfb7be9...` | DECODED_PCM16 |
| wp_SP_ARSteerable_SF | `shotgunFireDistant05` | 1 | 24000 | 1.0s | 0.0950 | **VERIFIED_REFERENCE** | mShotgunFireDistant, arSeekerFire | `ce555bfb7be9...` | DECODED_PCM16 |
| wp_SP_2007Pistol_SF | `lateReflectionsPistolGlock01` | 4 | 24000 | 2.0s | 0.0410 | **VERIFIED_REFERENCE** | arPistolFireOLD | `ddf8abbb7376...` | DECODED_PCM16 |
| wp_SP_2007Pistol_SF | `pistolFireDistant05` | 1 | 24000 | 1.0s | 0.0617 | **VERIFIED_REFERENCE** | mPistolFireDistant, arPistolFireOLD | `414f54dfe7f8...` | DECODED_PCM16 |
| RvGame | `pistolFire27` | 2 | 48000 | 1.5s | 0.2276 | **UNRESOLVED** | None | `250e18e24ab9...` | DECODED_PCM16 |
| RvGame | `pistolFire28` | 2 | 48000 | 1.5s | 0.2460 | **UNRESOLVED** | None | `22820a8a5f6e...` | DECODED_PCM16 |

### WEAPON_MECHANICAL (213 assets)

| Package | Asset Name | Ch | Rate (Hz) | Duration | RMS | Confidence | SoundCue Identity | SHA-256 (Source Vorbis) | Status |
|---------|------------|---:|----------:|---------:|----:|-----------:|-------------------|--------------------------|--------|
| wp_SP_ARPistol_SF | `bulletShellImpactBrassLarge01` | 1 | 44100 | 0.175s | 0.2336 | **UNRESOLVED** | None | `a08778ba4301...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `bulletShellImpactBrassLarge02` | 1 | 44100 | 0.2s | 0.2028 | **UNRESOLVED** | None | `710409d6adf1...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `bulletShellImpactBrassLarge03` | 1 | 44100 | 0.175s | 0.2051 | **UNRESOLVED** | None | `779bc803b3f1...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `bulletShellImpactBrassLarge04` | 1 | 44100 | 0.15s | 0.1635 | **UNRESOLVED** | None | `653ec66dc933...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `bulletShellImpactBrassSmall01` | 1 | 44100 | 0.15s | 0.0621 | **UNRESOLVED** | None | `ea02df107768...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `bulletShellImpactBrassSmall02` | 1 | 44100 | 0.15s | 0.0513 | **UNRESOLVED** | None | `8f5acf087023...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `bulletShellImpactBrassSmall03` | 1 | 44100 | 0.05s | 0.1578 | **UNRESOLVED** | None | `b8e86eab1bc5...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `bulletShellImpactBrassSmall04` | 1 | 44100 | 0.1s | 0.0611 | **UNRESOLVED** | None | `15aa3cd40114...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `bulletShellImpactBrassSmall05` | 1 | 44100 | 0.1s | 0.1143 | **UNRESOLVED** | None | `741e4c2d918a...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `bulletShellImpactBrassSmall06` | 1 | 44100 | 0.2s | 0.1672 | **UNRESOLVED** | None | `7971873599f9...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `bulletShellImpactBrassSmall07` | 1 | 44100 | 0.1s | 0.0704 | **UNRESOLVED** | None | `3ed01a1422c8...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `bulletShellImpactBrassSmall08` | 1 | 44100 | 0.2s | 0.1039 | **UNRESOLVED** | None | `301f69ea3b74...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `bulletShellImpactBrassStop01` | 1 | 44100 | 0.75s | 0.0546 | **UNRESOLVED** | None | `fcbe9ad6ad07...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `bulletShellImpactBrassStop02` | 1 | 44100 | 0.75s | 0.0630 | **UNRESOLVED** | None | `48e9bd91310c...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `bulletShellImpactBrassStop03` | 1 | 44100 | 0.75s | 0.0634 | **UNRESOLVED** | None | `8439fcde7c14...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `bulletShellImpactBrassStop04` | 1 | 44100 | 0.25s | 0.0784 | **UNRESOLVED** | None | `351210c5ef94...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `pistolReloadCartIn01` | 1 | 48000 | 0.33s | 0.2119 | **VERIFIED_REFERENCE** | pistolReloadCartIn | `a965c8c202d8...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `pistolReloadCartOut01` | 1 | 48000 | 0.33s | 0.1421 | **VERIFIED_REFERENCE** | pistolReloadCartOut | `2c729b645544...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `shotgunFireDry01` | 1 | 44100 | 0.25s | 0.0625 | **UNRESOLVED** | None | `4bd58a64f3dd...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `shotgunFireDry02` | 1 | 44100 | 0.25s | 0.0630 | **UNRESOLVED** | None | `f497719e4182...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `shotgunFireDry03` | 1 | 44100 | 0.25s | 0.0621 | **UNRESOLVED** | None | `7d2d46089e46...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `shotgunFireDry04` | 1 | 44100 | 0.25s | 0.0641 | **UNRESOLVED** | None | `dea99e83eea6...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `weaponFireLowAmmo01` | 1 | 48000 | 0.995s | 0.2034 | **UNRESOLVED** | None | `67984686da0c...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `weaponFireLowAmmo02` | 1 | 48000 | 0.995s | 0.1936 | **UNRESOLVED** | None | `834aac9ad41a...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `weaponFireLowAmmo03` | 1 | 48000 | 0.995s | 0.2086 | **UNRESOLVED** | None | `9dee9c9a04f9...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `weaponFireLowAmmo04` | 1 | 48000 | 0.995s | 0.1991 | **UNRESOLVED** | None | `923673c1aa79...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `weaponLightHolster01` | 1 | 48000 | 0.45s | 0.0494 | **HEURISTIC_MATCH** | weaponLightHolster | `640068dd2b63...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `weaponLightHolster02` | 1 | 48000 | 0.5s | 0.0590 | **HEURISTIC_MATCH** | weaponLightHolster | `7b1fcbac06c1...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `weaponLightHolster03` | 1 | 48000 | 0.498s | 0.0539 | **HEURISTIC_MATCH** | weaponLightHolster | `31464ef05e69...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassLarge01` | 1 | 44100 | 0.175s | 0.2336 | **UNRESOLVED** | None | `a08778ba4301...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassLarge02` | 1 | 44100 | 0.2s | 0.2028 | **UNRESOLVED** | None | `710409d6adf1...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassLarge03` | 1 | 44100 | 0.175s | 0.2051 | **UNRESOLVED** | None | `779bc803b3f1...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassLarge04` | 1 | 44100 | 0.15s | 0.1635 | **UNRESOLVED** | None | `653ec66dc933...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassSmall01` | 1 | 44100 | 0.15s | 0.0621 | **UNRESOLVED** | None | `ea02df107768...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassSmall02` | 1 | 44100 | 0.15s | 0.0513 | **UNRESOLVED** | None | `8f5acf087023...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassSmall03` | 1 | 44100 | 0.05s | 0.1578 | **UNRESOLVED** | None | `b8e86eab1bc5...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassSmall04` | 1 | 44100 | 0.1s | 0.0611 | **UNRESOLVED** | None | `15aa3cd40114...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassSmall05` | 1 | 44100 | 0.1s | 0.1143 | **UNRESOLVED** | None | `741e4c2d918a...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassSmall06` | 1 | 44100 | 0.2s | 0.1672 | **UNRESOLVED** | None | `7971873599f9...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassSmall07` | 1 | 44100 | 0.1s | 0.0704 | **UNRESOLVED** | None | `3ed01a1422c8...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassSmall08` | 1 | 44100 | 0.2s | 0.1039 | **UNRESOLVED** | None | `301f69ea3b74...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassStop01` | 1 | 44100 | 0.75s | 0.0546 | **UNRESOLVED** | None | `fcbe9ad6ad07...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassStop02` | 1 | 44100 | 0.75s | 0.0630 | **UNRESOLVED** | None | `48e9bd91310c...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassStop03` | 1 | 44100 | 0.75s | 0.0634 | **UNRESOLVED** | None | `8439fcde7c14...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `bulletShellImpactBrassStop04` | 1 | 44100 | 0.25s | 0.0784 | **UNRESOLVED** | None | `351210c5ef94...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `machinegunReloadCartIn01` | 1 | 48000 | 0.5s | 0.1883 | **VERIFIED_REFERENCE** | machinegunReloadCartIn | `1343e0e512ce...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `machinegunReloadCartOut01` | 1 | 48000 | 0.5s | 0.1200 | **VERIFIED_REFERENCE** | machinegunReloadCartOut | `106b9730dd10...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `shotgunFireDry01` | 1 | 44100 | 0.25s | 0.0625 | **UNRESOLVED** | None | `4bd58a64f3dd...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `shotgunFireDry02` | 1 | 44100 | 0.25s | 0.0630 | **UNRESOLVED** | None | `f497719e4182...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `shotgunFireDry03` | 1 | 44100 | 0.25s | 0.0621 | **UNRESOLVED** | None | `7d2d46089e46...` | DECODED_PCM16 |
| ... | *(163 additional WEAPON_MECHANICAL assets cataloged in JSON)* | | | | | | | | |

### E99_EFFECT (131 assets)

| Package | Asset Name | Ch | Rate (Hz) | Duration | RMS | Confidence | SoundCue Identity | SHA-256 (Source Vorbis) | Status |
|---------|------------|---:|----------:|---------:|----:|-----------:|-------------------|--------------------------|--------|
| wp_SP_ARPistol_SF | `tmdTubeClosePalm01` | 1 | 48000 | 0.5s | 0.0846 | **VERIFIED_REFERENCE** | tmdTubeCloseVial, tmdTubeClosePalm | `e66ca6b51fb8...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `tmdTubeCloseWrist01` | 1 | 48000 | 0.5s | 0.1638 | **VERIFIED_REFERENCE** | tmdTubeCloseWrist | `a1ac2834b746...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `tmdTubeOpenPalm01` | 1 | 48000 | 1.42s | 0.0758 | **VERIFIED_REFERENCE** | tmdTubeOpenPalm | `0210a3e85001...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `tmdTubeOpenVial01` | 1 | 48000 | 1.7455s | 0.1093 | **VERIFIED_REFERENCE** | tmdTubeCloseVial, tmdTubeOpenVial | `c49f10b55697...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `tmdTubeOpenWrist01` | 1 | 48000 | 0.6201s | 0.2290 | **VERIFIED_REFERENCE** | tmdTubeOpenWrist | `dfb003f7f41a...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `tmdTubeOut01` | 1 | 44100 | 0.5s | 0.1033 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `16904c6001df...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `tmdTubeOut02` | 1 | 44100 | 0.5s | 0.0877 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `070513e3b646...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `tmdTubeOut03` | 1 | 44100 | 0.5s | 0.0992 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `f05ca48eff9a...` | DECODED_PCM16 |
| wp_SP_ARPistol_SF | `tmdTubeOut04` | 1 | 44100 | 0.5s | 0.0997 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `70192c0604c5...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `tmdTubeClosePalm01` | 1 | 48000 | 0.5s | 0.0846 | **VERIFIED_REFERENCE** | tmdTubeCloseVial, tmdTubeClosePalm | `e66ca6b51fb8...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `tmdTubeCloseWrist01` | 1 | 48000 | 0.5s | 0.1638 | **VERIFIED_REFERENCE** | tmdTubeCloseWrist | `a1ac2834b746...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `tmdTubeOpenPalm01` | 1 | 48000 | 1.42s | 0.0758 | **VERIFIED_REFERENCE** | tmdTubeOpenPalm | `0210a3e85001...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `tmdTubeOpenVial01` | 1 | 48000 | 1.7455s | 0.1093 | **VERIFIED_REFERENCE** | tmdTubeCloseVial, tmdTubeOpenVial | `c49f10b55697...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `tmdTubeOpenWrist01` | 1 | 48000 | 0.6201s | 0.2290 | **VERIFIED_REFERENCE** | tmdTubeOpenWrist | `dfb003f7f41a...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `tmdTubeOut01` | 1 | 44100 | 0.5s | 0.1033 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `16904c6001df...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `tmdTubeOut02` | 1 | 44100 | 0.5s | 0.0877 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `070513e3b646...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `tmdTubeOut03` | 1 | 44100 | 0.5s | 0.0992 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `f05ca48eff9a...` | DECODED_PCM16 |
| wp_SP_ARMachineGun_SF | `tmdTubeOut04` | 1 | 44100 | 0.5s | 0.0997 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `70192c0604c5...` | DECODED_PCM16 |
| wp_SP_ARShotgun_SF | `tmdTubeClosePalm01` | 1 | 48000 | 0.5s | 0.0846 | **VERIFIED_REFERENCE** | tmdTubeCloseVial, tmdTubeClosePalm | `e66ca6b51fb8...` | DECODED_PCM16 |
| wp_SP_ARShotgun_SF | `tmdTubeCloseWrist01` | 1 | 48000 | 0.5s | 0.1638 | **VERIFIED_REFERENCE** | tmdTubeCloseWrist | `a1ac2834b746...` | DECODED_PCM16 |
| wp_SP_ARShotgun_SF | `tmdTubeOpenPalm01` | 1 | 48000 | 1.42s | 0.0758 | **VERIFIED_REFERENCE** | tmdTubeOpenPalm | `0210a3e85001...` | DECODED_PCM16 |
| wp_SP_ARShotgun_SF | `tmdTubeOpenVial01` | 1 | 48000 | 1.7455s | 0.1093 | **VERIFIED_REFERENCE** | tmdTubeCloseVial, tmdTubeOpenVial | `c49f10b55697...` | DECODED_PCM16 |
| wp_SP_ARShotgun_SF | `tmdTubeOpenWrist01` | 1 | 48000 | 0.6201s | 0.2290 | **VERIFIED_REFERENCE** | tmdTubeOpenWrist | `dfb003f7f41a...` | DECODED_PCM16 |
| wp_SP_ARShotgun_SF | `tmdTubeOut01` | 1 | 44100 | 0.5s | 0.1033 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `16904c6001df...` | DECODED_PCM16 |
| wp_SP_ARShotgun_SF | `tmdTubeOut02` | 1 | 44100 | 0.5s | 0.0877 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `070513e3b646...` | DECODED_PCM16 |
| wp_SP_ARShotgun_SF | `tmdTubeOut03` | 1 | 44100 | 0.5s | 0.0992 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `f05ca48eff9a...` | DECODED_PCM16 |
| wp_SP_ARShotgun_SF | `tmdTubeOut04` | 1 | 44100 | 0.5s | 0.0997 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `70192c0604c5...` | DECODED_PCM16 |
| wp_SP_ARSniperRifle_SF | `tmdTubeClosePalm01` | 1 | 48000 | 0.5s | 0.0846 | **VERIFIED_REFERENCE** | tmdTubeCloseVial, tmdTubeClosePalm | `e66ca6b51fb8...` | DECODED_PCM16 |
| wp_SP_ARSniperRifle_SF | `tmdTubeCloseWrist01` | 1 | 48000 | 0.5s | 0.1638 | **VERIFIED_REFERENCE** | tmdTubeCloseWrist | `a1ac2834b746...` | DECODED_PCM16 |
| wp_SP_ARSniperRifle_SF | `tmdTubeOpenPalm01` | 1 | 48000 | 1.42s | 0.0758 | **VERIFIED_REFERENCE** | tmdTubeOpenPalm | `0210a3e85001...` | DECODED_PCM16 |
| wp_SP_ARSniperRifle_SF | `tmdTubeOpenVial01` | 1 | 48000 | 1.7455s | 0.1093 | **VERIFIED_REFERENCE** | tmdTubeCloseVial, tmdTubeOpenVial | `c49f10b55697...` | DECODED_PCM16 |
| wp_SP_ARSniperRifle_SF | `tmdTubeOpenWrist01` | 1 | 48000 | 0.6201s | 0.2290 | **VERIFIED_REFERENCE** | tmdTubeOpenWrist | `dfb003f7f41a...` | DECODED_PCM16 |
| wp_SP_ARSniperRifle_SF | `tmdTubeOut01` | 1 | 44100 | 0.5s | 0.1033 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `16904c6001df...` | DECODED_PCM16 |
| wp_SP_ARSniperRifle_SF | `tmdTubeOut02` | 1 | 44100 | 0.5s | 0.0877 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `070513e3b646...` | DECODED_PCM16 |
| wp_SP_ARSniperRifle_SF | `tmdTubeOut03` | 1 | 44100 | 0.5s | 0.0992 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `f05ca48eff9a...` | DECODED_PCM16 |
| wp_SP_ARSniperRifle_SF | `tmdTubeOut04` | 1 | 44100 | 0.5s | 0.0997 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `70192c0604c5...` | DECODED_PCM16 |
| wp_SP_ARGattlingGun_SF | `tmdTubeClosePalm01` | 1 | 48000 | 0.5s | 0.0846 | **VERIFIED_REFERENCE** | tmdTubeCloseVial, tmdTubeClosePalm | `e66ca6b51fb8...` | DECODED_PCM16 |
| wp_SP_ARGattlingGun_SF | `tmdTubeCloseWrist01` | 1 | 48000 | 0.5s | 0.1638 | **VERIFIED_REFERENCE** | tmdTubeCloseWrist | `a1ac2834b746...` | DECODED_PCM16 |
| wp_SP_ARGattlingGun_SF | `tmdTubeOpenPalm01` | 1 | 48000 | 1.42s | 0.0758 | **VERIFIED_REFERENCE** | tmdTubeOpenPalm | `0210a3e85001...` | DECODED_PCM16 |
| wp_SP_ARGattlingGun_SF | `tmdTubeOpenVial01` | 1 | 48000 | 1.7455s | 0.1093 | **VERIFIED_REFERENCE** | tmdTubeCloseVial, tmdTubeOpenVial | `c49f10b55697...` | DECODED_PCM16 |
| wp_SP_ARGattlingGun_SF | `tmdTubeOpenWrist01` | 1 | 48000 | 0.6201s | 0.2290 | **VERIFIED_REFERENCE** | tmdTubeOpenWrist | `dfb003f7f41a...` | DECODED_PCM16 |
| wp_SP_ARGattlingGun_SF | `tmdTubeOut01` | 1 | 44100 | 0.5s | 0.1033 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `16904c6001df...` | DECODED_PCM16 |
| wp_SP_ARGattlingGun_SF | `tmdTubeOut02` | 1 | 44100 | 0.5s | 0.0877 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `070513e3b646...` | DECODED_PCM16 |
| wp_SP_ARGattlingGun_SF | `tmdTubeOut03` | 1 | 44100 | 0.5s | 0.0992 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `f05ca48eff9a...` | DECODED_PCM16 |
| wp_SP_ARGattlingGun_SF | `tmdTubeOut04` | 1 | 44100 | 0.5s | 0.0997 | **VERIFIED_REFERENCE** | mTMDTubeOut, tmdTubeOn | `70192c0604c5...` | DECODED_PCM16 |
| wp_SP_ARGrenadeLauncher_SF | `tmdTubeClosePalm01` | 1 | 48000 | 0.5s | 0.0846 | **VERIFIED_REFERENCE** | tmdTubeCloseVial, tmdTubeClosePalm | `e66ca6b51fb8...` | DECODED_PCM16 |
| wp_SP_ARGrenadeLauncher_SF | `tmdTubeCloseWrist01` | 1 | 48000 | 0.5s | 0.1638 | **VERIFIED_REFERENCE** | tmdTubeCloseWrist | `a1ac2834b746...` | DECODED_PCM16 |
| wp_SP_ARGrenadeLauncher_SF | `tmdTubeOpenPalm01` | 1 | 48000 | 1.42s | 0.0758 | **VERIFIED_REFERENCE** | tmdTubeOpenPalm | `0210a3e85001...` | DECODED_PCM16 |
| wp_SP_ARGrenadeLauncher_SF | `tmdTubeOpenVial01` | 1 | 48000 | 1.7455s | 0.1093 | **VERIFIED_REFERENCE** | tmdTubeCloseVial, tmdTubeOpenVial | `c49f10b55697...` | DECODED_PCM16 |
| wp_SP_ARGrenadeLauncher_SF | `tmdTubeOpenWrist01` | 1 | 48000 | 0.6201s | 0.2290 | **VERIFIED_REFERENCE** | tmdTubeOpenWrist | `dfb003f7f41a...` | DECODED_PCM16 |
| ... | *(81 additional E99_EFFECT assets cataloged in JSON)* | | | | | | | | |

### EXPLOSION (32 assets)

| Package | Asset Name | Ch | Rate (Hz) | Duration | RMS | Confidence | SoundCue Identity | SHA-256 (Source Vorbis) | Status |
|---------|------------|---:|----------:|---------:|----:|-----------:|-------------------|--------------------------|--------|
| wp_SP_ARGrenadeLauncher_SF | `explodeGrenade17` | 2 | 48000 | 2.5s | 0.2413 | **HEURISTIC_MATCH** | explodeGrenade | `ae1d1441717a...` | DECODED_PCM16 |
| wp_SP_ARGrenadeLauncher_SF | `explodeGrenade18` | 2 | 48000 | 2.5s | 0.2343 | **HEURISTIC_MATCH** | explodeGrenade | `aa69d44361bf...` | DECODED_PCM16 |
| wp_SP_ARGrenadeLauncher_SF | `explodeGrenade19` | 2 | 48000 | 2.5s | 0.2437 | **HEURISTIC_MATCH** | explodeGrenade | `fcebc3b8e821...` | DECODED_PCM16 |
| wp_SP_ARGrenadeLauncher_SF | `explodeGrenade20` | 2 | 48000 | 2.5s | 0.2318 | **HEURISTIC_MATCH** | explodeGrenade | `778ed7bb8c53...` | DECODED_PCM16 |
| wp_SP_ARGrenadeLauncher_SF | `explodeGrenadeDistant05` | 1 | 22050 | 2.0s | 0.1384 | **HEURISTIC_MATCH** | explodeGrenade | `ab64828c8424...` | DECODED_PCM16 |
| wp_SP_ARGrenadeLauncher_SF | `explodeGrenadeDistant06` | 1 | 22050 | 2.0s | 0.1303 | **HEURISTIC_MATCH** | explodeGrenade | `22a9d3df2996...` | DECODED_PCM16 |
| wp_SP_ARGrenadeLauncher_SF | `explodeGrenadeDistant07` | 1 | 22050 | 2.0s | 0.1344 | **HEURISTIC_MATCH** | explodeGrenade | `80bbf57f4a91...` | DECODED_PCM16 |
| wp_SP_ARGrenadeLauncher_SF | `explodeGrenadeDistant08` | 1 | 22050 | 2.0s | 0.1135 | **HEURISTIC_MATCH** | explodeGrenade | `119a1ed02c6d...` | DECODED_PCM16 |
| wp_SP_RocketLauncher_SF | `explodeGrenade17` | 2 | 48000 | 2.5s | 0.2413 | **HEURISTIC_MATCH** | explodeGrenade | `ae1d1441717a...` | DECODED_PCM16 |
| wp_SP_RocketLauncher_SF | `explodeGrenade18` | 2 | 48000 | 2.5s | 0.2343 | **HEURISTIC_MATCH** | explodeGrenade | `aa69d44361bf...` | DECODED_PCM16 |
| wp_SP_RocketLauncher_SF | `explodeGrenade19` | 2 | 48000 | 2.5s | 0.2437 | **HEURISTIC_MATCH** | explodeGrenade | `fcebc3b8e821...` | DECODED_PCM16 |
| wp_SP_RocketLauncher_SF | `explodeGrenade20` | 2 | 48000 | 2.5s | 0.2318 | **HEURISTIC_MATCH** | explodeGrenade | `778ed7bb8c53...` | DECODED_PCM16 |
| wp_SP_RocketLauncher_SF | `explodeGrenadeDistant05` | 1 | 22050 | 2.0s | 0.1384 | **HEURISTIC_MATCH** | explodeGrenade | `ab64828c8424...` | DECODED_PCM16 |
| wp_SP_RocketLauncher_SF | `explodeGrenadeDistant06` | 1 | 22050 | 2.0s | 0.1303 | **HEURISTIC_MATCH** | explodeGrenade | `22a9d3df2996...` | DECODED_PCM16 |
| wp_SP_RocketLauncher_SF | `explodeGrenadeDistant07` | 1 | 22050 | 2.0s | 0.1344 | **HEURISTIC_MATCH** | explodeGrenade | `80bbf57f4a91...` | DECODED_PCM16 |
| wp_SP_RocketLauncher_SF | `explodeGrenadeDistant08` | 1 | 22050 | 2.0s | 0.1135 | **HEURISTIC_MATCH** | explodeGrenade | `119a1ed02c6d...` | DECODED_PCM16 |
| wp_SP_ARSteerable_SF | `explodeGrenadeDistant05` | 1 | 22050 | 2.0s | 0.1384 | **UNRESOLVED** | None | `ab64828c8424...` | DECODED_PCM16 |
| wp_SP_ARSteerable_SF | `explodeGrenadeDistant06` | 1 | 22050 | 2.0s | 0.1303 | **UNRESOLVED** | None | `22a9d3df2996...` | DECODED_PCM16 |
| wp_SP_ARSteerable_SF | `explodeGrenadeDistant07` | 1 | 22050 | 2.0s | 0.1344 | **UNRESOLVED** | None | `80bbf57f4a91...` | DECODED_PCM16 |
| wp_SP_ARSteerable_SF | `explodeGrenadeDistant08` | 1 | 22050 | 2.0s | 0.1135 | **UNRESOLVED** | None | `119a1ed02c6d...` | DECODED_PCM16 |
| RvGame | `explodeLargeDistant01` | 1 | 22050 | 5.0s | 0.1250 | **UNRESOLVED** | None | `1a1b79039285...` | DECODED_PCM16 |
| RvGame | `explodeLargeDistant02` | 1 | 22050 | 5.0s | 0.1198 | **UNRESOLVED** | None | `3dcffcc16ba9...` | DECODED_PCM16 |
| RvGame | `explodeLargeDistant03` | 1 | 22050 | 5.0s | 0.1402 | **UNRESOLVED** | None | `deb043b0c66d...` | DECODED_PCM16 |
| RvGame | `explodeLargeDistant04` | 1 | 22050 | 5.0s | 0.1572 | **UNRESOLVED** | None | `8d0a017c1e96...` | DECODED_PCM16 |
| RvGame | `explodeProjectile01` | 2 | 48000 | 3.0s | 0.2724 | **UNRESOLVED** | None | `ba44ccf41fda...` | DECODED_PCM16 |
| RvGame | `explodeProjectile02` | 2 | 48000 | 3.0s | 0.2551 | **UNRESOLVED** | None | `009e0c1b8df7...` | DECODED_PCM16 |
| RvGame | `explodeProjectile03` | 2 | 48000 | 3.0s | 0.2649 | **UNRESOLVED** | None | `eb861ac069a5...` | DECODED_PCM16 |
| RvGame | `explodeProjectile04` | 2 | 48000 | 3.0s | 0.2725 | **UNRESOLVED** | None | `d24ec8cd9720...` | DECODED_PCM16 |
| SP_SL1_audio01_XSA | `fireExplosion01` | 1 | 44100 | 3.168s | 0.1871 | **UNRESOLVED** | None | `5cdb0c6adfee...` | DECODED_PCM16 |
| SP_SL1_audio01_XSA | `fireExplosion02` | 1 | 44100 | 2.126s | 0.2157 | **UNRESOLVED** | None | `3e5a240b29b0...` | DECODED_PCM16 |
| SP_SL1_audio01_XSA | `fireExplosion03` | 1 | 44100 | 1.9307s | 0.1359 | **UNRESOLVED** | None | `6bbe24b9b6d7...` | DECODED_PCM16 |
| SP_SL1_audio01_XSA | `fireExplosion04` | 1 | 44100 | 1.5308s | 0.2134 | **UNRESOLVED** | None | `b14b886dd996...` | DECODED_PCM16 |

### AMBIENCE (209 assets)

| Package | Asset Name | Ch | Rate (Hz) | Duration | RMS | Confidence | SoundCue Identity | SHA-256 (Source Vorbis) | Status |
|---------|------------|---:|----------:|---------:|----:|-----------:|-------------------|--------------------------|--------|
| RvGame | `waterSplashEnter01` | 1 | 44100 | 2.0101s | 0.1326 | **HEURISTIC_MATCH** | waterSplashEnter | `ce385666222b...` | DECODED_PCM16 |
| RvGame | `waterSplashEnter02` | 1 | 44100 | 1.55s | 0.1557 | **HEURISTIC_MATCH** | waterSplashEnter | `c2cffc45281f...` | DECODED_PCM16 |
| RvGame | `waterSplashEnter03` | 1 | 44100 | 1.0333s | 0.1518 | **HEURISTIC_MATCH** | waterSplashEnter | `822347368321...` | DECODED_PCM16 |
| RvGame | `waterSplashExit01` | 1 | 44100 | 1.8502s | 0.1315 | **HEURISTIC_MATCH** | waterSplashExit | `ff3420113908...` | DECODED_PCM16 |
| RvGame | `waterSplashExit02` | 1 | 44100 | 1.7667s | 0.1318 | **HEURISTIC_MATCH** | waterSplashExit | `c953ec4c775d...` | DECODED_PCM16 |
| RvGame | `waterSplashExit03` | 1 | 44100 | 1.5267s | 0.1561 | **HEURISTIC_MATCH** | waterSplashExit | `08deb95c46ed...` | DECODED_PCM16 |
| RvGame | `waterSplashHuge01` | 1 | 44100 | 1.8663s | 0.2384 | **VERIFIED_REFERENCE** | mWaterSplashHuge, waterSplashHuge | `5e282a418674...` | DECODED_PCM16 |
| RvGame | `waterUnderEnterQuad` | 4 | 24000 | 2.5s | 0.0959 | **VERIFIED_REFERENCE** | waterUnderwater | `e5abc2b76fd6...` | DECODED_PCM16 |
| RvGame | `waterUnderwaterQuad` | 4 | 24000 | 7.5s | 0.1284 | **HEURISTIC_MATCH** | waterUnderwater | `6942e265e51b...` | DECODED_PCM16 |
| RvGame | `waterWade01` | 1 | 44100 | 2.0s | 0.0549 | **HEURISTIC_MATCH** | waterWade | `19a409f28849...` | DECODED_PCM16 |
| RvGame | `waterWade02` | 1 | 44100 | 2.0s | 0.0611 | **HEURISTIC_MATCH** | waterWade | `fab71c862f2c...` | DECODED_PCM16 |
| RvGame | `waterWade03` | 1 | 44100 | 2.0s | 0.0611 | **HEURISTIC_MATCH** | waterWade | `ccdbcae49ca4...` | DECODED_PCM16 |
| RvGame | `waterWade04` | 1 | 44100 | 2.0s | 0.0702 | **HEURISTIC_MATCH** | waterWade | `1c73eb7d06f7...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `droneTurbine_L` | 1 | 22050 | 45.0s | 0.1683 | **UNRESOLVED** | None | `c88759913b90...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `droneTurbine_R` | 1 | 22050 | 45.0s | 0.1638 | **UNRESOLVED** | None | `fdcb9c48a322...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `thunderRumbleHuge01_L` | 1 | 16000 | 93.0s | 0.1087 | **UNRESOLVED** | None | `6ef27da3e317...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `thunderRumbleLight02_L` | 1 | 16000 | 50.0s | 0.0908 | **UNRESOLVED** | None | `d3e292af2042...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `thunderRumbleLight02_R` | 1 | 16000 | 50.0s | 0.0779 | **UNRESOLVED** | None | `c07b6908eca9...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `waterUnderwaterSub01_R` | 1 | 11025 | 6.1273s | 0.1528 | **UNRESOLVED** | None | `8d0188549877...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `waterWavesHeavyLake01_L` | 1 | 44100 | 7.9063s | 0.1332 | **UNRESOLVED** | None | `b7bb164671d6...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `waterWavesHeavyLake01_R` | 1 | 44100 | 7.0827s | 0.1334 | **UNRESOLVED** | None | `480e37baa337...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `waterWavesOcean01_L` | 1 | 44100 | 7.6823s | 0.1227 | **UNRESOLVED** | None | `7af8ca72134f...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `windAirToneQuad01` | 4 | 48000 | 5.0s | 0.0956 | **UNRESOLVED** | None | `6e753096bfc9...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `windStormHigh01_R` | 1 | 22050 | 60.0s | 0.1531 | **UNRESOLVED** | None | `ccc8d5343500...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `windStormLow01_R` | 1 | 22050 | 30.0s | 0.1716 | **UNRESOLVED** | None | `997e51c9c8f4...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `windStormSea01_R` | 1 | 44100 | 21.6226s | 0.1469 | **UNRESOLVED** | None | `3ed6c940d323...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `metalCrush01` | 1 | 16000 | 10.0s | 0.0805 | **UNRESOLVED** | None | `e3bc7c2b7fd8...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `metalCrush02` | 1 | 16000 | 10.0s | 0.0733 | **UNRESOLVED** | None | `64770f60466b...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `metalCrush03` | 1 | 16000 | 10.0s | 0.0789 | **UNRESOLVED** | None | `b625700213e8...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `metalCrush04` | 1 | 16000 | 10.0s | 0.0825 | **UNRESOLVED** | None | `63a5a684a6d5...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `metalCrush05` | 1 | 16000 | 10.0s | 0.0775 | **UNRESOLVED** | None | `ff007edc7fc7...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `metalCrush06` | 1 | 16000 | 10.0s | 0.0925 | **UNRESOLVED** | None | `373b6395cd76...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `metalCrush07` | 1 | 16000 | 15.0s | 0.1033 | **UNRESOLVED** | None | `39affddb7bf1...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `metalCrush08` | 1 | 16000 | 15.0s | 0.1035 | **UNRESOLVED** | None | `38f6b1204552...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `metalCrush09` | 1 | 16000 | 15.0s | 0.1039 | **UNRESOLVED** | None | `e1e8ad8b4c88...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `metalCrush10` | 1 | 16000 | 10.0s | 0.0712 | **UNRESOLVED** | None | `7f9a7540ed12...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `metalCrush11` | 1 | 16000 | 10.0s | 0.1018 | **UNRESOLVED** | None | `25023020d3a0...` | DECODED_PCM16 |
| SP_Freight_Audio_2010_XSA | `metalCrush12` | 1 | 16000 | 10.0s | 0.1055 | **UNRESOLVED** | None | `b51116d3ec10...` | DECODED_PCM16 |
| SP_RL1_audio01_XSA | `chainCreak01` | 1 | 44100 | 17.5s | 0.0357 | **UNRESOLVED** | None | `57ab1186f42d...` | DECODED_PCM16 |
| SP_RL1_audio01_XSA | `chainCreak02` | 1 | 44100 | 18.0s | 0.0278 | **UNRESOLVED** | None | `2dffafbd26e9...` | DECODED_PCM16 |
| SP_RL1_audio01_XSA | `fliesCouple01` | 1 | 44100 | 2.0395s | 0.0799 | **UNRESOLVED** | None | `f1824ae4da96...` | DECODED_PCM16 |
| SP_RL1_audio01_XSA | `fliesCouple02` | 1 | 44100 | 1.9614s | 0.0613 | **UNRESOLVED** | None | `0496af5730f6...` | DECODED_PCM16 |
| SP_RL1_audio01_XSA | `fliesCouple03` | 1 | 44100 | 1.9992s | 0.1494 | **UNRESOLVED** | None | `e0118d51d763...` | DECODED_PCM16 |
| SP_RL1_audio01_XSA | `fliesCouple04` | 1 | 44100 | 2.0009s | 0.1090 | **UNRESOLVED** | None | `b37c38876d08...` | DECODED_PCM16 |
| SP_RL1_audio01_XSA | `humElectricFizzle04` | 1 | 48000 | 2.0s | 0.1755 | **UNRESOLVED** | None | `be3b24feffec...` | DECODED_PCM16 |
| SP_RL1_audio01_XSA | `humLightFluorescentBright05` | 1 | 48000 | 0.1833s | 0.1794 | **UNRESOLVED** | None | `126e4bf5e771...` | DECODED_PCM16 |
| SP_RL1_audio01_XSA | `humLightHigh03` | 1 | 48000 | 0.996s | 0.1795 | **UNRESOLVED** | None | `ee0cc9883f6a...` | DECODED_PCM16 |
| SP_RL1_audio01_XSA | `humLightRatty05` | 1 | 48000 | 1.2497s | 0.1760 | **UNRESOLVED** | None | `38b9cb957129...` | DECODED_PCM16 |
| SP_RL1_audio01_XSA | `metalRattleContinuous01` | 1 | 44100 | 15.9991s | 0.0659 | **UNRESOLVED** | None | `3c6bfb74122a...` | DECODED_PCM16 |
| SP_RL1_audio01_XSA | `metalRattleContinuous02` | 1 | 44100 | 9.9667s | 0.0716 | **UNRESOLVED** | None | `7affd10ea99d...` | DECODED_PCM16 |
| ... | *(159 additional AMBIENCE assets cataloged in JSON)* | | | | | | | | |

$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../integration/session-asset-scope.ps1')
$allowed=@(
 'Binaries/vr-assets/reload/centurion/centurion_hd.svct',
 'Binaries/vr-assets/reload/kasimov/kasimov_hd.svks',
 'Binaries/vr-assets/reload/kasimov/kasimov_reload.svwa',
 'Binaries/vr-assets/reload/kasimov/kasimov_clip.svrk',
 'Binaries/vr-assets/reload/kasimov/kasimov_clip.svrt',
 'Binaries/vr-assets/reload/valkyrie/valkyrie_hd.svvk',
 'Binaries/vr-assets/reload/valkyrie/valkyrie_reload.svwa',
 'Binaries/vr-assets/reload/valkyrie/valkyrie_clip.svrk',
 'Binaries/vr-assets/reload/valkyrie/valkyrie_clip.svrt',
 'Binaries/vr-assets/reload/volk/volk_hd.svvh',
 'Binaries/vr-assets/reload/volk/volk_reload.svwa',
 'Binaries/vr-assets/reload/volk/volk_clip.svrk',
 'Binaries/vr-assets/reload/volk/volk_clip.svrt',
 'RvGame/CookedPC/UI_SinglePlayer_SF.xxx','RvGame/CookedPC/wp_SP_ARPistol_SF.xxx',
 'RvGame/CookedPC/wp_SP_ARSniperRifle_SF.xxx',
 'RvGame/CookedPC/wp_SP_ARMachineGun_SF.xxx','RvGame/CookedPC/wp_SP_ARShotgun_SF.xxx',
 'RvGame/CookedPC/MUSIC_exploreDrone01_XSA.xxx','RvGame/CookedPC/MUSIC_combatBossGrom01_XSA.xxx',
 'RvGame/CookedPC/SP_Freight_Audio_2010_XSA.xxx',
 'Binaries/texture-override-remaster/AR_E99Rifle_alt_d.dds',
 'Binaries/texture-override-remaster/AR_E99Rifle_n.dds',
 'Binaries/texture-override-remaster/AR_shotgun_d.dds',
 'Binaries/texture-override-remaster/AR_shotgun_n.dds',
 'Binaries/texture-override-remaster/inview_glove_d.dds',
 'Binaries/texture-override-remaster/inview_glove_n.dds',
 'Binaries/texture-override-remaster/handgun_d.dds',
 'Binaries/texture-override-remaster/handgun_n.dds',
 'Binaries/texture-override-remaster/ar_sniper_rifle_d.dds',
 'Binaries/texture-override-remaster/ar_sniper_rifle_n.dds',
 'Binaries/vr-assets/ui/remaster/wheel_neutral.png',
 'Binaries/vr-assets/audio/polish/story/a.wav','RvGame/CookedPC/SP_VI2_LOC_INT.xxx'
)
foreach($p in $allowed) {
 if(-not (Test-SessionAssetPath $p)){throw "Expected allowed: $p"}
 foreach($bad in @('../'+$p,$p+'/../x',$p+':stream','C:/'+$p,$p+'.exe')){
  if(Test-SessionAssetPath $bad){throw "Path escape admitted: $bad"}
 }
}
foreach($bad in @('Binaries/d3d9.dll','Binaries/dinput8.dll','Binaries/texture_override.ini',
 'Binaries/vr-assets/reload/volk/foreign.svvh','Binaries/vr-assets/reload/valkyrie/volk_hd.svvh',
 'Binaries/SingularityVR.ini','RvGame/CookedPC/Engine.xxx','RvGame/CookedPC/wp_MP_ARPistol_SF.xxx',
 'RvGame/CookedPC/MUSIC_combatBossGrom01.xxx','RvGame/CookedPC/ui_GFx_fonts.xxx',
 'Binaries/texture-override-remaster/unknown.dds','RvGame\CookedPC\RvGame.xxx',
 'RvGame/CookedPC/RvGame.xxx')){
 if(Test-SessionAssetPath $bad){throw "Unowned target admitted: $bad"}
}
Write-Output ('PASS: {0} owned paths and escape variants; foreign caches, DLL/config and native core excluded; no writes' -f $allowed.Count)

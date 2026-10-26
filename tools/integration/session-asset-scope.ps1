# Exact release asset ownership only; no DLL/config, arbitrary packages or MP.
function Test-SessionAssetPath([string]$Relative) {
    if($Relative -cmatch '^(Binaries/vr-assets/audio/polish/story/[A-Za-z0-9_.-]+\.wav|Binaries/vr-assets/localization/pl/polish_(text|story_voice)\.jsonl|Binaries/texture-override-remaster/revert_[dn]2\.dds|RvGame/CookedPC/[sS][pP]_[A-Za-z0-9_]+_LOC_INT\.xxx|Binaries/vr-assets/ui/remaster/[A-Za-z0-9_.-]+\.png|Binaries/vr-assets/hud/diegetic/[A-Za-z0-9_.-]+\.png)$') { return $true }
    $nativeBanks=@(
        'UI_SinglePlayer_SF.xxx',
        # RvGame.xxx is deliberately excluded. All current modified versions
        # fail native startup; the full candidate boots with the accepted core.
        # Keep that exact core as a protected dependency, never an asset delta.
        'wp_SP_ARPistol_SF.xxx','wp_SP_ARSniperRifle_SF.xxx',
        'wp_SP_ARMachineGun_SF.xxx','wp_SP_ARShotgun_SF.xxx',
        'MUSIC_exploreDrone01_XSA.xxx','MUSIC_combatBossGrom01_XSA.xxx',
        'SP_Freight_Audio_2010_XSA.xxx'
    )
    foreach($name in $nativeBanks) {
        if($Relative -ceq ('RvGame/CookedPC/'+$name)) { return $true }
    }
    $materials=@(
        'AR_E99Rifle_alt_d.dds','AR_E99Rifle_n.dds',
        'AR_shotgun_d.dds','AR_shotgun_n.dds',
        'inview_glove_d.dds','inview_glove_n.dds',
        'handgun_d.dds','handgun_n.dds',
        'ar_sniper_rifle_d.dds','ar_sniper_rifle_n.dds'
    )
    foreach($name in $materials) {
        if($Relative -ceq ('Binaries/texture-override-remaster/'+$name)) { return $true }
    }
    return $false
}

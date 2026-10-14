# First-area acceptance and next producer witnesses

2026-10-14, Steam PID37660, source34b180e, DLL2763A984…F48C0B7.
User PSVR2 result: stationary cinema GOOD, right hand/pistol grip GOOD,
first-area Polish NPC/recordings GOOD, Polish note glyphs GOOD. Small FPS dips
acceptable; final optimization deliberately deferred. This is scoped L4,
not full-game audio/font/weapon/menu acceptance. Logo and cursor replacement
remain BROKEN: user sees the arrow and not SINGULARITY VR. Slight laser/cursor
offset also reported. Do not promote the offline text/shape patch as fixed.

Cleanup: game closed;1927/1928 original identities restored independently.
INI contained only user changes ShowControllerArms0→1 and UiNarratorEnabled1→0.
Exact three-way cleanup preserved those choices and removed staged switches.
INI now202AA13A74BF1FF49104C8C507A1B9A6A6069D22DC43A529FCF6BFFC07430DBF.
Original user-after-test INI saved in the live ini-reconciled directory.
All other core/assets/WAV originals restored; no test left installed.

## Evidence, not another guessed patch

- The private last-run session log proves a LOCAL/BOTH cinema quad at3.127×1.594m,
  same composed1579×805 pixels, plus focused SteamVR rendering. Physical result
  establishes stability, not the log alone.
- Native Mouse export is attached by com.raven.MouseCursor in iis/SPMenus.
  Source patch changes its exclusive leaf, yet physical result is an arrow.
  Resolve actual LIVE MovieInfo.RawData before changing another SWF/register.
- Offline inspection independently finds ui_logos.logo Texture2D512×128 DXT5
  in ui_logos_SF.xxx. Decoded native mip displays the original SINGULARITY logo.
  This corrects the earlier inventory's claim that no logo texture exists.
  Its contribution to the displayed menu is still UNKNOWN; dimensions are not
  identity, and a texture existing is not proof that the runtime uses it.
- Native cursor coordinates are dispatched on input thread; overlay hit/laser
  uses the render frame. Possible stale-frame/viewport-to-stage mismatch is a
  hypothesis, NOT permission to add an arbitrary compensating offset.
- wp_SP_ARPistol_SF contains original cues arPistolFireNEW/mARPistolFire,
  wave arPistolFire03 and reload/shell sounds. Shared_ARPistol points to
  mWeaponFX and FX_handgunShellsEject; the FX object points to
  FX_MuzzleLockedFX_MuzzleLocked for first-person/ironsights. Cue/component
  inventory is L1 only; it does not prove playback or correct VR placement.
- Existing plugin ABI has only visual draw/events; no custom shot audio path
  was found in current MAIN source. Last-run HideMuzzleFx0 already allows native
  particles, while custom screen flash is ON. Do not mute/replay native sounds
  via WinMM or globally suppress particles to make the evidence look cleaner.

Offline fake lease of the witness candidate passed on 2026-10-14. The candidate
DLL was not copied into Steam. Installed hashes stayed at the accepted baseline
above. The witness remains default-OFF until a later leased session.

Source-only string locations, not a consumer proof: the private candidate changes
the iis HTML run that uses WFontLogo from SINGULARITY to SINGULARITY VR. SPMenus
still contains a styled SINGULARITY run in both the original and the candidate.
The candidate also sets a SPMenus MajorTitle text to SINGULARITY VR. The separate
texture ui_logos.logo is one DXT5 in a package of weapon HUD images. The SPMenus
token UI_Logos. occurs beside getSubstitutions and SCOPE_MP_PLAYER_MSG, so that
token is not evidence that the texture is the title the user sees. Compare a live
ui-asset witness FNV with these source fingerprints before editing either movie
or that texture: control iis 669B2A3F185327C8 (30959), control SPMenus
51B6A92D4DED91D0 (231953), candidate iis 2470EB3DBD9A78BB (31178), candidate
SPMenus 56AFE359868D7429 (232282).

Default-OFF Diagnostic.MenuAssetWitness uses existing input hooks, no new detour.
Eight cold focus asset fingerprints(max2MiB/array),24 coordinate samples at500ms,
six weapon identities. No renderer/audio/COM/state mutation. Compare native
RawData fingerprints with offline control/candidate before deciding which asset
owns logo/arrow. A matching fingerprint alone is NOT pixel validation. Probe
native weapon template references before replacing a native FX path.

## New user requests

- First building's projector picture appears at an incorrect angle/intermittently
  enters an eye. Producer/texture/pass UNKNOWN. No blind movie-wide stereo patch.
- Authentic weapon sound AND flash/smoke/shells requested. First prove the native
  cue/emitter consumers, preserve accepted tracking and shadows, then one gun.
- HUD SIDEcd76bac reviewed: design B(hand/weapon plates), Stage1 must first
  census individual native health/ammo clusters; no mCounts index guessing.
  Prompt-token replacement is separate and must follow current Sense bindings,
  replace native token/glyph in-place and preserve Polish sentence text.
- SIDE may prepare source SVG plates/icon atlas and optional lightweight frames.
  No renderer/Steam changes, no fictional ammo producer or fixed standing height.

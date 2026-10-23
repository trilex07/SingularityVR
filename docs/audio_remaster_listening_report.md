# Singularity VR — Audio Remaster & Restoration Research Report (Complete 8-Target Suite)

**Author:** Senior Game Audio Engineer & Sound Mastering Specialist  
**Branch:** `side/audio-music-sfx-remaster`  
**Target Game:** Singularity (2010), Raven Software, Unreal Engine 3 (v584, PC x86)  
**Status:** `READY FOR LISTENING REVIEW`  

## Executive Summary

Following user listening approval of the three pilot comparison sets (Centurion, Kasimov, TMD Impulse), production was expanded to complete the remaining five comparison sets, delivering the full **eight-target remaster suite**:

1. **Centurion Revolver** (`wp_SP_ARPistol_SF_arPistolFire03.wav`) — *Approved Pilot (Unchanged)*
2. **Kasimov Sniper Rifle** (`wp_SP_ARSniperRifle_SF_sniperFire08.wav`) — *Approved Pilot (Unchanged)*
3. **TMD Energy Impulse Blast** (`RvGame_tmdFireImpulse16.wav`) — *Approved Pilot (Unchanged)*
4. **AR9 Walkiria Assault Rifle** (`wp_SP_ARMachineGun_SF_machinegunFire57.wav`) — *New Candidate*
5. **Spikeshot Shotgun** (`wp_SP_ARShotgun_SF_shotgunFire35.wav`) — *New Candidate*
6. **Exploration Drone Soundtrack** (`MUSIC_exploreDrone01_XSA_CONCEPTexploreDrone01.wav`) — *New Candidate*
7. **Boss Combat Grom Soundtrack** (`MUSIC_combatBossGrom01_XSA_combatBossGrom01.wav`) — *New Candidate (5.1 Surround)*
8. **Island Ocean Storm Ambience** (`SP_Freight_Audio_2010_XSA_windStormHigh01_R.wav`) — *New Candidate (Mono Positional)*

Both **Conservative Restoration** and **Enhanced Remaster** variants are fully generated and preserved for every target.

---

## 1. Direct Float32 Vorbis Decoding Evidence & Synthesis Overshoot Analysis

To rigorously evaluate peak behavior across the full asset suite, raw Vorbis payloads were decoded directly into 32-bit floating point PCM (`pcm_f32le`) via FFmpeg without integer clamping:

### Empirical Float32 Decode Measurements (All 8 Targets)

| Asset Identity | Raw Vorbis File | Max Float Amplitude | Max Peak (dBFS) | Float Samples $\ge 1.000$ | % of Total File | PCM16 Clamped Runs | Max Clamped Run Length |
|:---|:---|---:|---:|---:|---:|---:|---:|
| **Weapon 1: Centurion Revolver (Primary Fire)** | `wp_SP_ARPistol_SF_arPistolFire03.ogg` | **1.60216** | **+4.09 dBFS** | **878** | 0.732% | 76 | **6 samples** |
| **Weapon 2: Kasimov Sniper Rifle (High-Velocity Fire)** | `wp_SP_ARSniperRifle_SF_sniperFire08.ogg` | **2.44173** | **+7.75 dBFS** | **6,055** | 2.803% | 400 | **422 samples** |
| **E99 Effect: TMD Energy Impulse Blast** | `RvGame_tmdFireImpulse16.ogg` | **1.78155** | **+5.02 dBFS** | **3,760** | 1.958% | 350 | **15 samples** |
| **Weapon 3: AR9 Walkiria Assault Rifle (Automatic Fire)** | `wp_SP_ARMachineGun_SF_machinegunFire57.ogg` | **2.23004** | **+6.97 dBFS** | **656** | 0.492% | 63 | **10 samples** |
| **Weapon 4: Spikeshot Shotgun (Primary Blast)** | `wp_SP_ARShotgun_SF_shotgunFire35.ogg` | **1.69383** | **+4.58 dBFS** | **721** | 0.496% | 17 | **4 samples** |
| **Track 1: Exploration Drone (Ambient Music)** | `MUSIC_exploreDrone01_XSA_CONCEPTexploreDrone01.ogg` | **0.63895** | **-3.89 dBFS** | **0** | 0.0% | 0 | **0 samples** |
| **Track 2: Boss Fight Grom (Combat Orchestral 5.1 Surround)** | `MUSIC_combatBossGrom01_XSA_combatBossGrom01.ogg` | **0.70973** | **-2.98 dBFS** | **0** | 0.0% | 0 | **0 samples** |
| **Ambience: Island Ocean Storm & Wind Loop** | `SP_Freight_Audio_2010_XSA_windStormHigh01_R.ogg` | **0.70243** | **-3.07 dBFS** | **0** | 0.0% | 0 | **0 samples** |

### Technical Findings on Vorbis Decoding and Pre-Encoding Limiting
- **Gunshots vs Music/Ambience Separation:** A sharp boundary exists in the 2010 asset suite: all 4 firearms and the TMD discharge were mastered hot (0 dBFS / clipped), producing float synthesis peaks up to **+7.75 dBFS (Kasimov)** and **+6.97 dBFS (Walkiria)**. In contrast, both music tracks and the storm ambience loop were authored with proper headroom (peaks at $-2.98$ to $-3.89\text{ dBFS}$), reporting **0 samples $\ge 1.0$** and **0 clamped runs**.
- **Distinguishing Vorbis Synthesis Overshoot from Pre-Encoding Clipping:** Vorbis MDCT synthesis filters can generate inter-sample peak overshoots exceeding $1.0$ on unclipped transients if the source material was pushed close to digital full scale ($0\text{ dBFS}$). However, in Kasimov (`sniperFire08.ogg`), a sustained run of **420 consecutive samples** (~8.8 ms) continuously floats above $1.000$ (between $1.0005$ and $1.0290$), which proves pre-encoding brickwall limiting / hard clipping in the studio before compression.

---

## 2. Reversible Level-Matched Audition Workflow

Because the remastered weapon candidates enforce a strict -1.0 dBTP ceiling to eliminate digital overs, they are 2.7 to 5.5 dB quieter in raw RMS than the 2010 clipped originals. Comparing them at raw volume introduces psychoacoustic loudness bias.

The audition player in `index.html` provides:
1. **Level-Matched Mode (Audition Default):** Attenuates the louder Original reference by its exact measured $\Delta\text{RMS}$ using reversible Web Audio GainNodes so that Original, Conservative, and Enhanced play at **identical perceived loudness**.
2. **Untouched Native Mode:** Sets all GainNodes to $0.00\text{ dB}$ (1:1 file playback) to audition the exact disk levels and headroom.
3. **Master Output Safety Margin:** A **$-2.0\text{ dB}$ master ceiling** (adjustable from $-6.0$ to $0.0\text{ dB}$) guarantees that browser D/A converters never clip during listening.
4. **Instant A/B Switching:** Single-click switcher buttons (`Original`, `Conservative`, `Enhanced`, `Stop`) allow instant back-to-back switching from the start of the gunshot.

---

## 3. Empirical QA Measurements Table (Complete 8-Target Suite)

| Candidate Set | Version | True Peak (dBTP) | Sample Peak (dBFS) | RMS Level (dBFS) | Δ RMS (dB) | Sat Samples (Proc / Orig) | Best Lag (Samples) | Onset Diff | Phase Corr |
|:---|:---|---:|---:|---:|---:|---:|---:|---:|---:|
| **Weapon 1: Centurion Revolver (Primary Fire)** | Original Reference | +1.80 | +0.00 | -12.49 | *Reference (0.00)* | 543 / 543 | 0 | 0 | 0.5039 |
| | Conservative | **-1.00** | -2.68 | -16.86 | -4.37 | **0** / 543 (0 intro) | **0** | 2 | 0.6793 |
| | Enhanced Remaster | **-1.00** | -2.68 | -16.63 | -4.14 | **0** / 543 (0 intro) | **0** | 2 | 0.6861 |
| **Weapon 2: Kasimov Sniper Rifle (High-Velocity Fire)** | Original Reference | +2.60 | +0.00 | -12.46 | *Reference (0.00)* | 4180 / 4180 | 0 | 0 | 0.6087 |
| | Conservative | **-1.00** | -3.60 | -17.97 | -5.52 | **0** / 4180 (0 intro) | **0** | 3 | 0.4625 |
| | Enhanced Remaster | **-1.00** | -3.41 | -17.69 | -5.23 | **0** / 4180 (0 intro) | **0** | 3 | 0.4529 |
| **E99 Effect: TMD Energy Impulse Blast** | Original Reference | +3.30 | +0.00 | -10.60 | *Reference (0.00)* | 1961 / 1961 | 0 | 0 | 0.5463 |
| | Conservative | **-1.00** | -3.60 | -14.78 | -4.17 | **0** / 1961 (0 intro) | **0** | 0 | 0.5134 |
| | Enhanced Remaster | **-1.00** | -4.30 | -14.95 | -4.35 | **0** / 1961 (0 intro) | **0** | 0 | 0.5241 |
| **Weapon 3: AR9 Walkiria Assault Rifle (Automatic Fire)** | Original Reference | +2.40 | +0.00 | -16.80 | *Reference (0.00)* | 414 / 414 | 0 | 0 | 0.4398 |
| | Conservative | **-1.00** | -3.03 | -20.60 | -3.80 | **0** / 414 (0 intro) | **0** | 1 | 0.4225 |
| | Enhanced Remaster | **-1.00** | -3.22 | -20.66 | -3.87 | **0** / 414 (0 intro) | **0** | 1 | 0.4286 |
| **Weapon 4: Spikeshot Shotgun (Primary Blast)** | Original Reference | +1.80 | +0.00 | -12.32 | *Reference (0.00)* | 420 / 420 | 0 | 0 | 0.075 |
| | Conservative | **-1.10** | -2.77 | -15.51 | -3.18 | **0** / 420 (0 intro) | **0** | 0 | 0.0784 |
| | Enhanced Remaster | **-1.00** | -2.68 | -15.05 | -2.73 | **0** / 420 (0 intro) | **0** | 0 | 0.0567 |
| **Track 1: Exploration Drone (Ambient Music)** | Original Reference | -3.90 | -3.89 | -21.00 | *Reference (0.00)* | 0 / 0 | 0 | 0 | 0.1294 |
| | Conservative | **-5.00** | -4.99 | -21.37 | -0.37 | **0** / 0 (0 intro) | **0** | -4062 | 0.135 |
| | Enhanced Remaster | **-4.50** | -4.49 | -21.02 | -0.01 | **0** / 0 (0 intro) | **0** | -4063 | 0.1278 |
| **Track 2: Boss Fight Grom (Combat Orchestral 5.1 Surround)** | Original Reference | -2.90 | -2.98 | -20.09 | *Reference (0.00)* | 0 / 0 | 0 | 0 | N/A |
| | Conservative | **-2.90** | -2.90 | -20.34 | -0.24 | **0** / 0 (0 intro) | **0** | 0 | N/A |
| | Enhanced Remaster | **-2.50** | -2.51 | -19.75 | +0.35 | **0** / 0 (0 intro) | **0** | 0 | N/A |
| **Ambience: Island Ocean Storm & Wind Loop** | Original Reference | -3.00 | -3.07 | -16.27 | *Reference (0.00)* | 0 / 0 | 0 | 0 | N/A |
| | Conservative | **-3.30** | -3.35 | -16.77 | -0.50 | **0** / 0 (0 intro) | **0** | 6 | N/A |
| | Enhanced Remaster | **-3.60** | -3.73 | -17.12 | -0.85 | **0** / 0 (0 intro) | **0** | 6 | N/A |

---

## 4. Distinction Between Tagged UProperty Pointers and Native Binary Serialization

The Object Reference Provenance distinguishes between two separate UE3 serialization mechanisms:

1. **Tagged `UProperty` Object Pointers:**
   - Verified through standard UE3 property tag headers (`FName` name, `FName` type, `int32` size, `int32` array index).
   - Examples: `USoundCue::FirstNode` (ObjectProperty), `USoundNodeMixer::ChildNodes` (ArrayProperty), `URSoundNodeCue::LinkedCue` (ObjectProperty), `USoundNodeAttenuation::ChildNodes`.
2. **Native C++ Binary Serialization (`TArray<USoundNode*> ChildNodes`):**
   - In classes like `SoundNodeModulator`, child node references are serialized natively by `USoundNode::Serialize(FArchive& Ar)` directly into the node binary payload following properties (`Ar << ChildNodes`), rather than as tagged UnrealScript properties.
   - In `SoundNodeRandom` (Spikeshot Shotgun), `ChildNodes` array is serialized with direct pointers to `shotgunFire35` and `shotgunFire36`.
3. **Companion Package Imports:**
   - For music (`MUSIC_exploreDrone01.xxx`, `MUSIC_combatBossGrom01.xxx`), the cue package imports the heavy `SoundNodeWave` payload from its companion `_XSA.xxx` extended sound archive.

### Exact DAG Path Provenance for All 8 Targets

### Weapon 1: Centurion Revolver (Primary Fire)
- **Confidence:** `VERIFIED_REFERENCE`
- **Target Asset:** `arPistolFire03` (`wp_SP_ARPistol_SF.xxx`)
- **Referencing Cue:** `arPistolFireNEW`
- **Tagged UProperty Segment:**
  `Cue(arPistolFireNEW) [FirstNode] -> SoundNodeMixer [ChildNodes[0]] -> SoundNodeAttenuation [ChildNodes[0]] -> RSoundNodeMultichannelCollapseAttenuation [ChildNodes[0]] -> RSoundNodeCue [LinkedCue] -> LinkedCue(mARPistolFire) [FirstNode] -> SoundNodeModulator`
- **Native Binary Serialization Segment:**
  `SoundNodeModulator -> SoundNodeWave(arPistolFire03) (via native USoundNode::ChildNodes binary TArray at node footer)`
- **Complete Verified Path:**
  `Cue(arPistolFireNEW) -> SoundNodeMixer -> SoundNodeAttenuation -> RSoundNodeMultichannelCollapseAttenuation -> RSoundNodeCue -> LinkedCue(mARPistolFire) -> SoundNodeModulator -> SoundNodeWave(arPistolFire03)`
- **Multi-Layer Architectural Role:**
  Primary close-range transient layer (Stereo 48kHz). Crossfades with Branch 2 (pistolFireDistant05, Mono 24kHz) at range and Branch 3 (lateReflectionsPistolGlock01, 4ch surround) for room acoustic reflections.

### Weapon 2: Kasimov Sniper Rifle (High-Velocity Fire)
- **Confidence:** `VERIFIED_REFERENCE`
- **Target Asset:** `sniperFire08` (`wp_SP_ARSniperRifle_SF.xxx`)
- **Referencing Cue:** `arSniperFire`
- **Tagged UProperty Segment:**
  `Cue(arSniperFire) [FirstNode] -> SoundNodeMixer [ChildNodes[0]] -> SoundNodeAttenuation [ChildNodes[0]] -> RSoundNodeMultichannelCollapseAttenuation [ChildNodes[0]] -> RSoundNodeCue [LinkedCue] -> LinkedCue(mSniperFire) [FirstNode] -> SoundNodeModulator`
- **Native Binary Serialization Segment:**
  `SoundNodeModulator -> SoundNodeWave(sniperFire08) (via native USoundNode::ChildNodes binary TArray at node footer)`
- **Complete Verified Path:**
  `Cue(arSniperFire) -> SoundNodeMixer -> SoundNodeAttenuation -> RSoundNodeMultichannelCollapseAttenuation -> RSoundNodeCue -> LinkedCue(mSniperFire) -> SoundNodeModulator -> SoundNodeWave(sniperFire08)`
- **Multi-Layer Architectural Role:**
  Primary high-velocity armor-piercing transient layer (Stereo 48kHz). Crossfades with Branch 2 (shotgunFireDistant05, Mono 24kHz distant report) and Branch 3 (lateReflectionsRifleFamas02, 4ch surround late reflections).

### E99 Effect: TMD Energy Impulse Blast
- **Confidence:** `VERIFIED_REFERENCE`
- **Target Asset:** `tmdFireImpulse16` (`RvGame.xxx`)
- **Referencing Cue:** `tmdFireImpulse`
- **Tagged UProperty Segment:**
  `Cue(tmdFireImpulse) [FirstNode] -> SoundNodeMixer [ChildNodes[0]] -> RSoundNodeCue [LinkedCue] -> LinkedCue(mTMDImpulseFire) [FirstNode] -> SoundNodeModulator`
- **Native Binary Serialization Segment:**
  `SoundNodeModulator -> SoundNodeWave(tmdFireImpulse16) (via native USoundNode::ChildNodes binary TArray at node footer)`
- **Complete Verified Path:**
  `Cue(tmdFireImpulse) -> SoundNodeMixer -> RSoundNodeCue -> LinkedCue(mTMDImpulseFire) -> SoundNodeModulator -> SoundNodeWave(tmdFireImpulse16)`
- **Multi-Layer Architectural Role:**
  Primary temporal distortion pulse (Stereo 48kHz). Concatenated with Branch 2 electrical crackle (SoundNodeConcatenator -> rElectricSpark01).

### Weapon 3: AR9 Walkiria Assault Rifle (Automatic Fire)
- **Confidence:** `VERIFIED_REFERENCE`
- **Target Asset:** `machinegunFire57` (`wp_SP_ARMachineGun_SF.xxx`)
- **Referencing Cue:** `arMachinegunFire`
- **Tagged UProperty Segment:**
  `Cue(arMachinegunFire) [FirstNode] -> SoundNodeMixer [ChildNodes[0]] -> SoundNodeAttenuation [ChildNodes[0]] -> RSoundNodeMultichannelCollapseAttenuation [ChildNodes[0]] -> RSoundNodeCue [LinkedCue] -> LinkedCue(mMachinegunFire) [FirstNode] -> SoundNodeModulator`
- **Native Binary Serialization Segment:**
  `SoundNodeModulator -> SoundNodeWave(machinegunFire57) (via native USoundNode::ChildNodes binary TArray at node footer)`
- **Complete Verified Path:**
  `Cue(arMachinegunFire) -> SoundNodeMixer -> SoundNodeAttenuation -> RSoundNodeMultichannelCollapseAttenuation -> RSoundNodeCue -> LinkedCue(mMachinegunFire) -> SoundNodeModulator -> SoundNodeWave(machinegunFire57)`
- **Multi-Layer Architectural Role:**
  Primary close-range automatic rifle transient layer (Stereo 48kHz). Crossfades with Branch 2 (machinegunFireDistant, Mono 24kHz) at range and Branch 3 (lateReflectionsMGFamas01, 4ch surround).

### Weapon 4: Spikeshot Shotgun (Primary Blast)
- **Confidence:** `VERIFIED_REFERENCE`
- **Target Asset:** `shotgunFire35` (`wp_SP_ARShotgun_SF.xxx`)
- **Referencing Cue:** `arShotgunFire`
- **Tagged UProperty Segment:**
  `Cue(arShotgunFire) [FirstNode] -> SoundNodeMixer [ChildNodes[0]] -> SoundNodeAttenuation [ChildNodes[0]] -> RSoundNodeMultichannelCollapseAttenuation [ChildNodes[0]] -> RSoundNodeCue [LinkedCue] -> LinkedCue(mShotgunFire) [FirstNode] -> SoundNodeModulator [Child] -> SoundNodeRandom [ChildNodes[0]]`
- **Native Binary Serialization Segment:**
  `SoundNodeModulator -> SoundNodeRandom (via native USoundNode::ChildNodes binary TArray) -> SoundNodeWave(shotgunFire35)`
- **Complete Verified Path:**
  `Cue(arShotgunFire) -> SoundNodeMixer -> SoundNodeAttenuation -> RSoundNodeMultichannelCollapseAttenuation -> RSoundNodeCue -> LinkedCue(mShotgunFire) -> SoundNodeModulator -> SoundNodeRandom -> SoundNodeWave(shotgunFire35)`
- **Multi-Layer Architectural Role:**
  Primary close-range shotgun blast transient layer (Stereo 48kHz). Picked randomly with shotgunFire36, crossfades with Branch 2 (shotgunFireDistant05, Mono 24kHz) and Branch 3 (lateReflectionsRifleFamas02, 4ch surround).

### Track 1: Exploration Drone (Ambient Music)
- **Confidence:** `VERIFIED_REFERENCE`
- **Target Asset:** `CONCEPTexploreDrone01` (`MUSIC_exploreDrone01_XSA.xxx`)
- **Referencing Cue:** `CONCEPTexploreDrone01`
- **Tagged UProperty Segment:**
  `Package(MUSIC_exploreDrone01.xxx) -> SoundCue(CONCEPTexploreDrone01) [FirstNode / Import] -> Package(MUSIC_exploreDrone01_XSA.xxx)`
- **Native Binary Serialization Segment:**
  `SoundNodeWave(CONCEPTexploreDrone01) Vorbis bulk slot 1 stream in MUSIC_exploreDrone01_XSA.xxx`
- **Complete Verified Path:**
  `MUSIC_exploreDrone01.xxx::SoundCue(CONCEPTexploreDrone01) -> MUSIC_exploreDrone01_XSA.xxx::SoundNodeWave(CONCEPTexploreDrone01)`
- **Multi-Layer Architectural Role:**
  Continuous exploration background music loop (Stereo 32kHz, 132.0s seamless boundary). Full dynamic range, wide atmospheric pads.

### Track 2: Boss Fight Grom (Combat Orchestral 5.1 Surround)
- **Confidence:** `VERIFIED_REFERENCE`
- **Target Asset:** `combatBossGrom01` (`MUSIC_combatBossGrom01_XSA.xxx`)
- **Referencing Cue:** `combatBossGrom01`
- **Tagged UProperty Segment:**
  `Package(MUSIC_combatBossGrom01.xxx) -> DynamicMusic(combatBossGrom01) [SeqAct_PlayDynamicMusic] -> SoundNodeWave(combatBossGrom01)`
- **Native Binary Serialization Segment:**
  `SoundNodeWave(combatBossGrom01) 6-channel 5.1 discrete Vorbis bulk stream in MUSIC_combatBossGrom01_XSA.xxx`
- **Complete Verified Path:**
  `MUSIC_combatBossGrom01.xxx::DynamicMusic(combatBossGrom01) -> MUSIC_combatBossGrom01_XSA.xxx::SoundNodeWave(combatBossGrom01) [5.1 Surround]`
- **Multi-Layer Architectural Role:**
  Epic cinematic combat theme for the Grom boss fight. Discrete 6-channel 5.1 surround (L, R, C, LFE, Ls, Rs). Retains true surround field in VR.

### Ambience: Island Ocean Storm & Wind Loop
- **Confidence:** `HEURISTIC_MATCH`
- **Target Asset:** `windStormHigh01_R` (`SP_Freight_Audio_2010_XSA.xxx`)
- **Referencing Cue:** `windStormHigh01`
- **Tagged UProperty Segment:**
  `Package(SP_Freight_Audio_2010.xxx) -> AmbientSoundSimpleToggleable -> SoundCue(windStormHigh01)`
- **Native Binary Serialization Segment:**
  `Referenced SoundNodeWave(windStormHigh01_R) in SP_Freight_Audio_2010_XSA.xxx`
- **Complete Verified Path:**
  `SP_Freight_Audio_2010.xxx::AAmbientSound -> SP_Freight_Audio_2010_XSA.xxx::SoundNodeWave(windStormHigh01_R) [Mono Positional]`
- **Multi-Layer Architectural Role:**
  Environmental coastal gale loop positioned in 3D world space. Strictly mono 22,050 Hz for spatial-audio HRTF positioning in OpenXR/PSVR2.

---

## 5. Nonlinear Waveshaping & Transient Listening Guidance

> [!IMPORTANT]
> **Nonlinear Waveshaping Disclosure:**
> The soft-knee hyperbolic tangent ($\tanh$) waveshaper applied at the -1.0 dBTP ceiling is a memoryless nonlinear saturation function. While it successfully guarantees zero digital clipping and zero limiter lookahead latency (0.000 ms lag), any nonlinear compression modifies the crest factor and high-frequency harmonics of sharp gunshot transients.
> 
> **Listening Test Objective:**
> Evaluate both the Conservative Restoration (which retains linear dynamics without heavy waveshaping) and the Enhanced Remaster (which shapes low-end thump and transient attack) to decide which aesthetic preset sounds best for each category.

---

## 6. Audio Handoff Guidelines for MAIN Integration

This section documents architectural requirements for MAIN when integrating approved audio into Singularity VR:

### 1. UE3 Package Layout & Bulk Slot 1 Vorbis Streams
- **Cooked Format:** Singularity UE3 (v584) stores PC audio in native platform bulk slot 1 starting with `OggS` headers.
- **Frame Budget Invariant:** In UE3 v584, `RawPCMDataSize` equals `Duration * SampleRate * NumChannels * 2`. Every processed file in this suite preserves the **100% exact sample frame count** of the original asset. Zero length delta guarantees no desynchronization with muzzle flash anim notifiers or loop seams.

### 2. Multichannel & Spatial Routing Invariants
- **Boss Combat Grom (5.1 Surround):** Must be authored as a discrete 6-channel Vorbis stream. Native UE3 XAudio2 mixer routes 6 discrete channels to front L/R, center, LFE, and rear surround. **Do not downmix to stereo.**
- **Storm Ambience (Mono Positional):** Authored as a 1-channel mono stream at 22,050 Hz. Native `AAmbientSound` actors perform 3D HRTF positioning. Mono assets must remain strictly mono for VR binaural spatialization.
- **Gunshot Multi-Layer Graph:** Weapons (Centurion, Kasimov, Walkiria, Spikeshot) consist of close stereo primary fire, distant mono report, and 4-channel surround late reflections collapsed dynamically via `RSoundNodeMultichannelCollapseAttenuation`. Modifying only the primary report preserves full distance crossfading and room reverberation.

### 3. In-Game Playback Loudness Calibration
- Because the Enhanced weapon files enforce a -1.0 dBTP ceiling without clipping, they are 2.7 to 5.5 dB quieter in raw RMS than the 2010 hard-clipped files.
- If MAIN desires higher weapon volume in-game, MAIN should adjust the SoundCue's native `VolumeMultiplier` or `SoundClass` volume rather than pushing the WAV files back into digital clipping distortion.

---

## 7. Deliverables & Listening Review

Audition the complete 8-target suite using the level-matched player at:

```
file:///C:/Users/Trilex/vr-mods/SingularityVR-audio-music-sfx-remaster/audio-remaster-private/audition/index.html
```

**Decision Status:** `READY FOR LISTENING REVIEW`
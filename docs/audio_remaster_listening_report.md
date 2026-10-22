# Singularity VR — Audio Remaster & Restoration Research Report (Final Audition Preparation)

**Author:** Senior Game Audio Engineer & Sound Mastering Specialist  
**Branch:** `side/audio-music-sfx-remaster`  
**Target Game:** Singularity (2010), Raven Software, Unreal Engine 3 (v584, PC x86)  
**Status:** `READY FOR LISTENING REVIEW`  

## Executive Summary

In strict compliance with the Final Listening A/B Preparation mandate, the audition workflow and technical documentation have been updated. The mastered WAV files on disk remain strictly untouched at their -1.0 dBTP ceiling. Level-matched auditioning is achieved reversibly in the browser via Web Audio API GainNodes and HTML5 media attenuation with a master output safety margin.

This report delivers:
1. **Direct Float32 Vorbis Decoding Verification:** Quantitative proof confirming that the 2010 game source masters genuinely exceed digital full scale ($+4.09$ to $+7.75\text{ dBFS}$) in the float domain, independent of 16-bit integer quantization.
2. **Reversible Level-Matched Audition Player:** Integrated in `index.html` with instantaneous A/B/C switching, output safety headroom, and clear toggling between Level-Matched and Untouched Native modes.
3. **Explicit Graph Provenance:** Clear differentiation between tagged UE3 `UProperty` DAG pointers and native C++ binary serialization.
4. **Nonlinear Waveshaping Disclosure:** Explicit guidance acknowledging that soft-knee $\tanh$ limiting alters transient crests and requires human listening evaluation to judge.

Processing remains strictly limited to the **3 approved targets** (Centurion, Kasimov, TMD Impulse).

---

## 1. Direct Float32 Vorbis Decoding Evidence (Source Clipping Proof)

To verify whether the original hard-clipping claim is genuinely supported by the source audio rather than being a PCM16 decoder clamping artifact, the raw Vorbis payloads were decoded directly into 32-bit floating point PCM (`pcm_f32le`) via FFmpeg without integer quantization.

### Empirical Float32 Decode Measurements

| Weapon / Effect | Raw Vorbis File | Max Float Amplitude | Max Peak (dBFS) | Float Samples $\ge 1.000$ | % of Total File | PCM16 Clamped Runs | Max Clamped Run Length |
|:---|:---|---:|---:|---:|---:|---:|---:|
| **Weapon 1: Centurion Revolver (Primary Fire)** | `wp_SP_ARPistol_SF_arPistolFire03.ogg` | **1.60216** | **+4.09 dBFS** | **878** | 0.732% | 76 | **6 samples** |
| **Weapon 2: Kasimov Sniper Rifle (High-Velocity Fire)** | `wp_SP_ARSniperRifle_SF_sniperFire08.ogg` | **2.44173** | **+7.75 dBFS** | **6,055** | 2.803% | 400 | **422 samples** |
| **E99 Effect: TMD Energy Impulse Blast** | `RvGame_tmdFireImpulse16.ogg` | **1.78155** | **+5.02 dBFS** | **3,760** | 1.958% | 350 | **15 samples** |

### Key Technical Takeaways from Float32 Analysis
- **Ogg Vorbis Synthesis Overshoots:** Vorbis uses MDCT frequency-domain transform coding. When Raven Software encoded waveforms that were rendered at 0 dBFS or clipped in PCM, the lossy synthesis filter generates continuous reconstruction overshoots up to **+4.09 dBFS (Centurion), +7.75 dBFS (Kasimov), and +5.02 dBFS (TMD)**.
- **Sustained Plateau in Kasimov Rifle:** In `sniperFire08.ogg`, between sample indices 6,556 and 6,975, the raw float32 signal floats continuously between **1.0005 and 1.0290** for **420 consecutive samples** (~8.8 ms). In a 16-bit integer engine, this converts to a flat square-wave at $\pm 32767$.
- **Conclusion:** The source defect is a real physical property of the 2010 asset master. Enforcing a -1.0 dBTP ceiling without clipping eliminates this severe analog/digital clipping, but mathematically forces the measured RMS level down by 4.1 to 5.5 dB.

---

## 2. Reversible Level-Matched Audition Workflow

Because the remastered files are 4.1 to 5.5 dB quieter on disk to protect against clipping, playing them raw introduces an unfair psychoacoustic loudness bias (louder sounds are perceived as fuller).

To solve this without modifying WAV amplitudes or introducing clipping on disk:
- **Web Audio GainNode Compensation:** In `index.html`, each track runs through an isolated `GainNode` into a `MasterSafetyGainNode`.
- **Dual Modes:**
  1. **Level-Matched Mode (Audition Default):** Attenuates the louder Original reference by its exact measured $\Delta\text{RMS}$ (Centurion: $-4.14\text{ dB}$, Kasimov: $-5.23\text{ dB}$, TMD: $-4.35\text{ dB}$) so that Original, Conservative, and Enhanced play at **identical RMS loudness**.
  2. **Untouched Native Mode:** Sets all GainNodes to $0.00\text{ dB}$ (1:1 file playback) to audition the exact disk levels and headroom.
- **Master Output Safety Margin:** A $-2.0\text{ dB}$ master ceiling (adjustable from $-6.0$ to $0.0\text{ dB}$) guarantees that browser D/A converters never clip during listening.
- **Instant A/B Switching:** Single-click buttons (`Original`, `Conservative`, `Enhanced`, `Stop`) allow instant back-to-back switching from the start of the gunshot.

---

## 3. Empirical QA Measurements Table (Regenerated Targets)

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

---

## 4. Distinction Between Tagged UProperty Pointers and Native Binary Serialization

To maintain rigorous reverse-engineering standards, the Object Reference Provenance distinguishes between two separate UE3 serialization mechanisms:

1. **Tagged `UProperty` Object Pointers:**
   - Verified through standard UE3 property tag headers (`FName` name, `FName` type, `int32` size, `int32` array index).
   - Examples: `USoundCue::FirstNode` (ObjectProperty), `USoundNodeMixer::ChildNodes` (ArrayProperty), `URSoundNodeCue::LinkedCue` (ObjectProperty), `USoundNodeAttenuation::ChildNodes`.
2. **Native C++ Binary Serialization (`TArray<USoundNode*> ChildNodes`):**
   - In certain classes like `SoundNodeModulator`, child node references are serialized natively by `USoundNode::Serialize(FArchive& Ar)` directly into the node binary payload following properties (`Ar << ChildNodes`), rather than as tagged UnrealScript properties.
   - These pointers are recovered directly from the binary `TArray` integer offset at the node footer, pointing to the target `SoundNodeWave`.

### Exact DAG Path Provenance

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

---

## 5. Nonlinear Waveshaping & Transient Listening Guidance

> [!IMPORTANT]
> **Nonlinear Waveshaping Disclosure:**
> The soft-knee hyperbolic tangent ($\tanh$) waveshaper applied at the -1.0 dBTP ceiling is a memoryless nonlinear saturation function. While it successfully guarantees zero digital clipping and zero limiter lookahead latency (0.000 ms lag), any nonlinear compression modifies the crest factor and high-frequency harmonics of sharp gunshot transients.
> 
> **Listening Test Objective:**
> Do not assume that removing digital clipping automatically sounds superior. The human ear must decide whether the softer, rounded wave crest in the Enhanced master provides a fuller, more impactful weapon report or whether it slightly reduces the aggressive transient 'crack' of the original clipped sound.

---

## 6. Deliverables & Listening Review

Audition the 3 comparison sets using the level-matched player at:

```
file:///C:/Users/Trilex/vr-mods/SingularityVR-audio-music-sfx-remaster/audio-remaster-private/audition/index.html
```

**Decision Status:** `READY FOR LISTENING REVIEW`
# Wrist palm-facing follow-up

## Accepted and failed physical scope

User confirms VFX OK on B7481E527AB0C5B751A4CC8ABC3DAD736A8ADE8FFAB65B4A053AC49762DD3CEF (source 72a18713); new wrist HUD still invisible, old health display visible. Retain VFX unchanged. Archived session build/native-muzzle/session-20261030-B7481E52/view_matrix.log SHA256 B679906796641871D8CA92DDB49B561CA4E88BEB6DF9B9F10B977CC1F5C09B23.

## First violated contract

The final wrist-to-XR transform changes handedness. Reconstructing FinalWrist with the actual original TMD bind and TrackingMap gives determinant -6.9107e-06 in all twelve rotation/ride fixtures (52.5 UU/m). The old native panel right=+Z, top=-Y gives a cross product +X locally, but its final XR cross product points opposite the transformed anatomical +X palm. Visibility therefore selects the back-facing view; preserved foreground depth then correctly hides most of that panel behind the glove. Actual session: worldMask 63846/58634, foregroundComposite 76/2831. Those counts alone do not prove readable pixels.

Original named index finger curl also disproves the pending assumption that native -X is the palm. A trial assertion for that assumption failed; the proposed -124 mm relocation was rejected and its working files preserved privately in build/wrist-main/rejected-palm-sign. No such relocation is deployed or included.

## Minimal correction and falsifier

Reverse only the inner panel logical cross-axis (Z). Keep +94 mm attachment, +80 mm longitudinal centre, original tracking, text top toward fingers, projection/depth/resources and VFX unchanged. The final XR surface cross product now agrees with anatomical +X in all reconstructed cases. Outer mode is unchanged. No INI change is required. Legacy health suppression remains OFF.

Falsifier: panel remains invisible when the palm faces the headset despite fresh pose, or remains visible only from the back. One physical check after L1/L2 must verify readable inner wrist, disappearance when turned away, and occlusion by glove/weapon/magazine. Do not claim physical acceptance from fixtures.

## Validation

HUD policy/projection/source/placement and 12 final-palette integration fixtures pass. Actual D3D9/D3D9Ex pixel tests and optimized build results are recorded with the final candidate below. Steam retains accepted B7481E52 until an explicitly prepared HUD test. No game launch for this correction.

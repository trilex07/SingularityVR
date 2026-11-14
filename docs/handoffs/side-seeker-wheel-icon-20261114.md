# Seeker wheel icon

The illustration follows Seeker's actual muzzle, scope, vent ribs, grip and stock.
Its reference was the side elevation of `steerable_world` in the local
`wp_SP_ARSteerable_SF.xxx`. The editable input for the UI asset pipeline is
`assets/source/ui_remaster/weapon_wheel/weapon_icon_seeker_master.png`.
No extracted mesh or DDS is included. The generator reuses the existing
Centurion/Valkyrie/Spikeshot/Kasimov toning and renders a comparison sheet:

```powershell
python asset-tools/build_seeker_wheel_icon.py
python asset-tools/build_seeker_wheel_icon.py --check
```

The remaster, rounded legacy and square legacy wheels all call the existing
Seeker icon function. It uploads this 128 × 128 image from embedded pixels,
uses the old vector as fallback, and releases the texture on Reset/ResetEx.
The game/VR installer needs no additional asset or atlas change.

Offline: x86 /O2 PE32 build and generator check passed. The isolated D3D9 and
D3D9Ex wheel fixture drew 65 changed pixels per eye at a 24 × 24 quad and
verified identical stereo pixels and a transparent gutter. Steam and PSVR2
were not run.

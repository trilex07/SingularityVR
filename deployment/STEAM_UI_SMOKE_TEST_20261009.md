# Steam UI smoke test — 2026-10-09

Launch path: Steam Library -> Singularity -> GRAJ.

Evidence captured:
- `steam-library.jpg` — Steam Library with Singularity selected/running.
- `steam-gameplay-live.png` — fresh game window after Steam launch; stereo gameplay scene and weapon visible.
- `steam-pause-live.png` — pause/menu layer; `VR SETTINGS` visible in both eyes and right-controller laser visible.

Observed:
- Steam launched `Singularity.exe` successfully.
- Splash completed and main menu rendered in SBS stereo.
- Keyboard Return moved from the main menu into gameplay.
- Gameplay rendered with the weapon and HUD.
- Escape opened the VR menu layer with `VR SETTINGS` visible.
- Diagnostic log reported TRUE STEREO ON and approximately 89.9 fps (11.12 ms/frame) during the run.

Limitation:
- This smoke test used the desktop control path for menu progression. It confirms the deployed game window and render path, but does not prove a physical Sense click or headset tracking.

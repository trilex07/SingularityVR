# Changelog — paczka 3D Katorga HUD

## 2026-10-14

- Utworzono szkielet paczki SIDE: handoff `docs/handoffs/side-hud-3d-attach-pack.md`, szablon `attachments/attach_schema.template.json`, `ATTACHMENTS.md`.
- **Dostarczono:** 6× `.glb`, `attachments/attach_schema.json`, tekstury, preview 1920×1080, `tools/hud/export_katorga_3d_pack.py`.
- Preview = concept art `katorga-target-*` (wizualna akceptacja); mesh = eksport PSK gry.
- **MAIN pack:** `weapon_wheel_catalog.json`, handoffy `main-hud-3d-integration.md`, `main-weapon-wheel-direct-select.md`, `weapon_wheel_direct_select.h`, `install-katorga-3d-assets.ps1`.
- **Holster biodro:** `holster_slots.json`, `holster_weapon_right.glb`, `holster_medkit_left.glb`, `main-holster-medkit-hip-slots.md`, `holster_hip_slots.h`.
- **Koło broni (holo radial):** `render_weapon_wheel_preview.py` + `wheel_weapon_draw.py`, `wheel/holo_wheel_style.json` (layout jak referencja Alyx-style). Atlas `weapon-*` nadal dla UV; `zmiana_broni.png` = stary concept AI.

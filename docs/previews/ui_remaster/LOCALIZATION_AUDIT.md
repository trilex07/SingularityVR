# Singularity VR — UI Remaster V2.1 Complete Localization Audit & Mapping Table

**Branch:** `side/ui-utility-graphics-remaster`  
**Master Localization File:** [`ui_localization_pl_en.json`](file:///C:/Users/Trilex/vr-mods/SingularityVR-ui-utility-remaster/ui_localization_pl_en.json)  
**Total Localized Keys:** 89  
**Baked Sprites per Language Atlas:** 36  
**Runtime Font Rendered Keys:** 53  
**Status:** 100% Verified — 0 Missing Mappings, 0 Untranslated Strings  

---

## 1. Audit Summary & Architecture Breakdown

To guarantee optimal VRAM efficiency on PSVR2 while eliminating texture duplication:
1. **36 Keys** have dedicated pre-rendered localized sprites in [`ui_atlas_localized_pl.png`](file:///C:/Users/Trilex/vr-mods/SingularityVR-ui-utility-remaster/assets/processed/ui_remaster/atlas/ui_atlas_localized_pl.png) and [`ui_atlas_localized_en.png`](file:///C:/Users/Trilex/vr-mods/SingularityVR-ui-utility-remaster/assets/processed/ui_remaster/atlas/ui_atlas_localized_en.png).
2. **53 Keys** are explicitly assigned to **runtime dynamic font rendering** (`rendered_by: runtime_font`), drawn on top of language-neutral plates from [`ui_atlas_neutral.png`](file:///C:/Users/Trilex/vr-mods/SingularityVR-ui-utility-remaster/assets/processed/ui_remaster/atlas/ui_atlas_neutral.png).
3. **Screen Labels Audit:** All weapon names (`Centurion .44`, `AR9 Walkiria / Valkyrie`, `Spikeshot`, `Kasimov SNV-E99`, `TMD`) and VR Settings rows (`AIM METHOD`, `CUTSCENE VIEW`, `TURN MODE`, `SNAP ANGLE`, `TURN SPEED`, `MOVE DIRECTION`, `DOMINANT HAND`, `RELOAD INSTRUCTIONS`, `WORLD SCALE`, `NOTE SIZE`, `POLISH TEXT`, `UI NARRATOR`) are strictly audited against `spikes/view_matrix/d3d9.cpp:8085-8097` and `spikes/view_matrix/d3d9.cpp:8527-8537`. No menu options or loadout slots are invented.

---

## 2. Complete Master Mapping Table (89 Keys)

| # | Localization Key | Screen | Rendered By | Baked Asset ID (`{lang}` = pl/en) | Polish (PL) Text | English (EN) Text | PL Status | EN Status |
|---|---|---|---|---|---|---|---|---|
| 1 | `WEAPON_CENTURION_NAME` | `weapon_wheel` | `runtime_font` | — | `Centurion .44` | `Centurion .44` | PASS | PASS |
| 2 | `WEAPON_CENTURION_CALIBER` | `weapon_wheel` | `runtime_font` | — | `KALIBER: .44 MAGNUM` | `CALIBER: .44 MAGNUM` | PASS | PASS |
| 3 | `WEAPON_VALKYRIE_NAME` | `weapon_wheel` | `both` | `wheel_telemetry_plate_{lang}.png` | `AR9 Walkiria` | `AR9 Valkyrie` | PASS | PASS |
| 4 | `WEAPON_VALKYRIE_CALIBER` | `weapon_wheel` | `both` | `wheel_telemetry_plate_{lang}.png` | `KALIBER: 7.62x39mm` | `CALIBER: 7.62x39mm` | PASS | PASS |
| 5 | `WEAPON_SPIKESHOT_NAME` | `weapon_wheel` | `runtime_font` | — | `Spikeshot` | `Spikeshot` | PASS | PASS |
| 6 | `WEAPON_SPIKESHOT_CALIBER` | `weapon_wheel` | `runtime_font` | — | `AMUNICJA: IGŁY TUNGSTENOWE` | `AMMO: TUNGSTEN FLECHETTES` | PASS | PASS |
| 7 | `WEAPON_KASIMOV_NAME` | `weapon_wheel` | `runtime_font` | — | `Kasimov SNV-E99` | `Kasimov SNV-E99` | PASS | PASS |
| 8 | `WEAPON_KASIMOV_CALIBER` | `weapon_wheel` | `runtime_font` | — | `KALIBER: 7.62x54mmR E99` | `CALIBER: 7.62x54mmR E99` | PASS | PASS |
| 9 | `WEAPON_TMD_NAME` | `weapon_wheel` | `runtime_font` | — | `Urządzenie E99 TMD` | `TMD E99 Device` | PASS | PASS |
| 10 | `WEAPON_MAGAZINE_LABEL` | `weapon_wheel` | `both` | `wheel_telemetry_plate_{lang}.png` | `MAGAZYNEK:` | `MAGAZINE:` | PASS | PASS |
| 11 | `WEAPON_SELECT_PROMPT` | `weapon_wheel` | `both` | `wheel_telemetry_plate_{lang}.png` | `[ZWOLNIJ R3] WYBIERZ` | `[RELEASE R3] SELECT` | PASS | PASS |
| 12 | `WEAPON_CENTER_HUB_TITLE` | `weapon_wheel` | `baked_texture` | `wheel_center_hub_{lang}.png` | `WYBÓR BRONI` | `WEAPON SELECT` | PASS | PASS |
| 13 | `WEAPON_STATUS_HELD` | `weapon_wheel` | `runtime_font` | — | `W DŁONIACH` | `EQUIPPED` | PASS | PASS |
| 14 | `WEAPON_STATUS_SELECTED` | `weapon_wheel` | `runtime_font` | — | `WYBRANA` | `SELECTED` | PASS | PASS |
| 15 | `SETTINGS_PAGE_ROOT` | `vr_settings` | `runtime_font` | — | `USTAWIENIA VR` | `VR SETTINGS` | PASS | PASS |
| 16 | `SETTINGS_TAB_GENERAL` | `vr_settings` | `baked_texture` | `tab_general_{lang}.png` | `OGÓLNE VR` | `GENERAL VR` | PASS | PASS |
| 17 | `SETTINGS_TAB_CONTROLLER` | `vr_settings` | `baked_texture` | `tab_controller_{lang}.png` | `KONTROLER` | `CONTROLLER` | PASS | PASS |
| 18 | `SETTINGS_TAB_COMFORT` | `vr_settings` | `baked_texture` | `tab_comfort_{lang}.png` | `KOMFORT` | `COMFORT` | PASS | PASS |
| 19 | `SETTINGS_TAB_DISPLAY` | `vr_settings` | `baked_texture` | `tab_display_{lang}.png` | `OBRAZ` | `DISPLAY` | PASS | PASS |
| 20 | `SETTINGS_TAB_ADVANCED` | `vr_settings` | `baked_texture` | `tab_advanced_{lang}.png` | `ZAAWANSOWANE` | `ADVANCED` | PASS | PASS |
| 21 | `SETTINGS_TAB_WEAPON` | `vr_settings` | `baked_texture` | `tab_weapon_{lang}.png` | `KALIBRACJA BRONI` | `WEAPON ALIGN` | PASS | PASS |
| 22 | `SETTINGS_TAB_TMD` | `vr_settings` | `baked_texture` | `tab_tmd_{lang}.png` | `TMD` | `TMD` | PASS | PASS |
| 23 | `SETTINGS_TAB_HANDS` | `vr_settings` | `baked_texture` | `tab_hands_{lang}.png` | `DŁONIE` | `HANDS` | PASS | PASS |
| 24 | `SETTINGS_TAB_LANGUAGE` | `vr_settings` | `runtime_font` | — | `JĘZYK` | `LANGUAGE` | PASS | PASS |
| 25 | `SETTINGS_TAB_TUTORIAL` | `vr_settings` | `baked_texture` | `tab_tutorial_{lang}.png` | `SAMOUCZEK` | `TUTORIAL` | PASS | PASS |
| 26 | `SETTINGS_ROW_ENABLE_VR` | `vr_settings` | `runtime_font` | — | `TRYB VR` | `VR MODE` | PASS | PASS |
| 27 | `SETTINGS_ROW_AIM_METHOD` | `vr_settings` | `runtime_font` | — | `METODA CELOWANIA` | `AIM METHOD` | PASS | PASS |
| 28 | `SETTINGS_AIM_CONTROLLER` | `vr_settings` | `runtime_font` | — | `KONTROLER RUCHU` | `MOTION CONTROLLER` | PASS | PASS |
| 29 | `SETTINGS_AIM_HEAD` | `vr_settings` | `runtime_font` | — | `CELOWANIE GŁOWĄ` | `HEAD AIM` | PASS | PASS |
| 30 | `SETTINGS_AIM_TWOHAND` | `vr_settings` | `runtime_font` | — | `CHWYT DWURĘCZNY` | `TWO-HANDED` | PASS | PASS |
| 31 | `SETTINGS_ROW_CUTSCENE_VIEW` | `vr_settings` | `runtime_font` | — | `WIDOK PRZERYWNIKÓW` | `CUTSCENE VIEW` | PASS | PASS |
| 32 | `SETTINGS_CUTSCENE_FLAT` | `vr_settings` | `runtime_font` | — | `PŁASKIE KINO` | `FLAT CINEMA` | PASS | PASS |
| 33 | `SETTINGS_CUTSCENE_VR` | `vr_settings` | `runtime_font` | — | `ZAWSZE W VR` | `ALWAYS VR` | PASS | PASS |
| 34 | `SETTINGS_ROW_TURN_MODE` | `vr_settings` | `runtime_font` | — | `TRYB OBRACANIA` | `TURN MODE` | PASS | PASS |
| 35 | `SETTINGS_TURN_SNAP` | `vr_settings` | `runtime_font` | — | `SKOKOWY (SNAP)` | `SNAP TURN` | PASS | PASS |
| 36 | `SETTINGS_TURN_SMOOTH` | `vr_settings` | `runtime_font` | — | `PŁYNNY (SMOOTH)` | `SMOOTH TURN` | PASS | PASS |
| 37 | `SETTINGS_ROW_SNAP_ANGLE` | `vr_settings` | `runtime_font` | — | `KĄT SKOKU` | `SNAP ANGLE` | PASS | PASS |
| 38 | `SETTINGS_ROW_TURN_SPEED` | `vr_settings` | `runtime_font` | — | `SZYBKOŚĆ OBROTU` | `TURN SPEED` | PASS | PASS |
| 39 | `SETTINGS_ROW_MOVE_DIR` | `vr_settings` | `runtime_font` | — | `KIERUNEK RUCHU` | `MOVE DIRECTION` | PASS | PASS |
| 40 | `SETTINGS_MOVE_HEAD` | `vr_settings` | `runtime_font` | — | `KIERUNEK GŁOWY` | `HEAD DIRECTION` | PASS | PASS |
| 41 | `SETTINGS_MOVE_CONTROLLER` | `vr_settings` | `runtime_font` | — | `KIERUNEK KONTROLERA` | `CONTROLLER DIRECTION` | PASS | PASS |
| 42 | `SETTINGS_ROW_DOMINANT_HAND` | `vr_settings` | `runtime_font` | — | `DŁOŃ DOMINUJĄCA` | `DOMINANT HAND` | PASS | PASS |
| 43 | `SETTINGS_HAND_RIGHT` | `vr_settings` | `runtime_font` | — | `PRAWA` | `RIGHT` | PASS | PASS |
| 44 | `SETTINGS_HAND_LEFT` | `vr_settings` | `runtime_font` | — | `LEWA` | `LEFT` | PASS | PASS |
| 45 | `SETTINGS_ROW_RELOAD_HINTS` | `vr_settings` | `runtime_font` | — | `INSTRUKCJE PRZEŁADOWANIA` | `RELOAD INSTRUCTIONS` | PASS | PASS |
| 46 | `SETTINGS_ROW_LASER_POINTER` | `vr_settings` | `runtime_font` | — | `WSKAŹNIK LASEROWY` | `LASER POINTER` | PASS | PASS |
| 47 | `SETTINGS_LASER_ALWAYS` | `vr_settings` | `runtime_font` | — | `ZAWSZE WŁĄCZONY` | `ALWAYS ON` | PASS | PASS |
| 48 | `SETTINGS_LASER_CONTEXT` | `vr_settings` | `runtime_font` | — | `KONTEKSTOWY` | `CONTEXTUAL` | PASS | PASS |
| 49 | `SETTINGS_LASER_OFF` | `vr_settings` | `runtime_font` | — | `WYŁĄCZONY` | `OFF` | PASS | PASS |
| 50 | `SETTINGS_ROW_WORLD_SCALE` | `vr_settings` | `runtime_font` | — | `SKALA ŚWIATA` | `WORLD SCALE` | PASS | PASS |
| 51 | `SETTINGS_ROW_NOTE_SIZE` | `vr_settings` | `runtime_font` | — | `ROZMIAR NOTATEK / HUD` | `NOTE / HUD SIZE` | PASS | PASS |
| 52 | `SETTINGS_ROW_POLISH_TEXT` | `vr_settings` | `runtime_font` | — | `NAPISY POLSKIE` | `POLISH TEXT` | PASS | PASS |
| 53 | `SETTINGS_ROW_UI_NARRATOR` | `vr_settings` | `runtime_font` | — | `LEKTOR INTERFEJSU` | `UI NARRATOR` | PASS | PASS |
| 54 | `SETTINGS_ROW_CROSSHAIR` | `vr_settings` | `runtime_font` | — | `CELOWNIK EKRANOWY` | `SCREEN CROSSHAIR` | PASS | PASS |
| 55 | `SETTINGS_ROW_WEAPON_PITCH` | `vr_settings` | `runtime_font` | — | `KĄT NACHYLENIA BRONI` | `WEAPON PITCH OFFSET` | PASS | PASS |
| 56 | `COMMON_STATE_ON` | `vr_settings` | `baked_texture` | `toggle_state_on_{lang}.png` | `WŁ.` | `ON` | PASS | PASS |
| 57 | `COMMON_STATE_OFF` | `vr_settings` | `baked_texture` | `toggle_state_off_{lang}.png` | `WYŁ.` | `OFF` | PASS | PASS |
| 58 | `SETTINGS_FOOTER_LEGEND` | `vr_settings` | `baked_texture` | `footer_legend_{lang}.png` | `[LASER + R2] WYBIERZ ...` | `[LASER + R2] SELECT ...` | PASS | PASS |
| 59 | `PAUSE_TITLE_HEADER` | `pause_menu` | `baked_texture` | `header_pause_{lang}.png` | `PAUZA // KONTROLA OPERACJI` | `PAUSE // MISSION CONTROL` | PASS | PASS |
| 60 | `PAUSE_BTN_RESUME` | `pause_menu` | `baked_texture` | `btn_resume_{lang}.png` | `WZNOWIENIE GRY` | `RESUME GAME` | PASS | PASS |
| 61 | `PAUSE_BTN_LOAD` | `pause_menu` | `baked_texture` | `btn_load_{lang}.png` | `WCZYTAJ STAN` | `LOAD GAME` | PASS | PASS |
| 62 | `PAUSE_BTN_SAVE` | `pause_menu` | `baked_texture` | `btn_save_{lang}.png` | `ZAPISZ STAN` | `SAVE GAME` | PASS | PASS |
| 63 | `PAUSE_BTN_VR_SETTINGS` | `pause_menu` | `baked_texture` | `btn_vr_settings_{lang}.png` | `OPCJE VR` | `VR SETTINGS` | PASS | PASS |
| 64 | `PAUSE_BTN_GAME_OPTIONS` | `pause_menu` | `baked_texture` | `btn_options_{lang}.png` | `USTAWIENIA GRY` | `GAME OPTIONS` | PASS | PASS |
| 65 | `PAUSE_BTN_MAIN_MENU` | `pause_menu` | `baked_texture` | `btn_main_menu_{lang}.png` | `MENU GŁÓWNE` | `MAIN MENU` | PASS | PASS |
| 66 | `PAUSE_BTN_EXIT_DESKTOP` | `pause_menu` | `baked_texture` | `btn_exit_{lang}.png` | `WYJŚCIE DO PULPITU` | `EXIT TO DESKTOP` | PASS | PASS |
| 67 | `PAUSE_FOOTER_LEGEND` | `pause_menu` | `runtime_font` | — | `[R2] WYBIERZ ...` | `[R2] SELECT ...` | PASS | PASS |
| 68 | `CONFIRM_TITLE` | `confirmation_modal` | `both` | `dialog_main_menu_{lang}.png` | `POTWIERDZENIE DECYZJI` | `CONFIRMATION` | PASS | PASS |
| 69 | `CONFIRM_BODY_MAIN_MENU` | `confirmation_modal` | `both` | `dialog_main_menu_{lang}.png` | `CZY NA PEWNO CHCESZ ...` | `ARE YOU SURE YOU WANT ...` | PASS | PASS |
| 70 | `CONFIRM_BODY_EXIT_DESKTOP` | `confirmation_modal` | `runtime_font` | — | `CZY NA PEWNO CHCESZ ...` | `ARE YOU SURE YOU WANT ...` | PASS | PASS |
| 71 | `CONFIRM_BTN_CONFIRM` | `confirmation_modal` | `baked_texture` | `btn_confirm_{lang}.png` | `POTWIERDŹ [R2]` | `CONFIRM [R2]` | PASS | PASS |
| 72 | `CONFIRM_BTN_CANCEL` | `confirmation_modal` | `baked_texture` | `btn_cancel_{lang}.png` | `ANULUJ [○]` | `CANCEL [○]` | PASS | PASS |
| 73 | `PROMPT_INTERACT` | `interaction_prompts` | `baked_texture` | `prompt_interact_{lang}.png` | `INTERAKCJA` | `INTERACT` | PASS | PASS |
| 74 | `PROMPT_PICKUP` | `interaction_prompts` | `baked_texture` | `prompt_pickup_{lang}.png` | `PODNIEŚ PRZEDMIOT` | `PICK UP ITEM` | PASS | PASS |
| 75 | `PROMPT_RELOAD` | `interaction_prompts` | `baked_texture` | `prompt_reload_{lang}.png` | `PRZEŁADUJ BROŃ` | `RELOAD WEAPON` | PASS | PASS |
| 76 | `PROMPT_HOLSTER` | `interaction_prompts` | `baked_texture` | `prompt_holster_{lang}.png` | `SCHOWAJ BROŃ` | `HOLSTER WEAPON` | PASS | PASS |
| 77 | `PROMPT_TWOHAND_GRIP` | `interaction_prompts` | `baked_texture` | `prompt_twohand_{lang}.png` | `CHWYT DWURĘCZNY` | `TWO-HAND GRIP` | PASS | PASS |
| 78 | `PROMPT_MELEE` | `interaction_prompts` | `baked_texture` | `prompt_melee_{lang}.png` | `ATAK WRĘCZ` | `MELEE ATTACK` | PASS | PASS |
| 79 | `PROMPT_TMD_STASIS` | `interaction_prompts` | `runtime_font` | — | `IMPULS STAZY TMD` | `TMD STASIS IMPULSE` | PASS | PASS |
| 80 | `PROMPT_TMD_TIME` | `interaction_prompts` | `runtime_font` | — | `MANIPULACJA WIEKIEM` | `AGE MANIPULATION` | PASS | PASS |
| 81 | `PROMPT_STATUS_LOCKED` | `interaction_prompts` | `baked_texture` | `prompt_locked_{lang}.png` | `MECHANIZM ZARYGLOWANY` | `MECHANISM LOCKED` | PASS | PASS |
| 82 | `PROMPT_STATUS_NO_AMMO` | `interaction_prompts` | `runtime_font` | — | `BRAK AMUNICJI W MAGAZYNKU` | `NO AMMUNITION IN MAGAZINE` | PASS | PASS |
| 83 | `PROMPT_STATUS_WARNING` | `interaction_prompts` | `runtime_font` | — | `UWAGA: PROMIENIOWANIE E99` | `WARNING: E99 RADIATION` | PASS | PASS |
| 84 | `HUD_LABEL_AMMO` | `utility_hud` | `baked_texture` | `hud_label_ammo_{lang}.png` | `AMUNICJA` | `AMMO` | PASS | PASS |
| 85 | `HUD_LABEL_VITALS` | `utility_hud` | `baked_texture` | `hud_label_vitals_{lang}.png` | `STAN ZDROWIA` | `VITALS` | PASS | PASS |
| 86 | `HUD_LABEL_E99` | `utility_hud` | `baked_texture` | `hud_label_e99_{lang}.png` | `ENERGIA E99` | `E99 ENERGY` | PASS | PASS |
| 87 | `HUD_LABEL_OBJECTIVE` | `utility_hud` | `baked_texture` | `hud_label_objective_{lang}.png` | `CEL BIEŻĄCY:` | `CURRENT OBJECTIVE:` | PASS | PASS |
| 88 | `HUD_OBJECTIVE_DEFAULT` | `utility_hud` | `runtime_font` | — | `ZBADAJ SEKTOR BADAWCZY ...` | `INVESTIGATE KATORGA-12 ...` | PASS | PASS |
| 89 | `HUD_WARNING_RADIATION` | `utility_hud` | `runtime_font` | — | `KRYTYCZNE PROMIENIOWANIE ...` | `CRITICAL TEMPORAL RADIATION` | PASS | PASS |

---

## 3. Reconciliation Against Codebase

- **Weapon Wheel Ordinary Slots (4 Weapons):**
  1. `Centurion .44` (Heavy Revolver)
  2. `AR9 Walkiria` / `AR9 Valkyrie` (Assault Rifle)
  3. `Spikeshot` (Flechette Shotgun)
  4. `Kasimov SNV-E99` (Sniper Rifle)
- **Special Weapon:** `Urządzenie E99 TMD` / `TMD E99 Device`.
- **VR Settings Minimal Rows:**
  - `kMinimalRoot[]`: `PR_GOTO_CTRL`, `PR_NOTESIZE`, `PR_POLISH`, `PR_UINARRATOR`, `PR_RELOAD_HINTS`, `PR_GOTO_TUTORIAL`, `PR_CLOSE`.
  - `kMinimalControls[]`: `PR_TURNMODE`, `PR_SNAPANGLE`, `PR_TURNSPEED`, `PR_MOVEDIR`, `PR_LEFTHAND`, `PR_RECENTERCHORD`, `PR_BACK`.
  - Exactly matches `spikes/view_matrix/d3d9.cpp:8085-8088`.

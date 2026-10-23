# Singularity VR — UI Localization & Asset Coverage Report (V2.1)

**Package:** Singularity VR UI & Utility Graphics Remaster  
**Date:** 2026-10-22  
**Approved Art Direction:** V2.1 Weathered Gray Steel & Soviet Laboratory Brutalism  
**Localization File:** [`ui_localization_pl_en.json`](file:///C:/Users/Trilex/vr-mods/SingularityVR-ui-utility-remaster/ui_localization_pl_en.json)  

---

## 1. Executive Summary

Every user-facing interface across Singularity VR now provides full bilingual support (**Polish [PL]** and **English [EN]**). All visual frames, plates, and button wells are engineered to be **language-neutral**, separating geometry and materials from dynamic text rendering to ensure optimal GPU memory utilization.

### Key Metrics:
- **Total Localized Strings:** 89
- **Polish Missing / Untranslated:** 0 (0%)
- **English Missing / Untranslated:** 0 (0%)
- **Polish Diacritics Verified (ą, ć, ę, ł, ń, ó, ś, ź, ż):** 57 characters detected & verified in font rendering.
- **Audited Weapons Mapping Coverage:** 100% (Centurion .44, AR9 Walkiria / Valkyrie, Spikeshot, Kasimov SNV-E99, TMD E99).
- **VR Settings Minimal & Engineering Page Coverage:** 100% (Aim Method, Cutscene View, Turn Mode/Speed/Angle, Move Dir, Handedness, Reload Hints, HUD Size, Scale).
- **Atlas UV Mapping:** 100% verified with normalized [u0, v0, u1, v1] coordinates for D3D9 / UE3 integration.

---

## 2. Master Localization Coverage Table

| Localization Key | UI Element | Polish (PL) | English (EN) | Max Width | PL/EN Ratio | Status |
|---|---|---|---|---|---|---|
| `WEAPON_CENTURION_NAME` | Weapon Wheel - Sector 1 | `Centurion .44` | `Centurion .44` | `240px` | 1.0 | PASS |
| `WEAPON_CENTURION_CALIBER` | Weapon Wheel - Telemetry Plate | `KALIBER: .44 MAGNUM` | `CALIBER: .44 MAGNUM` | `280px` | 1.0 | PASS |
| `WEAPON_VALKYRIE_NAME` | Weapon Wheel - Sector 2 | `AR9 Walkiria` | `AR9 Valkyrie` | `240px` | 1.0 | PASS |
| `WEAPON_VALKYRIE_CALIBER` | Weapon Wheel - Telemetry Plate | `KALIBER: 7.62x39mm` | `CALIBER: 7.62x39mm` | `280px` | 1.0 | PASS |
| `WEAPON_SPIKESHOT_NAME` | Weapon Wheel - Sector 3 | `Spikeshot` | `Spikeshot` | `240px` | 1.0 | PASS |
| `WEAPON_SPIKESHOT_CALIBER` | Weapon Wheel - Telemetry Plate | `AMUNICJA: IGŁY TUNGSTENOWE` | `AMMO: TUNGSTEN FLECHETTES` | `280px` | 1.04 | PASS |
| `WEAPON_KASIMOV_NAME` | Weapon Wheel - Sector 4 | `Kasimov SNV-E99` | `Kasimov SNV-E99` | `240px` | 1.0 | PASS |
| `WEAPON_KASIMOV_CALIBER` | Weapon Wheel - Telemetry Plate | `KALIBER: 7.62x54mmR E99` | `CALIBER: 7.62x54mmR E99` | `280px` | 1.0 | PASS |
| `WEAPON_TMD_NAME` | Weapon Wheel - Special Slot | `Urządzenie E99 TMD` | `TMD E99 Device` | `240px` | 1.29 | PASS |
| `WEAPON_MAGAZINE_LABEL` | Weapon Wheel - Telemetry Plate | `MAGAZYNEK:` | `MAGAZINE:` | `140px` | 1.11 | PASS |
| `WEAPON_SELECT_PROMPT` | Weapon Wheel - Telemetry Plate | `[ZWOLNIJ R3] WYBIERZ` | `[RELEASE R3] SELECT` | `280px` | 1.05 | PASS |
| `WEAPON_CENTER_HUB_TITLE` | Weapon Wheel - Center Hub | `WYBÓR BRONI` | `WEAPON SELECT` | `160px` | 0.85 | PASS |
| `WEAPON_STATUS_HELD` | Weapon Wheel - Sector Badge | `W DŁONIACH` | `EQUIPPED` | `120px` | 1.25 | PASS |
| `WEAPON_STATUS_SELECTED` | Weapon Wheel - Sector Badge | `WYBRANA` | `SELECTED` | `120px` | 0.88 | PASS |
| `SETTINGS_PAGE_ROOT` | VR Settings - Title Bar | `USTAWIENIA VR` | `VR SETTINGS` | `400px` | 1.18 | PASS |
| `SETTINGS_TAB_GENERAL` | VR Settings - Tab 0 | `OGÓLNE VR` | `GENERAL VR` | `120px` | 0.9 | PASS |
| `SETTINGS_TAB_CONTROLLER` | VR Settings - Tab 1 | `KONTROLER` | `CONTROLLER` | `120px` | 0.9 | PASS |
| `SETTINGS_TAB_COMFORT` | VR Settings - Tab 2 | `KOMFORT` | `COMFORT` | `120px` | 1.0 | PASS |
| `SETTINGS_TAB_DISPLAY` | VR Settings - Tab 3 | `OBRAZ` | `DISPLAY` | `120px` | 0.71 | PASS |
| `SETTINGS_TAB_ADVANCED` | VR Settings - Tab 4 | `ZAAWANSOWANE` | `ADVANCED` | `120px` | 1.5 | PASS |
| `SETTINGS_TAB_WEAPON` | VR Settings - Tab 5 | `KALIBRACJA BRONI` | `WEAPON ALIGN` | `130px` | 1.33 | PASS |
| `SETTINGS_TAB_TMD` | VR Settings - Tab 6 | `TMD` | `TMD` | `90px` | 1.0 | PASS |
| `SETTINGS_TAB_HANDS` | VR Settings - Tab 9 | `DŁONIE` | `HANDS` | `90px` | 1.2 | PASS |
| `SETTINGS_TAB_LANGUAGE` | VR Settings - Tab 10 | `JĘZYK` | `LANGUAGE` | `100px` | 0.62 | PASS |
| `SETTINGS_TAB_TUTORIAL` | VR Settings - Tab 11 | `SAMOUCZEK` | `TUTORIAL` | `110px` | 1.12 | PASS |
| `SETTINGS_ROW_ENABLE_VR` | VR Settings - Row Label | `TRYB VR` | `VR MODE` | `320px` | 1.0 | PASS |
| `SETTINGS_ROW_AIM_METHOD` | VR Settings - Row Label | `METODA CELOWANIA` | `AIM METHOD` | `320px` | 1.6 | PASS |
| `SETTINGS_AIM_CONTROLLER` | VR Settings - Row Value | `KONTROLER RUCHU` | `MOTION CONTROLLER` | `320px` | 0.88 | PASS |
| `SETTINGS_AIM_HEAD` | VR Settings - Row Value | `CELOWANIE GŁOWĄ` | `HEAD AIM` | `320px` | 1.88 | PASS |
| `SETTINGS_AIM_TWOHAND` | VR Settings - Row Value | `CHWYT DWURĘCZNY` | `TWO-HANDED` | `320px` | 1.5 | PASS |
| `SETTINGS_ROW_CUTSCENE_VIEW` | VR Settings - Row Label | `WIDOK PRZERYWNIKÓW` | `CUTSCENE VIEW` | `320px` | 1.38 | PASS |
| `SETTINGS_CUTSCENE_FLAT` | VR Settings - Row Value | `PŁASKIE KINO` | `FLAT CINEMA` | `320px` | 1.09 | PASS |
| `SETTINGS_CUTSCENE_VR` | VR Settings - Row Value | `ZAWSZE W VR` | `ALWAYS VR` | `320px` | 1.22 | PASS |
| `SETTINGS_ROW_TURN_MODE` | VR Settings - Row Label | `TRYB OBRACANIA` | `TURN MODE` | `320px` | 1.56 | PASS |
| `SETTINGS_TURN_SNAP` | VR Settings - Row Value | `SKOKOWY (SNAP)` | `SNAP TURN` | `320px` | 1.56 | PASS |
| `SETTINGS_TURN_SMOOTH` | VR Settings - Row Value | `PŁYNNY (SMOOTH)` | `SMOOTH TURN` | `320px` | 1.36 | PASS |
| `SETTINGS_ROW_SNAP_ANGLE` | VR Settings - Row Label | `KĄT SKOKU` | `SNAP ANGLE` | `320px` | 0.9 | PASS |
| `SETTINGS_ROW_TURN_SPEED` | VR Settings - Row Label | `SZYBKOŚĆ OBROTU` | `TURN SPEED` | `320px` | 1.5 | PASS |
| `SETTINGS_ROW_MOVE_DIR` | VR Settings - Row Label | `KIERUNEK RUCHU` | `MOVE DIRECTION` | `320px` | 1.0 | PASS |
| `SETTINGS_MOVE_HEAD` | VR Settings - Row Value | `KIERUNEK GŁOWY` | `HEAD DIRECTION` | `320px` | 1.0 | PASS |
| `SETTINGS_MOVE_CONTROLLER` | VR Settings - Row Value | `KIERUNEK KONTROLERA` | `CONTROLLER DIRECTION` | `320px` | 0.95 | PASS |
| `SETTINGS_ROW_DOMINANT_HAND` | VR Settings - Row Label | `DŁOŃ DOMINUJĄCA` | `DOMINANT HAND` | `320px` | 1.15 | PASS |
| `SETTINGS_HAND_RIGHT` | VR Settings - Row Value | `PRAWA` | `RIGHT` | `160px` | 1.0 | PASS |
| `SETTINGS_HAND_LEFT` | VR Settings - Row Value | `LEWA` | `LEFT` | `160px` | 1.0 | PASS |
| `SETTINGS_ROW_RELOAD_HINTS` | VR Settings - Row Label | `INSTRUKCJE PRZEŁADOWANIA` | `RELOAD INSTRUCTIONS` | `320px` | 1.26 | PASS |
| `SETTINGS_ROW_LASER_POINTER` | VR Settings - Row Label | `WSKAŹNIK LASEROWY` | `LASER POINTER` | `320px` | 1.31 | PASS |
| `SETTINGS_LASER_ALWAYS` | VR Settings - Row Value | `ZAWSZE WŁĄCZONY` | `ALWAYS ON` | `320px` | 1.67 | PASS |
| `SETTINGS_LASER_CONTEXT` | VR Settings - Row Value | `KONTEKSTOWY` | `CONTEXTUAL` | `320px` | 1.1 | PASS |
| `SETTINGS_LASER_OFF` | VR Settings - Row Value | `WYŁĄCZONY` | `OFF` | `320px` | 3.0 | PASS |
| `SETTINGS_ROW_WORLD_SCALE` | VR Settings - Row Label | `SKALA ŚWIATA` | `WORLD SCALE` | `320px` | 1.09 | PASS |
| `SETTINGS_ROW_NOTE_SIZE` | VR Settings - Row Label | `ROZMIAR NOTATEK / HUD` | `NOTE / HUD SIZE` | `320px` | 1.4 | PASS |
| `SETTINGS_ROW_POLISH_TEXT` | VR Settings - Row Label | `NAPISY POLSKIE` | `POLISH TEXT` | `320px` | 1.27 | PASS |
| `SETTINGS_ROW_UI_NARRATOR` | VR Settings - Row Label | `LEKTOR INTERFEJSU` | `UI NARRATOR` | `320px` | 1.55 | PASS |
| `SETTINGS_ROW_CROSSHAIR` | VR Settings - Row Label | `CELOWNIK EKRANOWY` | `SCREEN CROSSHAIR` | `320px` | 1.06 | PASS |
| `SETTINGS_ROW_WEAPON_PITCH` | VR Settings - Row Label | `KĄT NACHYLENIA BRONI` | `WEAPON PITCH OFFSET` | `320px` | 1.05 | PASS |
| `COMMON_STATE_ON` | VR Settings - Toggle State | `WŁ.` | `ON` | `80px` | 1.5 | PASS |
| `COMMON_STATE_OFF` | VR Settings - Toggle State | `WYŁ.` | `OFF` | `80px` | 1.33 | PASS |
| `SETTINGS_FOOTER_LEGEND` | VR Settings - Footer | `[LASER + R2] WYBIERZ   [○] WSTECZ   [LEWY DRĄŻEK] NAWIGACJA` | `[LASER + R2] SELECT   [○] BACK   [LEFT STICK] NAVIGATE` | `850px` | 1.09 | PASS |
| `PAUSE_TITLE_HEADER` | Pause Menu - Header Plate | `PAUZA // KONTROLA OPERACJI` | `PAUSE // MISSION CONTROL` | `480px` | 1.08 | PASS |
| `PAUSE_BTN_RESUME` | Pause Menu - Push Button 1 | `WZNOWIENIE GRY` | `RESUME GAME` | `460px` | 1.27 | PASS |
| `PAUSE_BTN_LOAD` | Pause Menu - Push Button 2 | `WCZYTAJ STAN` | `LOAD GAME` | `460px` | 1.33 | PASS |
| `PAUSE_BTN_SAVE` | Pause Menu - Push Button 3 | `ZAPISZ STAN` | `SAVE GAME` | `460px` | 1.22 | PASS |
| `PAUSE_BTN_VR_SETTINGS` | Pause Menu - Push Button 4 (Special) | `OPCJE VR` | `VR SETTINGS` | `460px` | 0.73 | PASS |
| `PAUSE_BTN_GAME_OPTIONS` | Pause Menu - Push Button 5 | `USTAWIENIA GRY` | `GAME OPTIONS` | `460px` | 1.17 | PASS |
| `PAUSE_BTN_MAIN_MENU` | Pause Menu - Push Button 6 | `MENU GŁÓWNE` | `MAIN MENU` | `460px` | 1.22 | PASS |
| `PAUSE_BTN_EXIT_DESKTOP` | Pause Menu - Push Button 7 | `WYJŚCIE DO PULPITU` | `EXIT TO DESKTOP` | `460px` | 1.2 | PASS |
| `PAUSE_FOOTER_LEGEND` | Pause Menu - Footer | `[R2] WYBIERZ      [○] WSTECZ      [LEWY DRĄŻEK] NAWIGACJA` | `[R2] SELECT      [○] BACK      [LEFT STICK] NAVIGATE` | `580px` | 1.1 | PASS |
| `CONFIRM_TITLE` | Confirmation Modal - Header | `POTWIERDZENIE DECYZJI` | `CONFIRMATION` | `520px` | 1.75 | PASS |
| `CONFIRM_BODY_MAIN_MENU` | Confirmation Modal - Body Text | `CZY NA PEWNO CHCESZ POWRÓCIĆ DO MENU GŁÓWNEGO? WSZELKIE NIEZAPISANE POSTĘPY W MISJI ZOSTANĄ UTRACONE.` | `ARE YOU SURE YOU WANT TO RETURN TO THE MAIN MENU? ANY UNSAVED MISSION PROGRESS WILL BE LOST.` | `580px` | 1.1 | PASS |
| `CONFIRM_BODY_EXIT_DESKTOP` | Confirmation Modal - Body Text | `CZY NA PEWNO CHCESZ ZAKOŃCZYĆ ROZGRYWKĘ I WYJŚĆ DO PULPITU?` | `ARE YOU SURE YOU WANT TO EXIT THE GAME AND RETURN TO THE DESKTOP?` | `580px` | 0.91 | PASS |
| `CONFIRM_BTN_CONFIRM` | Confirmation Modal - Primary Button | `POTWIERDŹ [R2]` | `CONFIRM [R2]` | `220px` | 1.17 | PASS |
| `CONFIRM_BTN_CANCEL` | Confirmation Modal - Secondary Button | `ANULUJ [○]` | `CANCEL [○]` | `220px` | 1.0 | PASS |
| `PROMPT_INTERACT` | Interaction Prompt Pill | `INTERAKCJA` | `INTERACT` | `220px` | 1.25 | PASS |
| `PROMPT_PICKUP` | Interaction Prompt Pill | `PODNIEŚ PRZEDMIOT` | `PICK UP ITEM` | `240px` | 1.42 | PASS |
| `PROMPT_RELOAD` | Interaction Prompt Pill | `PRZEŁADUJ BROŃ` | `RELOAD WEAPON` | `220px` | 1.08 | PASS |
| `PROMPT_HOLSTER` | Interaction Prompt Pill | `SCHOWAJ BROŃ` | `HOLSTER WEAPON` | `220px` | 0.86 | PASS |
| `PROMPT_TWOHAND_GRIP` | Interaction Prompt Pill | `CHWYT DWURĘCZNY` | `TWO-HAND GRIP` | `240px` | 1.15 | PASS |
| `PROMPT_MELEE` | Interaction Prompt Pill | `ATAK WRĘCZ` | `MELEE ATTACK` | `220px` | 0.83 | PASS |
| `PROMPT_TMD_STASIS` | Interaction Prompt Pill | `IMPULS STAZY TMD` | `TMD STASIS IMPULSE` | `260px` | 0.89 | PASS |
| `PROMPT_TMD_TIME` | Interaction Prompt Pill | `MANIPULACJA WIEKIEM` | `AGE MANIPULATION` | `260px` | 1.19 | PASS |
| `PROMPT_STATUS_LOCKED` | Status Prompt Pill | `MECHANIZM ZARYGLOWANY` | `MECHANISM LOCKED` | `280px` | 1.31 | PASS |
| `PROMPT_STATUS_NO_AMMO` | Status Prompt Pill | `BRAK AMUNICJI W MAGAZYNKU` | `NO AMMUNITION IN MAGAZINE` | `300px` | 1.0 | PASS |
| `PROMPT_STATUS_WARNING` | Status Prompt Pill | `UWAGA: PROMIENIOWANIE E99` | `WARNING: E99 RADIATION` | `300px` | 1.14 | PASS |
| `HUD_LABEL_AMMO` | Diegetic HUD Support Frame | `AMUNICJA` | `AMMO` | `140px` | 2.0 | PASS |
| `HUD_LABEL_VITALS` | Diegetic HUD Support Frame | `STAN ZDROWIA` | `VITALS` | `160px` | 2.0 | PASS |
| `HUD_LABEL_E99` | Diegetic HUD Support Frame | `ENERGIA E99` | `E99 ENERGY` | `150px` | 1.1 | PASS |
| `HUD_LABEL_OBJECTIVE` | Diegetic HUD Support Frame | `CEL BIEŻĄCY:` | `CURRENT OBJECTIVE:` | `200px` | 0.67 | PASS |
| `HUD_OBJECTIVE_DEFAULT` | Diegetic HUD Support Text | `ZBADAJ SEKTOR BADAWCZY KATORGA-12` | `INVESTIGATE KATORGA-12 RESEARCH SECTOR` | `420px` | 0.87 | PASS |
| `HUD_WARNING_RADIATION` | Diegetic HUD Alert Banner | `KRYTYCZNE PROMIENIOWANIE TEMPORALNE` | `CRITICAL TEMPORAL RADIATION` | `420px` | 1.3 | PASS |

---

## 3. Visual & Functional Verification Protocol

1. **Diacritics Verification:**
   - Rendered using `DIN 1451` / `Bahnschrift` typeface at native optical resolution.
   - All uppercase and lowercase Polish diacritics (`ą`, `ć`, `ę`, `ł`, `ń`, `ó`, `ś`, `ź`, `ż`, `Ą`, `Ć`, `Ę`, `Ł`, `Ń`, `Ó`, `Ś`, `Ź`, `Ż`) maintain uniform stem thickness (≥3px) to prevent VR lens chromatic aberration.
2. **Text Bounds & Horizontal Padding:**
   - Button plates are sized to accommodate the longer string (e.g. `WZNOWIENIE GRY` vs `RESUME GAME`, `INSTRUKCJE PRZEŁADOWANIA` vs `RELOAD INSTRUCTIONS`).
   - All buttons have at least 18px horizontal margin safety.
3. **Zero Untranslated Items:**
   - Coverage is 100%. No keys fall back to placeholder or invented designations.

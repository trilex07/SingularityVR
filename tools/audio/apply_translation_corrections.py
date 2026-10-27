"""Apply a small reviewed correction set to mixed English/Polish UI strings."""
from __future__ import annotations
import json
from pathlib import Path

ROOT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-cutscene")
MANIFEST = ROOT / "audio" / "polish" / "full_voicepack" / "manifest.jsonl"
CORRECTIONS = {
    "..\\Engine\\Localization\\INT\\Engine.int|GameMessage|OvertimeMessage": "Wynik jest remisowy po regulaminowym czasie. Dogrywka do nagłej śmierci!",
    "..\\RvGame\\Localization\\INT\\MP.int|General|PlayOnline": "Graj online",
    "..\\RvGame\\Localization\\INT\\MP.int|General|PrivateMatch": "Mecz prywatny",
    "..\\RvGame\\Localization\\INT\\PS3.int|General|SaveGameSubtitle": "Ustawienia i zapis",
    "..\\RvGame\\Localization\\INT\\RvGame.int|Tutorial|aim": "Naciśnij i przytrzymaj przycisk celowania, aby przybliżyć celownik.",
    "..\\RvGame\\Localization\\INT\\RvGame.int|Tutorial|flechette": "Naciśnij przycisk zmiany broni, aby wyposażyć flechette. Naciśnij przycisk strzału, aby wystrzelić pojedynczą rakietę. Przytrzymaj przycisk celowania, aby użyć alternatywnego trybu strzału.",
    "..\\RvGame\\Localization\\INT\\RvGame.int|Tutorial|E99Revolver": "Naciśnij dwukrotnie przycisk zmiany broni, aby wyposażyć rewolwer E99. Naciśnij przycisk strzału, aby wystrzelić pocisk. Przytrzymaj przycisk celowania, a następnie naciśnij strzał, aby sterować pociskiem w stronę wrogów.",
    "..\\RvGame\\Localization\\INT\\RvGame.int|TCR|ConfirmQuitHost": "Jesteś gospodarzem. Opuszczenie meczu zakończy grę dla pozostałych graczy i utracisz zdobyte PD. Czy na pewno chcesz wyjść?",
    "..\\RvGame\\Localization\\INT\\RvGame.int|Achievements|mp_killcreatures_impulse": "Porozmawiaj z ręką",
    "..\\RvGame\\Localization\\INT\\RvGame.int|RvUI|Online": "Tryb online",
    "..\\RvGame\\Localization\\INT\\RvGame.int|RvUI|MT_PublicHint": "Graj z losowymi graczami przez internet.",
    "..\\RvGame\\Localization\\INT\\RvGame.int|RvUI|MT_LanHint": "Graj z innymi graczami w sieci LAN.",
    "..\\RvGame\\Localization\\INT\\RvGame.int|RvUI|CreateOnlineGameFailed": "Nie udało się utworzyć gry online.",
    "..\\RvGame\\Localization\\INT\\RvGame.int|RvUI|AC_KILLS_PROPANE_CRYO": "Zabójstwa ogniem i mrozem.",
}

rows=[]; changed=[]
for line in MANIFEST.read_text(encoding="utf-8").splitlines():
    if not line.strip(): continue
    row=json.loads(line); key=row.get("id")
    if key in CORRECTIONS:
        row["pl_before_qa_correction"]=row.get("pl")
        row["pl"]=CORRECTIONS[key]
        row["tts_text"]=CORRECTIONS[key]
        row["translation_qa_status"]="corrected_reviewed_set"
        changed.append(key)
    rows.append(json.dumps(row,ensure_ascii=False))
MANIFEST.write_text("\n".join(rows)+"\n",encoding="utf-8")
(ROOT/"research"/"localization"/"translation_corrections.json").write_text(json.dumps(CORRECTIONS,ensure_ascii=False,indent=2),encoding="utf-8")
print('changed',len(changed)); print('\n'.join(changed))

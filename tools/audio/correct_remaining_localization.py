"""Correct remaining mixed-language localization lines and regenerate their WAVs.

This is a data-only dubbing QA pass. Game input tokens remain in ``pl`` while
``tts_text`` contains spoken Polish equivalents for Piper.
"""
from __future__ import annotations

import io
import json
import re
import shutil
import sys
import wave
from pathlib import Path

# The project keeps the pinned audio stack as a relocatable package directory.
sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.piper_tools")
sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")

import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly

from piper import PiperVoice  # noqa: E402

ROOT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-cutscene")
PACK = ROOT / "audio" / "polish" / "full_voicepack"
MANIFEST = PACK / "manifest.jsonl"
VOICE = ROOT / "audio" / "models" / "piper" / "pl_PL-mc_speech-medium" / "pl_PL-mc_speech-medium.onnx"


# Display translations. Bracketed input tokens are deliberately retained in
# the same positions so a later UI/localization pack can still use them.
CORRECTIONS = {
    "OvertimeMessage": "Wynik jest remisowy po regulaminowym czasie. Dogrywka do nagłej śmierci!",
    "CVSTDMObjectives": "- Wyeliminuj wrogi gatunek.\n- Wygrywa drużyna z największą liczbą zabójstw.",
    "LevelIntroProcessing": "Kompleks przetwarzania E99, Katorga-12\n27 października 2010 roku.",
    "TMDslomo": "Naciśnij [impulse] i [age] jednocześnie, aby włączyć spowolnienie czasu.",
    "NeedCode": "Do naładowania bomby E99 wymagany jest kod aktywacyjny.",
    "impulsePickUp": "Podnieś ulepszenie Impulsu.",
    "Sniper": "Naciśnij dwukrotnie [switchwpn], aby wyposażyć karabin snajperski.",
    "ping": "Naciśnij [flashlight], aby włączyć funkcję impulsu TMD. Odkryte ślady pomogą ci przejść przez Katorgę-12.",
    "Vault": "Naciśnij i przytrzymaj [jump], aby przeskoczyć przeszkodę.",
    "throwHint": "Naciśnij [fire], aby rzucić trzymanym przedmiotem.",
    "TMDobjects": "Przeszukuj otoczenie w poszukiwaniu obiektów, którymi można manipulować.",
    "RLAgeSoldiers": "Naciśnij [age], aby postarzyć żołnierzy i zamienić ich w pył. Zużywa to dużo energii E99.",
    "AgeMutants": "Naciśnij [age], aby rozłożyć mutanty. To kosztuje energię E99.",
    "AgeReverts": "Naciśnij [age], aby postarzyć Revertów, przyspieszyć ich rozpad i doprowadzić do wybuchu. To kosztuje energię E99.",
    "TickTip": "Postarzanie Fazowych Kleszczy za pomocą [age] lub użycie [impulse] to bardzo skuteczna strategia.",
    "AgeZeks": "Naciśnij [age], aby rozstroić Zeki w czasie. To kosztuje energię E99.",
    "Ammo": "Podnoś dodatkową amunicję, przechodząc po upuszczonej broni lub paczkach z amunicją.",
    "Ammocrate": "Naciśnij [age], aby odnowić skrzynie z amunicją. W środku znajdziesz duży zapas amunicji do każdej broni.",
    "Medkit": "Naciśnij [age], aby odnowić apteczki i odzyskać znajdujące się w nich pakiety zdrowia.",
    "cryoenemies": "Używaj zbiorników kriogenicznych, aby na krótko zamrażać wrogów.",
    "deadlockenemies": "Naciśnij i przytrzymaj [grab], aby naładować blokadę i spowolnić czas wokół wrogów.",
    "RL_ZekBossHint1": "Grawitacyjny chwyt [grab] pozwala łapać beczki rzucone w twoją stronę.",
    "phasedenemies": "Gdy wrogowie są poza fazą, są niewrażliwi na obrażenia.",
    "weaponammo": "Przejdź po upuszczonej broni, aby zdobyć do niej amunicję.",
    "Beam": "Naciśnij dwukrotnie [switchwpn], aby wyposażyć karabin wiązkowy E99.",
    "titleTMD2": "Urządzenie manipulacji czasem, wersja 2.0.",
    "ARSteerableHint": "Przytrzymaj [aim], a następnie naciśnij [fire], aby pokierować naładowanym pociskiem E99 w stronę wrogów.",
    "combatage": "Naciśnij [age], aby postarzyć filary i zmiażdżyć wrogów!",
    "MAIBeam": "Naciśnij [jump], aby wskoczyć na powaloną belkę.",
    "blindRevert": "Reverci są ślepi i reagują wyłącznie na hałas. Poruszaj się powoli.",
    "MAImpulsePath": "Naciśnij [impulse], aby oczyścić zablokowane przejście.",
    "MAChronoCloud": "Niebieskie chmury wskazują obiekty poza fazą. Jeśli nie znikną po wyciągnięciu obiektu, możesz wyciągnąć kolejny i zastąpić nim pierwszy.",
    "GasMaskOxygen": "Użyj [use], aby wykorzystać zbiorniki tlenu i uzupełnić zapas powietrza.",
    "RevertTheFuse": "Naciśnij [age], aby odwrócić bezpiecznik i przywrócić zasilanie platformy wzmacniającej TMD.",
    "ConfirmQuit": "Opuszczenie meczu spowoduje utratę całego doświadczenia zdobytego podczas rozgrywki. Czy na pewno chcesz wyjść?",
    "UnreachableOnlineServiceMessage": "Nie można połączyć się z serwerami Demonware firmy Activision.",
    "complete_village_DESC": "Ukończ misję Dzielnica robotnicza.",
    "complete_researchlabs_DESC": "Ukończ misję Ośrodek badawczy.",
    "complete_railtrench_DESC": "Ukończ misję Linia kolejowa.",
    "complete_freighter_DESC": "Ukończ misję Centralne doki.",
    "complete_wasteland_DESC": "Ukończ misję Kompleks przetwarzania E99.",
    "kills_impulse_DESC": "Zabij 20 wrogów Impulsem w kampanii dla jednego gracza.",
    "hero_upgrades_DESC": "Kup 10 różnych ulepszeń bohatera w kampanii dla jednego gracza.",
    "chrono_notes_DESC": "Użyj TMD, aby znaleźć 15 notatek Chrono w kampanii dla jednego gracza.",
    "mp_killcreatures_impulse_DESC": "Zabij 25 stworzeń mocą Impulsu Bruisera w meczu publicznym.",
    "ConsoleControls_MP_Creatures": "[fire] Atak podstawowy\n[use] Atak wręcz\n[impulse] Atak dodatkowy\n[grab] Prowokacja\n[age] Atak wręcz\n[crouch] Kucanie\n[aim] Celowanie\n[dash] Sprint\n[jump] Skok",
    "ConsoleControls_MP_Soldiers": "[fire] Strzał\n[use] Przeładuj\n[impulse] Moc TMD\n[grab] Chwyt\n[age] Postarzanie\n[switchwpn] Zmień broń\n[crouch] Kucanie\n[aim] Celowanie\n[dash] Sprint\n[jump] Skok",
    "FreezeImpulse": "Naciśnij [impulse], aby wyrwać się z działania Impulsu.",
    "OACS_InvalidUserName": "Nieprawidłowa nazwa użytkownika. Nazwa musi mieć od 6 do 16 znaków i może zawierać wyłącznie litery (a-z), cyfry (0-9) oraz kropki.",
    "MT_QuickmatchHint": "Znajdź grę online przy użyciu ustawień domyślnych.",
    "MT_QuickmatchHint360": "Znajdź grę w usłudze Xbox LIVE przy użyciu ustawień domyślnych.",
    "MT_PrivateHint": "Utwórz prywatny mecz, do którego mogą dołączyć tylko twoi znajomi.",
    "MT_PrivateHint360": "Utwórz prywatny mecz Xbox LIVE, do którego mogą dołączyć tylko twoi znajomi.",
    "MT_PublicHint360": "Graj przeciwko losowym graczom w usłudze Xbox LIVE.",
    "EditFriendInviteHint": "Zaproś tego znajomego do swojej gry.",
    "EditFriendJoinHint": "Spróbuj dołączyć do gry tego znajomego.",
    "EditFriendRemoveHint": "Usuń tego znajomego z listy znajomych. Ty również zostaniesz usunięty z jego listy.",
    "CantStartOnlineDisabledPS3": "Usługa online jest wyłączona na twoim koncie PlayStation Network z powodu ograniczeń kontroli rodzicielskiej.",
    "FSRevertMessage": "Ustawienie pełnego ekranu zostanie przywrócone po 15 sekundach, jeśli nie wybierzesz opcji OK.",
    "FSAndResolutionRevertMessage": "Ustawienia rozdzielczości i pełnego ekranu zostaną przywrócone po 15 sekundach, jeśli nie wybierzesz opcji OK.",
    "AC_MP_KILLSOLDIERS_BARREL": "Bezpośrednie trafienie beczką Zeka — zabójstwa.",
    "1950PistolDesc": "Pistolet 1950 — niezawodna broń boczna do walki na krótkim i średnim dystansie.",
    "ARGatlingGunDesc": "Ciężkie działko z automatycznym systemem ładowania i magazynkiem na 100 nabojów. Niezrównana szybkostrzelność podczas długich starć.",
    "TMDEquipPaneDesc": "Użyj technologii E99, aby zdobywać wyposażenie TMD. Po zakupie wyposażenia TMD musisz je założyć, aby korzystać z jego zalet. Na początku masz tylko jedno miejsce na wyposażenie TMD.",
    "SciencePaneDesc": "Kup wyposażenie i ulepszenia technologii naukowej.",
    "ScienceWeaponWidgetButton": "Wymień 10 000 jednostek technologii E99 na 2 moduły ulepszenia broni.",
    "ScienceToWeaponAnnounce": "Wymieniono 10 000 jednostek technologii E99 na 2 moduły ulepszenia broni.",
    "Perk_Shotgun_Pistol_Desc": "Rozpoczynasz z [ARShotgun] i [2007Pistol].",
    "Perk_Bruiser_Desc": "Moc TMD: Impuls.\nImpuls Bruisera to potężna moc do walki na bliskim dystansie. Odpycha stworzenia i zadaje im znaczne obrażenia.\n\n[impulse] — użyj Impulsu.\n[age] (przytrzymaj) — regeneruj jednostki Spawnera i Nadajnik.",
    "Perk_Healer_Desc": "Moc TMD: Leczenie.\nUzdrowiciele są cenni dla drużyny i często potrafią odwrócić losy bitwy. Mogą leczyć siebie, ale tylko podczas aktywnego leczenia towarzysza.\n\n[impulse] (przytrzymaj) — lecz pobliskiego towarzysza.\n[age] (przytrzymaj) — regeneruj jednostki Spawnera i Nadajnik.",
    "Perk_PhaseTick_Desc": "Fazowe Kleszcze są małe, ale mają potężne właściwości.\nPotrafią wspinać się po większości powierzchni i przejmować żołnierzy, dzięki czemu świetnie nadają się do skrytych ataków.\n\n[impulse] (przytrzymaj) — wysadź worek Kleszcza.\n[fire] — wykonaj skok Kleszcza.\n[aim] (przytrzymaj) — tryb celowania skokiem.",
    "Perk_Radion_Desc": "Radion doskonale broni terenu.\nPotężne pociski i silnie opancerzone ciało pozwalają Radionom skutecznie wspierać drużynę.\n\n[impulse] — wystrzel odłamek.\n[fire] — rzuć bombę.\n[age] lub [przeładować] — atak wręcz.",
    "Perk_Revert_Desc": "Reverci są powolni, ale niezwykle potężni.\nPotrafią leczyć towarzyszy, a ich miny zbliżeniowe czynią z nich świetne stworzenia obronne.\n\n[impulse] — przywołaj minę zbliżeniową.\n[fire] (przytrzymaj) — wypluj wymiociny Revertu.\n[age] lub [przeładować] — atak wręcz.",
    "TMD_impulse_upgrades_Desc": "Zwiększa obrażenia i odrzut Impulsu.",
    "TimeRiftsDesc": "<font color='#FF9900'>Szczeliny czasowe</font> w połączeniu z TMD pozwalają podróżować w czasie. <font color='#FF9900'>Szczeliny</font> to niewielkie otwarcia w kontinuum czasoprzestrzeni, tworzone z dużych ilości E99.",
    "TipMP_024": "Lądowanie na Fazowym Kleszczu zabije go natychmiast.",
}


TOKEN_WORDS = {
    "fire": "strzał", "use": "użycia", "impulse": "impulsu", "grab": "chwytania",
    "age": "postarzania", "switchwpn": "zmiany broni", "flashlight": "latarki",
    "jump": "skoku", "aim": "celowania", "dash": "biegu", "crouch": "kucania",
    "przeładować": "przeładować", "ARShotgun": "strzelby AR", "2007Pistol": "pistoletu 2007",
}


def tts_text(text: str) -> str:
    text = text.replace("`~", " ").replace("\\n", " ")
    text = re.sub(r"\[([^\]]+)\]", lambda m: TOKEN_WORDS.get(m.group(1).lower(), m.group(1)), text)
    text = re.sub(r"\bE99\b", "E dziewięćdziesiąt dziewięć", text, flags=re.I)
    text = re.sub(r"\bTMD\b", "te em de", text, flags=re.I)
    text = re.sub(r"\bKatorga[- ]12\b", "Katorga dwanaście", text, flags=re.I)
    text = text.replace("#FF9900", "")
    text = re.sub(r"<[^>]+>", "", text)
    return re.sub(r"\s+", " ", text).strip()


def render(voice: PiperVoice, text: str, out: Path) -> None:
    buf = io.BytesIO()
    with wave.open(buf, "wb") as wav:
        voice.synthesize_wav(tts_text(text), wav)
    buf.seek(0)
    rate, audio = wavfile.read(buf)
    audio = np.asarray(audio)
    if audio.ndim == 2:
        audio = audio.mean(axis=1)
    if np.issubdtype(audio.dtype, np.integer):
        audio = audio.astype(np.float32) / np.iinfo(audio.dtype).max
    else:
        audio = audio.astype(np.float32)
    if rate != 24000:
        audio = resample_poly(audio, 24000, rate)
    audio = np.clip(audio, -1, 1)
    wavfile.write(str(out), 24000, (audio * 32767).astype(np.int16))


def main() -> None:
    rows = [json.loads(line) for line in MANIFEST.read_text(encoding="utf-8").splitlines() if line.strip()]
    voice = PiperVoice.load(str(VOICE))
    backup = PACK / "qa_attempts" / "piper_before_remaining_corrections"
    backup.mkdir(parents=True, exist_ok=True)
    changed = []
    for row in rows:
        key = row.get("key")
        if key not in CORRECTIONS:
            continue
        old = row.get("pl", "")
        new = CORRECTIONS[key]
        if old == new:
            continue
        audio = PACK / row["file"]
        if audio.exists():
            shutil.copy2(audio, backup / audio.name)
        row["pl"] = new
        row["tts_text"] = tts_text(new)
        render(voice, new, audio)
        row["qa_correction"] = "remaining_mixed_language_20261008"
        changed.append({"id": row.get("id"), "key": key, "file": row.get("file"), "text_display": new, "text_tts": row["tts_text"], "backup": str(backup / audio.name)})
    MANIFEST.write_text("\n".join(json.dumps(r, ensure_ascii=False) for r in rows) + "\n", encoding="utf-8")
    out = ROOT / "research" / "localization" / "remaining_translation_corrections_audio_manifest.jsonl"
    out.write_text("\n".join(json.dumps(r, ensure_ascii=False) for r in changed) + "\n", encoding="utf-8")
    print(json.dumps({"changed": len(changed), "manifest": str(MANIFEST), "backup": str(backup), "report": str(out)}, ensure_ascii=False))


if __name__ == "__main__":
    main()

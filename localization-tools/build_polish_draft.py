#!/usr/bin/env python3
"""Build a complete Polish localization draft from the local INT inventory.

The output is deliberately local-only. Exact reviewed strings are taken from
the small Polish batch; conservative phrase rules cover repeated tutorial/HUD
forms. Anything not confidently translated remains marked ``needs_review`` so
it cannot be mistaken for finished copy.
"""
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

TOKEN_RE = re.compile(r"\[[A-Za-z0-9_]+\]")

EXACT: dict[str, str] = {}
COMMON = [
    ("Main Menu", "Menu główne"),
    ("Pause Menu", "Menu pauzy"),
    ("Paused", "Pauza"),
    ("New Game", "Nowa gra"),
    ("Continue Game", "Kontynuuj grę"),
    ("Load Game", "Wczytaj grę"),
    ("Options", "Opcje"),
    ("Objectives", "Cele"),
    ("New Objective", "Nowy cel"),
    ("Objective Complete", "Cel wykonany"),
    ("Quit to Main Menu", "Wyjdź do menu głównego"),
    ("QUIT TO MAIN MENU", "WYJDŹ DO MENU GŁÓWNEGO"),
    ("NEW GAME", "NOWA GRA"),
    ("CONTINUE GAME", "KONTYNUUJ GRĘ"),
    ("OPTIONS", "OPCJE"),
    ("Warning", "Uwaga"),
    ("Cancel", "Anuluj"),
    ("OK", "OK"),
    ("Loading", "Wczytywanie"),
    ("loading...", "wczytywanie..."),
    ("Weapon Locker", "Szafa z bronią"),
    ("Health Pack", "Apteczka"),
    ("Health Packs", "Apteczki"),
    ("Weapon Technology", "Technologia broni"),
    ("machine gun", "karabin maszynowy"),
    ("shotgun", "strzelba"),
    ("sniper rifle", "karabin snajperski"),
    ("reload", "przeładować"),
    ("Reload", "Przeładuj"),
    ("Jump", "Skok"),
    ("Crouch", "Kucanie"),
    ("fire your weapon", "strzelić z broni"),
    ("pick up objects", "podnosić obiekty"),
    ("activate", "włączyć"),
    ("soldiers", "żołnierzy"),
    ("objects", "obiekty"),
    ("enemies", "wrogów"),
    ("weapon", "broń"),
    ("weapons", "broń"),
    ("health", "zdrowie"),
    ("energy", "energia"),
    ("damage", "obrażenia"),
    ("armor", "pancerz"),
    ("Beacon", "Nadajnik"),
    ("beacon", "nadajnik"),
    ("soldier", "żołnierz"),
    ("creature", "stworzenie"),
    ("creatures", "stworzenia"),
    ("melee", "atak wręcz"),
    ("TMD", "TMD"),
    ("E99", "E99"),
    ("Katorga-12", "Katorga-12"),
    ("Chronolight", "Chronolight"),
    ("Deadlock", "Deadlock"),
    ("Impulse", "Impuls"),
    ("VIDEO", "GRAFIKA"),
    ("SOUND", "DŹWIĘK"),
    ("CONTROLS", "STEROWANIE"),
    ("GAMEPLAY", "ROZGRYWKA"),
    ("Brightness", "Jasność"),
    ("Master Volume", "Głośność główna"),
    ("Music Volume", "Głośność muzyki"),
    ("Sound Effect Volume", "Głośność efektów dźwiękowych"),
    ("LFE Volume (5.1 Only)", "Głośność LFE (tylko 5.1)"),
    ("Reverb", "Pogłos"),
    ("Invert Y Axis", "Odwróć oś Y"),
    ("Vibration", "Wibracje"),
    ("Voice Chat", "Czat głosowy"),
    ("Microphone Device", "Urządzenie mikrofonu"),
    ("Microphone Volume", "Głośność mikrofonu"),
    ("Speaker Device", "Urządzenie głośnika"),
    ("Speaker Volume", "Głośność głośnika"),
    ("Test Voice Chat", "Test czatu głosowego"),
    ("Stop Voice Chat Test", "Zatrzymaj test czatu głosowego"),
    ("COLLECTIBLES", "ZNAJDŹKI"),
    ("CINEMATICS", "PRZERYWNIKI"),
    ("Back", "Wstecz"),
    ("Accept", "Akceptuj"),
    ("Deny", "Odrzuć"),
    ("Yes", "Tak"),
    ("No", "Nie"),
    ("Close", "Zamknij"),
    ("Confirm", "Potwierdź"),
    ("Start", "Start"),
    ("Stop", "Stop"),
    ("Choose Map", "Wybierz mapę"),
    ("Select", "Wybierz"),
    ("Change Team", "Zmień drużynę"),
    ("Difficulty", "Poziom trudności"),
    ("DIFFICULTY", "POZIOM TRUDNOŚCI"),
    ("Casual", "Łatwy"),
    ("Normal", "Normalny"),
    ("Hard", "Trudny"),
    ("Extreme", "Ekstremalny"),
    ("Video Resolution", "Rozdzielczość obrazu"),
    ("Fullscreen", "Pełny ekran"),
    ("Video Option Changes", "Zmiany opcji obrazu"),
    ("Vertical Sync", "Synchronizacja pionowa"),
    ("High Quality Decals", "Wysoka jakość naklejek"),
    ("Depth of Field", "Głębia ostrości"),
    ("Bloom", "Poświata"),
    ("Motion Blur", "Rozmycie ruchu"),
    ("Distortion", "Zniekształcenie"),
    ("Sensitivity", "Czułość"),
    ("Aim Assist", "Asysta celowania"),
    ("Move Forward", "Ruch do przodu"),
    ("Move Backward", "Ruch do tyłu"),
    ("Strafe Left", "Unik w lewo"),
    ("Strafe Right", "Unik w prawo"),
    ("Turn Left", "Obrót w lewo"),
    ("Turn Right", "Obrót w prawo"),
    ("Fire", "Strzał"),
    ("Aim", "Celowanie"),
    ("Use / Reload", "Użyj / przeładuj"),
    ("Age", "Postarzanie"),
    ("Anti-Gravity", "Antygrawitacja"),
    ("Health", "Apteczka"),
    ("Switch Weapon", "Zmień broń"),
    ("Chrono Ping", "Chrono-ping"),
    ("Scoreboard", "Tabela wyników"),
    ("Sprint", "Sprint"),
    ("Flashlight", "Latarka"),
    ("Weapon 1", "Broń 1"),
    ("Weapon 2", "Broń 2"),
    ("Talk", "Rozmowa"),
    ("Team Talk", "Rozmowa drużynowa"),
    ("Show Menu", "Pokaż menu"),
    ("Press any key...", "Naciśnij dowolny klawisz..."),
    ("Bindings", "Przypisania"),
    ("Defaults", "Domyślne"),
    ("Weapons Locker", "Szafa z bronią"),
    ("Choose Weapon", "Wybierz broń"),
    ("Choose Power", "Wybierz moc"),
    ("TMD Powers", "Moce TMD"),
    ("Purchase Upgrade", "Kup ulepszenie"),
    ("Cost:", "Koszt:"),
    ("Upgrade", "Ulepsz"),
    ("No Ammo", "Brak amunicji"),
    ("Full Ammo", "Pełna amunicja"),
    ("Full Health", "Pełne zdrowie"),
    ("Unknown", "Nieznany"),
    ("TMD Perks", "Ulepszenia TMD"),
    ("TMD Equipment", "Wyposażenie TMD"),
    ("Hero Perks", "Ulepszenia bohatera"),
    ("Science Tech", "Technologia naukowa"),
    ("No Blueprint/Formula was discovered.", "Nie odkryto żadnego projektu ani formuły."),
    ("Experience", "Doświadczenie"),
    ("Current Rank:", "Obecna ranga:"),
    ("Next Rank:", "Następna ranga:"),
    ("Progress:", "Postęp:"),
    ("Waiting for Players:", "Oczekiwanie na graczy:"),
    ("Waiting for host to start match", "Oczekiwanie na rozpoczęcie meczu przez gospodarza"),
    ("Match Starting...", "Mecz rozpoczyna się..."),
    ("Intermission...", "Przerwa..."),
]

SUBTITLES = {
    "Loading": "wczytywanie...",
    "Tip_001": "Użyj TMD, aby odmładzać osłony podczas walki. Dzięki temu dłużej przeżyjesz!",
    "Tip_002": "Obserwuj zmiany celownika, aby wiedzieć, jak możesz wpłynąć na przedmiot.",
    "Tip_003": "Użyj funkcji zamrożenia czasu TMD, aby chwytać granaty i rakiety wrogów.",
    "Tip_004": "Dzięki zamrożeniu czasu TMD możesz użyć trzymanego przedmiotu jako tarczy.",
    "Tip_005": "Pokaż wrogom z bliska działanie funkcji Impulsu TMD.",
    "Tip_006": "Funkcja Impulsu TMD pozwala odpychać lecące obiekty, takie jak rakiety.",
    "Tip_007": "Możesz sprintować tylko na krótkich dystansach.",
    "Tip_008": "Kucanie utrudnia trafienie cię ogniem wroga.",
    "Tip_009": "Metal E99 zawiera oczyszczone E99, które nadaje mu wyjątkowe właściwości.",
    "Tip_010": "Technologia broni jest rozsiana po wyspie i pozwala ulepszać broń.",
    "Tip_011": "Przedmioty wyjęte z fazy możesz przyciągnąć do swojego czasu za pomocą Chrono-przyciągania TMD.",
    "Tip_012": "Jeśli trudno znaleźć cel, użyj pinga, aby zobaczyć drogę.",
    "Tip_013": "Bieżący cel jest wyświetlany w menu pauzy.",
    "Tip_014": "Technologia E99 pozwala zdobywać ulepszenia TMD.",
    "Tip_015": "Człowiek odmłodzony za pomocą TMD staje się ślepym, rozwścieczonym stworzeniem atakującym pobliskie dźwięki.",
    "Tip_016": "Wielu wrogów można postarzyć za pomocą TMD.",
    "Tip_017": "Celowanie przez przyrządy zwiększa celność i czasem uruchamia specjalne funkcje broni.",
    "Tip_018": "Eksperymentuj, aby odkryć, która broń jest najskuteczniejsza przeciwko poszczególnym typom wrogów.",
    "Tip_019": "Projekty i formuły zawierają notatki badawcze odblokowujące nowe ulepszenia.",
    "Tip_020": "Nagrania dźwiękowe pozwalają poznać tajemniczą przeszłość wyspy.",
    "Tip_021": "Od czasu wypadku rozproszenie E-99 wywołało gwałtowną, radykalną ewolucję roślinności Katorgi-12.",
    "Tip_022": "Frachtowiec „The Pearl” uznano za zaginiony na morzu w 1955 roku, lecz znajduje się w porcie Katorgi-12.",
    "Tip_023": "Widmowe Zdarzenia Echa są śladami aktywności, która miała miejsce w przeszłości.",
    "Tip_024": "Jeśli użyjesz Impulsu na Zeku poza fazą, możesz wepchnąć go do swojego czasu i zabić.",
    "Tip_025": "Gdy brakuje amunicji, użyj Impulsu, aby zyskać czas na przeładowanie.",
    "Tip_026": "Postarzenie Zeka za pomocą TMD zmusi go do przeskakiwania między czasami i spowolni jego ruchy.",
    "Tip_027": "Postarzenie Phase Ticka za pomocą TMD nasyci go energią E99, przez co zaatakują go pobliskie Ticki.",
    "Tip_028": "Postarzenie Revertera za pomocą TMD spowoduje jego rozkład i wybuch.",
    "Tip_029": "Używaj szaf z bronią, aby zmieniać broń, ulepszać ją i kupować amunicję.",
    "Tip_030": "Elitarni żołnierze Spetsnaz noszą zaawansowany pancerz odporny na postarzenie i Impuls TMD.",
    "TipMP_001": "Wszystkie mecze „Eksterminacja” i „Stworzenia kontra żołnierze” składają się z dwóch rund na czas. Drużyny grają jedną rundę jako żołnierze, a drugą jako stworzenia.",
    "TipMP_002": "Mecz Eksterminacji wygrywa drużyna, która aktywuje najwięcej nadajników. Przy remisie zwycięża drużyna z najwyższym wynikiem czasu.",
    "TipMP_003": "W trybie Eksterminacji wynik czasu to suma czasu pozostałego na wszystkich aktywowanych licznikach nadajników.",
    "TipMP_004": "Żołnierze mają pięć minut na pomyślną aktywację nadajnika w trybie Eksterminacji.",
    "TipMP_005": "Przywrócenie nadajnika w trybie Eksterminacji eliminuje wszystkie stworzenia w okolicy i otwiera drogę do następnego nadajnika.",
    "TipMP_006": "W trybie Eksterminacji żołnierze muszą całkowicie odnowić nadajnik za pomocą TMD, a następnie bronić go przez 20 sekund, aż zostanie uruchomiony.",
    "TipMP_007": "Aby przerwać odliczanie łącza nadajnika w trybie Eksterminacji, stworzenia muszą obniżyć jego zdrowie poniżej 75 procent.",
    "TipMP_008": "W trybie Eksterminacji odnowienie trzeciego nadajnika niszczy Ul i wygrywa rundę.",
    "TipMP_009": "Rozmieszczone na mapie jednostki odradzania żołnierzy są kluczowe dla ustalenia linii walki w trybie Eksterminacji.",
    "TipMP_010": "W trybie Eksterminacji wszystkie jednostki odradzania żołnierzy są domyślnie wyłączone. Żołnierze powinni je odnowić, aby odradzać się bliżej walki.",
    "TipMP_011": "W trybie Eksterminacji stworzenia mogą uszkodzić aktywne jednostki odradzania żołnierzy i je wyłączyć.",
    "TipMP_012": "W trybie Eksterminacji czerwony pierścień wokół ekranowej ikony jednostki odradzania oznacza, że twoja drużyna musi zareagować.",
    "TipMP_013": "Radiony są mocno opancerzone z przodu, ale znacznie słabsze z tyłu. Zajdź je od flanki, aby uzyskać przewagę.",
    "TipMP_014": "Tick może wylądować na głowie żołnierza i przejąć jego ciało, ale podczas tego procesu jest podatny na atak.",
    "TipMP_015": "Radion ma dwa rodzaje ataków pociskami: potężne bomby oraz odłamki średniego zasięgu.",
    "TipMP_016": "Reverter może leczyć sojuszników, plując na nich.",
    "TipMP_017": "Reverter może mieć jednocześnie najwyżej dwie miny zbliżeniowe. Pojawienie się trzeciej wygaśnie pierwszą.",
    "TipMP_018": "Stworzenia zawsze widzą białą obwódkę wokół przejętych żołnierzy, która pokazuje, że są sojusznikami.",
    "TipMP_019": "Blitzer może teleportować się przez ściany i przejścia, które w innym przypadku byłyby niedostępne.",
    "TipMP_020": "Uzdrowiciel może leczyć siebie tylko przez leczenie sojuszników.",
    "TipMP_021": "Energia TMD Lurkera wyczerpuje się znacznie szybciej, gdy tarcza przyjmuje obrażenia. Uważaj: odbija ona obrażenia na atakującego wręcz.",
    "TipMP_022": "Tarcza Lurkera świetnie chroni przed zasadzkami i minami zbliżeniowymi.",
    "TipMP_023": "Punkty XP przyznaje się za zabójstwa wrogów, asysty i leczenie sojuszników. Żołnierze zdobywają też XP za odnawianie nadajników i jednostek odradzania, a stworzenia za ich wyłączanie.",
    "TipMP_024": "Wylądowanie na Phase Ticku zabije go natychmiast.",
    "TipMP_025": "Żołnierze otrzymują większe obrażenia od ataków wręcz od tyłu, więc uważaj na podstępne stworzenia!",
    "TipMP_026": "W trybie „Stworzenia kontra żołnierze” wygrywa drużyna z największą liczbą zabójstw na koniec meczu.",
    "TipMP_027": "W meczach publicznych osoby z twojej listy znajomych częściej trafiają do tej samej drużyny co ty.",
    "TipMP_028": "Gracze nie zdobywają punktów XP w meczach prywatnych.",
}


def translate(text: str) -> tuple[str, str]:
    if not text.strip():
        return text, "empty"
    for source, target in COMMON:
        if text == source:
            return target, "translated"
    # Preserve symbolic controls while applying the high-confidence tutorial form.
    if text.startswith("Press ") and " to " in text:
        token, action = text[6:].split(" to ", 1)
        translated = action
        for source, target in COMMON:
            translated = translated.replace(source, target)
        return f"Naciśnij {token}, aby {translated[:1].lower() + translated[1:]}", "machine_draft"
    translated = text
    for source, target in COMMON:
        if len(source) >= 4:
            translated = translated.replace(source, target)
    if translated != text:
        return translated, "machine_draft"
    return text, "needs_review"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("inventory", type=Path, help="local Coalesced JSONL")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    batch_path = Path(__file__).parents[1] / "localization" / "pl" / "ui_batch.json"
    if batch_path.exists():
        for row in json.loads(batch_path.read_text(encoding="utf-8-sig")):
            EXACT[row["id"]] = row["pl"]
    count = {"translated": 0, "machine_draft": 0, "needs_review": 0, "empty": 0}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.inventory.open(encoding="utf-8") as source, args.output.open("w", encoding="utf-8", newline="\n") as dest:
        for line in source:
            row = json.loads(line)
            # Config/editor records are not player-facing localization. Keep all
            # .int records so the handoff remains complete and auditable.
            if not row["record"].lower().endswith(".int"):
                continue
            polish = EXACT.get(row["id"])
            status = "translated" if polish is not None else None
            if polish is None and row["record"].lower().endswith("subtitles.int") and row["key"] in SUBTITLES:
                polish, status = SUBTITLES[row["key"]], "translated"
            if polish is None:
                polish, status = translate(row["text"])
            count[status] += 1
            dest.write(json.dumps({
                "id": row["id"], "record": row["record"],
                "section": row["section"], "key": row["key"],
                "category": row["category"],
                "en": row["text"], "pl": polish, "status": status,
                "tokens": row["tokens"], "markup": row["markup"],
            }, ensure_ascii=False, separators=(",", ":")) + "\n")
    print(json.dumps({"output": str(args.output), "entries": sum(count.values()), "status": count}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

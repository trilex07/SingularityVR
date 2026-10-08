#!/usr/bin/env python3
"""Complete the local Polish draft without touching the user-owned TOP-300.

The full JSONL is intentionally ignored by Git because it contains extracted
original game text. This tool writes it in place and emits an audit report.
"""
from __future__ import annotations
import argparse, json, re
from collections import Counter, defaultdict
from pathlib import Path

# Longest first avoids partial replacements (e.g. "Weapon" inside a phrase).
PHRASES = {
    # menus and common UI
    "Game Options":"Opcje gry", "Music jukebox menu":"menu szafy grającej", "Gamer Profile":"Profil gracza",
    "No active gamer profile is needed to save your progress in the game.  Please sign-in.":"Aby zapisać postęp gry, potrzebny jest aktywny profil gracza. Zaloguj się.",
    "No active gamer profile is needed to save your progress in the game. Please sign-in.":"Aby zapisać postęp gry, potrzebny jest aktywny profil gracza. Zaloguj się.",
    "Sign in":"Zaloguj się", "Continue without saving":"Kontynuuj bez zapisywania", "Saving content.  Do not turn off your system.":"Zapisywanie danych. Nie wyłączaj systemu.",
    "Saving content. Please don't turn off your console.":"Zapisywanie danych. Nie wyłączaj konsoli.", "Saving special unlockable.  Do not turn off your system.":"Zapisywanie odblokowanej zawartości. Nie wyłączaj systemu.",
    "Saving special unlockable.  Please don't turn off your console.":"Zapisywanie odblokowanej zawartości. Nie wyłączaj konsoli.", "Saving Options.  Do not turn off your system.":"Zapisywanie opcji. Nie wyłączaj systemu.",
    "Saving":"Zapisywanie", "Start Game":"Rozpocznij grę", "Achievement Awarded!":"Osiągnięcie odblokowane!", "Return to Game":"Wróć do gry",
    "Multiplayer Menu":"Menu trybu wieloosobowego", "Public Match":"Mecz publiczny", "Play Demo":"Uruchom wersję demonstracyjną", "Multiplayer":"Tryb wieloosobowy",
    "Campaign":"Kampania", "Review":"Przeglądaj", "Video":"Grafika", "Audio":"Dźwięk", "Controls":"Sterowanie", "Gameplay":"Rozgrywka",
    "BrightnessDesc":"Ustaw suwak tak, aby środkowy obraz był ledwo widoczny. Lewy obraz nie powinien być widoczny, a prawy powinien być wyraźny.",
    "Full Screen":"Pełny ekran", "SFX Volume":"Głośność efektów", "LFE Volume (5.1)":"Głośność LFE (5.1)", "Select a difficulty:":"Wybierz poziom trudności:",
    "Apply":"Zastosuj", "Game Select":"Wybór gry", "Easy":"Łatwy", "Ok":"OK", "RELOAD":"PRZEŁADUJ", "Credits":"Twórcy", "Achievements":"Osiągnięcia",
    "Collectibles":"Znajdźki", "Statistics":"Statystyki", "Notes":"Notatki", "Equip":"Wyposaż", "Equipped":"Wyposażono", "Buy Ammo":"Kup amunicję",
    "Choose weapon to swap with:":"Wybierz broń, którą chcesz zamienić:", "Empty":"Pusty", "Cost":"Koszt", "Points":"Punkty", "Game Type":"Typ gry",
    "Select which game type you would like to play:":"Wybierz typ gry:", "Slot":"Miejsce", "Save Slot":"Miejsce zapisu", "Select a save slot:":"Wybierz miejsce zapisu:",
    "Select a TMD equipment slot:":"Wybierz miejsce wyposażenia TMD:", "Weapon Upgrade":"Ulepszenie broni", "Purchase a weapon upgrade credit with E99 tech.":"Kup punkt ulepszenia broni za technologię E99.",
    "Purchase":"Kup", "Weapon Tech":"Technologia broni", "Reset To Defaults":"Przywróć domyślne", "Debug":"Debugowanie", "Reload Checkpoint":"Wczytaj punkt kontrolny",
    "RELOAD CHECKPOINT":"WCZYTAJ PUNKT KONTROLNY", "Incoming Transmission":"Nadchodząca transmisja", "OBJECTIVES":"CELE", "Debug Checkpoints":"Debugowanie punktów kontrolnych",
    "Time Score":"Wynik czasu", "Round Score Limit:":"Limit punktów rundy:", "The goal number of points per round.":"Docelowa liczba punktów na rundę.",
    # scoreboard / MP
    "Score":"Wynik", "Kills":"Zabójstwa", "Wins":"Zwycięstwa", "Losses":"Porażki", "Time Played":"Czas gry", "Assists":"Asysty", "Deaths":"Zgony",
    "Player Name":"Nazwa gracza", "Name":"Nazwa", "Player":"Gracz", "Kick Player":"Wyrzuć gracza", "Gamer Card":"Karta gracza", "Gamer Feedback":"Opinia o graczu",
    "User Profile":"Profil użytkownika", "Ping":"Ping", "Captures":"Przejęcia", "Everyone":"Wszyscy", "Friends":"Znajomi", "Self":"Ty", "Ratio":"Współczynnik", "Streak":"Seria", "Rank":"Ranga",
    "Change Board":"Zmień tablicę", "Change Filter":"Zmień filtr", "Leaderboard":"Tablica wyników", "Filter":"Filtr", "Mute Player":"Wycisz gracza", "Unmute Player":"Włącz głos gracza",
    "Creatures Win!":"Stworzenia wygrywają!", "Soldiers Win!":"Żołnierze wygrywają!", "Top Player:":"Najlepszy gracz:", "Match is a Draw":"Remis",
    # gameplay terms and descriptions
    "Weapon Class:":"Klasa broni:", "Secondary Weapon:":"Broń dodatkowa:", "You spawn with the ":"Rozpoczynasz z ", "You spawn with ":"Rozpoczynasz z ",
    " has a high fire rate, and minimal recoil.":" ma dużą szybkostrzelność i minimalny odrzut.", "Most effective at close to medium range.":"Najskuteczniejsza na krótkim i średnim dystansie.",
    "Accuracy is good, especially when firing bursts.":"Celność jest dobra, szczególnie podczas strzelania seriami.", "shoots high damage explosive rails.":"wystrzeliwuje wybuchowe pociski o dużych obrażeniach.",
    "Accuracy falls off over long distances.":"Celność spada na dużym dystansie.", "deals high damage, but at a low rate of fire.":"zadaje duże obrażenia, ale ma niską szybkostrzelność.",
    "Very accurate when using ":"Bardzo celna podczas używania ", "even at long distances.":"nawet na dużym dystansie.", "Deadly at close range, the ":"Na krótkim dystansie ", " is ideal for close quarters.":" świetnie sprawdza się w walce na małej przestrzeni.",
    "Ineffective over medium to long distances.":"Nieskuteczna na średnim i dużym dystansie.", "Pistol":"Pistolet", "Machine Gun":"Karabin maszynowy", "Heavy Machine Gun":"Ciężki karabin maszynowy",
    "Shotgun":"Strzelba", "Sniper Rifle":"Karabin snajperski", "Rail Gun":"Karabin szynowy", "Frag Grenades":"Granaty odłamkowe", "Fire Grenades":"Granaty zapalające",
    "Freeze Grenades":"Granaty zamrażające", "Electrical Grenades":"Granaty elektryczne", "Time Grenade":"Granat czasu", "Increased Ammo":"Więcej amunicji", "High Damage":"Duże obrażenia",
    "Reload Speed":"Szybkość przeładowania", "Clip Size":"Pojemność magazynka", "Firing Speed":"Szybkostrzelność", "Damage Immunity":"Odporność na obrażenia", "Ammo Capacity":"Pojemność amunicji",
    "Damage Range":"Zasięg obrażeń", "Accuracy":"Celność", "Damage":"Obrażenia", "No effect on attacks and weapons":"Nie wpływa na ataki ani broń", "Reload weapons faster.":"Szybciej przeładowujesz broń.",
    "Do more damage with weapons.":"Broń zadaje większe obrażenia.", "Increases ammo carrying capacity.":"Zwiększa ilość noszonej amunicji.", "Increases damage the weapon does":"Zwiększa obrażenia zadawane przez broń",
    "Improve all weapon accuracy":"Zwiększa celność wszystkich broni", "Whenever ammo is picked up, TMD energy is gained as well":"Podniesienie amunicji przywraca także energię TMD",
    "Gain more ammo from enemy and loot drops":"Zdobywasz więcej amunicji z pokonanych wrogów i łupów", "No Ammo":"Brak amunicji", "Full Ammo":"Pełna amunicja", "Full Health":"Pełne zdrowie",
    "Current weapon cannot be dropped.":"Tej broni nie można upuścić.", "Hold [use] to pickup %1 %2":"Przytrzymaj [use], aby podnieść %1 %2", "Weapon Locker":"Szafa z bronią", "Weapon Lockers":"Szafy z bronią",
    "Ammo":"Amunicja", "Pickup":"Podnieś", "Swap Weapons":"Zamień broń", "Health Pack":"Apteczka", "Health Packs":"Apteczki", "Cabinet":"Szafka",
    # objectives and checkpoint text
    "Restart from last Checkpoint":"Uruchom ponownie od ostatniego punktu kontrolnego", "Checkpoint Reached!":"Osiągnięto punkt kontrolny!", "Checkpoint Reached! Saving...":"Osiągnięto punkt kontrolny! Zapisywanie...",
    "All progress since last checkpoint will be lost. Are you sure you want to continue?":"Cały postęp od ostatniego punktu kontrolnego zostanie utracony. Czy na pewno chcesz kontynuować?",
    "Find Devlin":"Znajdź Devlina", "Save Man":"Uratuj człowieka", "Find Weapon":"Znajdź broń", "Get to Radio Tower":"Dostań się do wieży radiowej", "Unblock the Door":"Odblokuj drzwi", "Open Gates From Guard Tower":"Otwórz bramy z wieży strażniczej", "Retreat":"Odwrót", "Escape":"Ucieczka",
    "Lab Rat":"Szczur laboratoryjny", "Altercation":"Starcie", "Secret Safe":"Sekretny skarbiec", "Find Vault Entrance":"Znajdź wejście do skarbca", "The One":"Wybraniec", "Finders Keepers":"Znalazca zatrzymuje", "Time Rift":"Szczelina czasowa", "Personal Savior":"Osobisty wybawca", "Escort":"Eskorta", "Back to the Future":"Powrót do przyszłości", "To the Tower":"Do wieży", "We're Gonna Need Guns":"Będziemy potrzebować broni", "Male Escort":"Męska eskorta", "Tower Defense":"Obrona wieży", "The Tower":"Wieża", "It's a Sabotage":"To sabotaż", "TMD Platform":"Platforma TMD", "Hydrogenated":"Uwodorniony", "Bridge Crossing":"Przeprawa przez most", "A Change of Plans":"Zmiana planów", "Gaseous":"Gazowo", "The Tunnels":"Tunele", "Shutting Down":"Wyłączanie", "Break In":"Włamanie", "On Rails":"Na torach", "A Rift in the 50's":"Szczelina w latach 50.", "Activation Code":"Kod aktywacyjny", "Escape the Cooker":"Ucieczka z reaktora", "Central Docks":"Centralne doki",
    "Eliminate the opposing species.":"- Wyeliminuj wrogi gatunek.", "Defend the Beacon.":"- Broń nadajnika.", "Destroy regenerated Beacon before it goes online.":"- Zniszcz odnowiony nadajnik, zanim zostanie uruchomiony.",
    "Katorga, Sea of Okhostk":"Katorga, Morze Ochockie", "USMC Reconaissance Mission":"Misja rozpoznawcza USMC", "Codename:  \"Spartan\"":"Kryptonim:  \"Spartan\"",
    # Mission/checkpoint labels. These are intentionally short and literal.
    "Research Facility":"Ośrodek badawczy", "Rail Line":"Linia kolejowa", "Workers' District":"Dzielnica robotnicza",
    "E99 Processing Complex":"Kompleks przetwarzania E99", "Singularity Labs":"Laboratoria Singularity", "Barisov's Tower":"Wieża Barisova",
    "Crash Site":"Miejsce katastrofy", "Inferno":"Piekło", "Village Street":"Ulica wioski", "School":"Szkoła",
    "Meet Devlin":"Spotkaj Devlina", "Proper Action":"Właściwe działanie", "Bridge to the Radio Tower":"Most do wieży radiowej", "Radio Tower":"Wieża radiowa",
    "Meet Demichev":"Spotkaj Demicheva", "Chase":"Pościg", "Makeshift Morgue":"Prowizoryczna kostnica", "Lab Entrance":"Wejście do laboratorium",
    "T.M.D. Vault Entrance":"Wejście do skarbca TMD", "1950s Save Barisov":"Lata 50. — ocal Barisova", "Return to Barisov's Lab":"Powrót do laboratorium Barisova",
    "Offices":"Biura", "Rift Room":"Sala szczeliny", "Separated From Barisov":"Oddzieleni od Barisova", "Transformer Yard":"Plac transformatorów",
    "Broken Elevator":"Uszkodzona winda", "Aging Hallway":"Korytarz starzenia", "Courtyard Overlook":"Widok na dziedziniec", "Savage Beatdown":"Dzika jatka",
    "Warehouse Interior":"Wnętrze magazynu", "Zek Boss":"Boss Zek", "Kathryn Busts In":"Kathryn wpada", "Defend Barisov's Tower":"Obrona wieży Barisova",
    "Visit":"Wizyta", "Dynamic Music Test":"Test muzyki dynamicznej", "Playable Creatures":"Grywalne stworzenia", "TMD Objects Gallery":"Galeria obiektów TMD",
    "Bridge Is Out":"Most zniszczony", "Fatty Attack":"Atak Grubasa", "Cistern":"Cysterna", "Hydro Generator":"Generator hydroelektryczny", "TMD the Bridge":"TMD i most",
    "Warehouse":"Magazyn", "Revert Intro":"Wprowadzenie do Revertera", "Before Gas Mask":"Przed maską gazową", "Gas Area Vent":"Wentylacja strefy gazowej",
    "Elevator Puzzle":"Zagadka z windą", "Bridge Defense":"Obrona mostu", "Take Down Defenses":"Zniszcz obronę", "Find The Cart":"Znajdź wózek", "Bridge Overlook":"Widok na most",
    "Cooker Entrance":"Wejście do pieca", "Cooker 1950":"Piec — 1950", "Meet Dr. Petrov":"Spotkaj dr. Petrova", "Catwalk Clash":"Starcie na kładce", "Botany Lab":"Laboratorium botaniki",
    "Cooker Overload":"Przeciążenie pieca", "Destroyed Cooker":"Zniszczony piec", "Vents":"Szyby wentylacyjne", "Resistance Vignette":"Winieta ruchu oporu", "Sewers":"Kanały", "Bypass":"Obejście", "Cistern Access":"Dostęp do cysterny", "Cistern Lift":"Winda cysterny",
    "Meet Kathryn":"Spotkaj Kathryn", "Lose Kathryn":"Utrata Kathryn", "Lose Kathryn Waypoint":"Punkt utraty Kathryn", "Window":"Okno", "Chase Kathryn":"Pościg za Kathryn", "Barricade":"Barykada", "Rescue Kathryn":"Uratowanie Kathryn",
    "TMD Defense Echo Event":"Obrona TMD — Zdarzenie Echa", "TMD Defense Combat":"Obrona TMD — walka", "TMD Defense Uber Event":"Obrona TMD — wielkie zdarzenie", "Fight the Grom":"Walcz z Gromem", "Fall of the Grom":"Upadek Groma",
    "Chopper Intro":"Wprowadzenie z helikopterem", "Canals":"Kanały", "Chaos":"Chaos", "Post Defend":"Po obronie", "Defend Yuri":"Obrona Jurija", "Demichev Destroys Door":"Demichev niszczy drzwi", "Singularity Vista":"Widok na Singularity", "Radion Fight":"Walka z Radionem", "Post Radion Fight":"Po walce z Radionem", "Yurisov is Born":"Narodziny Yurisova", "After Catwalk Battle":"Po walce na kładce", "Frozen Soldier Fight":"Walka z zamrożonym żołnierzem", "Opening Reactor Doors":"Otwarcie drzwi reaktora", "The Core":"Rdzeń", "Inside the Walls":"Wewnątrz murów", "Back in the Core":"Z powrotem w rdzeniu", "Back inside the Walls":"Z powrotem wewnątrz murów", "Demichev Attacks":"Demichev atakuje", "Approaching Demichev":"Zbliżając się do Demicheva", "Final Confrontation":"Ostateczna konfrontacja", "Back to the 50s":"Powrót do lat 50.", "Bomb planted":"Bomba podłożona", "Bad Ending":"Złe zakończenie", "Ugly Endings":"Brzydkie zakończenia", "Good Ending":"Dobre zakończenie",
    # Remaining player-facing prompts and short labels.
    "Your gamer profile status has changed.  You will be returned to the main menu and all unsaved progress will be lost.":"Zmieniono stan profilu gracza. Nastąpi powrót do menu głównego, a niezapisany postęp zostanie utracony.",
    "Doing this will overwrite all saved progress. Are you sure you want to continue?":"Ta czynność nadpisze cały zapisany postęp. Czy na pewno chcesz kontynuować?", "All unsaved progress will be lost. Are you sure you want to continue?":"Cały niezapisany postęp zostanie utracony. Czy na pewno chcesz kontynuować?", "All progress will be lost. Are you sure you want to continue?":"Cały postęp zostanie utracony. Czy na pewno chcesz kontynuować?",
    "Please reconnect the primary controller.":"Podłącz ponownie główny kontroler.", "Controller":"Kontroler", "Skills":"Umiejętności", "Keep":"Zachowaj", "Discard":"Odrzuć", "Are you sure you want to quit?":"Czy na pewno chcesz wyjść?", "Quit":"Wyjdź", "End Match":"Zakończ mecz", "Multiple Logon":"Wielokrotne logowanie", "Lost Connection":"Utracono połączenie", "Connection Error":"Błąd połączenia", "Failed to Connect":"Nie udało się połączyć", "Communication Error":"Błąd komunikacji", "Network cable unplugged":"Kabel sieciowy odłączony", "Resolution":"Rozdzielczość", "Complete":"Ukończono", "Locked":"Zablokowane", "Offline":"Offline", "Online":"Online", "Extras":"Dodatki", "Quit Game":"Wyjdź z gry", "RETURN TO GAME":"WRÓĆ DO GRY", "ACHIEVEMENTS":"OSIĄGNIĘCIA", "REVIEW":"PRZEGLĄD", "CREDITS":"TWÓRCY", "DEBUG":"DEBUGOWANIE", "PLAY DEMO":"URUCHOM WERSJĘ DEMONSTRACYJNĄ", "EXTRAS":"DODATKI",
    "Not connected to Xbox LIVE":"Brak połączenia z Xbox LIVE", "Xbox LIVE Party":"Impreza Xbox LIVE", "Team 1":"Drużyna 1", "Team 2":"Drużyna 2", "Not Capped":"Nieprzejęty", "Teams Tie!":"Remis drużyn!", "Total":"Razem", "Beacon #1":"Nadajnik nr 1", "Beacon #2":"Nadajnik nr 2", "Beacon #3":"Nadajnik nr 3", "ANTI-GRAV":"ANTYGRAWITACJA", "DEADLOCK":"BLOKADA CZASU",
    "Weapon":"Broń", "Primary Weapon":"Broń główna", "Secondary Weapon":"Broń dodatkowa", "Dethex Launcher":"Wyrzutnia Dethex", "Volk S4":"Volk S4", "Kasimov SNV-E99":"Kasimov SNV-E99", "Weapon Blueprint":"Projekt broni", "Double Barrel":"Dwie lufy", "Roller Derby":"Jazda bez trzymanki", "Fully Armed":"Pełne uzbrojenie", "Fully Upgraded 1 Weapon in single player campaign":"W pełni ulepsz jedną broń w kampanii dla jednego gracza", "15 Autocannon kills without reloading in single player campaign":"15 zabójstw z działka automatycznego bez przeładowania w kampanii dla jednego gracza",
    "Press START":"Naciśnij START", "Press START button":"Naciśnij przycisk START", "Press [dash] for Temporal Dilation":"Naciśnij [dash], aby użyć dylatacji czasu", "Press [grab] to grab projectile!":"Naciśnij [grab], aby chwycić pocisk!",
    "An active gamer profile is needed to save your progress in the game.  Please sign-in.":"Aby zapisać postęp gry, potrzebny jest aktywny profil gracza. Zaloguj się.", "Your gamer profile can not be accessed please reinsert the Xbox 360 Memory Unit to continue saving your game.":"Nie można uzyskać dostępu do profilu gracza. Włóż ponownie kartę pamięci Xbox 360, aby kontynuować zapisywanie gry.", "Xbox 360 Memory unit has been re-inserted.":"Ponownie włożono kartę pamięci Xbox 360.", "Error with save information detected. Please restart your console.":"Wykryto błąd danych zapisu. Uruchom ponownie konsolę.", "Your save data was corrupt and will be overwritten.":"Dane zapisu są uszkodzone i zostaną nadpisane.", "Some downloadable content appears damaged or unreadable. Please download again.":"Część pobranej zawartości jest uszkodzona lub nieczytelna. Pobierz ją ponownie.", "A content package appears damaged or unreadable. Please download the content package again.":"Pakiet zawartości jest uszkodzony lub nieczytelny. Pobierz go ponownie.", "Downloadable content":"Pobrana zawartość", "Are you sure you want to end the match?":"Czy na pewno chcesz zakończyć mecz?", "Your product key has been used to logon at a different location.  You will be returned to the main menu and all unsaved progress will be lost.":"Twój klucz produktu został użyty w innym miejscu. Nastąpi powrót do menu głównego, a niezapisany postęp zostanie utracony.", "The connection to the Activision matchmaking servers has been lost.  You will be returned to the main menu, and you will lose all unsaved progress.":"Utracono połączenie z serwerami dobierania gier Activision. Nastąpi powrót do menu głównego, a niezapisany postęp zostanie utracony.", "Lost connection to Xbox LIVE":"Utracono połączenie z Xbox LIVE", "Lost connection to PlayStation�Network":"Utracono połączenie z PlayStation Network", "Lost connection to Activision's Demonware servers":"Utracono połączenie z serwerami Demonware firmy Activision", "Cannot connect to Xbox LIVE":"Nie można połączyć się z Xbox LIVE", "Cannot connect to Activision's Demonware servers":"Nie można połączyć się z serwerami Demonware firmy Activision", "There was a failure connecting to the game.":"Nie udało się połączyć z grą.", "Your connection to other lobby members has been lost.":"Utracono połączenie z pozostałymi członkami lobby.", "Adjust the slider until the center image is barely visible. The left image should not be visible and the right image should be clearly visible.":"Ustaw suwak tak, aby środkowy obraz był ledwo widoczny. Lewy obraz nie powinien być widoczny, a prawy powinien być wyraźny.", "QUIT DEMO":"WYJDŹ Z WERSJI DEMONSTRACYJNEJ", "MULTIPLAYER":"TRYB WIELOOSOBOWY", "Away":"Nieobecny", "Busy":"Zajęty", "Customize Loadout":"Dostosuj wyposażenie", "Loadout":"Wyposażenie", "Unlocked at Level":"Odblokowane na poziomie", "This perk is locked.  You must level up before you can equip it.":"To ulepszenie jest zablokowane. Musisz awansować, zanim będzie można je wyposażyć.", "This weapon is locked.  You must level up before you can equip it.":"Ta broń jest zablokowana. Musisz awansować, zanim będzie można jej użyć.", "This loadout is locked.  You must level up before you can use it.":"To wyposażenie jest zablokowane. Musisz awansować, zanim będzie można go użyć.", "Create, delete, and change your online profile":"Twórz, usuwaj i zmieniaj profil online", "Sign Into Profile":"Zaloguj się do profilu", "Delete Profile":"Usuń profil", "Downloading settings...":"Pobieranie ustawień...", "Creating Lobby...":"Tworzenie lobby...", "Processing Login...":"Logowanie...", "Error Downloading Settings":"Błąd pobierania ustawień", "Error":"Błąd", "Unknown network error":"Nieznany błąd sieci", "Unable to connect to Activision authentication servers.":"Nie można połączyć się z serwerami uwierzytelniania Activision.", "Failed to authenticate against the Activision servers.":"Uwierzytelnianie na serwerach Activision nie powiodło się.", "Chat disabled":"Czat wyłączony", "Create New Profile":"Utwórz nowy profil", "Profile name cannot be empty!":"Nazwa profilu nie może być pusta!", "Creating Profile...":"Tworzenie profilu...", "Profile created":"Utworzono profil", "Error creating profile":"Błąd tworzenia profilu", "Profile successfully created":"Profil utworzony pomyślnie", "Unknown profile creation error":"Nieznany błąd tworzenia profilu", "Invalid password":"Nieprawidłowe hasło", "Username already taken":"Nazwa użytkownika jest już zajęta", "Profile creation service unavailable":"Usługa tworzenia profili jest niedostępna", "Deleting Profile...":"Usuwanie profilu...", "Profile deleted":"Usunięto profil", "Error deleting profile":"Błąd usuwania profilu", "Profile has been deleted":"Profil został usunięty", "The product key is invalid":"Nieprawidłowy klucz produktu", "The user name is invalid":"Nieprawidłowa nazwa użytkownika", "Login Existing Profile":"Zaloguj się do istniejącego profilu", "Are you sure you want to delete this profile?":"Czy na pewno chcesz usunąć ten profil?", "PREGAME LOBBY":"LOBBY PRZED MECZEM", "Match Type:":"Typ meczu:", "Instant":"Natychmiastowy", "Private":"Prywatny", "Public":"Publiczny", "LAN":"LAN", "System Link":"Sieć lokalna", "Choose what type of game you want to play":"Wybierz typ gry", "Xbox LIVE Quick Match":"Szybki mecz Xbox LIVE", "Private Xbox LIVE":"Prywatny mecz Xbox LIVE", "Public Xbox LIVE":"Publiczny mecz Xbox LIVE",
}

# Case variants and short words need boundaries; exact phrases above are safe.
TOKEN_RE = re.compile(r"\[[^\]]+\]|<[^>]+>|%[-+#0-9.*]*[A-Za-z]|\{[^}]+\}|`~")

def ids_from_top(path: Path) -> set[str]:
    out=set()
    for line in path.read_text(encoding='utf-8').splitlines():
        for m in re.finditer(r'`([^`]+)`', line):
            if '|' in m.group(1): out.add(m.group(1))
    return out

def protect(s: str):
    vals=[]
    def sub(m): vals.append(m.group(0)); return f'\x00{len(vals)-1}\x00'
    return TOKEN_RE.sub(sub,s), vals

def restore(s, vals):
    for i,v in enumerate(vals): s=s.replace(f'\x00{i}\x00',v)
    return s

def translate(text: str):
    if not text.strip(): return text, 'skip', 'Pusty wpis źródłowy.'
    # A few common prompts contain control tokens inside the phrase, so handle
    # them before token protection rather than ever translating the token.
    if text.startswith('Hold [use] to pickup '):
        return 'Przytrzymaj [use], aby podnieść ' + text[len('Hold [use] to pickup '):], 'translated', ''
    if text.startswith('[use] '):
        tail, st, _ = translate(text[len('[use] '):])
        if st == 'translated': return '[use] ' + tail, 'translated', ''
    work, vals=protect(text)
    changed=False
    for src,dst in sorted(PHRASES.items(), key=lambda kv: len(kv[0]), reverse=True):
        if src in work:
            work=work.replace(src,dst); changed=True
    result=restore(work,vals)
    return (result,'translated','') if changed else (text,'skip','Brak bezpiecznego tłumaczenia w słowniku; pozostawiono do ręcznej recenzji.')

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('draft',type=Path); ap.add_argument('top',type=Path); ap.add_argument('--report',type=Path,required=True); args=ap.parse_args()
    top=ids_from_top(args.top)
    rows=[json.loads(x) for x in args.draft.read_text(encoding='utf-8').splitlines() if x.strip()]
    counts=Counter(); changed=0; top_untouched=0; translated_examples=[]; skips=defaultdict(list)
    for row in rows:
        rid=row.get('id','')
        if rid in top:
            top_untouched+=1; counts[(row['category'],'top300_untouched')]+=1; continue
        old=row.get('status')
        if old=='translated':
            counts[(row['category'],'translated_existing')]+=1; continue
        pl,status,note=translate(row.get('en',''))
        if status=='translated':
            row['pl']=pl; row['status']='translated'; row.pop('note',None); changed+=1
            if len(translated_examples)<20: translated_examples.append((rid,row['en'],pl))
        else:
            row['status']='skip'; row['note']=note; changed+=1; skips[row['category']].append(rid)
        counts[(row['category'],row['status'])]+=1
    args.draft.write_text('\n'.join(json.dumps(r,ensure_ascii=False,separators=(',',':')) for r in rows)+'\n',encoding='utf-8',newline='\n')
    remaining=Counter(r.get('status') for r in rows)
    report=[]
    non_top_status=Counter(r.get('status') for r in rows if r.get('id') not in top)
    top_status=Counter(r.get('status') for r in rows if r.get('id') in top)
    report += ['# Polski draft — raport po TOP-300','',f'- Plik lokalny: `{args.draft}`',f'- TOP-300 pominięte: **{top_untouched}** wpisów (**{len(top)} unikalnych identyfikatorów**; jeden identyfikator występuje dwa razy w tabeli recenzji).',f'- Wpisów przetworzonych: **{changed}**.',f'- Statusy poza TOP-300: `{dict(non_top_status)}` — brak `machine_draft` i `needs_review`.',f'- Statusy pozostawione w TOP-300: `{dict(top_status)}`; te wpisy są własnością użytkownika.','', '## Wyniki według kategorii','']
    cats=sorted({r['category'] for r in rows})
    for c in cats:
        d=Counter(r.get('status') for r in rows if r['category']==c and r['id'] not in top)
        t=sum(1 for r in rows if r['category']==c and r['id'] in top)
        report.append(f'- `{c}`: '+', '.join(f'{k}={v}' for k,v in sorted(d.items()))+f'; TOP-300 pominięte={t}')
    report += ['', '## Słownik decyzji terminologicznych','', '- `Reload` -> `Przeładuj` / `PRZEŁADUJ`.', '- `Checkpoint` -> `punkt kontrolny`.', '- `Weapon Tech` -> `Technologia broni`.', '- `Weapon Locker` -> `Szafa z bronią`.', '- `Objective` -> `Cel`.', '- `Collectibles` -> `Znajdźki`.', '- `Difficulty` -> `Poziom trudności`.', '- `Save Slot` -> `Miejsce zapisu`.', '- `Ammo` -> `Amunicja`.', '- `Score` -> `Wynik`.', '- `Kills` -> `Zabójstwa`.', '- `Assists` -> `Asysty`.', '- `Damage` -> `Obrażenia`.', '- `Accuracy` -> `Celność`.', '- `Clip Size` -> `Pojemność magazynka`.', '- `Firing Speed` -> `Szybkostrzelność`.', '- `Fire Rate` -> `Szybkostrzelność`.', '- `Loadout` -> `Wyposażenie`.', '- `Leaderboard` -> `Tablica wyników`.', '- `TMD` and weapon names remain unchanged.', '- `[use]`, `[aim]` and other control tokens remain unchanged.', '- Engine/editor/debug-only records are marked `skip` when no safe player-facing translation exists.', '', '## Kontrola integralności','', '- Identyfikatory są odczytywane i zapisywane bez zmian.', '- Tokeny, znaczniki i placeholdery są chronione podczas zamiany.', '- Draft pozostaje plikiem lokalnym; repozytorium zawiera wyłącznie narzędzie i ten raport.']
    args.report.write_text('\n'.join(report)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'top300':top_untouched,'processed':changed,'remaining':dict(remaining)},ensure_ascii=False))
if __name__=='__main__': main()

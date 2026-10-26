"""Intelligent multi-tier Polish dialogue variants for SingularityVR dubbing fitting.

Ensures that if primary curated phrasing slightly overshoots native budget,
natural concise synonyms and military callouts are automatically tested.
"""
import re
import sys
from pathlib import Path

sys.path.append(str(Path(__file__).parent))
from curated_translations_551 import get_curated_pl


def normalize_en(text: str) -> str:
    t = text.lower().replace("'", "").replace("’", "")
    t = re.sub(r'[^a-zA-Z0-9\s]', ' ', t).strip()
    return re.sub(r'\s+', ' ', t)


def smooth_punctuation(text: str) -> str:
    """Replace internal terminal punctuation with comma, and ensure terminal punctuation is dot."""
    t = text.strip()
    t = re.sub(r'[\-\u2013\u2014\u2022>]+\s*', '', t)
    # Replace internal ! or ? or . followed by letter with comma
    t = re.sub(r'([!?])\s+(?=[A-Za-zżźćńółęąśŻŹĆĄŚĘŁÓŃ])', ', ', t)
    t = re.sub(r'(\.)\s+(?=[A-Za-zżźćńółęąśŻŹĆĄŚĘŁÓŃ])', ', ', t)
    # Replace terminal exclamation mark ! with dot . (Pocket-TTS misses/delays EOS on !)
    t = re.sub(r'[!]+$', '.', t)
    t = re.sub(r'\s+', ' ', t).strip()
    return t


# Master mapping of tailored concise variants for all dialogue lines (EXACT match on normalized English)
SHORT_SYNONYMS = {
    "got em": ["Dostał.", "Trafiony.", "Mam go."],
    "got him": ["Dostał.", "Mam go."],
    "got one": ["Jeden zdjęty.", "Dostał."],
    "whered he go": ["Gdzie jest?", "Gdzie on?"],
    "where did he go": ["Gdzie jest?", "Gdzie on?"],
    "hold up": ["Stój.", "Czekaj."],
    "hold position": ["Trzymać pozycję.", "Stój."],
    "on me": ["Do mnie.", "Za mną."],
    "on may": ["Do mnie.", "Za mną."],
    "move it": ["Ruchy.", "Ruszaj.", "Idź."],
    "ugh": ["Uch.", "Ech."],
    "argh": ["Agh.", "Uch."],
    "ahhh": ["Aaa.", "Uch."],
    "move up": ["Naprzód.", "Idź.", "Ruchy."],
    "move faster": ["Szybciej.", "Ruchy."],
    "move forward": ["Naprzód.", "Ruchy."],
    "move soldier": ["Ruszaj, żołnierzu.", "Ruchy."],
    "this way": ["Tędy.", "Za mną."],
    "this way renko": ["Tędy, Renko.", "Tędy."],
    "you got it": ["Dobra.", "Rozkaz.", "Zrobione."],
    "look left": ["W lewo.", "Z lewej."],
    "look right": ["W prawo.", "Z prawej."],
    "go get him": ["Brać go.", "Dopaść go.", "Bierz go."],
    "thanks": ["Dzięki."],
    "thank you": ["Dziękuję."],
    "brilliant": ["Świetnie.", "Doskonale."],
    "you go ill cover": ["Idź, osłaniam.", "Idź, kryję.", "Idź."],
    "well done": ["Świetnie.", "Dobra robota."],
    "checking right": ["Sprawdzam prawą.", "Prawa czysta."],
    "mmm": ["Tak.", "Mhm."],
    "checking left": ["Sprawdzam lewą.", "Lewa czysta."],
    "its safe": ["Bezpiecznie.", "Czysto."],
    "theres one": ["Jest jeden.", "Widzę go."],
    "gimme a hand": ["Pomóż mi.", "Pomóż."],
    "flank left": ["Z lewej.", "W lewo.", "Obejdź z lewej."],
    "you mind moving": ["Przesuń się.", "Z drogi."],
    "dead ahead": ["Prosto.", "Przed nami."],
    "all clear": ["Czysto."],
    "alls clear": ["Czysto."],
    "flank right": ["Z prawej.", "W prawo.", "Obejdź z prawej."],
    "lets pull back": ["Wycofujemy się.", "Odwrót."],
    "go behind them": ["Zajdź ich.", "Od tyłu."],
    "take cover": ["Do osłony.", "Kryć się."],
    "get to cover": ["Do osłony.", "Kryć się."],
    "find cover": ["Do osłony.", "Kryć się."],
    "retreat to cover": ["Do osłony.", "Odwrót."],
    "they found us": ["Znaleźli nas.", "Wykryli nas.", "Wróg."],
    "check the rooms": ["Sprawdzić sale.", "Sprawdźcie to.", "Sprawdzać."],
    "help im pinned down": ["Przygwoździli mnie.", "Pomocy."],
    "im pinned down": ["Przygwoździli mnie."],
    "help over here": ["Pomocy, tutaj."],
    "need to reload": ["Przeładowuję."],
    "i have to reload": ["Przeładowuję."],
    "choppers down": ["Śmigłowiec zestrzelony.", "Zestrzelony."],
    "nice shot": ["Dostał.", "Trafiony.", "Dobry strzał."],
    "go on captain": ["Dalej, kapitanie.", "Dalej.", "Naprzód."],
    "take my hand": ["Łap rękę.", "Trzymaj dłoń.", "Łap dłoń."],
    "lets just get outta here": ["Wynośmy się stąd.", "Uciekajmy."],
    "watch the corners": ["Pilnujcie rogów.", "Uwaga na rogi."],
    "keep it up renko": ["Tak trzymaj, Renko.", "Tak trzymaj."],
    "watch it renko": ["Uważaj, Renko.", "Uważaj.", "Ostrożnie."],
    "youre in my way": ["Stoisz mi na drodze.", "Z drogi."],
    "circle around behind them": ["Obejdźcie ich.", "Zajdźcie ich od tyłu."],
    "could you try to open it": ["Spróbujesz to otworzyć?", "Otworzysz to?"],
    "tracking intruder": ["Śledzę intruza.", "Namierzam go."],
    "hahaha": ["Ha, ha, ha.", "Ha ha."],
    "everybody move": ["Wszyscy, ruchy.", "Ruchy."],
    "engage engage": ["Ognia.", "Strzelać."],
    "i need cover fire": ["Kryj mnie.", "Potrzebuję osłony."],
    "give me cover fire": ["Kryj mnie.", "Kryj."],
    "love me": ["Kryj mnie.", "Kryj."],
    "please you should go": ["Powinieneś iść.", "Idź stąd."],
    "hes dead when i find him": ["Dorwę go i zabiję.", "Zabiję go."],
    "good shot renko": ["Dobry strzał, Renko.", "Dobry strzał.", "Trafiony."],
    "we have to get them all": ["Wystrzelać ich.", "Zabić wszystkich."],
    "damn spetsnolds": ["Cholera, Specnaz.", "Specnaz."],
    "damn spetsnaz": ["Przeklęty Specnaz.", "Specnaz."],
    "ill give them off when i work": ["Wyłączę je do pracy.", "Wyłączę je."],
    "i want them off when i work": ["Wyłączcie to.", "Chcę by były wyłączone."],
    "could you help me up renko": ["Pomożesz mi wejść, Renko?", "Pomożesz mi wejść?"],
    "open the door then cover me": ["Otwórz i mnie osłaniaj.", "Otwórz, kryj mnie."],
    "grab that shotgun and well go": ["Bierz strzelbę, idziemy.", "Bierz strzelbę.", "Bierz broń."],
    "have you found anything renko": ["Znalazłeś coś, Renko?", "Znalazłeś coś?"],
    "up here grab my hand": ["Tutaj, łap dłoń.", "Łap moją dłoń."],
    "up here grab my head": ["Tutaj, łap dłoń.", "Chwyć dłoń."],
    "just follow the open doors": ["Biegnij za otwartymi drzwiami.", "Za drzwiami."],
    "come on take my hand": ["Chodź, łap dłoń.", "Łap moją dłoń."],
    "nice build captain": ["Świetny strzał, kapitanie.", "Świetny strzał."],
    "i need suppressing fire": ["Ogień zaporowy.", "Kryj mnie."],
    "nice shooting lets move": ["Dobry strzał, ruszamy.", "Świetny strzał, w drogę."],
    "you know where it is tell me": ["Gdzie to jest, gadaj.", "Mów, gdzie to jest."],
    "quick gimme a hand with this cabinet": ["Szybko, pomóż mi z tą szafą.", "Pomóż z szafą."],
    "grab that shotgun and lets get moving": ["Bierz strzelbę i ruszamy.", "Bierz strzelbę, w drogę."],
    "go on grab that machine gun": ["Bierz ten karabin.", "Bierz karabin."],
    "there is no time to wait captain": ["Nie ma czasu, kapitanie.", "Nie mamy czasu."],
    "our embassy right now": ["Do naszej ambasady, natychmiast.", "Do ambasady."],
    "shhhh theyre close": ["Cicho, są blisko.", "Cicho."],
    "come on renko take my hand": ["Renko, łap moją dłoń.", "Łap dłoń."],
    "you mind untying me renko": ["Rozwiążesz mnie, Renko?", "Rozwiąż mnie."],
    "dont be shy take my hand": ["Śmiało, złap moją dłoń.", "Łap dłoń."],
    "go on take the shotgun and well move out": ["Bierz strzelbę i ruszajmy.", "Bierz broń, ruszamy."],
    "damn why doesnt this work": ["Cholera, czemu to nie działa?", "Czemu to nie działa?"],
    "nice work renko that did it": ["Dobra robota, Renko, udało się.", "Dobra robota, Renko."],
    "we are a security starter sir ed": ["Ochrona, zgłaszam się.", "Tu ochrona."],
    "renko we dont have all day": ["Renko, nie mamy całego dnia.", "Renko, pospiesz się."],
    "say again this is devlin 4 1": ["Powtórz, tu Devlin cztery-jeden.", "Tu Devlin, powtórz."],
    "dr barisov we made it inside the fence": ["Doktorze Barisow, jesteśmy za ogrodzeniem.", "Barisow, minęliśmy ogrodzenie."],
    "nimitz actual here whats your status captain": ["Tu dowódca Nimitz, kapitanie, jaki status?", "Tu Nimitz, jaki status?"],
    "alright were asking mike head back to my position": ["W porządku, wracaj na moją pozycję.", "Wracaj na moją pozycję."],
    "what are you waiting for take that shotgun and well move out": ["Bierz strzelbę i ruszamy.", "Na co czekasz, bierz strzelbę."],
    "scramble search and rescue asap": ["Ratownicy, natychmiast ruszać na poszukiwania.", "Ratownicy, ruszać natychmiast."],
    "get your team there captain were pulling you out": ["Przyprowadź drużynę, kapitanie, ewakuujemy was.", "Przyprowadź ludzi, ewakuujemy was."],
    "open the door renko ill hold this position": ["Otwórz drzwi, Renko, ja utrzymam pozycję.", "Otwórz drzwi, ja osłaniam."],
    "a squad saw me but i was able to lose them": ["Wypatrzył mnie patrol, ale ich zgubiłem.", "Patrol mnie zauważył, ale uciekłem."],
    "this is devlin 4 1 do you copy titan 1": ["Tu Devlin 4-1, słyszycie nas, Titan 1?", "Tu Devlin 4-1, odbiór Titan 1?"],
    "this is devlin 4 1 do you copy nimitz": ["Tu Devlin 4-1, słyszycie nas, Nimitz?", "Tu Devlin 4-1, odbiór Nimitz?"],
    "watch out renko more patrols are headed your way": ["Uważaj, Renko, zbliżają się patrole.", "Uważaj, idą kolejne patrole."],
    "we have company hold them off while i get the train rolling": ["Mamy gości, powstrzymaj ich, aż ruszę pociąg.", "Powstrzymaj ich, aż ruszę pociąg."],
    "finally come on renko lets get to the tower": ["Wreszcie, chodź, Renko, do wieży.", "Chodź, Renko, do wieży."],
    "renko grab the machine gun before they break in": ["Renko, bierz karabin, zanim się wedrą.", "Renko, bierz karabin."],
    "nimitz this is devlin 4 1 actual do you copy": ["Nimitz, tu dowódca Devlin cztery-jeden, odbiór?", "Tu Devlin 4-1, Nimitz, odbiór?"],
    "titan 1 this is devlin 4 1 actual do you copy": ["Titan 1, tu dowódca Devlin cztery-jeden, odbiór?", "Titan 1, tu Devlin, odbiór?"],
    "looks like a goddamn emp buckle up renko were going for a ride": ["To chyba impuls EMP, trzymaj się, Renko.", "To impuls EMP, trzymaj się."],
    "theres something going on here renko weve gotta contact the bird farm": ["Coś tu nie gra, Renko, musimy połączyć się z bazą.", "Coś tu nie gra, łącz się z bazą."],
    "ah yes lee": ["Tak.", "Słucham."],
    "here we go": ["Ruszamy.", "Idź."],
    "its on the way": ["Już jedzie.", "W drodze."],
    "the fence is on ready for the elevator": ["Ogrodzenie działa, czekamy na windę."],
    "renko could you give me a boost": ["Renko, podsadzisz mnie?", "Podsadzisz mnie?"],
    "theres a loose connection here": ["Tu jest luźny styk.", "Luźny przewód."],
    "the primary junction looks to be good": ["Główne złącze wygląda sprawnie."],
    "good job renko": ["Dobra robota, Renko.", "Świetnie."],
    "spartan team maintain present speed and heading": ["Spartan, utrzymać kurs i prędkość."],
    "not receiving from either blackfire": ["Brak sygnału z obu Blackfire."],
    "spartan team do you read": ["Spartan, słyszycie mnie?"],
    "go on take that sniper rifle and kill those maggots on the bridge": ["Bierz snajperkę i zdejmij te poczwary."],
    "stay frosty": ["Bądźcie czujni."],
    "enemy in the window": ["Wróg w oknie."],
    "target 12 oclock": ["Cel na dwunastej."],
    "they popped smoke": ["Rzucili dym."],
    "we need to fall back": ["Wycofujemy się.", "Odwrót."],
    "grenade incoming": ["Granat."],
    "take him out": ["Zdejmij go."],
    "all spartans are hurt": ["Wszyscy Spartanie są ranni."],
    "theyre right behind you": ["Są tuż za tobą."],
    "lets go": ["Idź.", "Ruszamy.", "Chodźmy."],
    "follow me": ["Za mną.", "Chodź."],
    "to the right": ["Na prawo.", "Z prawej."],
    "to the left": ["Na lewo.", "Z lewej."],
    "go right": ["W prawo."],
    "go left": ["W lewo."],
    "turn right": ["W prawo."],
    "turn left": ["W lewo."],
    "right ahead of us": ["Przed nami.", "Tuż przed nami."],
    "contact dead ahead": ["Kontakt z przodu.", "Wróg z przodu."],
    "contact to the left": ["Kontakt z lewej."],
    "contact to the right": ["Kontakt z prawej."],
    "contact above us": ["Kontakt z góry."],
    "contact on the roof": ["Kontakt na dachu."],
    "contact on the bridge": ["Kontakt na moście."],
    "enemy spotted": ["Widzę wroga.", "Wróg."],
    "enemy advancing": ["Wróg naciera."],
    "enemy to the left": ["Wróg z lewej."],
    "enemy to the right": ["Wróg z prawej."],
    "enemy up top": ["Wróg na górze."],
    "enemy is behind cover": ["Wróg za osłoną."],
    "theyre behind cover": ["Są za osłoną."],
    "target in cover": ["Cel za osłoną."],
    "target dead ahead": ["Cel przed nami."],
    "i have him": ["Mam go."],
    "i see him": ["Widzę go."],
    "there he is": ["Tam jest."],
    "theres one to the left": ["Jeden z lewej."],
    "theres one to the right": ["Jeden z prawej."],
    "quickly": ["Szybko.", "Prędko."],
    "hes down": ["Leży.", "Dostał."],
    "look out": ["Uważaj.", "Uwaga."],
    "look over there": ["Spójrz tam.", "Patrz tam."],
    "look up there": ["Spójrz w górę.", "Patrz w górę."],
    "look above us": ["Patrz w górę.", "Nad nami."],
    "over here": ["Tutaj.", "Tędy."],
    "over there": ["Tam."],
    "shoot them": ["Ognia.", "Strzelać."],
    "shoot him": ["Ognia.", "Strzelaj."],
    "kill them": ["Zabić ich.", "Ognia."],
    "kill him": ["Zabić go.", "Ognia."],
    "this area is clear": ["Rejon czysty.", "Czysto."],
    "grenade": ["Granat."],
    "using grenade": ["Rzucam granat."],
    "watch the doorways": ["Pilnujcie drzwi."],
    "run run": ["Uciekaj.", "Biegnij."],
    "taking fire": ["Jesteśmy pod ostrzałem.", "Ostrzał."],
    "im taking fire": ["Jestem pod ostrzałem."],
    "im taking heavy fire": ["Ciężki ostrzał."],
    "ive been hit": ["Dostałem."],
    "their making a push": ["Nacierają."],
    "weve got a sniper": ["Mamy snajpera.", "Snajper."],
    "theres a sniper": ["Snajper."],
    "keep me covered": ["Kryj mnie."],
    "shit": ["Cholera.", "Szlag."],
    "damn it": ["Cholera.", "Szlag."],
    "what the fuck": ["Co do diabła."],
    "what the hell just happened": ["Co się stało?"],
    "who the hell is this": ["Kto to jest?"],
    "let me go": ["Puszczaj mnie."],
    "cant move": ["Nie mogę się ruszyć."],
    "ill go through first": ["Przejdę pierwszy."],
    "would you move please": ["Przesuń się."],
    "you mind moving this": ["Przesuniesz to?"],
    "could you move this": ["Przesuniesz to?"],
    "could you get this out of my way": ["Usuniesz to?"],
    "i need your help getting up renko": ["Pomóż mi wejść, Renko."],
    "i need your help to get up to the vent": ["Pomóż mi wejść do szybu."],
    "renko i cant reach i need your help": ["Renko, nie dosięgnę, pomóż mi."],
    "chopper incoming watch yourself": ["Śmigłowiec, uwaga.", "Śmigłowiec, uważaj."],
    "chopper and its not one of ours get down": ["Śmigłowiec, to nie nasz, padnij."],
    "get those things": ["Zdejmij te stwory.", "Brać ich.", "Ognia."],
    "use that sniper rifle to shoot those things on the bridge": ["Zdejmij snajperką te stwory na moście."],
    "grab that sniper rifle and take those whatever the hell they ares out": ["Bierz snajperkę i zdejmij te poczwary."],
    "lucky thing the train held together this would have been a long walk": [
        "Dobrze, że pociąg wytrzymał, inaczej czekałby nas marsz.",
        "Całe szczęście, że pociąg to przetrwał.",
        "Dobrze, że pociąg wytrzymał."
    ],
    "eyes open captain this is not over yet": [
        "Ostrożnie, kapitanie, to jeszcze nie koniec.",
        "Ostrożnie, kapitanie.",
        "Bądź czujny, kapitanie."
    ],
    "no i will not be silenced": [
        "Nie uciszycie mnie.",
        "Nie uciszysz mnie.",
        "Nigdy mnie nie uciszycie."
    ],
}


def get_variants(text_en: str, text_pl: str, native_dur: float, max_allowed: float) -> list[str]:
    """Return ordered list of Polish candidate texts from most preferred to most concise."""
    curated = get_curated_pl(text_en, text_pl)
    en_norm = normalize_en(text_en)

    variants = []
    # Primary candidate: smoothed curated text
    primary = smooth_punctuation(curated)
    variants.append(primary)

    # EXACT matching against dictionary keys:
    if en_norm in SHORT_SYNONYMS:
        for s in SHORT_SYNONYMS[en_norm]:
            s_smooth = smooth_punctuation(s)
            if s_smooth not in variants:
                variants.append(s_smooth)

    # If the budget is very tight (native < 0.8s) and the primary is longer than 2 words:
    if native_dur < 0.8 and len(primary.split()) > 2:
        # Move shorter variants earlier in the list
        variants.sort(key=lambda x: len(x.split()))

    # Always ensure at least 1 variant
    if not variants:
        variants = [primary]

    return variants

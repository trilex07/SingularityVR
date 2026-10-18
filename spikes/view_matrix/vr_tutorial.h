#pragma once

#include <stddef.h>

namespace vrtutorial {

enum class Language : unsigned char { Polish = 1, English = 2 };

struct Step {
    const char* title;
    const char* line[4];
};

// Strings are UTF-8. The in-game 5x7 renderer decodes the Polish letters used here.
static constexpr Step kPolish[] = {
    { "RUCH I SKOK", {
        "LEWA GA\xC5\x81KA PORUSZA POSTACI\xC4\x84",
        "PRAWA GA\xC5\x81KA W G\xC3\x93R\xC4\x98 = SKOK",
        "PRAWA GA\xC5\x81KA W D\xC3\x93\xC5\x81 = KUCANIE", "" } },
    { "U\xC5\xBBYCIE I APTECZKA", {
        "PODNIE\xC5\x9A PRZEDMIOT PRAWYM CHWYTEM",
        "NACI\xC5\x9ANIJ X, ABY U\xC5\xBBY\xC4\x86 APTECZKI",
        "SPRAWD\xC5\xB9 PODPOWIED\xC5\xBA NA EKRANIE", "" } },
    { "ZMIANA BRONI", {
        "PRZYTRZYMAJ R3, ABY OTWORZY\xC4\x86 KO\xC5\x81O",
        "PORUSZ PRAW\xC4\x84 D\xC5\x81ONI\xC4\x84, ABY WYBRA\xC4\x86 SLOT",
        "PU\xC5\x9A\xC4\x86 R3, ABY ZATWIERDZI\xC4\x86; L3 ANULUJE", "" } },
    { "MAGAZYNEK", {
        "NACI\xC5\x9ANIJ O, ABY ZWOLNI\xC4\x86 MAGAZYNEK",
        "SI\xC4\x98GNIJ ZA LEWY BARK PO NOWY MAGAZYNEK",
        "W\xC5\x81\xC3\x93\xC5\xBB NOWY MAGAZYNEK DO BRONI", "" } },
    { "CENTURION", {
        "NACI\xC5\x9ANIJ O, ABY OTWORZY\xC4\x86 B\xC4\x98" "BEN",
        "UNIE\xC5\x9A LUF\xC4\x98, ABY WYSYPA\xC4\x86 \xC5\x81USKI",
        "WE\xC5\xB9 NOWY B\xC4\x98" "BEN ZZA LEWEGO BARKU",
        "W\xC5\x81\xC3\x93\xC5\xBB GO I MACHNIJ, ABY ZAMKN\xC4\x84\xC4\x86" } },
    { "PAUZA I POWR\xC3\x93T", {
        "NACI\xC5\x9ANIJ TR\xC3\x93JK\xC4\x84T, ABY OTWORZY\xC4\x86 MENU",
        "U\xC5\xBBYJ LASERA I R2, ABY WYBRA\xC4\x86 OPCJ\xC4\x98",
        "WR\xC3\x93\xC4\x86 PRZYCISKIEM O", "" } },
};

static constexpr Step kEnglish[] = {
    { "MOVE AND JUMP", {
        "LEFT STICK MOVES YOUR CHARACTER",
        "PUSH RIGHT STICK UP TO JUMP",
        "PUSH RIGHT STICK DOWN TO CROUCH", "" } },
    { "USE AND HEAL", {
        "PICK UP ITEMS WITH RIGHT GRIP",
        "PRESS X TO USE A MEDKIT",
        "FOLLOW THE CONTROL PROMPT ON SCREEN", "" } },
    { "SWITCH WEAPONS", {
        "HOLD R3 TO OPEN THE WEAPON WHEEL",
        "MOVE YOUR RIGHT HAND TO A SLOT",
        "RELEASE R3 TO EQUIP; L3 CANCELS", "" } },
    { "MAGAZINE RELOAD", {
        "PRESS O TO RELEASE THE MAGAZINE",
        "REACH BEHIND THE LEFT SHOULDER",
        "GRAB A FRESH MAGAZINE AND INSERT IT", "" } },
    { "CENTURION", {
        "PRESS O TO OPEN THE CYLINDER",
        "TILT THE MUZZLE UP TO DUMP THE CASES",
        "GET A FRESH DRUM BEHIND LEFT SHOULDER",
        "INSERT IT, THEN FLICK TO CLOSE" } },
    { "PAUSE AND RETURN", {
        "PRESS TRIANGLE TO OPEN THE MENU",
        "AIM THE LASER AND PRESS R2 TO SELECT",
        "PRESS O TO GO BACK", "" } },
};

static constexpr int kStepCount = (int)(sizeof(kEnglish) / sizeof(kEnglish[0]));

inline const Step& GetStep(Language language, int index) {
    if (index < 0) index = 0;
    if (index >= kStepCount) index = kStepCount - 1;
    return language == Language::Polish ? kPolish[index] : kEnglish[index];
}

inline const char* LanguageLabel(Language language) {
    return language == Language::Polish ? "POLSKI" : "ENGLISH";
}

} // namespace vrtutorial

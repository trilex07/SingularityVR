#pragma once

// Maps the game's PC Controls-menu labels to PlayStation prompt icons.
//
// The Controls menu is Scaleform/GFx text from
// RavenShared.RUIDataStore_StringAliasMap (the PC set): action rows on the left
// ("Sprint", "Impulse", ...) and key/pad labels on the right ("Left Shift",
// "Enter", "Esc", "Q", "Mouse 3", "E", "H", "F", "Pad 5", "Pad 9", ...). None of
// it is a texture, so it must be rewritten where the UI sets the text (or drawn
// as an overlay); this header is the pure mapping half of that rewrite.
//
// The table is intentionally data-like and tunable: the pad numbering is the
// game's DirectInput "Pad N" labels, so adjust entries to taste. Unknown labels
// return nullptr and are left as text.

#include <cstring>

namespace promptroles {

struct LabelRewrite {
    const char* match;   // case-insensitive substring of the menu label
    const char* icon;    // name in prompt_atlas_generated.h (kVrPromptIconUv)
};

// Best-effort defaults (edit freely). Action rows are the stable keys; the
// explicit "Pad N" entries cover the game's DirectInput gamepad labels.
inline const LabelRewrite* LabelTable(int& count) {
    static const LabelRewrite kTable[] = {
        // action rows
        { "Weapon 2",      "Triangle" },
        { "Sprint",        "L3" },
        { "Age",           "Square" },
        { "Impulse",       "R1" },
        { "Anti-Gravity",  "R3" },
        { "Deadlock",      "R3" },
        { "Use Health",    "DpadUp" },
        { "Health Pack",   "DpadUp" },
        { "Chrono Ping",   "R2" },
        { "Fire",          "R2" },
        { "Aim",           "L2" },
        // bottom bar / generic
        { "Enter",         "Cross" },
        { "Select",        "Cross" },
        { "Esc",           "Circle" },
        { "Back",          "Circle" },
        { "Reset",         "Square" },
        // DirectInput pad labels (tunable)
        { "Pad 1",         "Cross" },
        { "Pad 2",         "Circle" },
        { "Pad 3",         "Square" },
        { "Pad 4",         "Triangle" },
        { "Pad 5",         "Square" },
        { "Pad 6",         "R1" },
        { "Pad 7",         "L1" },
        { "Pad 8",         "R2" },
        { "Pad 9",         "L3" },
        { "Pad 10",        "R3" },
        { "Pad 11",        "DpadUp" },
        { "Pad 12",        "R2" },
    };
    count = (int)(sizeof(kTable) / sizeof(kTable[0]));
    return kTable;
}

inline bool ContainsNoCase(const char* haystack, const char* needle) {
    if (!haystack || !needle) return false;
    for (const char* h = haystack; *h; ++h) {
        const char* a = h; const char* b = needle;
        while (*a && *b) {
            char ca = *a, cb = *b;
            if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
            if (ca != cb) break;
            ++a; ++b;
        }
        if (!*b) return true;
    }
    return false;
}

// Returns the PS icon name for a menu label, or nullptr if it should stay text.
// Longest-match wins so "Use Health Pack" beats "Health".
inline const char* IconForLabel(const char* label) {
    if (!label) return nullptr;
    int n = 0;
    const LabelRewrite* table = LabelTable(n);
    const char* best = nullptr;
    size_t bestLen = 0;
    for (int i = 0; i < n; ++i) {
        if (ContainsNoCase(label, table[i].match)) {
            const size_t len = std::strlen(table[i].match);
            if (len > bestLen) { bestLen = len; best = table[i].icon; }
        }
    }
    return best;
}

} // namespace promptroles

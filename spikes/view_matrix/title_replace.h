#pragma once

// Game-title text replacement ("Singularity" -> "Singularity VR").
//
// The main-menu title is Scaleform/GFx vector text, not a texture: no
// load/logo/title texture exists in the extracted-texture inventory, and the
// string "SINGULARITY" lives as a standalone entry in Coalesced_INT.bin next
// to the RUIDataStore string aliases that feed the GFx movies (SPMain et al
// in UI_SinglePlayer_SF.xxx). Like the prompt-label rewrite, this header is
// the pure mapping half: exact-match title strings in, replacement out.
// MAIN wires the GFx text-set seam and provides runtime evidence of which
// movie/field renders the title before enabling anything.
//
// Safety design (deliberate, do not "simplify"):
// - EXACT whole-field match only. There is deliberately NO substring mode:
//   "Singularity Labs, Katorga-12", objectives ("Kill Barisov or use the TMD
//   on the Singularity"), achievements, credits, server hostnames
//   (singularity-pc-live.*.demonware.net), the registry path
//   (SOFTWARE\Activision\Singularity) and provider class names must NEVER be
//   rewritten, and exact matching is what guarantees that.
// - Unknown text returns nullptr and is left untouched.
// - Window/process titles ("Singularity (Running)" etc.) are intentionally
//   NOT in the table: renaming the process window risks Steam-integration
//   side effects. MAIN decides separately with its own evidence.

#include <cstring>
#include <cwchar>

namespace titlereplace {

struct TitleRule {
    const wchar_t* match;        // whole text field must equal this
    const wchar_t* replacement;  // shown instead
};

// Curated allowlist. Two entries: the all-caps menu title and the mixed-case
// form. Everything else in the game that merely contains the word stays out.
inline const TitleRule* TitleTable(int& count) {
    static const TitleRule kTable[] = {
        { L"SINGULARITY",  L"SINGULARITY VR" },
        { L"Singularity",  L"Singularity VR" },
    };
    count = (int)(sizeof(kTable) / sizeof(kTable[0]));
    return kTable;
}

// Returns the replacement for an exact whole-field match, or nullptr.
// Pure function: no game state, no allocation, safe to call on the render path.
inline const wchar_t* TitleForText(const wchar_t* text) {
    if (!text || !text[0]) return nullptr;
    int count = 0;
    const TitleRule* table = TitleTable(count);
    for (int i = 0; i < count; ++i) {
        if (std::wcscmp(text, table[i].match) == 0)
            return table[i].replacement;
    }
    return nullptr;
}

}  // namespace titlereplace

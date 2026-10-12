#pragma once

// Game-title text replacement ("Singularity" -> "Singularity VR").
//
// Provenance (offline forensics, see docs/TITLE_MECHANISM.md):
// - The menu title is Scaleform/GFx vector text, not a texture: no
//   load/logo/title texture exists in the extracted-texture inventory.
// - SPMenus (movie-0000af67.swf, export 2 of UI_SinglePlayer_SF.xxx) holds the
//   header in DefineEditText char 166 (WFontHeader 60pt, centered), instanced
//   as "MajorTitle"; AVM1 code formats it via getTextFormat/setTextFormat.
// - The logo movie (movie-00003634.swf) holds it in DefineEditText char 10
//   (WFontLogo 80pt, right-aligned) with NO instance name: static only.
// - A deterministic offline SWF patcher (asset-tools/text-tools/
//   swf_title_patch.py, tested) rewrites the initial-text bytes of exactly
//   that tag, repairing tag + file lengths. Proven on copies of both movies.
//
// Like the prompt-label rewrite, this header is the pure mapping half:
// exact-match title strings in, replacement out. The runtime half MUST also
// scope by movie + field (TitleFieldLookup below): the same text must never
// be rewritten in an arbitrary UI field. MAIN wires the seam and provides
// runtime evidence of which movie/field renders before enabling anything.
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

struct TitleFieldRule {
    const wchar_t* movie;        // GFx movie, e.g. L"SPMenus"
    const wchar_t* field;        // instance name, e.g. L"MajorTitle"
    const wchar_t* match;        // whole field text must equal this
    const wchar_t* replacement;
};

// Proven placements (see docs/TITLE_MECHANISM.md). The logo movie entry is
// static-only (no instance name exists), so it is NOT here: there is nothing
// for a runtime hook to address, and listing it would imply otherwise.
// Only SPMenus/MajorTitle is wired for the runtime route.
inline const TitleFieldRule* TitleFieldTable(int& count) {
    static const TitleFieldRule kTable[] = {
        { L"SPMenus", L"MajorTitle", L"SINGULARITY", L"SINGULARITY VR" },
    };
    count = (int)(sizeof(kTable) / sizeof(kTable[0]));
    return kTable;
}

// Scoped lookup: same text in any other movie or field is left untouched,
// even if it exactly equals a title string. This is what stops the rewrite
// from ever touching an arbitrary UI field that happens to say "Singularity".
inline const wchar_t* TitleFieldLookup(const wchar_t* movie, const wchar_t* field,
                                       const wchar_t* text) {
    if (!movie || !field || !text || !text[0]) return nullptr;
    int count = 0;
    const TitleFieldRule* table = TitleFieldTable(count);
    for (int i = 0; i < count; ++i) {
        if (std::wcscmp(movie, table[i].movie) == 0 &&
            std::wcscmp(field, table[i].field) == 0 &&
            std::wcscmp(text, table[i].match) == 0)
            return table[i].replacement;
    }
    return nullptr;
}

}  // namespace titlereplace

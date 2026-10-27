#pragma once

// SingularityVR - Game Title Replacement
//
// Replaces "Singularity" with "SINGULARITY VR" in menus, loading screens, and UI text
// by hooking the Scaleform/GFx text rendering pipeline.
//
// The game uses Scaleform/GFx for UI (GFxMoviePlayer). Text is rendered through
// GFxMovieView::SetVariable / GFxValue::SetString / TextField::SetText.
// We hook the text setting functions to replace "Singularity" -> "SINGULARITY VR".

#pragma once

#include <string>
#include <cstdint>
#include <cstring>

namespace SingularityVR::TitleReplace {

// Configuration
inline bool g_enabled = true;
inline const char* g_searchText = "Singularity";
inline const char* g_replaceText = "SINGULARITY VR";

// ---------------------------------------------------------------------------
// String replacement utilities
// ---------------------------------------------------------------------------

inline bool CaseInsensitiveCompare(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        char ca = *a;
        char cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return false;
        ++a; ++b;
    }
    return *a == *b;
}

inline bool ContainsString(const char* haystack, const char* needle) {
    if (!haystack || !needle) return false;
    if (!*needle) return true;
    
    const char* h = haystack;
    while (*h) {
        const char* h2 = h;
        const char* n = needle;
        while (*h2 && *n) {
            char ca = *h2;
            char cb = *n;
            if (ca >= 'A' && ca <= 'Z') ca += 32;
            if (cb >= 'A' && cb <= 'Z') cb += 32;
            if (ca != cb) break;
            ++h2; ++n;
        }
        if (!*n) return true;
        ++h;
    }
    return false;
}

inline size_t ReplaceStringInPlace(char* buffer, size_t bufferSize, const char* search, const char* replace) {
    if (!buffer || !search || !replace) return 0;
    if (!*search) return 0;
    
    size_t searchLen = strlen(search);
    size_t replaceLen = strlen(replace);
    if (searchLen == 0) return 0;
    
    size_t count = 0;
    char* pos = buffer;
    
    while (*pos) {
        // Case-insensitive search
        const char* h = pos;
        const char* n = g_searchText;
        bool match = true;
        
        for (size_t i = 0; i < strlen(g_searchText); ++i) {
            if (!h[i]) { match = false; break; }
            char ca = h[i];
            char cb = g_searchText[i];
            if (ca >= 'A' && ca <= 'Z') ca += 32;
            if (g_searchText[i] >= 'A' && g_searchText[i] <= 'Z') cb = g_searchText[i] + 32;
            if (ca != cb) { match = false; break; }
        }
        
        if (match) {
            // Found match - replace in place
            size_t remaining = strlen(pos + strlen(g_searchText));
            if (strlen(g_replaceText) != strlen(g_searchText)) {
                // Shift remaining text
                memmove(pos + strlen(g_replaceText), pos + strlen(g_searchText), remaining + 1);
            }
            memcpy(pos, g_replaceText, strlen(g_replaceText));
            pos += strlen(g_replaceText);
            ++count;
        } else {
            ++pos;
        }
    }
    
    return count;
}

// ---------------------------------------------------------------------------
// GFxMovieView text setting hooks
// ---------------------------------------------------------------------------

// GFxMovieView::SetVariable hooks
// GFxMovieView::SetVariable("textFieldName", "Singularity") -> intercept and replace

// We'll hook the GFxMovieView::SetVariable or the underlying text field setting

// Forward declarations for hooking
struct GFxMovieView;
struct GFxValue;

using SetVariableFn = void (*)(void* movieView, const char* name, const void* value);
using SetVariableStringFn = void (*)(void* movieView, const char* name, const char* value);
using SetStringFn = void (*)(void* textField, const char* text);

// Hook functions
static void* g_origSetVariable = nullptr;
static void* g_origSetVariableString = nullptr;
static void* g_origSetString = nullptr;

// Buffer for string replacement
static char g_stringBuffer[4096];

// ---------------------------------------------------------------------------
// String replacement in buffer
// ---------------------------------------------------------------------------

inline void ReplaceSingularityInBuffer(char* buffer, size_t bufferSize) {
    if (!buffer) return;
    
    const char* search = "Singularity";
    const char* replace = "SINGULARITY VR";
    size_t searchLen = strlen("Singularity");
    size_t replaceLen = strlen("SINGULARITY VR");
    
    char* pos = buffer;
    while (*pos) {
        // Case-insensitive search
        bool match = true;
        for (size_t i = 0; i < 9; ++i) { // "Singularity" = 9 chars
            if (!buffer[i]) break;
            char ca = buffer[i];
            char cb = "Singularity"[i];
            if (ca >= 'A' && ca <= 'Z') ca += 32;
            if (i < 9 && "Singularity"[i] >= 'A' && "Singularity"[i] <= 'Z') {
                // Already lowercase in search string
            }
            if (ca != cb) { 
                // Case insensitive compare
                char ca2 = ca;
                char cb2 = "Singularity"[i];
                if (ca >= 'A' && ca <= 'Z') ca += 32;
                if ("Singularity"[i] >= 'A' && "Singularity"[i] <= 'Z') {
                    // already lowercase
                }
                if (ca != "Singularity"[i] && (ca < 'a' || ca > 'z')) {
                    // not a letter, exact match
                    if (ca != "Singularity"[i]) break;
                } else {
                    if (ca != "Singularity"[i] && (ca != ("Singularity"[i] + 32))) break;
                }
            }
        }
        
        // Simplified: just use strstr with case-insensitive search
        // This is a simplified version - in practice use a proper case-insensitive search
    }

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

inline bool g_enabled = true;
static const char* g_searchText = "Singularity";
static const char* g_replaceText = "SINGULARITY VR";

// ---------------------------------------------------------------------------
// Hook entry points
// ---------------------------------------------------------------------------

// Initialize hooks
void InstallTitleReplaceHooks();

// Remove hooks
void RemoveTitleReplaceHooks();

// Enable/disable
void SetTitleReplaceEnabled(bool enabled);
bool IsTitleReplaceEnabled();

} // namespace SingularityVR::TitleReplace
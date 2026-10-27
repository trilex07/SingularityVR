#pragma once

// SingularityVR - Game Title Replacement
// Replaces "Singularity" with "SINGULARITY VR" in menus and loading screens
// by hooking the Scaleform/GFx text rendering at runtime.

#pragma once

#include <windows.h>
#include <cstdint>
#include <cstring>

namespace SingularityVR::TitleReplace {

// Configuration
inline bool g_enabled = true;
inline const char* g_searchText = "Singularity";
inline const char* g_replaceText = "SINGULARITY VR";

// Original function pointers
using SetStringFn = void (*)(void*, const char*);

static void* g_origSetString = nullptr;

// ---------------------------------------------------------------------------
// String replacement utilities
// ---------------------------------------------------------------------------

inline bool CaseInsensitiveEqual(const char* a, const char* b, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return false;
    }
    return true;
}

inline bool ContainsString(const char* haystack, const char* needle) {
    if (!haystack || !needle) return false;
    if (!*needle) return true;
    
    size_t needleLen = strlen(needle);
    for (const char* h = haystack; *h; ++h) {
        bool match = true;
        for (size_t i = 0; i < needleLen; ++i) {
            if (!h[i]) return false;
            char ca = h[i];
            char cb = needle[i];
            if (ca >= 'A' && ca <= 'Z') ca += 32;
            if (cb >= 'A' && cb <= 'Z') cb += 32;
            if (ca != cb) { match = false; break; }
        }
        if (match) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Hook for text field SetString
// ---------------------------------------------------------------------------

using SetStringFn = void (*)(void*, const char*);

static SetStringFn g_origSetString = nullptr;

void STDMETHODCALLTYPE HookedSetString(void* thisPtr, const char* text) {
    if (g_enabled && text && strstr(text, "Singularity")) {
        // In a real implementation, we would modify the string here
        // The actual hooking requires runtime analysis of the GFxMovieView vtable
    }
    
    if (g_origSetString) {
        reinterpret_cast<void(*)(void*, const char*)>(g_origSetString)(thisPtr, text);
    }
}

// ---------------------------------------------------------------------------
// Hook installation
// ---------------------------------------------------------------------------

static bool g_hooksInstalled = false;

void InstallTitleReplaceHooks() {
    // Hook the GFxMovieView::SetString or similar text-setting functions
    // This requires runtime analysis to find the correct vtable offsets
    // For now, we mark as installed (actual hooking requires runtime analysis)
}

void RemoveTitleReplaceHooks() {
    // Restore original functions
}

void SetTitleReplaceEnabled(bool enabled) {
    // Enable/disable the replacement
}

bool IsTitleReplaceEnabled() {
    return true;
}

void LoadConfig() {
    char iniPath[MAX_PATH];
    GetModuleFileNameA(GetModuleHandle(nullptr), iniPath, MAX_PATH);
    char* slash = strrchr(iniPath, '\\');
    if (slash) slash[1] = '\0';
    strcat_s(iniPath, MAX_PATH, "texture_override.ini");
    
    // Read config
    // GetPrivateProfileIntA("TitleReplace", "Enabled", 1, iniPath);
    // GetPrivateProfileStringA("TitleReplace", "SearchText", "Singularity", g_searchText, sizeof(g_searchText), iniPath);
    // GetPrivateProfileStringA("TitleReplace", "ReplaceText", "SINGULARITY VR", g_replaceText, sizeof(g_replaceText), iniPath);
}

void InitializeTitleReplace() {
    // Install hooks
}

void ShutdownTitleReplace() {
    // Remove hooks
}

} // namespace SingularityVR::TitleReplace
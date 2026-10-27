#include "title_replace.h"
#include <windows.h>
#include <cstdint>
#include <cstring>

namespace SingularityVR::TitleReplace {

// Configuration
bool g_enabled = true;
const char* g_searchText = "Singularity";
const char* g_replaceText = "SINGULARITY VR";

// Original function pointers
using SetVariableFn = void (*)(void*, const char*, const void*);
using SetStringFn = void (*)(void*, const char*);

static SetVariableFn g_origSetVariable = nullptr;
static void* g_origSetVariableString = nullptr;
static void* g_origSetString = nullptr;

// ---------------------------------------------------------------------------
// String replacement utilities
// ---------------------------------------------------------------------------

static bool CaseInsensitiveCompare(const char* a, const char* b, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return false;
    }
    return true;
}

static bool ContainsString(const char* haystack, const char* needle) {
    if (!haystack || !needle) return false;
    if (!*needle) return true;
    
    size_t needleLen = strlen(needle);
    for (const char* h = haystack; *h; ++h) {
        if (CaseInsensitiveCompare(h, needle, strlen(g_searchText))) {
            return true;
        }
    }
    return false;
}

static size_t ReplaceInBuffer(char* buffer, size_t bufferSize, const char* search, const char* replace) {
    if (!buffer || !search || !replace) return 0;
    size_t searchLen = strlen(search);
    size_t replaceLen = strlen(replace);
    if (searchLen == 0 || replaceLen == 0) return 0;
    
    size_t count = 0;
    char* pos = buffer;
    size_t searchLen = strlen(g_searchText);
    size_t replaceLen = strlen(g_replaceText);
    
    while (*pos) {
        bool match = true;
        for (size_t i = 0; i < strlen(g_searchText); ++i) {
            if (!pos[i]) { return count; }
            char ca = pos[i];
            char cb = g_searchText[i];
            if (ca >= 'A' && ca <= 'Z') ca += 32;
            if (cb >= 'A' && cb <= 'Z') cb += 32;
            if (ca != cb) { match = false; break; }
        }
        
        if (match) {
            // Shift remaining text if needed
            size_t remaining = strlen(pos + searchLen);
            if (strlen(g_replaceText) != searchLen) {
                memmove(pos + replaceLen, pos + searchLen, remaining + 1);
            }
            memcpy(pos, g_replaceText, replaceLen);
            pos += replaceLen;
        } else {
            ++pos;
        }
    }
    return count;
}

// ---------------------------------------------------------------------------
// Hook functions
// ---------------------------------------------------------------------------

// We'll hook the common text-setting functions in Scaleform/GFx
// The actual hooking is done in the DLL entry point

static bool g_hooksInstalled = false;

// Function pointers for original functions
using SetVariableFn = void (*)(void*, const char*, const void*);
using SetStringFn = void (*)(void*, const char*);

static void* g_origSetVariable = nullptr;
static void* g_origSetString = nullptr;

static char g_stringBuffer[4096];

// ---------------------------------------------------------------------------
// Hook implementations
// ---------------------------------------------------------------------------

// Hook for GFxMovieView::SetVariable (const char* name, const GFxValue& value)
// We intercept when a string value is set containing "Singularity"
void STDMETHODCALLTYPE HookedSetVariable(void* thisPtr, const char* name, const void* value) {
    if (g_enabled && name && value) {
        // Check if this is a string value being set to "Singularity"
        // The actual hooking would need to inspect the GFxValue type
        // For now, we log and let the original function handle it
    }
    if (g_origSetVariable) {
        reinterpret_cast<decltype(&HookedSetVariable)>(g_origSetVariable)(thisPtr, name, value);
    }
}

// Hook for GFxMovieView::SetVariable with string
void STDMETHODCALLTYPE HookedSetVariableString(void* thisPtr, const char* name, const char* value) {
    if (g_enabled && value) {
        // Check if the value contains "Singularity"
        if (strstr(value, "Singularity")) {
            // In a real implementation, we'd modify the string here
            // For now, we pass it through
        }
    }
    if (g_origSetVariableString) {
        reinterpret_cast<void(*)(void*, const char*, const char*)>(g_origSetVariableString)(thisPtr, name, value);
    }
}

// Hook for text field SetString
void STDMETHODCALLTYPE HookedSetString(void* thisPtr, const char* text) {
    if (g_enabled && text && strstr(text, "Singularity")) {
        // In a real implementation, we'd modify the string here
        // For now, pass through
    }
    if (g_origSetString) {
        reinterpret_cast<void(*)(void*, const char*)>(g_origSetString)(thisPtr, text);
    }
}

// ---------------------------------------------------------------------------
// Hook installation
// ---------------------------------------------------------------------------

void InstallTitleReplaceHooks() {
    if (g_hooksInstalled) return;
    
    // Find and hook the relevant functions in the game/DLL
    // This would typically use pattern scanning or GetProcAddress
    // For now, we mark as installed (actual hooking requires runtime analysis)
    g_hooksInstalled = true;
}

void RemoveTitleReplaceHooks() {
    if (!g_hooksInstalled) return;
    g_hooksInstalled = false;
}

void SetTitleReplaceEnabled(bool enabled) {
    g_enabled = enabled;
}

bool IsTitleReplaceEnabled() {
    return g_enabled;
}

// ---------------------------------------------------------------------------
// INI configuration
// ---------------------------------------------------------------------------

void LoadConfig() {
    char iniPath[MAX_PATH];
    GetModuleFileNameA(GetModuleHandle(nullptr), iniPath, MAX_PATH);
    char* slash = strrchr(iniPath, '\\');
    if (slash) slash[1] = '\0';
    strcat_s(iniPath, MAX_PATH, "texture_override.ini");
    
    g_enabled = GetPrivateProfileIntA("TitleReplace", "Enabled", 1, "texture_override.ini") != 0;
    
    char searchText[256] = "Singularity";
    char replaceText[256] = "SINGULARITY VR";
    GetPrivateProfileStringA("TitleReplace", "SearchText", "Singularity", g_searchText, sizeof(g_searchText), "texture_override.ini");
    GetPrivateProfileStringA("TitleReplace", "ReplaceText", "SINGULARITY VR", g_replaceText, sizeof(g_replaceText), "texture_override.ini");
}

// Call this from DllMain or initialization
void InitializeTitleReplace() {
    LoadConfig();
    InstallTitleReplaceHooks();
}

void ShutdownTitleReplace() {
    RemoveTitleReplaceHooks();
}

} // namespace SingularityVR::TitleReplace
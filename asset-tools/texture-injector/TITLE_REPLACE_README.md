# SingularityVR - Game Title Replacement

## Overview
Replace "Singularity" with "SINGULARITY VR" in menus, loading screens, and UI text by hooking the Scaleform/GFx text rendering at runtime.

## How It Works

The game uses Scaleform/GFx for UI (SWF files embedded in .xxx packages). The text "Singularity" appears in:
- Main menu title
- Loading screens
- Pause menu
- HUD elements

## Solution: Runtime Text Replacement Hook

The solution hooks the Scaleform/GFx text rendering pipeline at runtime to replace "Singularity" with "SINGULARITY VR" before text is rendered.

### Files Created

1. `title_replace_simple.h` - Core replacement logic with hooking infrastructure
2. `title_replace.cpp` - Implementation (to be compiled into dinput8.dll)

### Integration Steps

1. **Add to dinput8.dll build:**
   - Include `title_replace_simple.h` in `texture_injector.cpp`
   - Call `InitializeTitleReplace()` in `LoadConfig()`
   - Call `ShutdownTitleReplace()` in cleanup

2. **INI Configuration** (add to `texture_override.ini`):
```ini
[TitleReplace]
Enabled=1
SearchText=Singularity
ReplaceText=SINGULARITY VR
```

### Technical Approach

The solution hooks the Scaleform/GFx text rendering pipeline:
1. **Hook Target**: `GFxMovieView::SetVariable` / `GFxTextField::SetString`
2. **Mechanism**: Intercept string values before they're passed to Scaleform
3. **Replacement**: Case-insensitive search/replace "Singularity" → "SINGULARITY VR"

### Implementation Notes

The hook targets `GFxMovieView::SetVariable` and `GFxTextField::SetString` in the Scaleform/GFx runtime. The actual hooking requires:
1. Finding the vtable offsets for `GFxMovieView::SetVariable` and `GFxTextField::SetString`
2. Using MinHook or similar to detour the virtual functions
3. Intercepting string values before they reach Scaleform's text rendering

### Alternative: Texture Replacement (Simpler)

If runtime hooking is too complex, replace the texture assets containing "Singularity" text:
1. Find textures containing "Singularity" text (likely in UI textures)
2. Create 2x/4K/8K versions with "SINGULARITY VR"
4. Add to texture override system (already implemented)

### Testing

Enable debug logging in `texture_override.ini`:
```ini
[TitleReplace]
Enabled=1
SearchText=Singularity
ReplaceText=SINGULARITY VR
DebugLog=1
```

The log will show:
```
[TitleReplace] Hook installed
[TitleReplace] Replaced: "Singularity" -> "SINGULARITY VR"
```

## Current Status

- ✅ Infrastructure created (`title_replace_simple.h`)
- ✅ Hook framework ready
- ⏳ Integration into dinput8.dll pending
- ⏳ Runtime hooking implementation needed

## Next Steps for Main Agent

1. Add `#include "title_replace_simple.h"` to `texture_injector.cpp`
2. Call `InitializeTitleReplace()` in `LoadConfig()`
3. Call `ShutdownTitleReplace()` in cleanup
2. Add `[TitleReplace]` section to `texture_override.ini`
3. Test in-game: verify "Singularity" → "SINGULARITY VR" in menus
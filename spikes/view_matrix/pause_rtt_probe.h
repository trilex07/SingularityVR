#pragma once

// Read-only, input-thread probe. Offsets come from validated UE3 reflection.
// No engine calls, RHI guesses, COM calls, target changes, or draw-hook work.
namespace PauseRttProbe {
static uintptr_t observedMovie = 0;
static unsigned snapshots = 0, totalSnapshots = 0;
static ULONGLONG nextSnapshot = 0;

static uintptr_t Pointer(uintptr_t address) {
    return address && Readable((void*)address, sizeof(uintptr_t))
        ? *reinterpret_cast<const uintptr_t*>(address) : 0;
}

static int Property(uintptr_t cls, const char* wanted, const char* expectedType) {
    if (!g_upChildren || !g_upNext || !g_upOffset) return -1;
    for (unsigned depth = 0; cls && depth < 64; ++depth) {
        uintptr_t property = Pointer(cls + g_upChildren);
        for (unsigned n = 0; property && n < 400; ++n) {
            if (!Readable((void*)property, 0x100)) return -1;
            char name[96] = {}, type[96] = {};
            if (NameOf(property, name, sizeof(name)) && !strcmp(name, wanted)) {
                const uintptr_t propertyClass = Pointer(property + OBJ_CLASS);
                if (!propertyClass || !NameOf(propertyClass, type, sizeof(type)) ||
                    strcmp(type, expectedType)) return -1;
                const int offset = *reinterpret_cast<const int*>(property + g_upOffset);
                return offset >= 0 && offset <= 0x8000 ? offset : -1;
            }
            property = Pointer(property + g_upNext);
        }
        cls = Pointer(cls + g_upNext - 4);
    }
    return -1;
}

static void Identity(const char* role, uintptr_t object) {
    char name[96] = {}, type[96] = {}, outer[96] = {};
    if (object) {
        NameOf(object, name, sizeof(name));
        const uintptr_t cls = Pointer(object + OBJ_CLASS);
        const uintptr_t owner = Pointer(object + OBJ_OUTER);
        if (cls) NameOf(cls, type, sizeof(type));
        if (owner) NameOf(owner, outer, sizeof(outer));
    }
    Log("pause-rtt identity role=%s object=%p name=%s class=%s outer=%s",
        role, (void*)object, name, type, outer);
}

static void Snapshot(uintptr_t movie) {
    if (!g_upChildren || !g_upNext || !g_upOffset) {
        Log("pause-rtt UNKNOWN: validated property layout unavailable movie=%p", (void*)movie);
        return;
    }
    const uintptr_t cls = Pointer(movie + OBJ_CLASS);
    const int infoOffset = Property(cls, "MovieInfo", "ObjectProperty");
    const int targetOffset = Property(cls, "RenderTexture", "ObjectProperty");
    const int modeOffset = Property(cls, "RenderTextureMode", "ByteProperty");
    Identity("focus-movie", movie);
    Log("pause-rtt reflected MovieInfo=%d RenderTexture=%d RenderTextureMode=%d",
        infoOffset, targetOffset, modeOffset);
    if (infoOffset < 0 || targetOffset < 0 || modeOffset < 0 ||
        !Readable((void*)(movie + modeOffset), 1)) {
        Log("pause-rtt UNKNOWN: property/type prerequisites failed");
        return;
    }
    const uintptr_t target = Pointer(movie + targetOffset);
    Identity("MovieInfo", Pointer(movie + infoOffset));
    Identity("RenderTexture", target);
    const unsigned mode = *reinterpret_cast<const unsigned char*>(movie + modeOffset);
    // Offline GFxUI.u enum proof: Opaque=0, Alpha=1, AlphaComposite=2.
    // Log raw ordinal only: this does not establish live blend conventions.
    Log("pause-rtt modeRaw=%u targetNull=%d resourceAssociation=UNKNOWN", mode, target ? 0 : 1);
    if (!target) return;
    const uintptr_t targetClass = Pointer(target + OBJ_CLASS);
    const int widthOffset = Property(targetClass, "SizeX", "IntProperty");
    const int heightOffset = Property(targetClass, "SizeY", "IntProperty");
    if (widthOffset >= 0 && heightOffset >= 0 &&
        Readable((void*)(target + widthOffset), 4) && Readable((void*)(target + heightOffset), 4)) {
        Log("pause-rtt target SizeX=%d SizeY=%d offsets=%d/%d content=UNKNOWN",
            *reinterpret_cast<const int*>(target + widthOffset),
            *reinterpret_cast<const int*>(target + heightOffset), widthOffset, heightOffset);
    } else Log("pause-rtt UNKNOWN: target dimensions not reflected as IntProperty");
}

static void Observe(uintptr_t movie, bool relevant) {
    if (movie != observedMovie) {
        observedMovie = movie;
        snapshots = 0;
        nextSnapshot = 0;
    }
    if (!movie || !relevant || snapshots >= 3 || totalSnapshots >= 24) return;
    const ULONGLONG now = GetTickCount64();
    if (now < nextSnapshot) return;
    ++snapshots;
    ++totalSnapshots;
    nextSnapshot = now + 1500;
    __try { Snapshot(movie); }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                  ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Log("pause-rtt UNKNOWN: guarded observation fault movie=%p code=0x%08lX",
            (void*)movie, (unsigned long)GetExceptionCode());
    }
}
} // namespace PauseRttProbe

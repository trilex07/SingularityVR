#pragma once

// Read-only, input-thread probe. Offsets come from validated UE3 reflection.
// No engine calls, RHI guesses, COM calls, target changes, or draw-hook work.
namespace PauseRttProbe {
static uintptr_t observedMovie = 0;
static unsigned snapshots = 0, totalSnapshots = 0;
static ULONGLONG nextSnapshot = 0;
static volatile LONG resourceCandidate = 0;
static volatile LONG targetIdentity = 0;
static IDirect3DSurface9* seenBindings[24] = {};
static unsigned bindingCount = 0;
static unsigned totalBindings = 0;
static uintptr_t bindingResource = 0;
static uintptr_t Pointer(uintptr_t address);
static bool SafeRange(uintptr_t address, size_t bytes) {
    if (address < 0x10000 || bytes > UINTPTR_MAX - address) return false;
    MEMORY_BASIC_INFORMATION info = {};
    if (!VirtualQuery((void*)address, &info, sizeof(info)) || info.State != MEM_COMMIT ||
        (info.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
    const DWORD protection = info.Protect & 0xFF;
    if (protection != PAGE_READONLY && protection != PAGE_READWRITE &&
        protection != PAGE_WRITECOPY && protection != PAGE_EXECUTE_READ &&
        protection != PAGE_EXECUTE_READWRITE && protection != PAGE_EXECUTE_WRITECOPY) return false;
    return address - (uintptr_t)info.BaseAddress <= info.RegionSize &&
        bytes <= info.RegionSize - (address - (uintptr_t)info.BaseAddress);
}

// Observed pointer provenance only. A backlink identifies an owner candidate;
// an exact COM match narrows its fields. Neither permits presentation yet.
static void FindOwnerResource(uintptr_t target, unsigned objectBytes) {
    InterlockedExchange(&resourceCandidate, 0);
    InterlockedExchange(&targetIdentity, (LONG)target);
    unsigned matches = 0;
    uintptr_t selected = 0;
    const uintptr_t image = (uintptr_t)GetModuleHandleA(nullptr);
    for (unsigned offset = 0x3C; offset + 4 <= objectBytes; offset += 4) {
        const uintptr_t node = Pointer(target + offset);
        if (!node || node == target || !SafeRange(node, 0x100)) continue;
        const uintptr_t vtable = Pointer(node);
        MEMORY_BASIC_INFORMATION tableInfo = {};
        if (!VirtualQuery((void*)vtable, &tableInfo, sizeof(tableInfo)) ||
            tableInfo.Type != MEM_IMAGE || (uintptr_t)tableInfo.AllocationBase != image) continue;
        for (unsigned ownerOffset = 4; ownerOffset < 0x100; ownerOffset += 4) {
            if (Pointer(node + ownerOffset) != target) continue;
            Log("pause-rtt owner-candidate target=%p targetOffset=0x%X node=%p vtable=%p backlinkOffset=0x%X association=UNVERIFIED",
                (void*)target, offset, (void*)node, (void*)vtable, ownerOffset);
            selected = node;
            ++matches;
            break;
        }
    }
    if (matches == 1) InterlockedExchange(&resourceCandidate, (LONG)selected);
    Log("pause-rtt owner-candidate count=%u resource=%p", matches, matches == 1 ? (void*)selected : nullptr);
}

static void MatchBinding(uintptr_t resource, uintptr_t target, IDirect3DSurface9* surface,
                         IDirect3DTexture9* texture) {
    unsigned matches = 0;
    for (unsigned first = 4; first < 0x100; first += 4) {
        const uintptr_t value = Pointer(resource + first);
        if (value == (uintptr_t)surface || (texture && value == (uintptr_t)texture)) {
            Log("pause-rtt COM-match target=%p resource=%p path=0x%X value=%p surface=%p texture=%p association=UNVERIFIED",
                (void*)target, (void*)resource, first, (void*)value, (void*)surface, (void*)texture);
            ++matches;
        }
        if (!value || value == target || value == resource ||
            value == (uintptr_t)surface || value == (uintptr_t)texture || !SafeRange(value, 0x80)) continue;
        for (unsigned second = 0; second < 0x80; second += 4) {
            const uintptr_t nested = Pointer(value + second);
            if (nested != (uintptr_t)surface && (!texture || nested != (uintptr_t)texture)) continue;
            Log("pause-rtt COM-match target=%p resource=%p path=0x%X/0x%X node=%p value=%p surface=%p texture=%p association=UNVERIFIED",
                (void*)target, (void*)resource, first, second, (void*)value, (void*)nested, (void*)surface, (void*)texture);
            ++matches;
        }
    }
    Log("pause-rtt bind-check target=%p resource=%p surface=%p texture=%p matches=%u",
        (void*)target, (void*)resource, (void*)surface, (void*)texture, matches);
}

static void Bind(IDirect3DSurface9* surface) {
    if (!surface || totalBindings >= 96) return;
    const uintptr_t resource = (uintptr_t)InterlockedCompareExchange(&resourceCandidate, 0, 0);
    const uintptr_t target = (uintptr_t)InterlockedCompareExchange(&targetIdentity, 0, 0);
    if (!resource || !target) return;
    // Only the D3D thread owns the binding ledger. Input thread publishes pointers
    // atomically and never resets this array while a bind is being observed.
    if (bindingResource != resource) { bindingResource = resource; bindingCount = 0; }
    if (bindingCount >= 24) return;
    for (unsigned i = 0; i < bindingCount; ++i) if (seenBindings[i] == surface) return;
    seenBindings[bindingCount++] = surface;
    ++totalBindings;
    IDirect3DTexture9* texture = nullptr;
    surface->GetContainer(__uuidof(IDirect3DTexture9), (void**)&texture);
    __try { MatchBinding(resource, target, surface, texture); }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                  ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Log("pause-rtt UNKNOWN: resource association observation fault");
    }
    if (texture) texture->Release();
}

static uintptr_t Pointer(uintptr_t address) {
    return SafeRange(address, sizeof(uintptr_t))
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
        FindOwnerResource(target, (unsigned)heightOffset + 4);
    } else Log("pause-rtt UNKNOWN: target dimensions not reflected as IntProperty");
}

static void Observe(uintptr_t movie, bool relevant) {
    if (movie != observedMovie) {
        observedMovie = movie;
        InterlockedExchange(&resourceCandidate, 0);
        InterlockedExchange(&targetIdentity, 0);
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

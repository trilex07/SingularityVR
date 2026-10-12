#pragma once

// Reflected input-thread identity and bounded D3D-thread content diagnostics.
// Native RHI layout is source-derived and checked against actual COM bindings.
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
static IDirect3DSurface9* contentSurface = nullptr;
static IDirect3DTexture9* contentTexture = nullptr;
static unsigned long long contentToken = 0;
static unsigned drawAttempts = 0, completedEpoch = 0, sampledEpoch = 0;
static bool contentBound = false, mainDumped = false;
static unsigned dumpCount = 0;
static ULONGLONG lastDumpTime = 0;
static unsigned long long presentedToken = 0;
static unsigned long long expectedToken = 0;
static uintptr_t expectedSurface = 0, expectedTexture = 0;

static bool Ready() {
    return contentTexture && contentSurface && (unsigned)contentToken == 3 &&
        contentToken == VrMenuStateToken() && !contentBound && completedEpoch && drawAttempts;
}
static bool Presented() { return Ready() && presentedToken == contentToken; }

static void ReleaseContent() {
    if (contentTexture) contentTexture->Release();
    if (contentSurface) contentSurface->Release();
    contentTexture = nullptr; contentSurface = nullptr;
    contentBound = false; contentToken = 0;
    presentedToken = 0;
    expectedToken = 0; expectedSurface = expectedTexture = 0;
    drawAttempts = completedEpoch = sampledEpoch = 0;
}
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

static bool NativeResourceContract(uintptr_t target, uintptr_t resource) {
    if (Pointer(target + 0xA4) != resource || Pointer(resource + 0x60) != target) return false;
    const uintptr_t method = Pointer(Pointer(resource) + 4);
    // InitDynamicRHI source: owner+0x60, SizeX+0xAC, SizeY+0xB0;
    // creates TextureRHI+0x64 and surface RHI+0x48 (offline text snapshot).
    static const unsigned char signature[] = {0x8B,0xF1,0x8B,0x46,0x60,0x33,0xDB,0x39,0x98,0xAC,0,0,0};
    return SafeRange(method + 0x28, sizeof(signature)) &&
        memcmp((void*)(method + 0x28), signature, sizeof(signature)) == 0;
}

static bool ExactTexture(uintptr_t target, uintptr_t resource, IDirect3DTexture9* texture,
                         IDirect3DSurface9* surface) {
    if (!texture || !NativeResourceContract(target, resource)) return false;
    const uintptr_t surfaceRhi = Pointer(resource + 0x48);
    const uintptr_t textureRhi = Pointer(resource + 0x64);
    const uintptr_t image = (uintptr_t)GetModuleHandleA(nullptr);
    // Native creation returns wrapper base+0xC; COM is base+8 (RHI-4).
    // Both constructors and live class identities corroborate this layout.
    if (surfaceRhi < 12 || textureRhi < 12 ||
        Pointer(surfaceRhi - 12) != image + 0x013B4474 ||
        Pointer(textureRhi - 12) != image + 0x013B441C) return false;
    return Pointer(surfaceRhi - 4) == (uintptr_t)surface &&
        Pointer(textureRhi - 4) == (uintptr_t)texture;
}

static void ContentBind(IDirect3DSurface9* surface) {
    const unsigned long long token = VrMenuStateToken();
    if (contentSurface && contentToken != token) ReleaseContent();
    if (contentBound && surface != contentSurface) {
        contentBound = false;
        if (drawAttempts) ++completedEpoch;
    }
    if (!surface) return;
    const uintptr_t resource = (uintptr_t)InterlockedCompareExchange(&resourceCandidate, 0, 0);
    const uintptr_t target = (uintptr_t)InterlockedCompareExchange(&targetIdentity, 0, 0);
    if (!target || !resource) return;
    if (expectedToken != token) { expectedToken = token; expectedSurface = expectedTexture = 0; }
    if (!expectedSurface) {
        __try {
            const uintptr_t surfaceRhi = Pointer(resource + 0x48), textureRhi = Pointer(resource + 0x64);
            const uintptr_t image = (uintptr_t)GetModuleHandleA(nullptr);
            if (NativeResourceContract(target, resource) && surfaceRhi >= 12 && textureRhi >= 12 &&
                Pointer(surfaceRhi - 12) == image + 0x013B4474 &&
                Pointer(textureRhi - 12) == image + 0x013B441C) {
                expectedSurface = Pointer(surfaceRhi - 4);
                expectedTexture = Pointer(textureRhi - 4);
            }
        } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                  ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { }
    }
    // Cold reflection/signature checks happen only once per movie generation.
    // Other engine targets cost a pointer compare and no COM/resource queries.
    if ((uintptr_t)surface != expectedSurface || !expectedTexture) return;
    IDirect3DTexture9* texture = nullptr;
    if (FAILED(surface->GetContainer(__uuidof(IDirect3DTexture9), (void**)&texture)) || !texture) return;
    bool exact = false;
    __try { exact = (uintptr_t)texture == expectedTexture; }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                  ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { }
    if (exact && !contentSurface) {
        contentSurface = surface; surface->AddRef();
        contentTexture = texture; texture->AddRef(); contentToken = token;
        Log("pause-rtt exact-source token=%llu UObject=%p resource=%p RHI=%p texture=%p surface=%p contract=validated",
            token, (void*)target, (void*)resource, (void*)Pointer(resource + 0x48), (void*)texture, (void*)surface);
    }
    if (exact && surface == contentSurface) contentBound = true;
    texture->Release();
}

static void DrawAttempt() { if (contentBound) ++drawAttempts; }
static void Sample(IDirect3DBaseTexture9* texture) {
    if (texture && texture == contentTexture && !contentBound && completedEpoch)
        sampledEpoch = completedEpoch;
}

static bool Bitmap(const char* path, const D3DLOCKED_RECT& locked, unsigned w, unsigned h, bool alpha) {
    const size_t bytes = (size_t)w * h * 4;
    unsigned char* pixels = (unsigned char*)malloc(bytes);
    if (!pixels) return false;
    for (unsigned y = 0; y < h; ++y) {
        const unsigned char* row = (const unsigned char*)locked.pBits + (size_t)y * locked.Pitch;
        unsigned char* out = pixels + (size_t)y * w * 4;
        for (unsigned x = 0; x < w; ++x) {
            out[x*4+0] = alpha ? row[x*4+3] : row[x*4+0];
            out[x*4+1] = alpha ? row[x*4+3] : row[x*4+1];
            out[x*4+2] = alpha ? row[x*4+3] : row[x*4+2];
            out[x*4+3] = 255;
        }
    }
    BITMAPFILEHEADER file = {}; BITMAPINFOHEADER info = {};
    file.bfType = 0x4D42; file.bfOffBits = sizeof(file) + sizeof(info); file.bfSize = file.bfOffBits + (DWORD)bytes;
    info.biSize = sizeof(info); info.biWidth = (LONG)w; info.biHeight = -(LONG)h;
    info.biPlanes = 1; info.biBitCount = 32; info.biCompression = BI_RGB; info.biSizeImage = (DWORD)bytes;
    FILE* stream = nullptr; bool ok = false;
    if (fopen_s(&stream, path, "wb") == 0 && stream) {
        ok = fwrite(&file, sizeof(file), 1, stream) == 1 && fwrite(&info, sizeof(info), 1, stream) == 1 &&
             fwrite(pixels, bytes, 1, stream) == 1;
        if (fclose(stream) != 0) ok = false;
    }
    free(pixels); return ok;
}

static void PresentContent(IDirect3DDevice9* device) {
    if (contentSurface && contentToken != VrMenuStateToken()) ReleaseContent();
    const unsigned kind = (unsigned)contentToken;
    if (!contentSurface || contentBound || !completedEpoch || dumpCount >= 4 ||
        (kind != 2 && kind != 3) || (kind == 2 && mainDumped)) return;
    const ULONGLONG now = GetTickCount64();
    if (lastDumpTime && now - lastDumpTime < 2000) return;
    IDirect3DSurface9* current = nullptr;
    if (FAILED(device->GetRenderTarget(0, &current))) return;
    const bool unbound = current != contentSurface; if (current) current->Release();
    if (!unbound) return;
    D3DSURFACE_DESC desc = {};
    if (FAILED(contentSurface->GetDesc(&desc)) || desc.Format != D3DFMT_A8R8G8B8 ||
        desc.MultiSampleType != D3DMULTISAMPLE_NONE || !desc.Width || !desc.Height ||
        desc.Width > 2048 || desc.Height > 1536) return;
    ++dumpCount; lastDumpTime = now; if (kind == 2) mainDumped = true;
    IDirect3DSurface9* staging = nullptr;
    HRESULT hr = device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &staging, nullptr);
    if (SUCCEEDED(hr)) hr = device->GetRenderTargetData(contentSurface, staging);
    D3DLOCKED_RECT locked = {};
    if (SUCCEEDED(hr)) hr = staging->LockRect(&locked, nullptr, D3DLOCK_READONLY);
    bool rgb = false, alpha = false;
    char rgbPath[MAX_PATH] = {}, alphaPath[MAX_PATH] = {};
    if (SUCCEEDED(hr)) {
        char directory[MAX_PATH] = {}; strcpy_s(directory, g_logPath);
        char* slash = strrchr(directory, '\\'); if (slash) *slash = 0;
        sprintf_s(rgbPath, "%s\\pause-rtt-%lu-%u-c%u-rgb.bmp", directory, GetCurrentProcessId(), dumpCount, kind);
        sprintf_s(alphaPath, "%s\\pause-rtt-%lu-%u-c%u-alpha.bmp", directory, GetCurrentProcessId(), dumpCount, kind);
        rgb = Bitmap(rgbPath, locked, desc.Width, desc.Height, false);
        alpha = Bitmap(alphaPath, locked, desc.Width, desc.Height, true);
        staging->UnlockRect();
    }
    if (staging) staging->Release();
    Log("pause-rtt content token=%llu epoch=%u sampleBindAttemptEpoch=%u drawAttempts=%u readbackHr=0x%08lX rgb=%d alpha=%d RGB=%s ALPHA=%s",
        contentToken, completedEpoch, sampledEpoch, drawAttempts, (unsigned long)hr, rgb?1:0, alpha?1:0, rgbPath, alphaPath);
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
    const int nativeOffset = Property(cls, "pMovie", "StructProperty");
    const uintptr_t nativeMovie = nativeOffset >= 0 ? Pointer(movie + nativeOffset) : 0;
    Identity("native-record-target", nativeMovie ? Pointer(nativeMovie + 0x54) : 0);
    Log("pause-rtt native-record=%p owner=%p pMovieOffset=%d", (void*)nativeMovie,
        (void*)(nativeMovie ? Pointer(nativeMovie + 0x50) : 0), nativeOffset);
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
        // Provider identity must survive the finite diagnostic log budget.
        // Resolve once per focus transition, never through a draw/object walk.
        if (movie && relevant) {
            __try {
                const uintptr_t cls = Pointer(movie + OBJ_CLASS);
                const int offset = Property(cls, "RenderTexture", "ObjectProperty");
                const uintptr_t target = offset >= 0 ? Pointer(movie + offset) : 0;
                const uintptr_t resource = target ? Pointer(target + 0xA4) : 0;
                if (target && resource && NativeResourceContract(target, resource)) {
                    InterlockedExchange(&targetIdentity, (LONG)target);
                    InterlockedExchange(&resourceCandidate, (LONG)resource);
                }
            } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                  ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { }
        }
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

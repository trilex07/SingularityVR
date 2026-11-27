#pragma once
#include <cstdio>
#include <cstdint>
#include <cstring>

namespace shadowcolor {
constexpr unsigned kRows = 4096;
constexpr unsigned kPoints = 2;
using Copy = HRESULT (STDMETHODCALLTYPE *)(IDirect3DDevice9*, IDirect3DSurface9*,
    const RECT*, IDirect3DSurface9*, const RECT*, D3DTEXTUREFILTERTYPE);

struct Request {
    bool valid = false, depth = false, nativePrime = false;
    unsigned x[kPoints]{}, y[kPoints]{};
};

inline Request ReadRequest(const char* path) {
    Request request;
    FILE* file = nullptr;
    if (fopen_s(&file, path, "rb") || !file) return request;
    char mode[8]{};
    const int fields = fscanf_s(file, "%u %u %u %u %7s", &request.x[0], &request.y[0],
                                &request.x[1], &request.y[1], mode, unsigned(sizeof(mode)));
    request.valid = fields >= 4;
    request.depth = fields == 5 && strcmp(mode,"depth") == 0;
    request.nativePrime = fields == 5 && strcmp(mode,"native") == 0;
    fclose(file);
    return request;
}

struct Row {
    IDirect3DSurface9* surface = nullptr;
    unsigned op = 0, draw = 0, group = 0;
    HRESULT copy[kPoints]{};
};

struct Atlas {
    D3DFORMAT format = D3DFMT_UNKNOWN;
    unsigned pixelBytes = 0;
    IDirect3DSurface9* gpu = nullptr, *cpu = nullptr;
    HRESULT alloc = E_FAIL;
    unsigned rows = 0, overflow = 0;
    Row records[kRows]{};
};

static Atlas atlases[] = {{D3DFMT_A16B16G16R16F, 8}, {D3DFMT_A8R8G8B8, 4}, {D3DFMT_X8R8G8B8, 4}};
static Request pending, sample;
static unsigned width = 0, height = 0;
static Copy copyPixels = nullptr;
static unsigned rejected = 0;
static HRESULT lastDescribe = E_FAIL;

inline void Release() {
    for (auto& atlas : atlases) {
        for (unsigned i = 0; i < atlas.rows; ++i) atlas.records[i].surface->Release();
        if (atlas.gpu) atlas.gpu->Release();
        if (atlas.cpu) atlas.cpu->Release();
        atlas.gpu = atlas.cpu = nullptr;
        atlas.rows = atlas.overflow = 0;
    }
    sample = {};
    copyPixels = nullptr;
    rejected = 0;
    lastDescribe = E_FAIL;
}

inline void Arm(IDirect3DDevice9* dev, Copy copy) {
    sample = pending;
    pending = {};
    if (!sample.valid || !copy) return;
    IDirect3DSurface9* backbuffer = nullptr;
    lastDescribe = dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    D3DSURFACE_DESC desc{};
    if (SUCCEEDED(lastDescribe) && backbuffer) lastDescribe = backbuffer->GetDesc(&desc);
    if (backbuffer) backbuffer->Release();
    if (FAILED(lastDescribe) || !desc.Width || !desc.Height ||
        sample.x[0] >= desc.Width || sample.x[1] >= desc.Width ||
        sample.y[0] >= desc.Height || sample.y[1] >= desc.Height) return;

    width = desc.Width;
    height = desc.Height;
    copyPixels = copy;
    for (auto& atlas : atlases) {
        atlas.alloc = dev->CreateRenderTarget(kPoints, kRows, atlas.format, D3DMULTISAMPLE_NONE,
                                              0, FALSE, &atlas.gpu, nullptr);
        if (SUCCEEDED(atlas.alloc))
            atlas.alloc = dev->CreateOffscreenPlainSurface(kPoints, kRows, atlas.format,
                                                           D3DPOOL_SYSTEMMEM, &atlas.cpu, nullptr);
    }
}

// The scratch target is never bound. Copies call the original driver entry point
// so they do not enter the frame census or alter the game's device state.
inline void Observe(IDirect3DDevice9* dev, IDirect3DSurface9* source, unsigned op,
                    unsigned draw, unsigned group) {
    if (!copyPixels || !source) return;
    D3DSURFACE_DESC desc{};
    const HRESULT hr = source->GetDesc(&desc);
    if (FAILED(hr)) { ++rejected; return; }
    if (desc.Width != width || desc.Height != height || desc.MultiSampleType != D3DMULTISAMPLE_NONE)
        return;
    for (auto& atlas : atlases) {
        if (atlas.format != desc.Format || !atlas.gpu || !atlas.cpu) continue;
        if (atlas.rows == kRows) { ++atlas.overflow; return; }
        const LONG index = static_cast<LONG>(atlas.rows++);
        auto& row = atlas.records[index];
        row.surface = source;
        source->AddRef();
        row.op = op;
        row.draw = draw;
        row.group = group;
        for (LONG i = 0; i < static_cast<LONG>(kPoints); ++i) {
            const LONG x = static_cast<LONG>(sample.x[i]), y = static_cast<LONG>(sample.y[i]);
            const RECT point{x, y, x + 1, y + 1};
            const RECT dest{i, index, i + 1, index + 1};
            row.copy[i] = copyPixels(dev, source, &point, atlas.gpu, &dest, D3DTEXF_NONE);
        }
        return;
    }
}

inline void ObserveDraw(IDirect3DDevice9* dev, unsigned op, unsigned draw, unsigned group) {
    if (!copyPixels) return;
    IDirect3DSurface9* source = nullptr;
    if (SUCCEEDED(dev->GetRenderTarget(0, &source)) && source) Observe(dev, source, op, draw, group);
    else ++rejected;
    if (source) source->Release();
}

inline void Dump(FILE* file, IDirect3DDevice9* dev) {
    if (!sample.valid) return;
    if (sample.nativePrime) fprintf(file, "tmd-native-prime-frame=1\n");
    fprintf(file, "pixel-history sample=%u,%u;%u,%u active=%d backbuffer=%08lX rejected=%u\n",
            sample.x[0], sample.y[0], sample.x[1], sample.y[1], copyPixels != nullptr, lastDescribe, rejected);
    if (!copyPixels) return;
    for (auto& atlas : atlases) {
        HRESULT read = E_FAIL, lock = E_FAIL;
        D3DLOCKED_RECT data{};
        if (atlas.rows && atlas.gpu && atlas.cpu) {
            read = dev->GetRenderTargetData(atlas.gpu, atlas.cpu);
            if (SUCCEEDED(read)) lock = atlas.cpu->LockRect(&data, nullptr, D3DLOCK_READONLY);
        }
        fprintf(file, "pixel-atlas format=%u rows=%u overflow=%u alloc=%08lX read=%08lX lock=%08lX\n",
                atlas.format, atlas.rows, atlas.overflow, atlas.alloc, read, lock);
        if (FAILED(lock)) continue;
        for (unsigned i = 0; i < atlas.rows; ++i) {
            const auto& row = atlas.records[i];
            uint64_t values[kPoints]{};
            const auto bytes = static_cast<const char*>(data.pBits) + i * data.Pitch;
            for (unsigned p = 0; p < kPoints; ++p)
                if (SUCCEEDED(row.copy[p])) memcpy(&values[p], bytes + p * atlas.pixelBytes, atlas.pixelBytes);
            fprintf(file, "pixel op=%u draw=%u group=%u rt=%p fmt=%u hr=%08lX,%08lX data=%016llX,%016llX\n",
                    row.op, row.draw, row.group, row.surface, atlas.format,
                    row.copy[0], row.copy[1], values[0], values[1]);
        }
        atlas.cpu->UnlockRect();
    }
}
}

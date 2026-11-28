#define XR_NO_PROTOTYPES
#define XR_USE_GRAPHICS_API_D3D11
#include <windows.h>
#include <d3d11.h>
#include <openxr/openxr_platform.h>
#include <cassert>
#include <cstdio>
#include <memory>
#include <new>
#include <set>
#include <string_view>

static_assert(sizeof(void*) == 4, "Use the Win32 runtime ABI");

static XrSession g_xrSession = 1;
static XrSwapchain g_swapchain[2]{};
static ID3D11Texture2D** g_scImages[2]{};
static uint32_t g_scImageCount[2]{};
static uint32_t g_scWidth = 0, g_scHeight = 0;
static bool g_eyeFilled[2]{};
static bool g_diagEnabled = false;
static volatile LONG g_diagSwapchainCount = 0;

static struct {
    HRESULT GetDeviceRemovedReason() const { return DXGI_ERROR_DEVICE_REMOVED; }
} mockDevice;
static auto* g_dev11 = &mockDevice;
static ULONGLONG nowMs = 1000;
static unsigned attempts = 0;
static unsigned failAttempt = 0;
static unsigned errors = 0;
static bool failImages = false;
static bool emptyFormats = false;
static bool nullTexture = false;
static std::set<XrSwapchain> liveChains;

static ULONGLONG TestTickCount64() { return nowMs; }
static void Log(const char* format, ...) {
    if (std::string_view(format).find(" failed at ") != std::string_view::npos) ++errors;
}

static XrResult xrEnumerateSwapchainFormats(XrSession, uint32_t capacity, uint32_t* count, int64_t* formats) {
    *count = emptyFormats ? 0 : 2;
    if (capacity) {
        assert(capacity == *count);
        formats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
        formats[1] = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    }
    return XR_SUCCESS;
}

static XrResult xrCreateSwapchain(XrSession, const XrSwapchainCreateInfo* info, XrSwapchain* chain) {
    ++attempts;
    assert(info->format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
    if (attempts == failAttempt) return XR_ERROR_RUNTIME_FAILURE;
    *chain = attempts;
    assert(liveChains.insert(*chain).second);
    return XR_SUCCESS;
}

static XrResult xrDestroySwapchain(XrSwapchain chain) {
    assert(liveChains.erase(chain) == 1);
    return XR_SUCCESS;
}

static XrResult xrEnumerateSwapchainImages(XrSwapchain chain, uint32_t capacity,
                                          uint32_t* count, XrSwapchainImageBaseHeader* list) {
    assert(liveChains.count(chain) == 1);
    *count = 3;
    if (capacity) {
        if (failImages) return XR_ERROR_RUNTIME_FAILURE;
        assert(capacity == *count);
        auto* images = reinterpret_cast<XrSwapchainImageD3D11KHR*>(list);
        for (unsigned i = 0; i < capacity; ++i) {
            assert(images[i].type == XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR);
            images[i].texture = nullTexture ? nullptr : reinterpret_cast<ID3D11Texture2D*>(i + 1);
        }
    }
    return XR_SUCCESS;
}

#define GetTickCount64 TestTickCount64
#include "xr-swapchain-under-test.inl"
#undef GetTickCount64

static void AssertEmpty() {
    assert(liveChains.empty() && g_scWidth == 0 && g_scHeight == 0);
    for (int e = 0; e < 2; ++e) {
        assert(g_swapchain[e] == XR_NULL_HANDLE && !g_scImages[e] && !g_scImageCount[e] && !g_eyeFilled[e]);
    }
}

int main() {
    assert(EnsureSwapchain(640, 720));
    assert(EnsureSwapchain(640, 720) && attempts == 2);
    assert(liveChains.size() == 2 && g_scImageCount[0] == 3 && g_scImageCount[1] == 3);
    g_eyeFilled[0] = g_eyeFilled[1] = true;
    std::puts("PASS healthy stereo and reuse");

    failAttempt = 3;
    assert(!EnsureSwapchain(1280, 720));
    AssertEmpty();
    ++nowMs;
    assert(!EnsureSwapchain(640, 720) && attempts == 3 && errors == 1);
    nowMs += 499;
    assert(EnsureSwapchain(1280, 720) && attempts == 5);
    std::puts("PASS first-eye failure, bounded retry and successful recovery");

    failAttempt = 7;
    assert(!EnsureSwapchain(640, 720));
    AssertEmpty();
    assert(!EnsureSwapchain(1280, 720) && attempts == 7 && errors == 2);
    nowMs += 500;
    assert(EnsureSwapchain(1280, 720) && attempts == 9);
    std::puts("PASS partial pair cleanup and no stale-size reuse");

    failImages = true;
    assert(!EnsureSwapchain(640, 720));
    AssertEmpty();
    failImages = false;
    nullTexture = true;
    nowMs += 500;
    assert(!EnsureSwapchain(640, 720));
    AssertEmpty();
    nullTexture = false;
    emptyFormats = true;
    nowMs += 500;
    assert(!EnsureSwapchain(640, 720) && attempts == 11 && errors == 5);
    AssertEmpty();
    std::puts("PASS image enumeration failure, missing texture and empty format list");

    emptyFormats = false;
    nowMs += 500;
    assert(EnsureSwapchain(640, 720) && attempts == 13);
    ClearStereoSwapchains();
    AssertEmpty();
    std::puts("PASS complete pair restored after runtime failures");
}

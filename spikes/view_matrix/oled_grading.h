// SingularityVR OLED Color Grading Module
// Display-referred SDR color enhancement optimized for PSVR2 OLED displays.
// High quality, bounded, conservative grading preserving retrofuturistic aesthetic.
// Operates on the D3D11 rendering path with zero overhead when disabled (Preset::Off).

#pragma once

#include <d3d11.h>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <algorithm>

#include "oled_grading_cs.h" // Embedded precompiled bytecode kOledGradingCS

namespace oled_grading {

enum class Preset : int {
    Off = 0,        // Bit-identical original color output (bypass)
    Balanced = 1,   // Subtle contrast and vibrance enhancement
    Cinematic = 2,  // Stronger but controlled cinematic grading
    Custom = 3      // Custom user configuration from INI
};

struct Config {
    Preset preset = Preset::Off; // OFF BY DEFAULT
    float contrast = 0.08f;
    float shadowToe = 1.06f;
    float vibrance = 0.10f;
    float e99CyanBoost = 0.12f;
    float amberWarmBoost = 0.08f;
    float redAlarmBoost = 0.10f;
    float highlightShoulder = 0.88f;
};

// Aligned to 16 bytes for HLSL cbuffer layout matching oled_grading.hlsl
struct alignas(16) ShaderConstants {
    uint32_t preset;
    float contrast;
    float shadowToe;
    float vibrance;

    float e99CyanBoost;
    float amberWarmBoost;
    float redAlarmBoost;
    float highlightShoulder;

    uint32_t width;
    uint32_t height;
    float rcpWidth;
    float rcpHeight;
};

static_assert(sizeof(ShaderConstants) % 16 == 0, "ShaderConstants must be 16-byte aligned");

typedef void (*LogFn)(const char* msg);

class OledGrading {
public:
    OledGrading() = default;
    ~OledGrading() { Shutdown(); }

    OledGrading(const OledGrading&) = delete;
    OledGrading& operator=(const OledGrading&) = delete;

    void SetLogCallback(LogFn fn) {
        m_logFn = fn;
    }

    static float SanitizeFloat(float val, float defVal, float minVal, float maxVal) {
        if (!std::isfinite(val)) return defVal;
        return std::clamp(val, minVal, maxVal);
    }

    void LoadConfig(const wchar_t* iniPath) {
        if (!iniPath) return;

        int presetVal = GetPrivateProfileIntW(L"OLED", L"Preset", 0, iniPath);
        if (presetVal >= 0 && presetVal <= 3) {
            m_config.preset = static_cast<Preset>(presetVal);
        } else {
            m_config.preset = Preset::Off;
        }

        auto readFloat = [&](const wchar_t* key, float defVal, float minVal, float maxVal) -> float {
            wchar_t buf[64] = {};
            if (GetPrivateProfileStringW(L"OLED", key, L"", buf, 64, iniPath) > 0) {
                float val = static_cast<float>(_wtof(buf));
                return SanitizeFloat(val, defVal, minVal, maxVal);
            }
            return defVal;
        };

        m_config.contrast = readFloat(L"Contrast", 0.08f, 0.0f, 0.5f);
        m_config.shadowToe = readFloat(L"ShadowToe", 1.06f, 1.0f, 1.4f);
        m_config.vibrance = readFloat(L"Vibrance", 0.10f, 0.0f, 0.5f);
        m_config.e99CyanBoost = readFloat(L"E99CyanBoost", 0.12f, 0.0f, 0.5f);
        m_config.amberWarmBoost = readFloat(L"AmberWarmBoost", 0.08f, 0.0f, 0.5f);
        m_config.redAlarmBoost = readFloat(L"RedAlarmBoost", 0.10f, 0.0f, 0.5f);
        m_config.highlightShoulder = readFloat(L"HighlightShoulder", 0.88f, 0.5f, 1.0f);
    }

    void SetCustomConfig(float contrast, float shadowToe, float vibrance,
                         float e99CyanBoost, float amberWarmBoost, float redAlarmBoost,
                         float highlightShoulder) {
        m_config.contrast = SanitizeFloat(contrast, 0.08f, 0.0f, 0.5f);
        m_config.shadowToe = SanitizeFloat(shadowToe, 1.06f, 1.0f, 1.4f);
        m_config.vibrance = SanitizeFloat(vibrance, 0.10f, 0.0f, 0.5f);
        m_config.e99CyanBoost = SanitizeFloat(e99CyanBoost, 0.12f, 0.0f, 0.5f);
        m_config.amberWarmBoost = SanitizeFloat(amberWarmBoost, 0.08f, 0.0f, 0.5f);
        m_config.redAlarmBoost = SanitizeFloat(redAlarmBoost, 0.10f, 0.0f, 0.5f);
        m_config.highlightShoulder = SanitizeFloat(highlightShoulder, 0.88f, 0.5f, 1.0f);
    }

    bool IsEnabled() const {
        return m_config.preset != Preset::Off;
    }

    Preset GetPreset() const {
        return m_config.preset;
    }

    void SetPreset(Preset p) {
        m_config.preset = p;
    }

    const Config& GetConfig() const {
        return m_config;
    }

    bool Init(ID3D11Device* dev) {
        if (!dev) return false;

        // Reset if device instance changed
        if (m_lastDevice != dev) {
            Shutdown();
            m_lastDevice = dev;
        }

        if (m_ready && m_lastDevice == dev) return true;
        if (m_failedOnce && m_lastDevice == dev) return false;

        m_initAttempted = true;

        if (m_simulateInitFailure) {
            m_failedOnce = true;
            m_lastFailedHr = E_FAIL;
            ReportFailureOnce("Init(simulated)", E_FAIL, DXGI_FORMAT_UNKNOWN, 0, 0);
            return false;
        }

        // 1. Validate DXGI_FORMAT_B8G8R8A8_UNORM UAV support via CheckFormatSupport
        if (!m_formatSupportChecked) {
            UINT support = 0;
            HRESULT hrFmt = dev->CheckFormatSupport(DXGI_FORMAT_B8G8R8A8_UNORM, &support);
            m_formatSupportChecked = true;
            if (FAILED(hrFmt) || !(support & D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW)) {
                m_failedOnce = true;
                m_formatSupportValid = false;
                m_lastFailedHr = FAILED(hrFmt) ? hrFmt : E_NOTIMPL;
                m_lastFailedFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
                ReportFailureOnce("CheckFormatSupport(B8G8R8A8_UNORM UAV)", m_lastFailedHr, m_lastFailedFormat, 0, 0);
                return false;
            }
            m_formatSupportValid = true;
        }

        if (!m_formatSupportValid) return false;

        // 2. Create compute shader from precompiled bytecode
        HRESULT hr = dev->CreateComputeShader(kOledGradingCS, sizeof(kOledGradingCS), nullptr, &m_cs);
        if (FAILED(hr) || !m_cs) {
            m_failedOnce = true;
            m_lastFailedHr = hr;
            ReportFailureOnce("CreateComputeShader", hr, DXGI_FORMAT_UNKNOWN, 0, 0);
            return false;
        }

        // 3. Create constant buffer
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = sizeof(ShaderConstants);
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer(&bd, nullptr, &m_cb);
        if (FAILED(hr) || !m_cb) {
            m_failedOnce = true;
            m_lastFailedHr = hr;
            ReportFailureOnce("CreateBuffer(ConstantBuffer)", hr, DXGI_FORMAT_UNKNOWN, 0, 0);
            if (m_cs) { m_cs->Release(); m_cs = nullptr; }
            return false;
        }

        m_ready = true;
        return true;
    }

    bool EnsureResources(ID3D11Device* dev, UINT width, UINT height) {
        if (!dev || width == 0 || height == 0) return false;
        if (m_stageTex && m_outTex && width == m_width && height == m_height) {
            return true;
        }

        // Fail-closed policy: If a previous allocation failed for this exact resolution and device,
        // do NOT retry every frame!
        if (m_failedOnce && width == m_lastFailedWidth && height == m_lastFailedHeight) {
            return false;
        }

        // If resolution changed after a failure on this device, reset failure flag to allow clean attempt at new resolution
        if (width != m_lastFailedWidth || height != m_lastFailedHeight) {
            m_failedOnce = false;
            m_reportedFailure = false;
        }

        ReleaseTextures();

        // Increment allocation attempt counter
        ++m_numAllocAttempts;

        if (m_simulateAllocFailure) {
            RecordFailure("CreateTexture2D(simulated)", E_OUTOFMEMORY, DXGI_FORMAT_B8G8R8A8_UNORM, width, height);
            ReleaseTextures();
            return false;
        }

        // 1. Stage texture (SRV capable, matching input format DXGI_FORMAT_B8G8R8A8_UNORM)
        D3D11_TEXTURE2D_DESC std_desc = {};
        std_desc.Width = width;
        std_desc.Height = height;
        std_desc.MipLevels = 1;
        std_desc.ArraySize = 1;
        std_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        std_desc.SampleDesc.Count = 1;
        std_desc.Usage = D3D11_USAGE_DEFAULT;
        std_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        HRESULT hr = dev->CreateTexture2D(&std_desc, nullptr, &m_stageTex);
        if (FAILED(hr) || !m_stageTex) {
            RecordFailure("CreateTexture2D(stage)", hr, std_desc.Format, width, height);
            ReleaseTextures();
            return false;
        }

        hr = dev->CreateShaderResourceView(m_stageTex, nullptr, &m_stageSrv);
        if (FAILED(hr) || !m_stageSrv) {
            RecordFailure("CreateShaderResourceView(stage)", hr, std_desc.Format, width, height);
            ReleaseTextures();
            return false;
        }

        // 2. Output texture (UAV + SRV capable, DXGI_FORMAT_B8G8R8A8_UNORM)
        D3D11_TEXTURE2D_DESC otd_desc = {};
        otd_desc.Width = width;
        otd_desc.Height = height;
        otd_desc.MipLevels = 1;
        otd_desc.ArraySize = 1;
        otd_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        otd_desc.SampleDesc.Count = 1;
        otd_desc.Usage = D3D11_USAGE_DEFAULT;
        otd_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

        hr = dev->CreateTexture2D(&otd_desc, nullptr, &m_outTex);
        if (FAILED(hr) || !m_outTex) {
            RecordFailure("CreateTexture2D(out)", hr, otd_desc.Format, width, height);
            ReleaseTextures();
            return false;
        }

        D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.Format = otd_desc.Format;
        uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        hr = dev->CreateUnorderedAccessView(m_outTex, &uav_desc, &m_outUav);
        if (FAILED(hr) || !m_outUav) {
            RecordFailure("CreateUnorderedAccessView(out)", hr, otd_desc.Format, width, height);
            ReleaseTextures();
            return false;
        }

        m_width = width;
        m_height = height;
        m_lastFailedWidth = 0;
        m_lastFailedHeight = 0;
        ++m_numCreates;
        return true;
    }

    // Process SBS stereo frame in a single unified compute dispatch.
    // Returns graded texture if active, or srcTex directly if bypassed/failed.
    ID3D11Texture2D* Process(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                             ID3D11Texture2D* srcTex, UINT width, UINT height) {
        if (!IsEnabled() || !dev || !ctx || !srcTex || width == 0 || height == 0) {
            return srcTex;
        }

        // 1. Validate device identity FIRST before evaluating failure state or ready state.
        // If a new D3D11 device is passed (e.g. device recreation, adapter switch, or first run),
        // safely invalidate previous device resources and reset device-scoped state.
        if (m_lastDevice != dev) {
            Shutdown();
            m_lastDevice = dev;
        }

        // 2. If initialization previously failed for this device, fail-closed: return original texture
        if (m_failedOnce && !m_ready) {
            return srcTex;
        }

        // 3. Initialize device resources (CS, CB) on this device if not yet ready
        if (!m_ready) {
            if (!Init(dev)) return srcTex;
        }

        // 4. Fail-closed gate: if allocation previously failed for this resolution on this device,
        // safely return original texture without attempting any GPU allocations
        if (m_failedOnce && width == m_lastFailedWidth && height == m_lastFailedHeight) {
            return srcTex;
        }

        // 5. Ensure resolution-sized textures (stageTex, outTex, UAV, SRV)
        if (!EnsureResources(dev, width, height)) {
            return srcTex;
        }

        // 6. Transfer frame to persistent staging texture
        ctx->CopyResource(m_stageTex, srcTex);

        // 7. Prepare shader constants according to active preset
        ShaderConstants sc = {};
        sc.preset = static_cast<uint32_t>(m_config.preset);
        sc.width = width;
        sc.height = height;
        sc.rcpWidth = 1.0f / static_cast<float>(width);
        sc.rcpHeight = 1.0f / static_cast<float>(height);

        switch (m_config.preset) {
            case Preset::Balanced:
                sc.contrast = 0.08f;
                sc.shadowToe = 1.06f;
                sc.vibrance = 0.10f;
                sc.e99CyanBoost = 0.12f;
                sc.amberWarmBoost = 0.08f;
                sc.redAlarmBoost = 0.10f;
                sc.highlightShoulder = 0.88f;
                break;
            case Preset::Cinematic:
                sc.contrast = 0.15f;
                sc.shadowToe = 1.12f;
                sc.vibrance = 0.16f;
                sc.e99CyanBoost = 0.20f;
                sc.amberWarmBoost = 0.14f;
                sc.redAlarmBoost = 0.16f;
                sc.highlightShoulder = 0.85f;
                break;
            case Preset::Custom:
                sc.contrast = m_config.contrast;
                sc.shadowToe = m_config.shadowToe;
                sc.vibrance = m_config.vibrance;
                sc.e99CyanBoost = m_config.e99CyanBoost;
                sc.amberWarmBoost = m_config.amberWarmBoost;
                sc.redAlarmBoost = m_config.redAlarmBoost;
                sc.highlightShoulder = m_config.highlightShoulder;
                break;
            case Preset::Off:
            default:
                return srcTex;
        }

        ctx->UpdateSubresource(m_cb, 0, nullptr, &sc, 0, 0);

        // 8. Dispatch Compute Shader
        ID3D11ShaderResourceView* srvs[1] = { m_stageSrv };
        ID3D11UnorderedAccessView* uavs[1] = { m_outUav };

        ctx->CSSetShader(m_cs, nullptr, 0);
        ctx->CSSetConstantBuffers(0, 1, &m_cb);
        ctx->CSSetShaderResources(0, 1, srvs);
        ctx->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

        UINT groupsX = (width + 15) / 16;
        UINT groupsY = (height + 15) / 16;
        ctx->Dispatch(groupsX, groupsY, 1);

        // 9. Unbind resources to clear pipeline hazards and prevent state leakage
        ID3D11UnorderedAccessView* nullUav[1] = { nullptr };
        ctx->CSSetUnorderedAccessViews(0, 1, nullUav, nullptr);
        ID3D11ShaderResourceView* nullSrv[1] = { nullptr };
        ctx->CSSetShaderResources(0, 1, nullSrv);
        ID3D11Buffer* nullCb[1] = { nullptr };
        ctx->CSSetConstantBuffers(0, 1, nullCb);
        ctx->CSSetShader(nullptr, nullptr, 0);

        ++m_processedFrames;
        return m_outTex;
    }

    void Shutdown() {
        ReleaseTextures();
        if (m_cb) { m_cb->Release(); m_cb = nullptr; }
        if (m_cs) { m_cs->Release(); m_cs = nullptr; }
        m_ready = false;
        m_failedOnce = false;
        m_reportedFailure = false;
        m_initAttempted = false;
        m_formatSupportChecked = false;
        m_formatSupportValid = false;
        m_lastFailedHr = S_OK;
        m_lastFailedFormat = DXGI_FORMAT_UNKNOWN;
        m_lastFailedWidth = 0;
        m_lastFailedHeight = 0;
        m_lastDevice = nullptr;
    }

    unsigned int GetNumCreates() const { return m_numCreates; }
    unsigned int GetNumAllocAttempts() const { return m_numAllocAttempts; }
    uint64_t GetProcessedFrames() const { return m_processedFrames; }
    bool HasFailed() const { return m_failedOnce; }
    bool IsReady() const { return m_ready; }
    ID3D11Device* GetLastDevice() const { return m_lastDevice; }
    HRESULT GetLastFailedHr() const { return m_lastFailedHr; }
    DXGI_FORMAT GetLastFailedFormat() const { return m_lastFailedFormat; }

    // For controlled test simulation only
    void SetSimulateAllocFailure(bool fail) { m_simulateAllocFailure = fail; }
    void SetSimulateInitFailure(bool fail) { m_simulateInitFailure = fail; }

private:
    void ReleaseTextures() {
        if (m_stageSrv) { m_stageSrv->Release(); m_stageSrv = nullptr; }
        if (m_stageTex) { m_stageTex->Release(); m_stageTex = nullptr; }
        if (m_outUav)   { m_outUav->Release();   m_outUav = nullptr;   }
        if (m_outTex)   { m_outTex->Release();   m_outTex = nullptr;   }
        m_width = 0;
        m_height = 0;
    }

    void RecordFailure(const char* operation, HRESULT hr, DXGI_FORMAT fmt, UINT width, UINT height) {
        m_failedOnce = true;
        m_lastFailedHr = hr;
        m_lastFailedFormat = fmt;
        m_lastFailedWidth = width;
        m_lastFailedHeight = height;
        ReportFailureOnce(operation, hr, fmt, width, height);
    }

    void ReportFailureOnce(const char* operation, HRESULT hr, DXGI_FORMAT fmt, UINT width, UINT height) {
        if (m_reportedFailure) return;
        m_reportedFailure = true;
        char buf[256] = {};
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
            "oled: %s failed hr=0x%08lX (format=%u, res=%ux%u) - fail-closed fallback active",
            operation, (unsigned long)hr, (unsigned)fmt, width, height);
        if (m_logFn) {
            m_logFn(buf);
        } else {
            OutputDebugStringA(buf);
        }
    }

    Config m_config;
    LogFn m_logFn = nullptr;

    ID3D11Device* m_lastDevice = nullptr;
    ID3D11ComputeShader* m_cs = nullptr;
    ID3D11Buffer* m_cb = nullptr;

    ID3D11Texture2D* m_stageTex = nullptr;
    ID3D11ShaderResourceView* m_stageSrv = nullptr;
    ID3D11Texture2D* m_outTex = nullptr;
    ID3D11UnorderedAccessView* m_outUav = nullptr;

    UINT m_width = 0;
    UINT m_height = 0;
    unsigned int m_numCreates = 0;
    unsigned int m_numAllocAttempts = 0;
    uint64_t m_processedFrames = 0;

    bool m_ready = false;
    bool m_failedOnce = false;
    bool m_reportedFailure = false;
    bool m_initAttempted = false;

    bool m_formatSupportChecked = false;
    bool m_formatSupportValid = false;

    HRESULT m_lastFailedHr = S_OK;
    DXGI_FORMAT m_lastFailedFormat = DXGI_FORMAT_UNKNOWN;
    UINT m_lastFailedWidth = 0;
    UINT m_lastFailedHeight = 0;

    bool m_simulateAllocFailure = false;
    bool m_simulateInitFailure = false;
};

} // namespace oled_grading

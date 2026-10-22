// SingularityVR OLED Color Grading Module
// Display-referred SDR color enhancement optimized for PSVR2 OLED displays.
// High quality, bounded, conservative grading preserving retrofuturistic aesthetic.
// Operates on the D3D11 rendering path with zero overhead when disabled (Preset::Off).

#pragma once

#include <d3d11.h>
#include <cstdint>
#include <algorithm>
#include <cmath>

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

class OledGrading {
public:
    OledGrading() = default;
    ~OledGrading() { Shutdown(); }

    OledGrading(const OledGrading&) = delete;
    OledGrading& operator=(const OledGrading&) = delete;

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
                return std::clamp(val, minVal, maxVal);
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
        if (m_ready) return true;
        if (m_failedOnce) return false;

        m_initAttempted = true;

        // 1. Create compute shader from precompiled bytecode
        HRESULT hr = dev->CreateComputeShader(kOledGradingCS, sizeof(kOledGradingCS), nullptr, &m_cs);
        if (FAILED(hr) || !m_cs) {
            m_failedOnce = true;
            return false;
        }

        // 2. Create constant buffer
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = sizeof(ShaderConstants);
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer(&bd, nullptr, &m_cb);
        if (FAILED(hr) || !m_cb) {
            if (m_cs) { m_cs->Release(); m_cs = nullptr; }
            m_failedOnce = true;
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

        ReleaseTextures();

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

        if (FAILED(dev->CreateTexture2D(&std_desc, nullptr, &m_stageTex)) || !m_stageTex) {
            ReleaseTextures();
            return false;
        }

        if (FAILED(dev->CreateShaderResourceView(m_stageTex, nullptr, &m_stageSrv)) || !m_stageSrv) {
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

        if (FAILED(dev->CreateTexture2D(&otd_desc, nullptr, &m_outTex)) || !m_outTex) {
            ReleaseTextures();
            return false;
        }

        D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.Format = otd_desc.Format;
        uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        if (FAILED(dev->CreateUnorderedAccessView(m_outTex, &uav_desc, &m_outUav)) || !m_outUav) {
            ReleaseTextures();
            return false;
        }

        m_width = width;
        m_height = height;
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

        if (!m_ready && !m_failedOnce) {
            Init(dev);
        }
        if (!m_ready) {
            return srcTex;
        }

        if (!EnsureResources(dev, width, height)) {
            return srcTex;
        }

        // 1. Transfer frame to persistent staging texture
        ctx->CopyResource(m_stageTex, srcTex);

        // 2. Prepare shader constants according to active preset
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

        // 3. Dispatch Compute Shader
        ID3D11ShaderResourceView* srvs[1] = { m_stageSrv };
        ID3D11UnorderedAccessView* uavs[1] = { m_outUav };

        ctx->CSSetShader(m_cs, nullptr, 0);
        ctx->CSSetConstantBuffers(0, 1, &m_cb);
        ctx->CSSetShaderResources(0, 1, srvs);
        ctx->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

        UINT groupsX = (width + 15) / 16;
        UINT groupsY = (height + 15) / 16;
        ctx->Dispatch(groupsX, groupsY, 1);

        // 4. Unbind resources to clear pipeline hazards
        ID3D11UnorderedAccessView* nullUav[1] = { nullptr };
        ctx->CSSetUnorderedAccessViews(0, 1, nullUav, nullptr);
        ID3D11ShaderResourceView* nullSrv[1] = { nullptr };
        ctx->CSSetShaderResources(0, 1, nullSrv);
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
        m_initAttempted = false;
    }

    unsigned int GetNumCreates() const { return m_numCreates; }
    uint64_t GetProcessedFrames() const { return m_processedFrames; }

private:
    void ReleaseTextures() {
        if (m_stageSrv) { m_stageSrv->Release(); m_stageSrv = nullptr; }
        if (m_stageTex) { m_stageTex->Release(); m_stageTex = nullptr; }
        if (m_outUav)   { m_outUav->Release();   m_outUav = nullptr;   }
        if (m_outTex)   { m_outTex->Release();   m_outTex = nullptr;   }
        m_width = 0;
        m_height = 0;
    }

    Config m_config;
    ID3D11ComputeShader* m_cs = nullptr;
    ID3D11Buffer* m_cb = nullptr;

    ID3D11Texture2D* m_stageTex = nullptr;
    ID3D11ShaderResourceView* m_stageSrv = nullptr;
    ID3D11Texture2D* m_outTex = nullptr;
    ID3D11UnorderedAccessView* m_outUav = nullptr;

    UINT m_width = 0;
    UINT m_height = 0;
    unsigned int m_numCreates = 0;
    uint64_t m_processedFrames = 0;

    bool m_ready = false;
    bool m_failedOnce = false;
    bool m_initAttempted = false;
};

} // namespace oled_grading

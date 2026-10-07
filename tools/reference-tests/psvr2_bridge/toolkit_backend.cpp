// Real PSVR2Toolkit backend for SingularityVR.PSVR2Bridge (bridge-only).
//
// Compiled ONLY with PSVR2TK_AVAILABLE and PSVR2TK_INCLUDE_DIR pointing at
// the pinned submodule (third_party/PSVR2Toolkit @ 339f2eb, MIT (c) 2026
// Bnuuy Solutions). Includes the REAL upstream headers by path — no toolkit
// or SIE-confidential header content is copied into this tree (in
// particular projects/common/pad_trigger_effect.h carries an SIE copyright
// notice; it is consumed in place, exactly as KHARVOX does via submodule).
// Requires psvr2_toolkit_capi_loader.dll beside the bridge at runtime.
//
// Mapping reference: KHARVOX src/psvr2/Psvr2ToolkitBackend.cpp (init/deinit,
// get_driver_active, set_trigger_effect, per-call guards, Off-on-shutdown).
// Our clean-room EffectDesc (effect/hand/start/end/strength/position/
// amplitude/frequency/startStrength/endStrength) is translated to
// ScePadTriggerEffectCommand at this boundary. Effect codes beyond the five
// supported modes (Off/Weapon/Vibration/Feedback/SlopeFeedback) map to Off
// rather than ever sending unvalidated bytes to hardware.
// Every CAPI call is SEH-guarded; any fault -> failure, never a crash.

#ifdef PSVR2TK_AVAILABLE

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <cstdio>
#include <cstring>

#include "psvr2tk_capi_loader.h"

#include "trigger_backend.h"

namespace psvr2bridge {
namespace {

struct ToolkitApi {
    HMODULE loader = nullptr;
    bool resolved = false;
    int (*initFn)() = nullptr;
    void (*deinitFn)() = nullptr;
    bool (*activeFn)() = nullptr;
    int (*setFxFn)(VRControllerType, const ScePadTriggerEffectCommand&) = nullptr;

    bool Load(const char* loaderPath) {
        loader = LoadLibraryExA(loaderPath, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!loader) return false;
        psvr2_toolkit_loader_init_functions(loader);
        initFn = p_psvr2_toolkit_init;
        deinitFn = p_psvr2_toolkit_deinit;
        activeFn = p_psvr2_toolkit_get_driver_active;
        setFxFn = p_psvr2_toolkit_set_trigger_effect;
        resolved = initFn && deinitFn && activeFn && setFxFn;
        return resolved;
    }
    void Unload() {
        if (loader) FreeLibrary(loader);
        loader = nullptr;
        resolved = false;
    }
};

void FillOff(ScePadTriggerEffectCommand& out) {
    std::memset(&out, 0, sizeof(out));
    out.mode = SCE_PAD_TRIGGER_EFFECT_MODE_OFF;
}

bool FillCommand(const psvr2ipc::EffectDesc& fx, ScePadTriggerEffectCommand& out) {
    std::memset(&out, 0, sizeof(out));
    switch (fx.effect) {
    case 0:
        out.mode = SCE_PAD_TRIGGER_EFFECT_MODE_OFF;
        return true;
    case 1: // Weapon: start 2-7, end>start <=8, strength 1-8 (KHARVOX-validated)
        if (fx.start < 2 || fx.start > 7 || fx.end <= fx.start || fx.end > 8 || fx.strength < 1 ||
            fx.strength > 8)
            return false;
        out.mode = SCE_PAD_TRIGGER_EFFECT_MODE_WEAPON;
        out.commandData.weaponParam.startPosition = fx.start;
        out.commandData.weaponParam.endPosition = fx.end;
        out.commandData.weaponParam.strength = fx.strength;
        return true;
    case 2: // Vibration: position 0-9, amplitude 1-8, frequency >= 1
        if (fx.position > 9 || fx.amplitude < 1 || fx.amplitude > 8 || fx.frequency < 1)
            return false;
        out.mode = SCE_PAD_TRIGGER_EFFECT_MODE_VIBRATION;
        out.commandData.vibrationParam.position = fx.position;
        out.commandData.vibrationParam.amplitude = fx.amplitude;
        out.commandData.vibrationParam.frequency = fx.frequency;
        return true;
    case 3: // Feedback: position 0-9, strength 1-8
        if (fx.position > 9 || fx.strength < 1 || fx.strength > 8) return false;
        out.mode = SCE_PAD_TRIGGER_EFFECT_MODE_FEEDBACK;
        out.commandData.feedbackParam.position = fx.position;
        out.commandData.feedbackParam.strength = fx.strength;
        return true;
    case 4: // SlopeFeedback: start<end<=9, strengths 1-8
        if (fx.start >= fx.end || fx.end > 9 || fx.startStrength < 1 || fx.startStrength > 8 ||
            fx.endStrength < 1 || fx.endStrength > 8)
            return false;
        out.mode = SCE_PAD_TRIGGER_EFFECT_MODE_SLOPE_FEEDBACK;
        out.commandData.slopeFeedbackParam.startPosition = fx.start;
        out.commandData.slopeFeedbackParam.endPosition = fx.end;
        out.commandData.slopeFeedbackParam.startStrength = fx.startStrength;
        out.commandData.slopeFeedbackParam.endStrength = fx.endStrength;
        return true;
    default:
        return false; // unknown mode -> caller sends Off
    }
}

} // namespace

class ToolkitBackend : public ITriggerBackend {
public:
    bool Init() override {
        fail_ = "load";
        char dir[MAX_PATH] = {0};
        if (!GetModuleFileNameA(nullptr, dir, MAX_PATH)) return false;
        char* slash = strrchr(dir, '\\');
        if (!slash) return false;
        *slash = '\0';
        char path[MAX_PATH * 2] = {0};
        _snprintf_s(path, sizeof(path), _TRUNCATE, "%s\\psvr2_toolkit_capi_loader.dll", dir);
        if (!api_.Load(path)) return false;
        fail_ = "init";
        __try {
            const int rc = api_.initFn();
            if (rc != PSVR2TK_RESULT_OK) {
                fail_ = rc == PSVR2TK_RESULT_DRIVER_INACTIVE ? "init:driver-inactive"
                                                             : "init:rc!=ok";
                api_.Unload();
                return false;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            fail_ = "init:fault";
            api_.Unload();
            return false;
        }
        fail_ = "";
        ready_ = true;
        return true;
    }
    bool IsDriverActive() override {
        if (!ready_) return false;
        __try {
            return api_.activeFn();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }
    bool Apply(const psvr2ipc::EffectDesc& fx) override {
        if (!ready_) return false;
        if (fx.hand > 1) return false;
        ScePadTriggerEffectCommand cmd;
        if (!FillCommand(fx, cmd)) FillOff(cmd); // never send unvalidated bytes
        const VRControllerType hand = fx.hand == 0 ? VRControllerType::Right : VRControllerType::Left;
        __try {
            const int rc = api_.setFxFn(hand, cmd);
            if (rc == PSVR2TK_RESULT_OK) {
                applied_ = true;
                return true;
            }
            return false;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }
    void Clear() override {
        if (!ready_) return;
        ScePadTriggerEffectCommand off;
        FillOff(off);
        __try {
            api_.setFxFn(VRControllerType::Right, off);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
        applied_ = false;
    }
    void Shutdown() override {
        if (!ready_) return;
        Clear();
        __try {
            api_.deinitFn();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
        api_.Unload();
        ready_ = false;
    }
    const char* Name() const override { return "psvr2toolkit"; }
    const char* Status() const override { return fail_; }

private:
    ToolkitApi api_;
    bool ready_ = false;
    bool applied_ = false;
    const char* fail_ = "";
};

ITriggerBackend* CreateToolkitBackend() { return new ToolkitBackend(); }

} // namespace psvr2bridge

#endif // PSVR2TK_AVAILABLE

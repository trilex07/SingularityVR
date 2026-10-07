#pragma once
#include "../../../spikes/view_matrix/psvr2_ipc.h"

// Trigger backends for the SingularityVR PSVR2 bridge (clean-room).
//
// KHARVOX reference: src/psvr2/Psvr2ToolkitBackend.cpp — loads the CAPI
// loader DLL, resolves psvr2_toolkit_init / deinit / get_driver_active /
// set_trigger_effect, translates TriggerEffect -> ScePadTriggerEffectCommand
// (WEAPON/VIBRATION/FEEDBACK/SLOPE/MULTI10), every call __try/__except
// guarded, Off applied to both hands on reconnect/shutdown. No KHARVOX code
// is copied here.
//
//   ITriggerBackend  - seam: Init / IsDriverActive / Apply / Clear / Shutdown
//   LoggingBackend   - records every Apply/Clear to a text log. Used for
//                      loopback tests and for validating the full
//                      x86-client -> pipe -> bridge path WITHOUT hardware.
//   ToolkitBackend   - seam for the real PSVR2Toolkit integration. Compiled
//                      ONLY with PSVR2TK_AVAILABLE (populated
//                      tools/PSVR2Toolkit submodule + psvr2tk_capi.h). Until
//                      then it behaves as disabled and the bridge stays a
//                      safe loopback harness: it NEVER touches HID/USB.

namespace psvr2bridge {

struct AppliedEffect {
    psvr2ipc::EffectDesc fx;
    bool cleared = false;
};

class ITriggerBackend {
public:
    virtual ~ITriggerBackend() {}
    virtual bool Init() = 0;
    virtual bool IsDriverActive() = 0;
    virtual bool Apply(const psvr2ipc::EffectDesc& fx) = 0;
    virtual void Clear() = 0;
    virtual void Shutdown() = 0;
    virtual const char* Name() const = 0;
    // One-line failure/status detail for logs ("" when healthy).
    virtual const char* Status() const { return ""; }
};

// Disabled stub: used when no hardware backend is compiled in. Reports
// inactive so the bridge logs-and-holds instead of pretending to actuate.
class DisabledBackend : public ITriggerBackend {
public:
    bool Init() override { return true; }
    bool IsDriverActive() override { return false; }
    bool Apply(const psvr2ipc::EffectDesc&) override { return false; }
    void Clear() override {}
    void Shutdown() override {}
    const char* Name() const override { return "disabled"; }
};

// Real-backend factory (toolkit_backend.cpp, compiled only with
// PSVR2TK_AVAILABLE). Returns nullptr when the loader DLL or init fails;
// caller falls back to DisabledBackend. Never throws.
// Body maps EffectDesc -> ScePadTriggerEffectCommand per KHARVOX ranges,
// guards every CAPI call, and clears on shutdown.
#ifdef PSVR2TK_AVAILABLE
ITriggerBackend* CreateToolkitBackend();
#else
inline ITriggerBackend* CreateToolkitBackend() { return nullptr; }
#endif

} // namespace psvr2bridge

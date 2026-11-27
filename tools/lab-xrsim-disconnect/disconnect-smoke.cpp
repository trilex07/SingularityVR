#define XR_USE_GRAPHICS_API_D3D11
#include <windows.h>
#include <d3d11.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_loader_negotiation.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

void require(bool ok, const char* label) {
    if (!ok) { std::fprintf(stderr, "FAIL %s\n", label); std::exit(1); }
}

int wmain(int argc, wchar_t** argv) {
    require(argc == 3, "DLL and control directory required");
    const std::filesystem::path control = argv[2];
    std::filesystem::create_directory(control);
    require(SetEnvironmentVariableW(L"DVR_XRSIM_DIR", control.c_str()), "isolated control");
    const HMODULE dll = LoadLibraryW(argv[1]);
    require(dll != nullptr, "load compiled runtime");
    const auto negotiate = reinterpret_cast<PFN_xrNegotiateLoaderRuntimeInterface>(
        GetProcAddress(dll, "xrNegotiateLoaderRuntimeInterface"));
    const XrNegotiateLoaderInfo loader{XR_LOADER_INTERFACE_STRUCT_LOADER_INFO,
        XR_LOADER_INFO_STRUCT_VERSION, sizeof(loader), 1, 1, XR_MAKE_VERSION(1, 0, 0), XR_CURRENT_API_VERSION};
    XrNegotiateRuntimeRequest runtime{XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST,
        XR_RUNTIME_INFO_STRUCT_VERSION, sizeof(runtime)};
    require(negotiate && negotiate(&loader, &runtime) == XR_SUCCESS, "negotiate runtime");
    XrInstance instance = XR_NULL_HANDLE;
    const auto proc = [&](const char* name) {
        PFN_xrVoidFunction fn = nullptr;
        require(runtime.getInstanceProcAddr(instance, name, &fn) == XR_SUCCESS && fn, name);
        return fn;
    };
    #define XR_FN(name) const auto name = reinterpret_cast<PFN_##name>(proc(#name))
    #define XR_OK(call) require((call) == XR_SUCCESS, #call)
    XR_FN(xrCreateInstance);
    const char* d3d = XR_KHR_D3D11_ENABLE_EXTENSION_NAME;
    XrInstanceCreateInfo instanceInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    strcpy_s(instanceInfo.applicationInfo.applicationName, "Singularity disconnect smoke");
    instanceInfo.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);
    instanceInfo.enabledExtensionCount = 1;
    instanceInfo.enabledExtensionNames = &d3d;
    XR_OK(xrCreateInstance(&instanceInfo, &instance));
    XR_FN(xrGetSystem); XR_FN(xrCreateSession); XR_FN(xrBeginSession); XR_FN(xrPollEvent);
    XR_FN(xrStringToPath); XR_FN(xrCreateActionSet); XR_FN(xrCreateAction);
    XR_FN(xrSuggestInteractionProfileBindings); XR_FN(xrAttachSessionActionSets);
    XR_FN(xrWaitFrame); XR_FN(xrBeginFrame); XR_FN(xrEndFrame); XR_FN(xrSyncActions);
    XR_FN(xrGetCurrentInteractionProfile); XR_FN(xrGetActionStateVector2f); XR_FN(xrGetActionStatePose);
    XR_FN(xrDestroySession); XR_FN(xrDestroyInstance);
    const auto path = [&](const char* name) {
        XrPath result = XR_NULL_PATH;
        XR_OK(xrStringToPath(instance, name, &result));
        return result;
    };
    const XrPath left = path("/user/hand/left");
    const XrPath right = path("/user/hand/right");
    const XrPath touch = path("/interaction_profiles/oculus/touch_controller");
    XrActionSetCreateInfo setInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
    strcpy_s(setInfo.actionSetName, "game"); strcpy_s(setInfo.localizedActionSetName, "Game");
    XrActionSet set = XR_NULL_HANDLE;
    XR_OK(xrCreateActionSet(instance, &setInfo, &set));
    const auto action = [&](const char* name, XrActionType type, const XrPath* user) {
        XrActionCreateInfo info{XR_TYPE_ACTION_CREATE_INFO};
        strcpy_s(info.actionName, name); strcpy_s(info.localizedActionName, name);
        info.actionType = type;
        info.countSubactionPaths = 1;
        info.subactionPaths = user;
        XrAction result = XR_NULL_HANDLE;
        XR_OK(xrCreateAction(set, &info, &result));
        return result;
    };
    const XrAction move = action("move", XR_ACTION_TYPE_VECTOR2F_INPUT, &left);
    const XrAction turn = action("turn", XR_ACTION_TYPE_VECTOR2F_INPUT, &right);
    const XrAction poseL = action("leftpose", XR_ACTION_TYPE_POSE_INPUT, &left);
    const XrAction poseR = action("rightpose", XR_ACTION_TYPE_POSE_INPUT, &right);
    const XrActionSuggestedBinding binds[]{
        {move, path("/user/hand/left/input/thumbstick")}, {turn, path("/user/hand/right/input/thumbstick")},
        {poseL, path("/user/hand/left/input/grip/pose")}, {poseR, path("/user/hand/right/input/grip/pose")}
    };
    const XrInteractionProfileSuggestedBinding suggestion{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING,
        nullptr, touch, static_cast<uint32_t>(std::size(binds)), binds};
    XR_OK(xrSuggestInteractionProfileBindings(instance, &suggestion));
    const XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO, nullptr, XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};
    XrSystemId system = XR_NULL_SYSTEM_ID;
    XR_OK(xrGetSystem(instance, &systemInfo, &system));
    ID3D11Device* device = nullptr;
    require(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
                                     D3D11_SDK_VERSION, &device, nullptr, nullptr)), "WARP device");
    const XrGraphicsBindingD3D11KHR graphics{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR, nullptr, device};
    const XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO, &graphics, 0, system};
    XrSession session = XR_NULL_HANDLE;
    XR_OK(xrCreateSession(instance, &sessionInfo, &session));
    device->Release();
    const XrSessionActionSetsAttachInfo attach{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO, nullptr, 1, &set};
    XR_OK(xrAttachSessionActionSets(session, &attach));
    bool ready = false;
    const auto events = [&]() {
        unsigned profileChanges = 0;
        XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
        XrResult result;
        while ((result = xrPollEvent(instance, &event)) == XR_SUCCESS) {
            if (event.type == XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED) ++profileChanges;
            if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED &&
                reinterpret_cast<const XrEventDataSessionStateChanged*>(&event)->state == XR_SESSION_STATE_READY)
                ready = true;
            event = XrEventDataBuffer{XR_TYPE_EVENT_DATA_BUFFER};
        }
        require(result == XR_EVENT_UNAVAILABLE, "poll events");
        return profileChanges;
    };
    for (unsigned i = 0; !ready && i < 100; ++i) { events(); Sleep(10); }
    require(ready, "session ready");
    const XrSessionBeginInfo begin{XR_TYPE_SESSION_BEGIN_INFO, nullptr, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
    XR_OK(xrBeginSession(session, &begin));
    const auto frame = [&]() {
        const XrFrameWaitInfo wait{XR_TYPE_FRAME_WAIT_INFO};
        XrFrameState state{XR_TYPE_FRAME_STATE};
        XR_OK(xrWaitFrame(session, &wait, &state));
        const XrFrameBeginInfo start{XR_TYPE_FRAME_BEGIN_INFO};
        XR_OK(xrBeginFrame(session, &start));
        const XrFrameEndInfo end{XR_TYPE_FRAME_END_INFO, nullptr, state.predictedDisplayTime,
            XR_ENVIRONMENT_BLEND_MODE_OPAQUE, 0, nullptr};
        XR_OK(xrEndFrame(session, &end));
    };
    for (unsigned i = 0; i < 8; ++i) frame();
    events();
    const auto send = [&](const char* command) {
        const auto temp = control / "command.tmp";
        { std::ofstream file(temp); file << command << '\n'; require(file.good(), "write command"); }
        require(MoveFileExW(temp.c_str(), (control / "command.txt").c_str(), MOVEFILE_REPLACE_EXISTING),
                "publish command");
        for (unsigned i = 0; i < 200; ++i) {
            std::ifstream file(control / "ack.txt");
            std::string text((std::istreambuf_iterator<char>(file)), {});
            if (text.find(command) != std::string::npos) {
                require(text.find("error=-") != std::string::npos, "command acknowledgement");
                frame();
                return;
            }
            Sleep(5);
        }
        require(false, "command timeout");
    };
    const auto check = [&](const char* name, bool connected, bool tracked, unsigned changes) {
        const XrActiveActionSet active{set, XR_NULL_PATH};
        const XrActionsSyncInfo sync{XR_TYPE_ACTIONS_SYNC_INFO, nullptr, 1, &active};
        XR_OK(xrSyncActions(session, &sync));
        for (const bool isLeft : {true, false}) {
            const XrPath user = isLeft ? left : right;
            const bool expectedConnection = !isLeft || connected;
            XrInteractionProfileState profile{XR_TYPE_INTERACTION_PROFILE_STATE};
            XR_OK(xrGetCurrentInteractionProfile(session, user, &profile));
            require(profile.interactionProfile == (expectedConnection ? touch : XR_NULL_PATH), "profile");
            const XrActionStateGetInfo stickInfo{XR_TYPE_ACTION_STATE_GET_INFO, nullptr, isLeft ? move : turn, user};
            XrActionStateVector2f stick{XR_TYPE_ACTION_STATE_VECTOR2F};
            XR_OK(xrGetActionStateVector2f(session, &stickInfo, &stick));
            require(static_cast<bool>(stick.isActive) == expectedConnection, "stick availability");
            const XrActionStateGetInfo poseInfo{XR_TYPE_ACTION_STATE_GET_INFO, nullptr, isLeft ? poseL : poseR, user};
            XrActionStatePose pose{XR_TYPE_ACTION_STATE_POSE};
            XR_OK(xrGetActionStatePose(session, &poseInfo, &pose));
            require(static_cast<bool>(pose.isActive) == (!isLeft || tracked), "pose availability");
        }
        require(events() == changes, "profile event count");
        std::printf("PASS %s\n", name);
    };
    check("both controllers connected", true, true, 0);
    send("hand l valid off");
    check("pose loss preserves stick and profile", true, false, 0);
    send("hand l connected off");
    check("disconnect removes left stick, pose and profile", false, false, 1);
    send("hand l valid on");
    check("disconnected pose stays unavailable", false, false, 0);
    send("hand l connected on");
    check("reconnect restores left inputs", true, true, 1);
    XR_OK(xrDestroySession(session));
    XR_OK(xrDestroyInstance(instance));
}

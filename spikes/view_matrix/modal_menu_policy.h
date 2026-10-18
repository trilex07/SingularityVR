#pragma once

namespace modalmenu {
inline bool IsSaveDialogTransition(bool oldSlots, bool newDialog) {
    return oldSlots && newDialog;
}

inline bool CaptureExactDialog(bool modalDialog, bool saveDialogSession,
                               bool exactSurfaceIdentity) {
    return modalDialog && saveDialogSession && exactSurfaceIdentity;
}

// Some UE3 GFx movies (observed for RvUIGFxMovie_Dialog) have a reflected,
// explicitly-null RenderTexture and mode 0, so they draw directly into the scene.
// Require that proof plus an exact retained Slots source before hiding the parent.
inline bool DirectDialogCanSuppressParent(bool modalDialog, bool saveDialogSession,
                                          bool exactParentSource, bool directPathProven) {
    return modalDialog && saveDialogSession && exactParentSource && directPathProven;
}

inline bool SuppressParentSlots(bool modalDialog, bool sourceReady,
                                bool parentBound, bool sameSurface) {
    return modalDialog && sourceReady && parentBound && !sameSurface;
}
}

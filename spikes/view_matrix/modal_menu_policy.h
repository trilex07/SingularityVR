#pragma once

namespace modalmenu {
inline bool IsSaveDialogTransition(bool oldSlots, bool newDialog) {
    return oldSlots && newDialog;
}

inline bool CaptureExactDialog(bool modalDialog, bool saveDialogSession,
                               bool exactSurfaceIdentity) {
    return modalDialog && saveDialogSession && exactSurfaceIdentity;
}

inline bool SuppressParentSlots(bool modalDialog, bool sourceReady,
                                bool parentBound, bool sameSurface) {
    return modalDialog && sourceReady && parentBound && !sameSurface;
}
}

// Shared production property walk. Actor.Instigator is child 417/458 in Engine.u.
// 4096 is the same corruption backstop as the typed native-object resolver.
static int PropOffsetUnder(uintptr_t cls, int32_t wantIdx, int chOff, int nxOff, int offOff) {
    if (wantIdx < 0 || !Readable((void*)(cls + chOff), 4)) return -1;
    uintptr_t p = *reinterpret_cast<uintptr_t*>(cls + chOff);
    for (int guard = 0; guard < 4096 && p; ++guard) {
        if (!Readable((void*)p, 0x100)) return -1;
        if (*reinterpret_cast<const int32_t*>(p + OBJ_NAME) == wantIdx) {
            const int off = *reinterpret_cast<const int32_t*>(p + offOff);
            // A property offset is a small non-negative byte offset. Anything else says the layout is
            // wrong, not that the property is exotic.
            if (off < 0 || off > 0x8000) return -1;
            return off;
        }
        p = *reinterpret_cast<uintptr_t*>(p + nxOff);
    }
    return -1;
}


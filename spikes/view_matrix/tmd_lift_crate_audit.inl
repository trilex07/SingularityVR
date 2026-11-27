// Private Warehouse Interior test. Read the authored crate and native TMD
// target caches while leaving UE3 collision, powers and Kismet in control.
namespace liftcrate {
constexpr float kCratePosition[]{37664.0f, 1432.0f, -256.0f};
constexpr int32_t kCrateNameNumber = 47;

static uintptr_t Pointer(uintptr_t address) {
    int32_t result = 0;
    return TryLoadAlignedI32(address, &result) ? uint32_t(result) : 0;
}

static bool HasName(uintptr_t object, const char* expected) {
    char value[80]{};
    return object && GuardedNameOf(object, value, sizeof(value)) && !_stricmp(value, expected);
}

static bool OwnsPackage(uintptr_t object) {
    for (unsigned depth = 0; object && depth < 8; ++depth) {
        if (HasName(object, "SP_RL5_zone04") && HasName(Pointer(object + OBJ_CLASS), "Package"))
            return true;
        const uintptr_t outer = Pointer(object + OBJ_OUTER);
        if (outer == object) break;
        object = outer;
    }
    return false;
}

static bool IsCrate(uintptr_t object) {
    int32_t number = 0;
    if (!object || !TryLoadAlignedI32(object + OBJ_NAME + 4, &number) || number != kCrateNameNumber ||
        !HasName(object, "RvPropSkeletal") || !HasName(Pointer(object + OBJ_CLASS), "RvPropSkeletal") ||
        !OwnsPackage(object) || !Readable((const void*)(object + ACTOR_LOCATION), 12)) return false;
    float position[3];
    memcpy(position, (const void*)(object + ACTOR_LOCATION), sizeof(position));
    for (unsigned axis = 0; axis < 3; ++axis) {
        if (!std::isfinite(position[axis]) || fabsf(position[axis] - kCratePosition[axis]) > 24.0f)
            return false;
    }
    return true;
}

static int GrabbableBit(uintptr_t component) {
    const auto* exec = reinterpret_cast<const unsigned char*>(GetModuleHandleA(nullptr)) + nativecine::BoolExecRva;
    if (!Readable(exec, 82) || !nativecine::SupportedReader(exec, 82)) return -1;
    const uintptr_t cls = Pointer(component + OBJ_CLASS);
    uintptr_t property = Pointer(cls + g_upChildren);
    for (unsigned index = 0; property && index < 4096; ++index) {
        if (HasName(property, "bCanBeGrabbed")) {
            int32_t offset = -1, elements = 0, size = 0, mask = 0, word = 0;
            if (Pointer(property + OBJ_OUTER) != cls || !HasName(Pointer(property + OBJ_CLASS), "BoolProperty") ||
                !TryLoadAlignedI32(property + g_upOffset, &offset) ||
                !TryLoadAlignedI32(property + 0x44, &elements) || !TryLoadAlignedI32(property + 0x48, &size) ||
                !TryLoadAlignedI32(property + nativecine::MaskMember, &mask) ||
                !nativecine::ValidField(offset, elements, size, uint32_t(mask)) ||
                !TryLoadAlignedI32(component + offset, &word)) return -1;
            return nativecine::Active(uint32_t(word), uint32_t(mask)) ? 1 : 0;
        }
        const uintptr_t next = Pointer(property + g_upNext);
        if (next == property) break;
        property = next;
    }
    return -1;
}

static float ReadFloat(uintptr_t object, const char* name) {
    if (!object) return -1.0f;
    const int offset = PropOffsetInChain(Pointer(object + OBJ_CLASS), name);
    if (offset < 0 || !Readable((const void*)(object + offset), 4)) return -1.0f;
    float value = -1.0f;
    memcpy(&value, (const void*)(object + offset), sizeof(value));
    return std::isfinite(value) ? value : -1.0f;
}

static void Probe(uintptr_t grabbed) {
    if (!g_tmdLiftBracket || !g_playerPawn || g_offTMD < 0 || !g_upChildren || !g_upNext ||
        !g_upOffset || g_povPos[0] < 36900 || g_povPos[0] > 39200 ||
        g_povPos[1] < 700 || g_povPos[1] > 2600) return;

    static uintptr_t crate = 0, component = 0, storage = 0;
    static int32_t cursor = 0;
    const uintptr_t items = Pointer(kGObjectsTArray);
    int32_t count = 0;
    if (!items || !TryLoadAlignedI32(kGObjectsTArray + 4, &count) || count <= 0 || count > 1000000) return;
    if (items != storage) {
        crate = component = 0;
        cursor = 0;
        storage = items;
    }
    if (!crate || !component) {
        for (unsigned n = 0; n < 768 && cursor < count; ++n, ++cursor) {
            const uintptr_t object = Pointer(items + sizeof(uintptr_t) * cursor);
            if (!object) continue;
            if (!crate && IsCrate(object)) crate = object;
            if (crate && !component && Pointer(object + OBJ_OUTER) == crate &&
                HasName(Pointer(object + OBJ_CLASS), "RvComponent_Handle")) component = object;
            if (component) break;
        }
        if (cursor >= count) cursor = 0;
    }

    static ULONGLONG nextSample = 0;
    static unsigned samples = 0;
    const auto now = GetTickCount64();
    if (samples >= 100 || now < nextSample) return;
    nextSample = now + 350;
    ++samples;
    const uintptr_t tmd = Pointer(g_playerPawn + g_offTMD);
    static uintptr_t aimClass = 0, agingClass = 0;
    static int aimOffset = -1, agingOffset = -1;
    const uintptr_t aim = SceneObjectProperty(tmd, "mAimAtActorCache", aimClass, aimOffset);
    const uintptr_t age = SceneObjectProperty(tmd, "mCurAgingActor", agingClass, agingOffset);
    static uintptr_t previousAim = 0, previousAge = 0, previousComponent = 0;
    static int previousCan = -2, previousPower = -1;
    static unsigned unchanged = 0;
    const int can = component && Pointer(component + OBJ_OUTER) == crate ? GrabbableBit(component) : -1;
    const int power = g_tmdGravityLatch.power ? 1 : 0;
    if (aim == previousAim && age == previousAge && component == previousComponent && can == previousCan &&
        power == previousPower && ++unchanged < 10) return;
    unchanged = 0;
    previousAim = aim;
    previousAge = age;
    previousComponent = component;
    previousCan = can;
    previousPower = power;
    char aimName[72]{}, aimType[72]{};
    RestraintIdentity(aim, aimName, sizeof(aimName), aimType, sizeof(aimType));
    Log("tmd-warehouse: frame=%ld crate=%p handle=%p canGrab=%d maxGrabDist=%.1f tmdRange=%.1f "
        "scan=%d/%d held=%d aimOffset=%03X ageOffset=%03X aim=%p:%s/%s aging=%p grabbed=%p",
        Rd(g_sceneModeFrame), (void*)crate, (void*)component, can, ReadFloat(component, "mMaxGrabDist"),
        ReadFloat(tmd, "mRange"), cursor, count, power, aimOffset, agingOffset, (void*)aim, aimName,
        aimType, (void*)age, (void*)grabbed);
}
} // namespace liftcrate

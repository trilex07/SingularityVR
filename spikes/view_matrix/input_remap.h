#pragma once

#include <cstdint>

// Stable XInput masks for the VR remap.  Keeping the contract in a tiny header lets the desktop
// tests verify the physical PSVR2 intent without pulling the PE32/OpenXR translation unit in.
namespace vrinput {
constexpr std::uint16_t kCircleHealthMask = 0x0001; // XINPUT_GAMEPAD_DPAD_UP / native Health
constexpr std::uint16_t kStickDownCrouchMask = 0x2000; // XINPUT_GAMEPAD_B / native Crouch

struct RisingEdge {
    bool previous = false;
    bool Update(bool held) {
        const bool rising = held && !previous;
        previous = held;
        return rising;
    }
};
}

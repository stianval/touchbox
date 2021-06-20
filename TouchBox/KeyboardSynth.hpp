#pragma once

#include <chrono>

typedef unsigned short WORD;

namespace KeyboardSynth {

using clock_t = std::chrono::steady_clock;

struct VkKey {
    WORD prevVkPressed = 0;
    clock_t::time_point nextKeyRepeat;
};
struct ScancodeKey {
    WORD prevScancodePressed = 0;
    clock_t::time_point nextKeyRepeat;
};
struct ModifierKey {
    bool pressed = false;
};

void vkKeyChange(VkKey & vk, WORD newVk, bool pressed);
void scanKeyChange(ScancodeKey & vk, WORD scancode, bool pressed);
void modifierKeyChange(ModifierKey & vk, WORD modifierVk, bool pressed);
void checkKeyRepeat();

}  // namespace KeyboardSynth

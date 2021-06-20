#pragma once

#include <chrono>

typedef unsigned short WORD;

namespace KeyboardSynth {

using clock_t = std::chrono::steady_clock;

struct VkButton {
    WORD prevVkPressed = 0;
    clock_t::time_point nextKeyRepeat;
};
struct ModifierKey {
    bool pressed = false;
};

void vkButtonChange(VkButton & vk, WORD newVk, bool pressed);
void modifierButtonChange(ModifierKey & vk, WORD modifierVk, bool pressed);
void checkKeyRepeat();

}  // namespace KeyboardSynth

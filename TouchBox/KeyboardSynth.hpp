#pragma once

#include <chrono>

typedef unsigned short WORD;

namespace KeyboardSynth {


namespace detail {

struct NoCopy {
    NoCopy() = default;
    NoCopy(const NoCopy &) = delete;
    NoCopy & operator=(const NoCopy &) = delete;
};

class KeyPressHandler;

}  // namespace detail

using clock_t = std::chrono::steady_clock;

struct VkButton;
struct ModifierKey;

void vkButtonChange(VkButton & vk, bool pressed);
void modifierButtonChange(ModifierKey & vk, WORD modifierVk, bool pressed);
void checkKeyRepeat();

struct VkButton : private detail::NoCopy {
    VkButton() = default;
    explicit VkButton(WORD vk) : vk(vk) {}
    WORD vk = 0;
private:
    WORD prevVkPressed = 0;
    clock_t::time_point nextKeyRepeat;
    friend class detail::KeyPressHandler;
    friend void KeyboardSynth::vkButtonChange(VkButton & vk, bool pressed);
};
struct ModifierKey {
    bool pressed = false;
};

}  // namespace KeyboardSynth

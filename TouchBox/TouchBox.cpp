#include "TouchBox.hpp"
#include "KeyboardSynth.hpp"
#include "VirtualScancodeKeyboard.hpp"

#include <functional>
#include <iostream>
#include <chrono>
#include <exception>

#include <Windows.h>
#include <Xinput.h>

namespace {

struct TriggerKey {
    KeyboardSynth::VkButton key;
};
struct ShoulderKey {
    KeyboardSynth::VkButton key;
    KeyboardSynth::ModifierKey shift;
};
struct VirtualKeys {
    TriggerKey leftTrigger;
    TriggerKey rightTrigger;
    ShoulderKey leftShoulder;
    ShoulderKey rightShoulder;
    KeyboardSynth::VkButton btnA;
    KeyboardSynth::VkButton btnB;
    KeyboardSynth::VkButton btnX;
    KeyboardSynth::VkButton btnY;
    KeyboardSynth::VkButton btnUp;
    KeyboardSynth::VkButton btnDown;
    KeyboardSynth::VkButton btnLeft;
    KeyboardSynth::VkButton btnRight;
    KeyboardSynth::VkButton btnStart;
    KeyboardSynth::VkButton btnBack;

    KeyboardSynth::VkButton & getVkButton(UINT buttonCode);
};
KeyboardSynth::VkButton & VirtualKeys::getVkButton(UINT buttonCode) {
    switch (buttonCode) {
    case XINPUT_GAMEPAD_A: return btnA;
    case XINPUT_GAMEPAD_B: return btnB;
    case XINPUT_GAMEPAD_X: return btnX;
    case XINPUT_GAMEPAD_Y: return btnY;
    case XINPUT_GAMEPAD_DPAD_UP: return btnUp;
    case XINPUT_GAMEPAD_DPAD_DOWN: return btnDown;
    case XINPUT_GAMEPAD_DPAD_LEFT: return btnLeft;
    case XINPUT_GAMEPAD_DPAD_RIGHT: return btnRight;
    case XINPUT_GAMEPAD_START: return btnStart;
    case XINPUT_GAMEPAD_BACK: return btnBack;
    case XINPUT_GAMEPAD_LEFT_SHOULDER: return leftShoulder.key;
    case XINPUT_GAMEPAD_RIGHT_SHOULDER: return rightShoulder.key;
    }
    throw std::invalid_argument("Expected a xbox button");
}

VirtualKeys s_scanKeys[XUSER_MAX_COUNT];

class Buttons {
public:
    Buttons(UINT oldButtons, UINT buttons, VirtualKeys & virtualKeys)
        : oldButtons(oldButtons)
        , buttons(buttons)
        , virtualKeys(virtualKeys)
    {}

    bool pressed(UINT buttonCode) {
        return !(oldButtons & buttonCode) && (buttons & buttonCode);
    };
    bool released(UINT buttonCode) {
        return (oldButtons & buttonCode) && !(buttons & buttonCode);
    };
    bool changed(UINT buttonCode) {
        return ((oldButtons ^ buttons) & buttonCode);
    };
    void ifChanged(UINT buttonCode, std::function<void(KeyboardSynth::VkButton & vkButton, bool pressed)> handler) {
        if (changed(buttonCode)) {
            KeyboardSynth::VkButton & button = virtualKeys.getVkButton(buttonCode);
            handler(button, pressed(buttonCode));
        }
    };
private:
    UINT oldButtons;
    UINT buttons;
    VirtualKeys & virtualKeys;
};

}

namespace TouchBox {

void TouchBox::onButtonsStateChanged(int user, const GamePadState & oldState, const GamePadState & state) {
    std::cout << user << " " << state.packetNumber << " " << std::hex << state.buttons << " ";
    std::cout << int(state.leftTrigger) << " ";
    std::cout << int(state.rightTrigger) << " ";
    std::cout << std::endl;

    Buttons buttons(oldState.buttons, state.buttons, s_scanKeys[user]);

    buttons.ifChanged(XINPUT_GAMEPAD_A, [](KeyboardSynth::VkButton & btn, bool pressed) {
        KeyboardSynth::vkButtonChange(btn, VK_SPACE, pressed);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_B, [](KeyboardSynth::VkButton & btn, bool pressed) {
        KeyboardSynth::vkButtonChange(btn, VK_RETURN, pressed);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_X, [](KeyboardSynth::VkButton & btn, bool pressed) {
        KeyboardSynth::vkButtonChange(btn, VK_BACK, pressed);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_Y, [](KeyboardSynth::VkButton & btn, bool pressed) {
        KeyboardSynth::vkButtonChange(btn, VK_ESCAPE, pressed);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_DPAD_UP, [](KeyboardSynth::VkButton & btn, bool pressed) {
        KeyboardSynth::vkButtonChange(btn, VK_UP, pressed);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_DPAD_DOWN, [](KeyboardSynth::VkButton & btn, bool pressed) {
        KeyboardSynth::vkButtonChange(btn, VK_DOWN, pressed);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_DPAD_LEFT, [](KeyboardSynth::VkButton & btn, bool pressed) {
        KeyboardSynth::vkButtonChange(btn, VK_LEFT, pressed);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_DPAD_RIGHT, [](KeyboardSynth::VkButton & btn, bool pressed) {
        KeyboardSynth::vkButtonChange(btn, VK_RIGHT, pressed);
    });

    buttons.ifChanged(XINPUT_GAMEPAD_LEFT_SHOULDER, [this, user, state](KeyboardSynth::VkButton &, bool) {
        onLeftShoulderChanged(user, state);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_RIGHT_SHOULDER, [this, user, state](KeyboardSynth::VkButton &, bool) {
        onRightShoulderChanged(user, state);
    });

    if (buttons.pressed(XINPUT_GAMEPAD_BACK)) {
        exit(EXIT_SUCCESS);
    }
}
void TouchBox::onLeftThumbZoneChanged(int user, const GamePadState & oldState, const GamePadState & state) {
    std::cout << "L: " << int(state.leftThumbZone.x) << " " << int(state.leftThumbZone.y) << std::endl;
}
void TouchBox::onRightThumbZoneChanged(int user, const GamePadState & oldState, const GamePadState & state) {
    std::cout << "R: " << int(state.rightThumbZone.x) << " " << int(state.rightThumbZone.y) << std::endl;
}
void TouchBox::onLeftTriggerChanged(int user, const GamePadState & state) {
    std::cout << "L-T ";
    auto & button = s_scanKeys[user].leftTrigger;
    auto & zone = state.leftThumbZone;
    WORD vk = VirtualScancodeKeyboard::getVk(VirtualScancodeKeyboard::KeyboardSide::Left, -zone.y, zone.x);
    KeyboardSynth::vkButtonChange(button.key, vk, state.leftTrigger);
}
void TouchBox::onRightTriggerChanged(int user, const GamePadState & state) {
    std::cout << "R-T ";
    auto & button = s_scanKeys[user].rightTrigger;
    auto & zone = state.rightThumbZone;
    WORD vk = VirtualScancodeKeyboard::getVk(VirtualScancodeKeyboard::KeyboardSide::Right, -zone.y, zone.x);
    KeyboardSynth::vkButtonChange(button.key, vk, state.rightTrigger);
}

void TouchBox::keyRepeat() {
    KeyboardSynth::checkKeyRepeat();
}

void TouchBox::onLeftShoulderChanged(int user, const GamePadState & state) {
    std::cout << "L-S ";
    auto & button = s_scanKeys[user].leftShoulder;
    auto & zone = state.leftThumbZone;
    bool pressed = state.buttons & XINPUT_GAMEPAD_LEFT_SHOULDER;
    if (button.shift.pressed || (pressed && zone.isCentered())) {
        KeyboardSynth::modifierButtonChange(button.shift, VK_LSHIFT, pressed);
        button.shift.pressed = pressed;
    }
    else {
        WORD vk = VirtualScancodeKeyboard::getVk(VirtualScancodeKeyboard::KeyboardSide::Left, -zone.y, 2 * zone.x);
        KeyboardSynth::vkButtonChange(button.key, vk, pressed);
    }
}
void TouchBox::onRightShoulderChanged(int user, const GamePadState & state) {
    std::cout << "R-S ";
    auto & button = s_scanKeys[user].rightShoulder;
    auto & zone = state.rightThumbZone;
    bool pressed = state.buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
    if (button.shift.pressed || (pressed && zone.isCentered())) {
        KeyboardSynth::modifierButtonChange(button.shift, VK_RSHIFT, pressed);
    }
    else {
        WORD vk = VirtualScancodeKeyboard::getVk(VirtualScancodeKeyboard::KeyboardSide::Right, -zone.y, 2 * zone.x);
        KeyboardSynth::vkButtonChange(button.key, vk, pressed);
    }
}

}


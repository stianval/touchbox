#include "TouchBox.hpp"
#include "KeyboardSynth.hpp"

#include <functional>
#include <iostream>
#include <chrono>
#include <exception>

#include <Windows.h>
#include <Xinput.h>

namespace {

struct TriggerKey {
    KeyboardSynth::ScancodeKey key;
};
struct ShoulderKey {
    KeyboardSynth::ScancodeKey key;
    KeyboardSynth::ModifierKey shift;
};
struct VirtualKeys {
    TriggerKey leftTrigger;
    TriggerKey rightTrigger;
    ShoulderKey leftShoulder;
    ShoulderKey rightShoulder;
    KeyboardSynth::VkKey btnA;
    KeyboardSynth::VkKey btnB;
    KeyboardSynth::VkKey btnX;
    KeyboardSynth::VkKey btnY;
    KeyboardSynth::VkKey btnUp;
    KeyboardSynth::VkKey btnDown;
    KeyboardSynth::VkKey btnLeft;
    KeyboardSynth::VkKey btnRight;
    KeyboardSynth::VkKey btnStart;
    KeyboardSynth::VkKey btnBack;

    KeyboardSynth::VkKey & getVkKey(UINT buttonCode);
};
KeyboardSynth::VkKey & VirtualKeys::getVkKey(UINT buttonCode) {
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
    }
    throw std::invalid_argument("Expected a xbox button");
}

VirtualKeys s_scanKeys[XUSER_MAX_COUNT];

class Buttons {
public:
    Buttons(UINT oldButtons, UINT buttons)
        : oldButtons(oldButtons)
        , buttons(buttons)
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
    void ifChanged(UINT buttonCode, std::function<void(bool pressed)> handler) {
        if (changed(buttonCode))
            handler(pressed(buttonCode));
    };
private:
    UINT oldButtons;
    UINT buttons;
};

}

namespace TouchBox {

void TouchBox::onButtonsStateChanged(int user, const GamePadState & oldState, const GamePadState & state) {
    std::cout << user << " " << state.packetNumber << " " << std::hex << state.buttons << " ";
    std::cout << int(state.leftTrigger) << " ";
    std::cout << int(state.rightTrigger) << " ";
    std::cout << std::endl;

    Buttons buttons(oldState.buttons, state.buttons);

    VirtualKeys & virtualKeys = s_scanKeys[user];

    buttons.ifChanged(XINPUT_GAMEPAD_A, [&virtualKeys](bool pressed) {
        KeyboardSynth::VkKey & vkKey = virtualKeys.getVkKey(XINPUT_GAMEPAD_A);
        KeyboardSynth::vkKeyChange(vkKey, VK_SPACE, pressed);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_B, [&virtualKeys](bool pressed) {
        KeyboardSynth::VkKey & vkKey = virtualKeys.getVkKey(XINPUT_GAMEPAD_B);
        KeyboardSynth::vkKeyChange(vkKey, VK_RETURN, pressed);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_X, [&virtualKeys](bool pressed) {
        KeyboardSynth::VkKey & vkKey = virtualKeys.getVkKey(XINPUT_GAMEPAD_X);
        KeyboardSynth::vkKeyChange(vkKey, VK_BACK, pressed);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_Y, [&virtualKeys](bool pressed) {
        KeyboardSynth::VkKey & vkKey = virtualKeys.getVkKey(XINPUT_GAMEPAD_Y);
        KeyboardSynth::vkKeyChange(vkKey, VK_ESCAPE, pressed);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_DPAD_UP, [&virtualKeys](bool pressed) {
        KeyboardSynth::VkKey & vkKey = virtualKeys.getVkKey(XINPUT_GAMEPAD_DPAD_UP);
        KeyboardSynth::vkKeyChange(vkKey, VK_UP, pressed);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_DPAD_DOWN, [&virtualKeys](bool pressed) {
        KeyboardSynth::VkKey & vkKey = virtualKeys.getVkKey(XINPUT_GAMEPAD_DPAD_DOWN);
        KeyboardSynth::vkKeyChange(vkKey, VK_DOWN, pressed);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_DPAD_LEFT, [&virtualKeys](bool pressed) {
        KeyboardSynth::VkKey & vkKey = virtualKeys.getVkKey(XINPUT_GAMEPAD_DPAD_LEFT);
        KeyboardSynth::vkKeyChange(vkKey, VK_LEFT, pressed);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_DPAD_RIGHT, [&virtualKeys](bool pressed) {
        KeyboardSynth::VkKey & vkKey = virtualKeys.getVkKey(XINPUT_GAMEPAD_DPAD_RIGHT);
        KeyboardSynth::vkKeyChange(vkKey, VK_RIGHT, pressed);
    });

    buttons.ifChanged(XINPUT_GAMEPAD_LEFT_SHOULDER, [this, user, state](bool) {
        onLeftShoulderChanged(user, state);
    });
    buttons.ifChanged(XINPUT_GAMEPAD_RIGHT_SHOULDER, [this, user, state](bool) {
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
    auto & vk = s_scanKeys[user].leftTrigger;
    auto & zone = state.leftThumbZone;
    KeyboardSynth::scanKeyChange(vk.key, state.leftTrigger, KeyboardSynth::KeyboardSide::Left, -zone.y, zone.x);
}
void TouchBox::onRightTriggerChanged(int user, const GamePadState & state) {
    std::cout << "R-T ";
    auto & vk = s_scanKeys[user].rightTrigger;
    auto & zone = state.rightThumbZone;
    KeyboardSynth::scanKeyChange(vk.key, state.rightTrigger, KeyboardSynth::KeyboardSide::Right, -zone.y, zone.x);
}

void TouchBox::keyRepeat() {
    KeyboardSynth::checkKeyRepeat();
}

void TouchBox::onLeftShoulderChanged(int user, const GamePadState & state) {
    std::cout << "L-S ";
    auto & vk = s_scanKeys[user].leftShoulder;
    auto & zone = state.leftThumbZone;
    bool pressed = state.buttons & XINPUT_GAMEPAD_LEFT_SHOULDER;
    if (vk.shift.pressed || (pressed && zone.isCentered())) {
        KeyboardSynth::modifierKeyChange(vk.shift, VK_LSHIFT, pressed);
        vk.shift.pressed = pressed;
    }
    else {
        scanKeyChange(vk.key, pressed, KeyboardSynth::KeyboardSide::Left, -zone.y, 2 * zone.x);
    }
}
void TouchBox::onRightShoulderChanged(int user, const GamePadState & state) {
    std::cout << "R-S ";
    auto & vk = s_scanKeys[user].rightShoulder;
    auto & zone = state.rightThumbZone;
    bool pressed = state.buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
    if (vk.shift.pressed || (pressed && zone.isCentered())) {
        KeyboardSynth::modifierKeyChange(vk.shift, VK_RSHIFT, pressed);
    }
    else {
        scanKeyChange(vk.key, pressed, KeyboardSynth::KeyboardSide::Right, -zone.y, 2 * zone.x);
    }
}

}


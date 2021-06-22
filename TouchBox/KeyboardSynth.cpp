#include "KeyboardSynth.hpp"

#include <Windows.h>

#include <chrono>
#include <functional>
#include <iostream>
#include <unordered_set>

namespace KeyboardSynth {

namespace {

void sendKeyboardVkEvent(WORD vk, bool pressed) {
    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    if (!pressed)
        input.ki.dwFlags |= KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

}  // namespace

namespace detail {

class KeyPressHandler {
public:
    void checkKeyRepeat(VkButton & vk);
    void checkKeyRepeat();
    void addPressed(VkButton & vk);
    void removePressed(VkButton & vk);

    static KeyPressHandler & getInstance();
private:
    KeyPressHandler();

    clock_t::duration keyboardDelay;
    clock_t::duration keyboardRepeat;
    std::unordered_set<VkButton*> pressedVkButtons;
};

KeyPressHandler::KeyPressHandler() {
    using namespace std::chrono_literals;

    int delay, speed;
    SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &delay, 0);
    SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &speed, 0);

    keyboardDelay = std::chrono::duration_cast<std::chrono::milliseconds>(250ms * (3 - double(delay)) / 3 + 1s * double(delay) / 3);
    double hz_s = 2.5 * (31 - speed) / 31 + 30.0 * speed / 31;
    keyboardRepeat = std::chrono::duration_cast<std::chrono::milliseconds>(1s / hz_s);
}

KeyPressHandler & KeyPressHandler::getInstance() {
    static KeyPressHandler instance;
    return instance;
}

void KeyPressHandler::checkKeyRepeat(VkButton & vk) {
    if (!vk.prevVkPressed)
        return;
    if (clock_t::now() > vk.nextKeyRepeat) {
        sendKeyboardVkEvent(vk.prevVkPressed, true);
        vk.nextKeyRepeat += keyboardRepeat;
    }
}

void KeyPressHandler::checkKeyRepeat() {
    for (VkButton * vkKey : pressedVkButtons) {
        checkKeyRepeat(*vkKey);
    }
}

void KeyPressHandler::addPressed(VkButton & button) {
    button.prevVkPressed = button.vk;
    button.nextKeyRepeat = clock_t::now() + keyboardDelay;
    pressedVkButtons.insert(&button);
    sendKeyboardVkEvent(button.prevVkPressed, true);
}

void KeyPressHandler::removePressed(VkButton & button) {
    sendKeyboardVkEvent(button.prevVkPressed, false);
    button.prevVkPressed = 0;
    pressedVkButtons.erase(&button);
}

}  // namespace detail

void vkButtonChange(VkButton & button, bool pressed) {
    bool released = !pressed;
    auto & keyPressHandler = detail::KeyPressHandler::getInstance();
    if (released && !button.prevVkPressed) {
        std::cout << "VK_ALREADY_RELEASED" << std::endl;
        return;
    }
    if (released) {
        std::cout << "VK_RELEASED" << std::endl;
        keyPressHandler.removePressed(button);
        return;
    }

    bool isAlreadyPressed = 0x2 & GetAsyncKeyState(button.vk);
    if (!isAlreadyPressed) {
        std::cout << "VK_PRESSED" << std::endl;
        keyPressHandler.addPressed(button);
        return;
    }
    std::cout << "VK_IGNORED" << std::endl;
}

void modifierButtonChange(ModifierKey & vk, WORD modifierVk, bool pressed) {
    sendKeyboardVkEvent(modifierVk, pressed);
    vk.pressed = pressed;
}

void checkKeyRepeat() {
    detail::KeyPressHandler::getInstance().checkKeyRepeat();
}

}  // namespace KeyboardSynth


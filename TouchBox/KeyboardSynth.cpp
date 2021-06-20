#include "KeyboardSynth.hpp"

#include <Windows.h>

#include <chrono>
#include <functional>
#include <iostream>
#include <unordered_set>

namespace KeyboardSynth {
namespace {

struct KeyRepeatTiming {
    clock_t::duration keyboardDelay;
    clock_t::duration keyboardRepeat;
    static const KeyRepeatTiming & getInstance();
private:
    KeyRepeatTiming();
};

KeyRepeatTiming::KeyRepeatTiming() {
    using namespace std::chrono_literals;

    int delay, speed;
    SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &delay, 0);
    SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &speed, 0);

    keyboardDelay = std::chrono::duration_cast<std::chrono::milliseconds>(250ms * (3 - double(delay)) / 3 + 1s * double(delay) / 3);
    double hz_s = 2.5 * (31 - speed) / 31 + 30.0 * speed / 31;
    keyboardRepeat = std::chrono::duration_cast<std::chrono::milliseconds>(1s / hz_s);
}

const KeyRepeatTiming & KeyRepeatTiming::getInstance() {
    static KeyRepeatTiming instance;
    return instance;
}

void sendKeyboardScanEvent(WORD scancode, bool pressed) {
    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = scancode;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    if (!pressed)
        input.ki.dwFlags |= KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void sendKeyboardVkEvent(WORD vk, bool pressed) {
    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    if (!pressed)
        input.ki.dwFlags |= KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void checkKeyRepeat(ScancodeKey & vk) {
    if (!vk.prevScancodePressed)
        return;
    if (clock_t::now() > vk.nextKeyRepeat) {
        sendKeyboardScanEvent(vk.prevScancodePressed, true);
        vk.nextKeyRepeat += KeyRepeatTiming::getInstance().keyboardRepeat;
    }
}

void checkKeyRepeat(VkKey & vk) {
    if (!vk.prevVkPressed)
        return;
    if (clock_t::now() > vk.nextKeyRepeat) {
        sendKeyboardVkEvent(vk.prevVkPressed, true);
        vk.nextKeyRepeat += KeyRepeatTiming::getInstance().keyboardRepeat;
    }
}

std::unordered_set<ScancodeKey*> pressedScanKeys;
std::unordered_set<VkKey*> pressedVkKeys;

}  // namespace

void scanKeyChange(ScancodeKey & scanKey, WORD scancode, bool pressed) {
    bool released = !pressed;
    if (released && !scanKey.prevScancodePressed)
        return;
    if (released) {
        std::cout << "RELEASED" << std::endl;
        sendKeyboardScanEvent(scanKey.prevScancodePressed, false);
        scanKey.prevScancodePressed = 0;
        pressedScanKeys.erase(&scanKey);
        return;
    }
    scanKey.prevScancodePressed = 0; // shouldn't happen that it is non-zero, but reset it anyway just in case
    bool isAlreadyPressed = 0x2 & GetAsyncKeyState(MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX));
    if (!isAlreadyPressed) {
        std::cout << "PRESSED" << std::endl;
        scanKey.prevScancodePressed = scancode;
        scanKey.nextKeyRepeat = clock_t::now() + KeyRepeatTiming::getInstance().keyboardDelay;
        pressedScanKeys.insert(&scanKey);
        sendKeyboardScanEvent(scanKey.prevScancodePressed, true);
        return;
    }
    std::cout << "IGNORED" << std::endl;
}

void vkKeyChange(VkKey & vkKey, WORD newVk, bool pressed) {
    bool released = !pressed;
    if (released && !vkKey.prevVkPressed) {
        std::cout << "VK_ALREADY_RELEASED" << std::endl;
        return;
    }
    if (released) {
        std::cout << "VK_RELEASED" << std::endl;
        sendKeyboardVkEvent(vkKey.prevVkPressed, false);
        vkKey.prevVkPressed = 0;
        pressedVkKeys.erase(&vkKey);
        return;
    }

    bool isAlreadyPressed = 0x2 & GetAsyncKeyState(newVk);
    if (!isAlreadyPressed) {
        std::cout << "VK_PRESSED" << std::endl;
        vkKey.prevVkPressed = newVk;
        vkKey.nextKeyRepeat = clock_t::now() + KeyRepeatTiming::getInstance().keyboardDelay;
        pressedVkKeys.insert(&vkKey);
        sendKeyboardVkEvent(vkKey.prevVkPressed, true);
        return;
    }
    std::cout << "VK_IGNORED" << std::endl;
}

void modifierKeyChange(ModifierKey & vk, WORD modifierVk, bool pressed) {
    sendKeyboardVkEvent(modifierVk, pressed);
    vk.pressed = pressed;
}

void checkKeyRepeat() {
    for (VkKey * vkKey : pressedVkKeys) {
        checkKeyRepeat(*vkKey);
    }
    for (ScancodeKey * scanKey : pressedScanKeys) {
        checkKeyRepeat(*scanKey);
    }
}

}  // namespace KeyboardSynth


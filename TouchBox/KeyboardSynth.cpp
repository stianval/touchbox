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

const WORD scancodes_intl[6][14] = {
    { 0x01,  0x3B, 0x3C, 0x3D, 0x3E,  0x3F, 0x40, 0x41, 0x42,  0x43, 0x44, 0x57, 0x58 }, // F-keys
    { 0x29, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E }, // Number row
    { 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1C, 0x1C },
    { 0x3A, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x2B, 0x1C },
    { 0x2A, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x36, 0x36 },
    { 0x1D, 0x38, 0x39, 0x39, 0x39, 0x39, 0x39, 0x39, 0x39, 0x39, 0x39, 0x39, 0xE038, 0xE01D }
};

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

void scanKeyChange(ScancodeKey & scanKey, bool pressed, KeyboardSide keyboardSide, int rowAdjust, int colAdjust) {
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
    int scancodeRow = 3;
    int scancodeCol;
    switch (keyboardSide) {
    case KeyboardSide::Left:
        scancodeCol = 3;
        break;
    case KeyboardSide::Right:
        scancodeCol = 8;
        break;
    }
    scancodeRow += rowAdjust;
    scancodeCol += colAdjust;
    WORD scancode = scancodes_intl[scancodeRow][scancodeCol];
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


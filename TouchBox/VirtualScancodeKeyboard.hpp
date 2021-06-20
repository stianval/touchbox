#pragma once

typedef unsigned short WORD;

namespace VirtualScancodeKeyboard {

enum class KeyboardSide {
    Left,
    Right
};

WORD getScancode(KeyboardSide keyboardSide, int rowAdjust, int colAdjust);
WORD getVk(KeyboardSide keyboardSide, int rowAdjust, int colAdjust);

}

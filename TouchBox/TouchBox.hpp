#pragma once
#include "../XInputReader/XInputReader.hpp"
#include <chrono>

namespace TouchBox {

class TouchBox {
public:
    using GamepadState = XInputReader::GamepadState;

    void onButtonsStateChanged(int user, const GamepadState & oldState, const GamepadState & state);
    void onLeftThumbZoneChanged(int user, const GamepadState & oldState, const GamepadState & state);
    void onRightThumbZoneChanged(int user, const GamepadState & oldState, const GamepadState & state);
    void onLeftTriggerChanged(int user, const GamepadState & state);
    void onRightTriggerChanged(int user, const GamepadState & state);

    void keyRepeat();

private:
    void onLeftShoulderChanged(int user, const GamepadState & state);
    void onRightShoulderChanged(int user, const GamepadState & state);
};

}  // namespace TouchBox

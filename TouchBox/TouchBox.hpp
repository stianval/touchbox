#pragma once
#include "../XInputReader/XInputReader.hpp"
#include <chrono>

namespace TouchBox {

class TouchBox {
public:
    using GamePadState = XInputReader::GamePadState;

    void onButtonsStateChanged(int user, const GamePadState & oldState, const GamePadState & state);
    void onLeftThumbZoneChanged(int user, const GamePadState & oldState, const GamePadState & state);
    void onRightThumbZoneChanged(int user, const GamePadState & oldState, const GamePadState & state);
    void onLeftTriggerChanged(int user, const GamePadState & state);
    void onRightTriggerChanged(int user, const GamePadState & state);

    void keyRepeat();

private:
    void onLeftShoulderChanged(int user, const GamePadState & state);
    void onRightShoulderChanged(int user, const GamePadState & state);
};

}  // namespace TouchBox

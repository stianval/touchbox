#include "xinputreader.hpp"

#include <Windows.h>
#include <Xinput.h>

namespace {

using namespace XInputReader;

constexpr double cot_pi_8 = 2.414213562373095048801688724209;
int signum(int x) {
    return (x > 0) - (x < 0);
}

bool triggerIsTriggered(int triggerValue) {
    return triggerValue > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
}

bool operator==(const ThumbZone & a, const ThumbZone & b) {
    return a.x == b.x && a.y == b.y;
}
bool operator!=(const ThumbZone & a, const ThumbZone & b) {
    return !(a == b);
}

enum class ThumbStick {
    Left,
    Right
};
ThumbZone detectThumbZone8Way(const XINPUT_GAMEPAD & gamePad, ThumbStick thumbStick) {
    SHORT x, y;
    int deadzoneSquared;
    switch (thumbStick) {
    case ThumbStick::Left:
        x = gamePad.sThumbLX;
        y = gamePad.sThumbLY;
        deadzoneSquared = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE * XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
        break;
    case ThumbStick::Right:
        x = gamePad.sThumbRX;
        y = gamePad.sThumbRY;
        deadzoneSquared = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE * XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
        break;
    }

    ThumbZone res;

    int magnitudeSquared = x*x + y*y;
    if (magnitudeSquared > deadzoneSquared) {
        if (y != 0 && abs(double(x) / y) < cot_pi_8)
            res.y = signum(y);
        if (x != 0 && abs(double(y) / x) < cot_pi_8)
            res.x = signum(x);
    }

    return res;
}
GamepadState createGamePadState(const XINPUT_STATE & state) {
    return {
        state.dwPacketNumber,
        state.Gamepad.wButtons,
        detectThumbZone8Way(state.Gamepad, ThumbStick::Left),
        detectThumbZone8Way(state.Gamepad, ThumbStick::Right),
        triggerIsTriggered(state.Gamepad.bLeftTrigger),
        triggerIsTriggered(state.Gamepad.bRightTrigger),
    };
}
GamepadState s_states[XUSER_MAX_COUNT];

}  // namespace

namespace XInputReader {

void readInput(InputHandler & handler) {
    DWORD dwResult;
    for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
        GamepadState & state = s_states[i];

        XINPUT_STATE xinputState;
        ZeroMemory(&xinputState, sizeof(XINPUT_STATE));

        // Simply get the state of the controller from XInput.
        dwResult = XInputGetState(i, &xinputState);

        if (dwResult != ERROR_SUCCESS)
            continue; // Controller not connected
        if (xinputState.dwPacketNumber == state.packetNumber)
            continue;

        auto oldState = state;
        state = createGamePadState(xinputState);

        if (state.buttons != oldState.buttons)
            handler.handler_->onButtonsStateChanged(i, oldState, state);
        if (state.leftThumbZone != oldState.leftThumbZone)
            handler.handler_->onLeftThumbZoneChanged(i, oldState, state);
        if (state.rightThumbZone != oldState.rightThumbZone)
            handler.handler_->onRightThumbZoneChanged(i, oldState, state);
        if (state.leftTrigger != oldState.leftTrigger)
            handler.handler_->onLeftTriggerChanged(i, state);
        if (state.rightTrigger != oldState.rightTrigger)
            handler.handler_->onRightTriggerChanged(i, state);
    }
}

}  // namespace XInputReader

#pragma once
#include <Windows.h>
#include <memory>

namespace XInputReader {

struct ThumbZone {
    signed char x = 0;
    signed char y = 0;
    bool isCentered() const { return !(x || y); }
};

struct GamePadState {
    DWORD packetNumber = 0;
    WORD buttons = 0;
    ThumbZone leftThumbZone;
    ThumbZone rightThumbZone;
    bool leftTrigger = false;
    bool rightTrigger = false;
};

class InputHandler {
public:
    template <typename T>
    explicit InputHandler(T & handler)
        : handler_(std::make_unique<HandlerImpl<T>>(handler))
    {}

private:
    struct HandlerInterface {
        virtual void onButtonsStateChanged(int user, const GamePadState & oldState, const GamePadState & state) = 0;
        virtual void onLeftThumbZoneChanged(int user, const GamePadState & oldState, const GamePadState & state) = 0;
        virtual void onRightThumbZoneChanged(int user, const GamePadState & oldState, const GamePadState & state) = 0;
        virtual void onLeftTriggerChanged(int user, const GamePadState & state) = 0;
        virtual void onRightTriggerChanged(int user, const GamePadState & state) = 0;
    };

    template <typename T>
    struct HandlerImpl : public HandlerInterface {
        HandlerImpl(T & handler)
            : handler(handler)
        {}
        void onButtonsStateChanged(int user, const GamePadState & oldState, const GamePadState & state) override {
            handler.onButtonsStateChanged(user, oldState, state);
        }
        void onLeftThumbZoneChanged(int user, const GamePadState & oldState, const GamePadState & state) override {
            handler.onLeftThumbZoneChanged(user, oldState, state);
        }
        void onRightThumbZoneChanged(int user, const GamePadState & oldState, const GamePadState & state) override {
            handler.onRightThumbZoneChanged(user, oldState, state);
        }
        void onLeftTriggerChanged(int user, const GamePadState & state) override {
            handler.onLeftTriggerChanged(user, state);
        }
        void onRightTriggerChanged(int user, const GamePadState & state) override {
            handler.onRightTriggerChanged(user, state);
        }
        T & handler;
    };

    std::unique_ptr<HandlerInterface> handler_;
    friend void readInput(InputHandler & handler);
};

void readInput(InputHandler & handler);

}

#include "TouchBox.hpp"

#include "../XInputReader/XInputReader.hpp"
#include <Windows.h>

int main() {
    TouchBox::TouchBox app;
    XInputReader::InputHandler inputHandler(app);

    for (;;) {
        XInputReader::readInput(inputHandler);
        app.keyRepeat();
        Sleep(1);
    }
}

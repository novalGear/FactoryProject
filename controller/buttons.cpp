#include <Bounce2.h>

// пины подключения тактовых кнопок

const int BUTTON_PINS[] = {13, 12, 14, 27};
const int NUM_BUTTONS = sizeof(BUTTON_PINS) / sizeof(BUTTON_PINS[0]);

Bounce buttons[NUM_BUTTONS];

const int debounce_interval = 20;

void buttons_setup() {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].attach(BUTTON_PINS[i], INPUT_PULLUP);
        buttons[i].interval(debounce_interval);
    }
}

void buttons_update() {
    for ( int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].update();
    }
}

bool is_pressed(const int button_index) {
    if (button_index >= 0 && button_index < NUM_BUTTONS)
        return buttons[button_index].fell();
    else {
        Serial.printf("button index out of range (0 - %d): %d\n", NUM_BUTTONS - 1, button_index);
        return false;
    }
}

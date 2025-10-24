#include <OneButton.h>

const int BUTTON_PINS[] = {13, 12, 14, 27};
const int NUM_BUTTONS = sizeof(BUTTON_PINS) / sizeof(BUTTON_PINS[0]);

bool buttonEventOccurred = false; //
int pressedButtonIndex = -1; // Номер кнопки (-1 = нет события)
int pressedButtonType = 0;

// Определение констант для типов событий
const int EVENT_NONE = 0;
const int EVENT_CLICK = 1;
const int EVENT_DOUBLE_CLICK = 2;
const int EVENT_PRESS = 3;

OneButton* buttons[NUM_BUTTONS];

// лютый кринж, пока костыли ====================================

void handleButton0Click() {
    if (!buttonEventOccurred) {
        buttonEventOccurred = true;
        pressedButtonIndex = 0;
        pressedButtonType = EVENT_CLICK;
    }
}

void handleButton1Click() {
    if (!buttonEventOccurred) {
        buttonEventOccurred = true;
        pressedButtonIndex = 1;
        pressedButtonType = EVENT_CLICK;
    }
}

void handleButton2Click() {
    if (!buttonEventOccurred) {
        buttonEventOccurred = true;
        pressedButtonIndex = 2;
        pressedButtonType = EVENT_CLICK;
    }
}

void handleButton3Click() {
    if (!buttonEventOccurred) {
        buttonEventOccurred = true;
        pressedButtonIndex = 3;
        pressedButtonType = EVENT_CLICK;
    }
}

typedef void (*ClickHandler)();

ClickHandler clickHandlers[NUM_BUTTONS] = {
    handleButton0Click,
    handleButton1Click,
    handleButton2Click,
    handleButton3Click
};
// конец кринжа =======================================

void buttons_setup() {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(BUTTON_PINS[i], INPUT_PULLUP);
        buttons[i] = new OneButton(BUTTON_PINS[i], true);
        buttons[i]->attachClick(clickHandlers[i]);
    }
}
void buttons_update() {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i]->tick();
    }
}

bool check_button_event() {
    return buttonEventOccurred;
}

int get_pressedButtonIndex() {
    return pressedButtonIndex;
}

int get_pressedButtonType() {
    return pressedButtonType;
}

void reset_ButtonEvent() {
    buttonEventOccurred = false;
    pressedButtonIndex = -1;
    pressedButtonType = EVENT_NONE;
}

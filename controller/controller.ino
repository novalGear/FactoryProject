#include "OLED_screen.h"
#include "buttons.h"
#include "sensors.h"

void setup() {
    Serial.begin(115200);

    buttons_setup();
    OLED_screen_setup();
    temp_sensors_setup();
}

void loop() {

    buttons_update();
    for (int i = 0; i < 4; i++) {
        if (is_pressed(i)) {
            Serial.printf("Button %d pressed\n", i);
        }
    }

    temperature_sensors_update();
}

#include "OLED_screen.h"
#include "buttons.h"
#include "sensors.h"

void setup() {
    Serial.begin(115200);

    buttons_setup();
    OLED_screen_setup();
    temp_sensors_setup();
    co2_sensor_setup();
}

void loop() {
    buttons_update();
    temperature_sensors_update();
    co2_sensor_update();

    if (check_button_event()) {
        String msg = "button " + String (get_pressedButtonIndex()) + " down";
        Serial.println(msg);
        // menu(get_pressedButtonIndex());
        reset_ButtonEvent();
    }
}

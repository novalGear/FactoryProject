#include "OLED_screen.h"
#include "buttons.h"
#include "sensors.h"
#include "motor.h"
#include "menu.h"

void setup() {
    Serial.begin(115200);

    OLED_screen_setup();
    temp_sensors_setup();
    co2_sensor_setup();

    motor_setup();
    buttons_setup();
    menu_setup();       // обязательно после сенсоров и дисплея
}

void loop() {
    display_regular_update();
    buttons_update();
    // temperature_sensors_update();
    // co2_sensor_update();

    if (check_button_event()) {
        String msg = "button " + String (get_pressedButtonIndex()) + " down";
        Serial.println(msg);
        processButtonPress(get_pressedButtonIndex());
        reset_ButtonEvent();
    }
}

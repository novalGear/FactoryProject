#include "OLED_screen.h"
#include "buttons.h"
#include "sensors.h"
#include "motor_impl.h"
#include "menu.h"

void setup() {
    Serial.begin(115200);

    // OLED_screen_setup();
    // temp_sensors_setup();
    // co2_sensor_setup();

    motor_setup();
    // buttons_setup();
    // menu_setup();               // обязательно после сенсоров и дисплея
    delay(10000);
}

void loop() {
    unint_motor_move(5.0, 1, 200);
    delay(5000);
    unint_motor_move(5.0, -1, 200);
    delay(5000);
    // motor_test();

    // updateDisplay();            // здесь обновляем данные для дисплея
    // display_regular_update();   // здесь с фиксированной частотой посылаем новые данные на дисплей

    // buttons_update();
    // temperature_sensors_update();
    // co2_sensor_update();

    // // upd_metric();
    // // if (!is_metric_ok()) {
    // //     if (is_co2_critical()) {
    // //         // делаем проветривание
    // //     }
    // //     if (is_temp_critical()) {
    // //         // ищем точку где температура поприятнее будет, если это вообще возможно. Хотя бы динамику собрать
    // //         // если комфортная температура установится через несколько минут - ничего не делаем,
    // //         // т.е тут проверить динамику изменения температуры с последнего действия
    // //         // если совсем медленно температура идет в нужном направлении, тогда уже можно покрутиться куда-то
    // //     }
    // // }

    // if (check_button_event()) {
    //     // String msg = "button " + String (get_pressedButtonIndex()) + " down";
    //     // Serial.println(msg);
    //     processButtonPress(get_pressedButtonIndex());   // изменяем состояние меню
    //     reset_ButtonEvent();
    // }
}

#include "OLED_screen.h"
#include "buttons.h"
#include "sensors.h"
#include "menu.h"
#include "window_controller.h"
#include "tgbot.h"

WindowController windowController;
TelegramBot telegramBot;
void setup() {
    Serial.begin(115200);
    motor_setup();

    OLED_screen_setup();
    temp_sensors_setup();
    co2_sensor_setup();

    telegramBot.init();

    buttons_setup();
    menu_setup();               // обязательно после сенсоров и дисплея

    unint_motor_move(2000, 0, 100);
}

void loop() {
    return;
    windowController.update();
    updateDisplay();            // здесь обновляем данные для дисплея
    display_regular_update();   // здесь с фиксированной частотой посылаем новые данные на дисплей

    buttons_update();
    temperature_sensors_update();
    co2_sensor_update();

    telegramBot.update(windowController);
    delay(5000); // Основной цикл каждые 5 секунд
}

void log_system_status(float metric) {
    Serial.println("=== SYSTEM STATUS ===");
    Serial.print("Metric: ");
    Serial.print(metric, 1);
    Serial.print(", Window: ");
    Serial.print(windowController.getCurrentPosition() * 100);
    Serial.println("% open");
    Serial.println("====================");
}

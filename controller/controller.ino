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

    OLED_screen_setup();
    temp_sensors_setup();
    co2_sensor_setup();

    telegramBot.init();

    motor_setup();
    buttons_setup();
    menu_setup();               // обязательно после сенсоров и дисплея
}

void loop() {
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

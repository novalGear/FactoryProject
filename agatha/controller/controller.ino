#include <Arduino.h>
#include "WeightSensor.h"
#include "TelegramBot.h"
#include "Config.h"

// Глобальные объекты
WeightSensor scale(HX711_DT_PIN, HX711_SCK_PIN);
TelegramBot bot(BOT_TOKEN, CHAT_ID);

unsigned long lastTelegramCheck = 0;
const unsigned long TELEGRAM_CHECK_INTERVAL = 1000;  // проверка сообщений каждую секунду

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("Starting ESP32 Weight Bot...");

    // Инициализация датчика (начальный коэффициент)
    scale.begin(-14.60f);  // подберите свой

    // WiFi + Telegram
    if (!bot.connectWiFi()) {
        Serial.println("WiFi connection failed!");
        while(1) delay(1000);
    }

    Serial.println("Bot ready. Send /help to chat.");
}

void loop() {
    // Проверка новых сообщений Telegram
    unsigned long now = millis();
    if (now - lastTelegramCheck >= TELEGRAM_CHECK_INTERVAL) {
        lastTelegramCheck = now;
        bot.checkMessages();
    }

    delay(10);  // небольшая пауза
}

#include "TelegramBot.h"
#include "WeightSensor.h"  // для обработки команд

TelegramBot::TelegramBot(const char* botToken, const char* chatId)
    : _bot(botToken, _client), _chatId(chatId) {
    _client.setCACert(TELEGRAM_CERTIFICATE);
}

bool TelegramBot::connectWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        return true;
    }
    return false;
}

bool TelegramBot::checkMessages() {
    int numNewMessages = _bot.getUpdates(_bot.last_message_received + 1);

    for (int i = 0; i < numNewMessages; i++) {
        String chat_id = String(_bot.messages[i].chat_id);
        if (chat_id != _chatId) {
            continue;
        }

        _lastMessage = _bot.messages[i].text;
        String response = handleWeightCommand(_lastMessage);

        if (response.length() > 0) {
            _bot.sendMessage(chat_id, response, "");
        }
    }
    return numNewMessages > 0;
}

String TelegramBot::handleWeightCommand(const String& command) {
    command.toLowerCase();

    if (command == "/get_weight") {
        return "Вес: " + String(WeightSensor::getWeight(), 2) + " г";
    }
    else if (command == "/tare") {
        WeightSensor::tare();
        return "Датчик обнулён (тара установлена)";
    }
    else if (command.startsWith("/calibration ")) {
        float weight = command.substring(12).toFloat();
        if (weight > 0) {
            WeightSensor::calibrate(weight);
            return "Калибровка выполнена для груза " + String(weight) + " г";
        }
        return "Ошибка: укажите вес в граммах, например /calibration 100";
    }
    else if (command == "/help") {
        return "/get_weight - текущий вес\n/tare - обнулить\n/calibration N - калибровка с грузом N г";
    }
    return "Неизвестная команда. /help - справка";
}

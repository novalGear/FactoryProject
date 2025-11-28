#include "tgbot.h"

WiFiClientSecure client;

void TelegramBot::init() {
    // Подключение к WiFi (данные теперь из tgbotconfig.h)
    WiFi.begin(ssid, password);  // ssid и password из tgbotconfig.h
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Подключаемся к WiFi...");
    }
    Serial.println("Подключено к WiFi");

    // Настройка SSL для Telegram
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

    // Создание объекта бота (BOT_TOKEN из tgbotconfig.h)
    bot = new UniversalTelegramBot(BOT_TOKEN, client);

    // Выводим информацию о белом списке
    Serial.println("Белый список пользователей:");
    for (const String& user_id : allowedUsers) {
        Serial.println("  - " + user_id);
    }

    Serial.println("Telegram бот инициализирован");
}

// Остальные функции остаются без изменений...
bool TelegramBot::isUserAllowed(String user_id) {
    for (const String& allowed_id : allowedUsers) {
        if (allowed_id == user_id) {
            return true;
        }
    }
    return false;
}
void TelegramBot::addAllowedUser(String user_id) {
    // Добавляем в локальный список
    allowedUsers.push_back(user_id);

    // Также можно обновлять глобальный список, если нужно
    ::allowedUsers.push_back(user_id);

    Serial.println("Добавлен пользователь в белый список: " + user_id);
}

void TelegramBot::sendNotAllowedMessage(String chat_id) {
    String message = "🚫 **Доступ запрещен**\n\n";
    message += "Ваш ID: `" + chat_id + "`\n";
    message += "Обратитесь к администратору для получения доступа.";
    bot->sendMessage(chat_id, message, "Markdown");

    Serial.println("Попытка доступа от неавторизованного пользователя: " + chat_id);
}


void TelegramBot::update(WindowController& windowController) {
    if (millis() - lastUpdateTime > UPDATE_INTERVAL) {
        handleMessages(windowController);
        lastUpdateTime = millis();
    }
}

void TelegramBot::handleMessages(WindowController& windowController) {
    int numNewMessages = bot->getUpdates(bot->last_message_received + 1);

    while (numNewMessages) {
        Serial.println("Получено сообщение Telegram");

        for (int i = 0; i < numNewMessages; i++) {
            String chat_id = String(bot->messages[i].chat_id);
            String text = bot->messages[i].text;

            if (text == "/start") {
                String welcome = "**Бот управления окнами**\n\n";
                welcome += "Доступные команды:\n";
                welcome += "`/status` - текущие показания\n";
                welcome += "`/settings` - настройки параметров\n";
                bot->sendMessage(chat_id, welcome, "Markdown");
            }
            else if (text == "/status") {
                sendStatusLog(chat_id, windowController);
            }
            else if (text == "/settings") {
                showSettingsMenu(chat_id, windowController);
            }
            else if (text.startsWith("/set_")) {
                handleParameterSetting(chat_id, text, windowController);
            }
            else {
                bot->sendMessage(chat_id, "Неизвестная команда. Используйте /start", "");
            }
        }
        numNewMessages = bot->getUpdates(bot->last_message_received + 1);
    }
}
void TelegramBot::sendStatusLog(String chat_id, WindowController& windowController) {
    windowController.updateRecentData();
    WindowConfig config = windowController.getConfig();
    RecentData recentData = windowController.getRecentData();

    String message = "**ТЕКУЩИЕ ПОКАЗАНИЯ СИСТЕМЫ**\n\n";

    // Реальные данные с датчиков
    message += "**ДАННЫЕ С ДАТЧИКОВ:**\n";
    message += "Температура: " + String(recentData.temperature, 1) + "°C";
    if (recentData.tempSensorError) message += " ⚠️(ошибка датчика)";
    message += "\n";

    message += "CO2: " + String(recentData.co2, 0) + " ppm";
    if (recentData.co2SensorError) message += " ⚠️(ошибка датчика)";
    message += "\n\n";

    // Рассчитанные метрики
    message += "**РАСЧЕТНЫЕ МЕТРИКИ:**\n";
    message += "Общая метрика: " + String(recentData.totalMetric, 2) + "\n";
    message += "Метрика температуры: " + String(recentData.temperatureMetric, 2) + "\n";
    message += "Метрика CO2: " + String(recentData.co2Metric, 2) + "\n\n";

    // Состояние системы
    message += "⚙️ **СОСТОЯНИЕ СИСТЕМЫ:**\n";
    message += "Позиция окна: " + String(recentData.windowPosition) + "/9\n";
    message += "Целевая метрика: " + String(config.metricTarget, 1) + "\n";
    message += "Допуск: ±" + String(config.metricMargin, 1) + "\n\n";

    // Настройки
    message += "**НАСТРОЙКИ:**\n";
    message += "Идеальная температура: " + String(config.tempIdeal, 1) + "°C\n";
    message += "Идеальный CO2: " + String(config.co2Ideal) + " ppm\n";

    // Статус ошибок
    message += "\n🔧 **СТАТУС:** ";
    if (recentData.tempSensorError || recentData.co2SensorError) {
        message += "⚠️ Есть ошибки датчиков";
    } else {
        message += "✅ Норма";
    }

    // Время обновления
    unsigned long secondsAgo = (millis() - recentData.timestamp) / 1000;
    message += "\n🕒 Данные обновлены: " + String(secondsAgo) + " сек назад";

    bot->sendMessage(chat_id, message, "Markdown");
}

void TelegramBot::showSettingsMenu(String chat_id, WindowController& windowController) {
    WindowConfig config = windowController.getConfig();

    String message = "⚙️ **НАСТРОЙКИ ПАРАМЕТРОВ**\n\n";
    message += "**Текущие значения:**\n";
    message += "Температура:\n";
    message += "  - Идеальная: " + String(config.tempIdeal) + "°C\n";
    message += "  - Критический макс: " + String(config.tempCriticalHigh) + "°C\n";
    message += "  - Критический мин: " + String(config.tempCriticalLow) + "°C\n\n";

    message += "CO2:\n";
    message += "  - Идеальный: " + String(config.co2Ideal) + " ppm\n";
    message += "  - Критический: " + String(config.co2CriticalHigh) + " ppm\n\n";

    message += "**Команды для изменения:**\n";
    message += "`/set_temp_ideal 23.5` - идеальная температура\n";
    message += "`/set_temp_high 35` - макс температура\n";
    message += "`/set_temp_low 10` - мин температура\n";
    message += "`/set_co2_ideal 800` - идеальный CO2\n";
    message += "`/set_co2_high 2500` - критический CO2\n";

    bot->sendMessage(chat_id, message, "Markdown");
}

void TelegramBot::handleParameterSetting(String chat_id, String command, WindowController& windowController) {
    WindowConfig config = windowController.getConfig();
    String response = "";

    if (command.startsWith("/set_temp_ideal ")) {
        config.tempIdeal = command.substring(16).toFloat();
        response = "Идеальная температура: " + String(config.tempIdeal) + "°C";
    }
    else if (command.startsWith("/set_temp_high ")) {
        config.tempCriticalHigh = command.substring(15).toFloat();
        response = "Макс температура: " + String(config.tempCriticalHigh) + "°C";
    }
    else if (command.startsWith("/set_temp_low ")) {
        config.tempCriticalLow = command.substring(14).toFloat();
        response = "Мин температура: " + String(config.tempCriticalLow) + "°C";
    }
    else if (command.startsWith("/set_co2_ideal ")) {
        config.co2Ideal = command.substring(15).toInt();
        response = "Идеальный CO2: " + String(config.co2Ideal) + " ppm";
    }
    else if (command.startsWith("/set_co2_high ")) {
        config.co2CriticalHigh = command.substring(14).toInt();
        response = "Критический CO2: " + String(config.co2CriticalHigh) + " ppm";
    }
    else {
        response = "Неизвестная команда. Используйте /settings для списка команд";
    }

    windowController.setConfig(config);

    bot->sendMessage(chat_id, response, "");
}

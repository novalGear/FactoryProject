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
                welcome += "`/mode` - управление режимом работы\n";
                welcome += "`/set_position N` - установить позицию окна (только в ручном режиме, N от 0 до 9)";
                bot->sendMessage(chat_id, welcome, "Markdown");
            }
            else if (text == "/status") {
                sendStatusLog(chat_id, windowController);
            }
            else if (text == "/settings") {
                showSettingsMenu(chat_id, windowController);
            }
            else if (text == "/mode") {
                showModeMenu(chat_id, windowController);
            }
            else if (text == "/mode_auto") {
                setMode(chat_id, WindowMode::AUTO, windowController);
            }
            else if (text == "/mode_manual") {
                setMode(chat_id, WindowMode::MANUAL, windowController);
            }
            else if (text.startsWith("/set_position ")) {
                handleSetPosition(chat_id, text, windowController);
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

void TelegramBot::showModeMenu(String chat_id, WindowController& windowController) {
    WindowConfig config = windowController.getConfig();

    String message = "🎛️ **УПРАВЛЕНИЕ РЕЖИМОМ РАБОТЫ**\n\n";
    message += "Текущий режим: ";

    switch (config.currentMode) {
        case WindowMode::AUTO:
            message += "🔘 **AUTO (автоматический)**\n\n";
            message += "В этом режиме система автоматически управляет окнами на основе:\n";
            message += "• Температуры в помещении\n";
            message += "• Уровня CO2\n";
            message += "• Разницы температур внутри/снаружи\n";
            message += "\n⚠️ Ручное управление отключено\n";
            break;
        case WindowMode::MANUAL:
            message += "✋ **MANUAL (ручной)**\n\n";
            message += "В этом режиме окна управляются только вручную.\n";
            message += "Автоматические корректировки отключены.\n";
            message += "\n**Ручное управление позицией:**\n";
            message += "Используйте `/set_position N` где N от 0 до 9\n";
            message += "• 0 - полностью закрыто\n";
            message += "• 9 - полностью открыто\n";
            break;
        default:
            message += "❓ **UNKNOWN**\n\n";
            break;
    }

    message += "\n**Выберите режим:**\n";
    message += "`/mode_auto` - переключить в автоматический режим\n";
    message += "`/mode_manual` - переключить в ручной режим\n";

    bot->sendMessage(chat_id, message, "Markdown");
}

void TelegramBot::handleSetPosition(String chat_id, String command, WindowController& windowController) {
    WindowConfig config = windowController.getConfig();

    // Проверяем, что находимся в ручном режиме
    if (config.currentMode != WindowMode::MANUAL) {
        String error = "❌ **Ошибка: неверный режим**\n\n";
        error += "Команда `/set_position` доступна только в **ручном режиме**.\n";
        error += "Текущий режим: ";

        switch (config.currentMode) {
            case WindowMode::AUTO:
                error += "AUTO (автоматический)";
                break;
            case WindowMode::MANUAL:
                error += "MANUAL (ручной)"; // Не должно случиться, но на всякий случай
                break;
            default:
                error += "UNKNOWN";
                break;
        }

        error += "\n\nИспользуйте `/mode_manual` для переключения в ручной режим.";
        bot->sendMessage(chat_id, error, "Markdown");
        return;
    }

    // Парсим позицию
    String posStr = command.substring(14); // "/set_position " = 14 символов
    posStr.trim();

    if (posStr.length() == 0) {
        bot->sendMessage(chat_id, "❌ **Ошибка: укажите позицию**\n\nИспользуйте: `/set_position N` где N от 0 до 9", "Markdown");
        return;
    }

    int position = posStr.toInt();

    // Проверяем диапазон
    if (position < 0 || position > 9) {
        bot->sendMessage(chat_id, "❌ **Ошибка: недопустимая позиция**\n\nПозиция должна быть от 0 до 9.", "Markdown");
        return;
    }

    // Устанавливаем позицию
    bool success = windowController.setManualPosition(position);

    if (success) {
        String response = "✅ **Позиция окна изменена**\n\n";
        response += "Установлена позиция: **" + String(position) + "/9**\n";

        if (position == 0) {
            response += "Окно полностью закрыто.";
        } else if (position == 9) {
            response += "Окно полностью открыто.";
        } else {
            response += "Окно открыто на " + String(position) + "/9.";
        }

        bot->sendMessage(chat_id, response, "Markdown");

        // Логируем
        Serial.print("Установлена ручная позиция окна: ");
        Serial.println(position);
    } else {
        bot->sendMessage(chat_id, "❌ **Ошибка при установке позиции**\n\nНе удалось установить позицию окна.", "Markdown");
    }
}

void TelegramBot::sendStatusLog(String chat_id, WindowController& controller) {
    RecentData data = controller.getRecentData();
    String message = "=== System Status ===\n";
    message += "Temperature: " + String(data.temperature, 1) + "°C\n";
    message += "Outside: " + String(data.outsideTemp, 1) + "°C\n";
    message += "CO2: " + String(data.co2) + " ppm\n";
    message += "Window: " + String(data.windowPosition) + "/9\n";
    message += "Total Metric: " + String(data.totalMetric, 1);

    // Добавляем информацию о режиме
    message += "\nMode: ";
    switch (controller.getConfig().currentMode) {
        case WindowMode::AUTO:
            message += "AUTO (автоматический)";
            break;
        case WindowMode::MANUAL:
            message += "MANUAL (ручной)";
            break;
        default:
            message += "UNKNOWN";
            break;
    }

    bot->sendMessage(chat_id, message, "");
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

    message += "Режим работы: ";
    switch (config.currentMode) {
        case WindowMode::AUTO:
            message += "AUTO (автоматический)";
            break;
        case WindowMode::MANUAL:
            message += "MANUAL (ручной)";
            break;
        default:
            message += "UNKNOWN";
            break;
    }
    message += "\n\n";

    message += "**Команды для изменения:**\n";
    message += "`/set_temp_ideal 23.5` - идеальная температура\n";
    message += "`/set_temp_high 35` - макс температура\n";
    message += "`/set_temp_low 10` - мин температура\n";
    message += "`/set_co2_ideal 800` - идеальный CO2\n";
    message += "`/set_co2_high 2500` - критический CO2\n";
    message += "`/mode` - управление режимом работы\n";

    bot->sendMessage(chat_id, message, "Markdown");
}

void TelegramBot::setMode(String chat_id, WindowMode mode, WindowController& windowController) {
    WindowConfig config = windowController.getConfig();
    config.currentMode = mode;
    windowController.setConfig(config);

    String message = "✅ **Режим работы изменен**\n\n";

    switch (mode) {
        case WindowMode::AUTO:
            message += "Установлен режим: **AUTO (автоматический)**\n";
            message += "Система будет автоматически управлять окнами.";
            break;
        case WindowMode::MANUAL:
            message += "Установлен режим: **MANUAL (ручной)**\n";
            message += "Автоматическое управление отключено.";
            break;
        default:
            message += "Установлен неизвестный режим.";
            break;
    }

    bot->sendMessage(chat_id, message, "Markdown");

    // Логируем изменение
    Serial.print("Режим изменен на: ");
    switch (mode) {
        case WindowMode::AUTO:
            Serial.println("AUTO");
            break;
        case WindowMode::MANUAL:
            Serial.println("MANUAL");
            break;
    }
}

void TelegramBot::handleParameterSetting(String chat_id, String command, WindowController& windowController) {
    WindowConfig config = windowController.getConfig();
    String response = "";

    if (command.startsWith("/set_temp_ideal ")) {
        config.tempIdeal = command.substring(16).toFloat();
        response = "✅ Идеальная температура: " + String(config.tempIdeal) + "°C";
    }
    else if (command.startsWith("/set_temp_high ")) {
        config.tempCriticalHigh = command.substring(15).toFloat();
        response = "✅ Макс температура: " + String(config.tempCriticalHigh) + "°C";
    }
    else if (command.startsWith("/set_temp_low ")) {
        config.tempCriticalLow = command.substring(14).toFloat();
        response = "✅ Мин температура: " + String(config.tempCriticalLow) + "°C";
    }
    else if (command.startsWith("/set_co2_ideal ")) {
        config.co2Ideal = command.substring(15).toInt();
        response = "✅ Идеальный CO2: " + String(config.co2Ideal) + " ppm";
    }
    else if (command.startsWith("/set_co2_high ")) {
        config.co2CriticalHigh = command.substring(14).toInt();
        response = "✅ Критический CO2: " + String(config.co2CriticalHigh) + " ppm";
    }
    else if (command.startsWith("/set_mode ")) {
        String modeStr = command.substring(10);
        modeStr.toLowerCase();

        if (modeStr == "auto") {
            config.currentMode = WindowMode::AUTO;
            response = "✅ Режим изменен на: AUTO (автоматический)";
        }
        else if (modeStr == "manual") {
            config.currentMode = WindowMode::MANUAL;
            response = "✅ Режим изменен на: MANUAL (ручной)";
        }
        else {
            response = "❌ Неизвестный режим. Используйте 'auto' или 'manual'";
        }
    }
    else {
        response = "❌ Неизвестная команда. Используйте /settings для списка команд";
    }

    windowController.setConfig(config);

    bot->sendMessage(chat_id, response, "");
}

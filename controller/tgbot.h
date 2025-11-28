#pragma once

#include "window_controller.h"
#include "tgbotconfig.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <vector>

class TelegramBot {
private:
    UniversalTelegramBot* bot;
    unsigned long lastUpdateTime = 0;
    const unsigned long UPDATE_INTERVAL = 1000;

    std::vector<String> allowedUsers = ::allowedUsers;  // Используем глобальный список

    bool isUserAllowed(String user_id);
    void handleMessages(WindowController& windowController);
    void sendStatusLog(String chat_id, WindowController& windowController);
    void showSettingsMenu(String chat_id, WindowController& windowController);
    void handleParameterSetting(String chat_id, String command, WindowController& windowController);
    void sendNotAllowedMessage(String chat_id);

public:
    void init();
    void update(WindowController& windowController);
    void addAllowedUser(String user_id); // Опционально, для runtime добавления
};

#pragma once

#include "window_controller.h"
#include "tgbotconfig.h"
#include "motor_impl.h"

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
    void sendNotAllowedMessage(String chat_id);
    void sendStatusToAll(WindowController& windowController);
    void sendStatusLog(String chat_id, WindowController& windowController);

    void showSettingsMenu(String chat_id, WindowController& windowController);
    void showModeMenu(String chat_id, WindowController& windowController);
    void showWindowMenu(String chat_id, WindowController& windowController);

    void setMode(String chat_id, WindowMode mode, WindowController& windowController);

    void handleMessages(WindowController& windowController);
    void handleParameterSetting(String chat_id, String command, WindowController& windowController);
    void handleSetPosition(String chat_id, String command, WindowController& windowController);
    void handleHoming(String chat_id, WindowController& windowController);

public:
    void init();
    void update(WindowController& windowController);
    void addAllowedUser(String user_id); // Опционально, для runtime добавления
};

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "UniversalTelegramBot.h"
#include "ArduinoJson.h"

class TelegramBot {
public:
    TelegramBot(const char* botToken, const char* chatId);
    bool connectWiFi();
    bool checkMessages();

    // Команды для датчика
    String handleWeightCommand(const String& command);

private:
    WiFiClientSecure _client;
    UniversalTelegramBot _bot;
    const char* _chatId;

    String _lastMessage;
};

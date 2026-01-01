#pragma once

// WiFi
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Telegram
#define BOT_TOKEN "YOUR_BOT_TOKEN"  // @BotFather
#define CHAT_ID "YOUR_CHAT_ID"      // Ваш chat ID

// HX711 pins
#define HX711_DT_PIN  21
#define HX711_SCK_PIN 22

// Telegram certificate (ROOT для bot API)
const char* TELEGRAM_CERTIFICATE = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n" \
" [...] (полный сертификат Telegram, взять из библиотеки UniversalTelegramBot)\n" \
"-----END CERTIFICATE-----\n";

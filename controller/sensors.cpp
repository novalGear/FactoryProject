#include <OneWire.h>
#include <DallasTemperature.h>
#include <HardwareSerial.h>

#include "OLED_screen.h"

// Temparute sensors ============================================================================================================ //

const int TEMP_INTERVAL   = 2000;
const int TEMP_READ_DELAY = 1000;

// Пин подключения DS18B20
const int TEMP_1 = 4;
const int TEMP_2 = 5;
const int TEMP_3 = 23;

OneWire oneWire1(TEMP_1);
OneWire oneWire2(TEMP_2);
OneWire oneWire3(TEMP_3);

DallasTemperature temp_sensor1(&oneWire1);
DallasTemperature temp_sensor2(&oneWire2);
DallasTemperature temp_sensor3(&oneWire3);

void temp_sensors_setup() {

    pinMode(TEMP_1, INPUT_PULLUP);
    pinMode(TEMP_2, INPUT_PULLUP);
    pinMode(TEMP_3, INPUT_PULLUP);

    temp_sensor1.begin();
    temp_sensor2.begin();
    temp_sensor3.begin();
}

void read_and_display_temp() {
    float temperatureC1 = temp_sensor1.getTempCByIndex(0);
    float temperatureC2 = temp_sensor2.getTempCByIndex(0);
    float temperatureC3 = temp_sensor3.getTempCByIndex(0);

    if (temperatureC1 == DEVICE_DISCONNECTED_C || temperatureC2 != DEVICE_DISCONNECTED_C || temperatureC3 != DEVICE_DISCONNECTED_C) {
        display_temp_err();
    }

    float temp_avg = (temperatureC1 + temperatureC2) / 2;
    // float temp_avg = (temperatureC1 + temperatureC2 + temperatureC3) / 3;
    display_temperature(temp_avg);
}

void temp_request() {
    temp_sensor1.requestTemperatures();
    temp_sensor2.requestTemperatures();
    temp_sensor3.requestTemperatures();
}

void temperature_sensors_update() {
    unsigned long now = millis();
    static bool temp_ready = false;
    static unsigned long lastTempRequest = 0;

    if (!temp_ready && (now - lastTempRequest >= TEMP_READ_DELAY)) {
        read_and_display_temp();
        temp_ready = true;
    }

    if (temp_ready && (now - lastTempRequest >= TEMP_INTERVAL)) {
        temp_request();
        lastTempRequest = now;
        temp_ready = false;
    }
}

// CO2 sensor =================================================================================================================== //

// Создаём отдельный Serial для MH-Z19B (ESP32 поддерживает несколько UART)
HardwareSerial MHZ19Serial(2); // Используем UART2

const int RX_PIN = 16; // GPIO16 → подключаем к Tx датчика
const int TX_PIN = 17; // GPIO17 → подключаем к Rx датчика

void co2_sensor_setup() {
    MHZ19Serial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // 9600 бод, 8N1
}

void co2_level_request() {
    byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    MHZ19Serial.write(cmd, 9);
}

void co2_read_and_display() {
    byte response[9];
    MHZ19Serial.readBytes(response, 9);

    // Проверка заголовка и контрольной суммы
    if (response[0] == 0xFF && response[1] == 0x86) {
      // Расчёт контрольной суммы
      byte checksum = 0;
      for (int i = 1; i < 8; i++) checksum += response[i];
      checksum = 255 - checksum + 1;

      if (response[8] == checksum) {
        int co2 = (response[2] << 8) + response[3];
        Serial.print("CO2: ");
        Serial.print(co2);
        Serial.println(" ppm");
      } else {
        Serial.println("Ошибка контрольной суммы");
      }
    } else {
      Serial.println("Неверный ответ от датчика");
    }
}

void co2_sensor_update() {
    unsigned long now = millis();
    static bool data_ready = false;
    static unsigned long lastRequest = 0;


}

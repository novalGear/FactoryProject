#include <OneWire.h>
#include <DallasTemperature.h>
#include <HardwareSerial.h>
#include <assert.h>
#include "OLED_screen.h"
#include "sensors.h"

extern const int SENSORS_COUNT;

// =============== Константы ===============

const int TEMP_INTERVAL   = 10000;  // интервал между запросами измерения (мс)
const int TEMP_READ_DELAY = 1000;   // задержка перед чтением температуры (мс) — датчикам нужно ~750 мс на 12 битах

const int RESOLUTION_BITS = 10;     // устанавливаем точность измерения в битах (12 максимум)

enum TempState {
    STATE_WAITING_FOR_READ,     // ждём, когда можно прочитать
    STATE_WAITING_FOR_REQUEST   // ждём, когда можно запросить
};

OneWire* oneWires[SENSORS_COUNT];

struct TempSensor {
    int pin;
    DallasTemperature* sensor;
    float last_tempC;
    bool error;
};

TempSensor temp_sensors[SENSORS_COUNT] = {
    { .pin = 4,   .sensor = nullptr, .last_tempC = 0.0, .error = false },
    { .pin = 5,   .sensor = nullptr, .last_tempC = 0.0, .error = false },
    { .pin = 23,  .sensor = nullptr, .last_tempC = 0.0, .error = false }
};

void temp_sensors_setup() {
    for (int i = 0; i < SENSORS_COUNT; i++) {
        pinMode(temp_sensors[i].pin, INPUT_PULLUP);
        oneWires[i] = new OneWire(temp_sensors[i].pin);
        temp_sensors[i].sensor = new DallasTemperature(oneWires[i]);
        temp_sensors[i].sensor->begin();
        temp_sensors[i].sensor->setResolution(RESOLUTION_BITS);
    }
}

void temp_request() {
    for (int i = 0; i < SENSORS_COUNT; i++) {
        temp_sensors[i].sensor->setWaitForConversion(false);
        temp_sensors[i].sensor->requestTemperatures();
    }
}

void temp_sensors_read() {
    for (int i = 0; i < SENSORS_COUNT; i++) {
        float temp = temp_sensors[i].sensor->getTempCByIndex(0);
        String msg = String(i) + ":" + String(temp);
        Serial.println(msg);

        if (temp == DEVICE_DISCONNECTED_C) {
            temp_sensors[i].last_tempC = NAN;
            temp_sensors[i].error = true;
        } else {
            temp_sensors[i].last_tempC = temp;
            temp_sensors[i].error = false;
        }
    }
}
//
// void read_and_display_temp() {
//     temp_sensors_read();
//
//     for (int i = 0; i < SENSORS_COUNT; i++) {
//         display_temperature(i, temp_sensors[i].last_tempC);
//         display_temp_err(i, temp_sensors[i].error);
//     }
// }

void temperature_sensors_update() {
    static TempState temp_state = STATE_WAITING_FOR_READ;
    static unsigned long lastTempAction = 0;

    unsigned long now = millis();

    switch (temp_state) {
        case STATE_WAITING_FOR_READ:
            if (now - lastTempAction >= TEMP_READ_DELAY) {
                temp_sensors_read();
                temp_state = STATE_WAITING_FOR_REQUEST;
            }
            break;

        case STATE_WAITING_FOR_REQUEST:
            if (now - lastTempAction >= TEMP_INTERVAL) {
                temp_request();
                lastTempAction = now;
                temp_state = STATE_WAITING_FOR_READ;
            }
            break;
    }
}

float get_sensor_recent_temp(int sensor_ind) {
    assert(sensor_ind < SENSORS_COUNT && sensor_ind > 0);
    if (sensor_ind > SENSORS_COUNT || sensor_ind < 0) {return false;}
    return temp_sensors[sensor_ind].last_tempC;
}

bool get_sensor_error(int sensor_ind) {
    assert(sensor_ind < SENSORS_COUNT && sensor_ind > 0);
    if (sensor_ind > SENSORS_COUNT || sensor_ind < 0) {return false;}
    return temp_sensors[sensor_ind].error;
}

// CO2 sensor =================================================================================================================== //

const int co2_optimal = 800; // ppm

HardwareSerial MHZ19Serial(2); // Используем UART2

const int RX_PIN = 16; // GPIO16 → подключаем к Tx датчика
const int TX_PIN = 17; // GPIO17 → подключаем к Rx датчика

// --- Глобальные переменные для хранения состояния и данных CO2 ---
int last_co2_ppm = -1; // Инициализируем значением, указывающим на отсутствие данных
unsigned long last_co2_read_time = 0; // Время последнего успешного чтения
bool co2_read_error = false; // Флаг ошибки при чтении

// --- Функции ---
void co2_sensor_setup() {
    MHZ19Serial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // 9600 бод, 8N1
}

void co2_level_request() {
    byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    MHZ19Serial.write(cmd, 9);
    Serial.println("MH-Z19B: Request sent"); // Отладка
}

void co2_read_and_display() {
    // Проверяем, есть ли данные для чтения в буфере
    if (MHZ19Serial.available() >= 9) {
        byte response[9];
        // Попробуем прочитать 9 байт
        int bytesRead = MHZ19Serial.readBytes(response, 9);

        if (bytesRead == 9) { // Убедимся, что прочитали ровно 9
            // Проверка заголовка и контрольной суммы
            if (response[0] == 0xFF && response[1] == 0x86) {
                // Расчёт контрольной суммы
                byte checksum = 0;
                for (int i = 1; i < 8; i++) checksum += response[i];
                checksum = 255 - checksum + 1;

                if (response[8] == checksum) {
                    int co2 = (response[2] << 8) + response[3];
                    display_CO2(co2, co2_optimal);
                    Serial.print("CO2: ");
                    Serial.print(co2);
                    Serial.println(" ppm");
                    // --- Сохраняем данные ---
                    last_co2_ppm = co2;
                    last_co2_read_time = millis(); // Запоминаем время успешного чтения
                    co2_read_error = false; // Сбрасываем флаг ошибки
                    // ------------------------
                } else {
                    Serial.println("Ошибка контрольной суммы");
                    co2_read_error = true;
                }
            } else {
                Serial.println("Неверный ответ от датчика (заголовок)");
                co2_read_error = true;
            }
        } else {
            // Неожиданно прочитали меньше 9 байт, несмотря на available() >= 9
            // Это может указывать на дребезг, шум или сбой.
            Serial.println("Ошибка: прочитано не 9 байт при наличии данных");
            // Очищаем буфер, чтобы избежать чтения старых/неполных данных
            while (MHZ19Serial.available()) {
                MHZ19Serial.read();
            }
            co2_read_error = true;
        }
    } else {
        // Serial.println("MH-Z19B: Not enough bytes available yet"); // Отладка (опционально)
    }
}

// --- ОБНОВЛЕННАЯ ФУНКЦИЯ ---
void co2_sensor_update() {
    static unsigned long lastRequest = 0;
    const unsigned long REQUEST_INTERVAL = 10000; // Запрашивать раз в 10 секунд (пример)
    const unsigned long READ_TIMEOUT = 500;       // Таймаут ожидания ответа (500 мс)

    unsigned long now = millis();
    static unsigned long readStartTime = 0; // Время отправки последнего запроса
    static bool waiting_for_response = false; // Флаг: ждём ответ

    // Проверяем, пора ли отправлять новый запрос
    if (!waiting_for_response && (now - lastRequest >= REQUEST_INTERVAL)) {
        co2_level_request(); // Отправляем запрос
        lastRequest = now;   // Обновляем время запроса
        readStartTime = now; // Запоминаем, когда начали ждать
        waiting_for_response = true; // Устанавливаем флаг ожидания
        co2_read_error = false; // Сбрасываем флаг ошибки, так как начали новый цикл
    }

    // Проверяем, ждём ли мы ответ и не истёк ли таймаут
    if (waiting_for_response && (now - readStartTime < READ_TIMEOUT)) {
        // Пытаемся прочитать ответ
        co2_read_and_display();
        // Если co2_read_and_display успешно прочитал и обработал ответ,
        // он установит last_co2_read_time и сбросит флаги.
        // Проверим, обновилось ли время последнего чтения с момента отправки запроса
        if (last_co2_read_time >= readStartTime) {
            // Успешно прочитали ответ после этого запроса
            waiting_for_response = false; // Сбрасываем флаг ожидания
        }
    } else if (waiting_for_response && (now - readStartTime >= READ_TIMEOUT)) {
        // Таймаут ожидания ответа истёк
        Serial.println("MH-Z19B: Read timeout");
        co2_read_error = true; // Устанавливаем флаг ошибки
        waiting_for_response = false; // Сбрасываем флаг ожидания, чтобы можно было сделать новый запрос
    }

    // Если мы не ждём ответа, можно, например, проверить last_co2_ppm или last_co2_read_time
    // для принятия решений в других частях программы.
    // Например, если last_co2_read_time старше 30 секунд, показать ошибку.
    // if (now - last_co2_read_time > 30000 && !co2_read_error) {
    //    Serial.println("MH-Z19B: No recent data, sensor might be disconnected");
    //    co2_read_error = true; // Устанавливаем флаг, если давно не было данных
    // }
}

// --- Функция для получения последнего значения ---
int get_last_co2_ppm() {
    return last_co2_ppm;
}

int get_optimal_co2_ppm() {
    return co2_optimal;
}

bool get_co2_read_error() {
    return co2_read_error;
}

unsigned long get_last_co2_read_time() {
    return last_co2_read_time;
}

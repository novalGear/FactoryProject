#include "window_controller.h"
#include "sensors.h"
#include <cmath>
#include <Arduino.h>

const unsigned long DECISION_INTERVAL = 60 * 1000;

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// metrics ======================================================================================================================//
float WindowController::calculateTemperatureMetric() {
    if (recentData.tempSensorError) return config.tempErrorFallback;

    float temp_metric = abs(recentData.temperature - config.tempIdeal) * config.tempWeightMultiplier;
    temp_metric = constrain(temp_metric, 0.0f, 100.0f);
    return temp_metric;
}

float WindowController::calculateCO2Metric() {
    if (recentData.co2SensorError) return config.co2ErrorFallback;

    float co2_metric = 0.0f;
    if (recentData.co2 > config.co2Ideal) {
        co2_metric = (recentData.co2 - config.co2Ideal) / config.co2WeightDivisor;
    }
    co2_metric = constrain(co2_metric, 0.0f, 100.0f);
    return co2_metric;
}

float WindowController::calculateTotalMetric() {
    float tempMetric = calculateTemperatureMetric();
    float co2Metric = calculateCO2Metric();
    return (tempMetric * config.tempWeight) + (co2Metric * config.co2Weight);
}

// controller ===================================================================================================================//

// Реализация PositionHistory методов
void WindowController::PositionHistory::addRecord(float metric, unsigned long timestamp) {
    records[head] = {metric, timestamp};
    head = (head + 1) % HISTORY_SIZE;
    if (count < HISTORY_SIZE) count++;
}

float WindowController::PositionHistory::getWeightedMetric(unsigned long currentTime) const {
    if (count == 0) return -1.0f;

    float sumMetric = 0.0f;
    float sumWeight = 0.0f;

    for (int i = 0; i < count; i++) {
        float ageHours = (currentTime - records[i].timestamp) / 3600000.0f;
        float weight = exp(-ageHours);

        sumMetric += records[i].metric * weight;
        sumWeight += weight;
    }

    return (sumWeight >= MIN_WEIGHT_THRESHOLD) ? (sumMetric / sumWeight) : -1.0f;
}

float WindowController::PositionHistory::getTotalWeight(unsigned long currentTime) const {
    float totalWeight = 0.0f;
    for (int i = 0; i < count; i++) {
        float ageHours = (currentTime - records[i].timestamp) / 3600000.0f;
        totalWeight += exp(-ageHours);
    }
    return totalWeight;
}

float WindowController::getCurrentPosition() const {
    return get_current_position_index() / (float)POSITION_LEVELS;
}

void WindowController::updateRecentData() {
    // Обновляем температуру
    float totalTemp = 0.0f;
    int validSensors = 0;
    recentData.tempSensorError = true; // Предполагаем ошибку

    for (int i = 0; i < SENSORS_COUNT; i++) {
        if (!get_sensor_error(i)) {
            totalTemp += get_sensor_recent_temp(i);
            validSensors++;
            recentData.tempSensorError = false;
        }
    }

    if (validSensors > 0) {
        recentData.temperature = totalTemp / validSensors;
    } else {
        recentData.temperature = NAN; // или config.tempErrorFallback
    }

    // Обновляем CO2
    recentData.co2SensorError = get_co2_read_error();
    if (!recentData.co2SensorError) {
        recentData.co2 = get_last_co2_ppm();
    } else {
        recentData.co2 = NAN;
    }

    // Рассчитываем метрики
    recentData.temperatureMetric = calculateTemperatureMetric();
    recentData.co2Metric = calculateCO2Metric();
    recentData.totalMetric = calculateTotalMetric();

    // Позиция окна
    recentData.windowPosition = get_current_position_index();
    recentData.timestamp = millis();
}

// Обновляем collectData чтобы использовать updateRecentData
void WindowController::collectData(unsigned long currentTime) {
    updateRecentData(); // Сначала обновляем данные

    int positionIndex = recentData.windowPosition; // Используем уже рассчитанное
    float currentMetric = recentData.totalMetric;  // Используем уже рассчитанное

    positionHistories[positionIndex].addRecord(currentMetric, currentTime);

    Serial.print("Data collected: pos=");
    Serial.print(positionIndex);
    Serial.print(", metric=");
    Serial.print(currentMetric, 2);
    Serial.print(", time=");
    Serial.println(currentTime);
}

void WindowController::makeDecision(unsigned long currentTime) {
    float currentMetric = calculateTotalMetric();
    float metricTrend = calculateMetricTrend(currentTime);
    float predictedMetric = currentMetric + metricTrend * config.predictionTime;

    Serial.print("Decision: currMetric=");
    Serial.print(currentMetric, 2);
    Serial.print(", trend=");
    Serial.print(metricTrend, 4);
    Serial.print(", predicted=");
    Serial.print(predictedMetric, 2);

    if (predictedMetric > config.metricTarget + config.metricMargin) {
        Serial.println(" - NEED ACTION (too high)");
        takeAction(currentTime, true);
    }
    else if (predictedMetric < config.metricTarget - config.metricMargin) {
        Serial.println(" - NEED ACTION (too low)");
        takeAction(currentTime, false);
    }
    else {
        Serial.println(" - NO ACTION needed");
    }
}

void WindowController::takeAction(unsigned long currentTime, bool needToImprove) {
    int bestPosition = findBestPosition(currentTime, needToImprove);
    int currentPositionIndex = get_current_position_index();

    if (bestPosition != -1 && bestPosition != currentPositionIndex) {
        Serial.print("Moving to position: ");
        Serial.println(bestPosition);
        change_pos(bestPosition); // Используем готовую функцию из motor_impl.h
    } else {
        Serial.println("No better position found or already at best");
    }
}

int WindowController::findBestPosition(unsigned long currentTime, bool needToImprove) const {
    int bestPosition = -1;
    float bestMetric = needToImprove ? 1000.0f : -1000.0f;

    for (int i = 0; i < POSITION_LEVELS; i++) {
        float weightedMetric = positionHistories[i].getWeightedMetric(currentTime);
        float totalWeight = positionHistories[i].getTotalWeight(currentTime);

        if (weightedMetric >= 0.0f && totalWeight >= MIN_WEIGHT_THRESHOLD) {
            Serial.print("  Pos ");
            Serial.print(i);
            Serial.print(": metric=");
            Serial.print(weightedMetric, 2);
            Serial.print(", weight=");
            Serial.print(totalWeight, 2);

            bool isBetter = needToImprove ? (weightedMetric < bestMetric) : (weightedMetric > bestMetric);

            if (isBetter) {
                bestMetric = weightedMetric;
                bestPosition = i;
                Serial.println(" - NEW BEST");
            } else {
                Serial.println();
            }
        }
    }

    return bestPosition;
}

float WindowController::calculateMetricTrend(unsigned long currentTime) const {
    int currentPosIndex = get_current_position_index();
    const PositionHistory& history = positionHistories[currentPosIndex];

    if (history.count < 3) return 0.0f;

    int lastIndex = (history.head - 1 + HISTORY_SIZE) % HISTORY_SIZE;
    int prevIndex = (history.head - 2 + HISTORY_SIZE) % HISTORY_SIZE;

    float lastMetric = history.records[lastIndex].metric;
    float prevMetric = history.records[prevIndex].metric;
    unsigned long lastTime = history.records[lastIndex].timestamp;
    unsigned long prevTime = history.records[prevIndex].timestamp;

    if (lastTime == prevTime) return 0.0f;

    return (lastMetric - prevMetric) / ((lastTime - prevTime) / 1000.0f);
}

void WindowController::update() {
    unsigned long currentTime = millis();

    // 1. ПРИОРИТЕТ: Проверка аварийных условий (каждые 10 секунд)
    if (currentTime - lastEmergencyCheckTime >= emergencyConfig.emergencyCheckInterval) {
        if (checkEmergencyConditions()) {
            handleEmergency();
            lastEmergencyCheckTime = currentTime;
            return; // Прерываем обычную работу при аварии
        }
        lastEmergencyCheckTime = currentTime;
    }

    // 2. Обычная работа (сбор данных и принятие решений)
    if (currentTime - lastDataCollectionTime >= DATA_COLLECTION_INTERVAL) {
        collectData(currentTime);
        lastDataCollectionTime = currentTime;
    }

    if (currentTime - lastDecisionTime >= DECISION_INTERVAL) {
        makeDecision(currentTime);
        lastDecisionTime = currentTime;
    }
}

// emergencies ==================================================================================================================//

bool WindowController::checkEmergencyConditions() {
    // Проверка критической температуры
    for (int i = 0; i < SENSORS_COUNT; i++) {
        if (!get_sensor_error(i)) {
            float temp = get_sensor_recent_temp(i);
            if (temp >= emergencyConfig.tempCriticalHigh) {
                Serial.print("EMERGENCY: Critical high temperature: ");
                Serial.print(temp, 1);
                Serial.println("°C");
                return true;
            }
            if (temp <= emergencyConfig.tempCriticalLow) {
                Serial.print("EMERGENCY: Critical low temperature: ");
                Serial.print(temp, 1);
                Serial.println("°C");
                return true;
            }
        }
    }

    // Проверка критического CO2
    if (!get_co2_read_error()) {
        int co2 = get_last_co2_ppm();
        if (co2 >= emergencyConfig.co2CriticalHigh) {
            Serial.print("EMERGENCY: Critical CO2 level: ");
            Serial.print(co2);
            Serial.println("ppm");
            return true;
        }
    }

    // Проверка сбоя датчиков (если все датчики температуры не работают)
    bool allTempSensorsFailed = true;
    for (int i = 0; i < SENSORS_COUNT; i++) {
        if (!get_sensor_error(i)) {
            allTempSensorsFailed = false;
            break;
        }
    }

    if (allTempSensorsFailed && get_co2_read_error()) {
        Serial.println("EMERGENCY: All sensors failed!");
        return true;
    }

    return false;
}

void WindowController::handleEmergency() {
    Serial.println("=== EMERGENCY HANDLING ===");

    // Принудительно открываем окно полностью
    emergencyFullOpen();

    // Дополнительные действия при аварии
    // - Можно отправить уведомление
    // - Включить сигнализацию
    // - Записать в лог аварию

    Serial.println("Emergency handling completed - window fully open");
}

void WindowController::emergencyFullOpen() {
    Serial.println("EMERGENCY: Moving to fully open position");

    // Открываем на максимальную позицию (POSITION_LEVELS - 1)
    change_pos(POSITION_LEVELS - 1);

    // Ждем завершения движения (блокирующе)
    delay(5000); // Даем время на полное открытие

    Serial.println("EMERGENCY: Window fully opened");
}

#include "window_controller.h"
#include "sensor_sim.h"
#include <cmath>
#include <Arduino.h>

const unsigned long DECISION_INTERVAL = 60 * 1000;

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void WindowController::setMode(WindowMode newMode) {
    if (currentMode == newMode) return;

    Serial.print("Changing mode from ");
    Serial.print(static_cast<int>(currentMode));
    Serial.print(" to ");
    Serial.println(static_cast<int>(newMode));

    currentMode = newMode;

    // Сброс состояния при смене режима
    if (newMode == WindowMode::SHORT_TERM) {
        shortTermMetrics.clear();
    }
}

// работа с датчиками и мотором =================================================================================================//

void WindowController::setManualPosition(int position) {
    if (currentMode != WindowMode::MANUAL) {
        Serial.println("Warning: Setting manual position while not in MANUAL mode");
    }
    position = constrain(position, 0, POSITION_LEVELS - 1);
    Serial.print("MANUAL: Setting position to ");
    Serial.println(position);
    change_pos(position);
}


float WindowController::getCurrentPosition() const {
    return get_current_position_index() / (float)POSITION_LEVELS;
}

void WindowController::updateRecentData() {
    unsigned long currentTime = millis();

    // Обновляем комнатную температуру
    recentData.tempSensorError = get_room_sensor_error();
    if (!recentData.tempSensorError) {
        recentData.temperature = get_room_temp();
    } else {
        recentData.temperature = NAN;
        Serial.println("WARNING: Room temperature sensor error");
    }

    // Обновляем наружную температуру
    recentData.outsideTemp = get_outside_temp();
    recentData.outsideSensorError = get_outside_sensor_error();
    if (recentData.outsideSensorError) {
        Serial.println("WARNING: Outside temperature sensor error");
    }

    // Обновляем CO2
    recentData.co2SensorError = get_co2_read_error();
    if (!recentData.co2SensorError) {
        recentData.co2 = get_last_co2_ppm();
    } else {
        recentData.co2 = -1;
        Serial.println("WARNING: CO2 sensor error");
    }

    // Рассчитываем метрики
    recentData.temperatureMetric = calculateTemperatureMetric();
    recentData.co2Metric = calculateCO2Metric();
    recentData.totalMetric = calculateTotalMetric();

    // Позиция окна
    recentData.windowPosition = get_current_position_index();
    recentData.timestamp = currentTime;

    // Логируем обновление (для отладки)
    // static unsigned long lastLogTime = 0;
    // if (currentTime - lastLogTime > 5000) { // Логируем каждые 5 секунд
    //     Serial.print("RecentData updated: Room=");
    //     Serial.print(recentData.temperature, 1);
    //     Serial.print("°C, Outside=");
    //     Serial.print(recentData.outsideTemp, 1);
    //     Serial.print("°C, CO2=");
    //     Serial.print(recentData.co2);
    //     Serial.print("ppm, Window=");
    //     Serial.print(recentData.windowPosition);
    //     Serial.print(", TotalMetric=");
    //     Serial.println(recentData.totalMetric, 2);
    //     lastLogTime = currentTime;
    // }
}

void WindowController::collectData(unsigned long currentTime) {
    updateRecentData(); // Сначала обновляем данные

    // НЕ записываем в positionHistories - мы ее больше не используем
    // Просто логируем сбор данных
    Serial.print("RecentData: ");
    Serial.print("Temp=");
    Serial.print(recentData.temperature, 1);
    Serial.print("°C, Outside=");
    Serial.print(recentData.outsideTemp, 1);
    Serial.print("°C, CO2=");
    Serial.print(recentData.co2);
    Serial.print("ppm, TempMetric=");
    Serial.print(recentData.temperatureMetric, 2);
    Serial.print(", CO2Metric=");
    Serial.print(recentData.co2Metric, 2);
    Serial.print(", TotalMetric=");
    Serial.print(recentData.totalMetric, 2);
    Serial.print(", Window=");
    Serial.print(recentData.windowPosition);
    Serial.print(", Time=");
    Serial.println(recentData.timestamp);
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
//
//     Serial.println("tempMetric: " + String(tempMetric));
//     Serial.println("co2Metric: "  + String(co2Metric));
    return (tempMetric * config.tempWeight) + (co2Metric * config.co2Weight);
}

// обновление данных ==============================================================================================================//

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

// логика управления ============================================================================================================//
void WindowController::tick() {
    update();
}

void WindowController::update() {
    virtualTime++;

    // 1. Проверка экстренных условий (каждые 10 тиков)
    if (virtualTime - lastEmergencyCheckTime >= 10) {
        EmergencyType emergency = checkEmergencyConditions();

        if (emergency != EmergencyType::NONE) {
            handleEmergency(emergency);
        }
        else if (currentMode == WindowMode::EMERGENCY) {
            if (shouldExitEmergencyMode(virtualTime)) {
                currentEmergency = EmergencyType::NONE;
                setMode(WindowMode::AUTO);
                Serial.println("Exiting emergency mode, returning to AUTO");
            }
        }

        lastEmergencyCheckTime = virtualTime;
    }

    // Если в экстренном режиме - пропускаем обычную логику
    if (currentMode == WindowMode::EMERGENCY) {
        return;
    }

    // 2. Сбор данных при каждом тике
    if (virtualTime - lastDataCollectionTime >= DATA_COLLECTION_INTERVAL) {
        collectData(virtualTime);
        lastDataCollectionTime = virtualTime;

        // Обновляем буфер последних измерений
        updateRecentMetrics(recentData.totalMetric);
    }

    // 3. Принятие решений только каждые 6 тиков
    if (virtualTime - lastDecisionTime >= DECISION_INTERVAL) {
        float currentMetric = recentData.totalMetric;
        float metricTrend = calculateMetricTrend();
        float predictedMetric = currentMetric + metricTrend * 18; // Прогноз на 18 тиков вперед

        Serial.print("=== DECISION AT TICK ");
        Serial.print(virtualTime);
        Serial.print(" ===");
        Serial.print(" Current: ");
        Serial.print(currentMetric, 2);
        Serial.print(", Trend: ");
        Serial.print(metricTrend, 3);
        Serial.print("/tick, Predicted: ");
        Serial.print(predictedMetric, 2);
        Serial.println(" ===");

        // Отладочная информация о последних измерениях
        Serial.print("Recent metrics: ");
        for (int i = 0; i < recentMetricsCount; i++) {
            int idx = (recentMetricsIndex - recentMetricsCount + i + RECENT_METRICS_SIZE) % RECENT_METRICS_SIZE;
            Serial.print(recentMetrics[idx], 1);
            Serial.print(" ");
        }
        Serial.println();

        switch(currentMode) {
            case WindowMode::AUTO:
                make_decision_auto_ST(virtualTime, currentMetric, predictedMetric);
                break;
            case WindowMode::BINARY:
                makeDecisionBinary(currentMetric);
                break;
            case WindowMode::SHORT_TERM:
                makeDecisionShortTerm(currentMetric);
                break;
            case WindowMode::MANUAL:
                Serial.println("MANUAL mode - no auto decisions");
                break;
            default:
                break;
        }
        lastDecisionTime = virtualTime;
    }
}

bool WindowController::shouldExitEmergencyMode(unsigned long currentTime) {
    // Для CO2 аварии - ждем фиксированное время
    if (currentTime - emergencyStartTime >= CO2_EMERGENCY_DURATION) {
        return true;
    }

    // Для температурных аварий - проверяем, нормализовалась ли температура
    if (!get_room_sensor_error()) {
        float roomTemp = get_room_temp();
        if (roomTemp <= emergencyConfig.tempCriticalHigh &&
            roomTemp >= emergencyConfig.tempCriticalLow) {
            return true;
        }
    }

    // Минимальное время в аварийном режиме
    return (currentTime - emergencyStartTime >= TEMP_EMERGENCY_DURATION);
}

bool WindowController::need2Improve(float metric) {
    // Serial.println();
    // Serial.println("metric: " + String(metric));
    // Serial.println("target:" + String(config.metricTarget));
    // Serial.println(metric - config.metricTarget);
    // Serial.println(config.metricMargin);
    // Serial.println((metric - config.metricTarget) > config.metricMargin);
    return (metric - config.metricTarget) > config.metricMargin;
}

void WindowController::make_decision_auto_ST(unsigned long currentTime, float currentMetric, float predictedMetric) {
    // Определяем необходимость улучшения
    if (!need2Improve(currentMetric)) {
        Serial.println("Good metric (" + String(currentMetric) + "), no actions needed");
        return;
    } else if (!need2Improve(predictedMetric)) {
        Serial.println("Good trend (" + String(currentMetric) + "->" + String(predictedMetric) + "), metric will stabilize soon");
        return;
    }

    // Обновляем данные
    updateRecentData();

    // Получаем текущую позицию
    int currentPosition = recentData.windowPosition;

    // 1. Рассчитываем "полезность" открытия и закрытия
    float openBenefit = 0.0f;
    float closeBenefit = 0.0f;

    // Вклад CO2
    if (recentData.co2 > config.co2Ideal && !recentData.co2SensorError) {
        float co2Excess = recentData.co2 - config.co2Ideal;
        openBenefit += (co2Excess / 100.0f) * 10.0f;
        closeBenefit -= (co2Excess / 100.0f) * 5.0f;
    }

    // Вклад температуры
    if (!recentData.outsideSensorError) {
        float tempDiff = recentData.outsideTemp - recentData.temperature;
        float roomToIdeal = config.tempIdeal - recentData.temperature;

        if (recentData.temperature > config.tempIdeal && tempDiff < 0) {
            // Жарко в комнате, холодно снаружи - открытие охладит
            openBenefit += abs(roomToIdeal) * 2.0f;
            closeBenefit -= abs(roomToIdeal) * 1.0f;
        }
        else if (recentData.temperature < config.tempIdeal && tempDiff > 0) {
            // Холодно в комнате, тепло снаружи - открытие нагреет
            openBenefit += abs(roomToIdeal) * 2.0f;
            closeBenefit -= abs(roomToIdeal) * 1.0f;
        }
        else {
            // Открытие ухудшит температурные условия
            openBenefit -= abs(roomToIdeal) * 1.0f;
            closeBenefit += abs(roomToIdeal) * 2.0f;
        }
    }

    Serial.print("  Direction analysis: openBenefit=");
    Serial.print(openBenefit, 2);
    Serial.print(", closeBenefit=");
    Serial.print(closeBenefit, 2);

    // 2. Определяем направление движения
    const float MIN_BENEFIT_THRESHOLD = 3.0f;
    int direction = 0; // 0 = остаться, 1 = открыть, -1 = закрыть

    if (openBenefit - closeBenefit > MIN_BENEFIT_THRESHOLD) {
        direction = 1;
        Serial.println(" - DECISION: OPEN");
    }
    else if (closeBenefit - openBenefit > MIN_BENEFIT_THRESHOLD) {
        direction = -1;
        Serial.println(" - DECISION: CLOSE");
    }
    else {
        Serial.println(" - DECISION: HOLD");
        return;
    }

    // 3. Выполняем движение на одну позицию
    int newPosition = currentPosition + direction;
    newPosition = constrain(newPosition, 0, POSITION_LEVELS - 1);

    if (newPosition != currentPosition) {
        Serial.print("  Moving from ");
        Serial.print(currentPosition);
        Serial.print(" to ");
        Serial.println(newPosition);
        change_pos(newPosition);

        // НЕ записываем в историю позиций - мы ее больше не используем
    }
}

void WindowController::makeDecisionAuto(unsigned long currentTime, float currentMetric, float predictedMetric) {
    if ((predictedMetric > config.metricTarget + config.metricMargin) ||
        (predictedMetric < config.metricTarget - config.metricMargin)) {
        takeActionAuto(currentTime, currentMetric, predictedMetric);
    } else {
        Serial.println(" - AUTO: No action needed");
    }
}

void WindowController::makeDecisionBinary(float currentMetric) {
    int currentPosition = get_current_position_index();

    if ((currentMetric > config.binaryOpenThreshold && currentPosition != POSITION_LEVELS - 1) ||
        (currentMetric < config.binaryCloseThreshold && currentPosition != 0)) {
        takeActionBinary(currentMetric);
    } else {
        Serial.println(" - BINARY: No action needed");
    }
}

void WindowController::makeDecisionShortTerm(float currentMetric) {
    if (shortTermMetrics.size() < 2) {
        Serial.println(" - SHORT_TERM: Not enough data");
        return;
    }

    float oldestMetric = shortTermMetrics.front();
    float metricChange = currentMetric - oldestMetric;

    if (abs(metricChange) > config.shortTermSensitivity) {
        takeActionShortTerm(currentMetric);
    } else {
        Serial.println(" - SHORT_TERM: No significant change");
    }
}

void WindowController::takeActionAuto(unsigned long currentTime, float currentMetric, float predictedMetric) {
    bool needToImprove = predictedMetric > config.metricTarget + config.metricMargin;

//     // Используем новую short-term логику вместо исторической
//     int bestPosition = find_best_pos_ST(needToImprove, currentMetric, predictedMetric);
//     int currentPosition = get_current_position_index();
//
//     if (bestPosition != -1 && bestPosition != currentPosition) {
//         Serial.print("AUTO: Moving to position ");
//         Serial.println(bestPosition);
//         change_pos(bestPosition);
//
//         // Записываем решение в историю для будущего обучения
//         positionHistories[bestPosition].addRecord(currentMetric, currentTime);
//     } else {
//         Serial.println("AUTO: No better position found");
//     }
}

void WindowController::takeActionBinary(float currentMetric) {
    int currentPosition = get_current_position_index();

    if (currentMetric > config.binaryOpenThreshold && currentPosition != POSITION_LEVELS - 1) {
        Serial.println("BINARY: Opening fully");
        change_pos(POSITION_LEVELS - 1);
    }
    else if (currentMetric < config.binaryCloseThreshold && currentPosition != 0) {
        Serial.println("BINARY: Closing fully");
        change_pos(0);
    }
}

void WindowController::takeActionShortTerm(float currentMetric) {
    float oldestMetric = shortTermMetrics.front();
    float metricChange = currentMetric - oldestMetric;
    int currentPosition = get_current_position_index();
    int newPosition = currentPosition;

    if (metricChange > 0) {
        newPosition = min(currentPosition + 1, POSITION_LEVELS - 1);
        Serial.print("SHORT_TERM: Opening to ");
        Serial.println(newPosition);
    } else {
        newPosition = max(currentPosition - 1, 0);
        Serial.print("SHORT_TERM: Closing to ");
        Serial.println(newPosition);
    }

    change_pos(newPosition);
}

// поиск наилучшей позиции ======================================================================================================//


int WindowController::findBestPosition(unsigned long currentTime, bool needToImprove) const {
    int bestPosition = -1;
    float bestMetric = needToImprove ? 1000.0f : -1000.0f;

    // Получаем текущие данные
    float insideTemp = get_room_temp();
    float outsideTemp = get_outside_temp();
    bool outsideSensorError = get_outside_sensor_error();

    // Определяем температурный тренд
    float tempDiff = outsideTemp - insideTemp;
    bool coolingNeeded = insideTemp > config.tempIdeal;
    bool heatingNeeded = insideTemp < config.tempIdeal;

    // Определяем, поможет ли открытие окна
    bool openingHelps = false;
    if (!outsideSensorError) {
        if (coolingNeeded && tempDiff < 0) {
            openingHelps = true; // Снаружи холоднее - открытие охладит
        } else if (heatingNeeded && tempDiff > 0) {
            openingHelps = true; // Снаружи теплее - открытие нагреет
        }
    }

    // Получаем текущий CO2
    int currentCO2 = get_last_co2_ppm();
    bool co2High = currentCO2 > config.co2Ideal;

    for (int i = 0; i < POSITION_LEVELS; i++) {
        float weightedMetric = positionHistories[i].getWeightedMetric(currentTime);
        float totalWeight = positionHistories[i].getTotalWeight(currentTime);

        if (weightedMetric >= 0.0f && totalWeight >= MIN_WEIGHT_THRESHOLD) {
            // Корректируем метрику на основе текущих условий
            float adjustedMetric = weightedMetric;

            // Учет температурных условий
            if (!outsideSensorError) {
                if (openingHelps) {
                    // Открытие помогает - награждаем более открытые позиции
                    float opennessBonus = (i / (float)(POSITION_LEVELS - 1)) * 10.0f;
                    adjustedMetric -= opennessBonus;
                } else {
                    // Открытие не помогает - наказываем более открытые позиции
                    float opennessPenalty = (i / (float)(POSITION_LEVELS - 1)) * 10.0f;
                    adjustedMetric += opennessPenalty;
                }
            }

            // Учет CO2
            if (co2High) {
                // Высокий CO2 - награждаем более открытые позиции
                float co2Bonus = (i / (float)(POSITION_LEVELS - 1)) * 15.0f;
                adjustedMetric -= co2Bonus;
            }

            Serial.print("  Pos ");
            Serial.print(i);
            Serial.print(": base_metric=");
            Serial.print(weightedMetric, 2);
            Serial.print(", adjusted_metric=");
            Serial.print(adjustedMetric, 2);
            Serial.print(", weight=");
            Serial.print(totalWeight, 2);

            bool isBetter = needToImprove ? (adjustedMetric < bestMetric) : (adjustedMetric > bestMetric);

            if (isBetter) {
                bestMetric = adjustedMetric;
                bestPosition = i;
                Serial.println(" - NEW BEST");
            } else {
                // Serial.println();
            }
        }
    }

    return bestPosition;
}

void WindowController::updateRecentMetrics(float metric) {
    recentMetrics[recentMetricsIndex] = metric;
    recentMetricsIndex = (recentMetricsIndex + 1) % RECENT_METRICS_SIZE;
    if (recentMetricsCount < RECENT_METRICS_SIZE) {
        recentMetricsCount++;
    }
}

float WindowController::calculateMetricTrend() const {
    if (recentMetricsCount < 2) return 0.0f;

    // Простой расчет тренда: разница между последним и первым измерением
    int firstIndex = (recentMetricsIndex - recentMetricsCount + RECENT_METRICS_SIZE) % RECENT_METRICS_SIZE;
    int lastIndex = (recentMetricsIndex - 1 + RECENT_METRICS_SIZE) % RECENT_METRICS_SIZE;

    float firstMetric = recentMetrics[firstIndex];
    float lastMetric = recentMetrics[lastIndex];

    // Тренд = изменение за (recentMetricsCount - 1) тиков
    return (lastMetric - firstMetric) / (recentMetricsCount - 1);
}

// emergencies ==================================================================================================================//

EmergencyType WindowController::checkEmergencyConditions() {
    // 1. ПРИОРИТЕТ: Критический CO2
    if (!get_co2_read_error()) {
        int co2 = get_last_co2_ppm();
        if (co2 >= emergencyConfig.co2CriticalHigh) {
            Serial.print("EMERGENCY: Critical CO2 level: ");
            Serial.print(co2);
            Serial.println("ppm");
            return EmergencyType::CO2_CRITICAL;
        }
    }

    // 2. Критическая температура в комнате
    if (!get_room_sensor_error()) {
        float roomTemp = get_room_temp();
        float outsideTemp = get_outside_temp();
        bool outsideSensorOk = !get_outside_sensor_error();

        if (roomTemp >= emergencyConfig.tempCriticalHigh) {
            if (outsideSensorOk && outsideTemp < roomTemp) {
                Serial.print("EMERGENCY: Critical high temperature - opening will help: ");
                Serial.print(roomTemp, 1);
                Serial.print("°C, outside ");
                Serial.print(outsideTemp, 1);
                Serial.println("°C");
                return EmergencyType::TEMP_CRITICAL_HELP;
            } else {
                Serial.print("EMERGENCY: Critical high temperature - opening will harm: ");
                Serial.print(roomTemp, 1);
                Serial.print("°C, outside ");
                Serial.print(outsideTemp, 1);
                Serial.println("°C");
                return EmergencyType::TEMP_CRITICAL_HARM;
            }
        }

        if (roomTemp <= emergencyConfig.tempCriticalLow) {
            if (outsideSensorOk && outsideTemp > roomTemp) {
                Serial.print("EMERGENCY: Critical low temperature - opening will help: ");
                Serial.print(roomTemp, 1);
                Serial.print("°C, outside ");
                Serial.print(outsideTemp, 1);
                Serial.println("°C");
                return EmergencyType::TEMP_CRITICAL_HELP;
            } else {
                Serial.print("EMERGENCY: Critical low temperature - opening will harm: ");
                Serial.print(roomTemp, 1);
                Serial.print("°C, outside ");
                Serial.print(outsideTemp, 1);
                Serial.println("°C");
                return EmergencyType::TEMP_CRITICAL_HARM;
            }
        }
    }

    // 3. Отказ датчиков
    if (get_room_sensor_error() && get_co2_read_error()) {
        Serial.println("EMERGENCY: All sensors failed!");
        return EmergencyType::SENSOR_FAILURE;
    }

    return EmergencyType::NONE;
}

void WindowController::handleEmergency(EmergencyType emergencyType) {
    emergencyStartTime = millis();

    switch(emergencyType) {
        case EmergencyType::CO2_CRITICAL:
            handleCo2Emergency();
            break;
        case EmergencyType::TEMP_CRITICAL_HELP:
            handleTempEmergency(true);
            break;
        case EmergencyType::TEMP_CRITICAL_HARM:
            handleTempEmergency(false);
            break;
        case EmergencyType::SENSOR_FAILURE:
            handleSensorFailure();
            break;
        default:
            break;
    }
}

void WindowController::handleCo2Emergency() {
    // Для CO2 - всегда полное открытие
    Serial.println("CO2 EMERGENCY: Full opening for ventilation");
    change_pos(POSITION_LEVELS - 1);
    setMode(WindowMode::EMERGENCY);
}

void WindowController::handleTempEmergency(bool willHelp) {
    if (willHelp) {
        // Открытие поможет - полное открытие
        Serial.println("TEMP EMERGENCY: Full opening to normalize temperature");
        change_pos(POSITION_LEVELS - 1);
    } else {
        // Открытие навредит - полное закрытие
        Serial.println("TEMP EMERGENCY: Full closing to preserve temperature");
        change_pos(0);
    }
    setMode(WindowMode::EMERGENCY);
}

void WindowController::handleSensorFailure() {
    // При отказе датчиков - консервативная стратегия: оставляем как есть
    Serial.println("SENSOR FAILURE: Maintaining current position");
    // Не меняем позицию, но переводим в ручной режим для безопасности
    setMode(WindowMode::MANUAL);
}

void WindowController::emergencyFullOpen() {
    Serial.println("EMERGENCY: Moving to fully open position");

    // Открываем на максимальную позицию (POSITION_LEVELS - 1)
    change_pos(POSITION_LEVELS - 1);

    // Ждем завершения движения (блокирующе)
    delay(5000); // Даем время на полное открытие

    Serial.println("EMERGENCY: Window fully opened");
}

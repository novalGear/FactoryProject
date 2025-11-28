#pragma once

#include <deque>

#include "sensor_sim.h"
#include "motor_sim.h"

enum class EmergencyType {
    NONE,
    CO2_CRITICAL,       // Высокий CO2 - всегда открывать
    TEMP_CRITICAL_HELP, // Критическая температура, открытие поможет
    TEMP_CRITICAL_HARM, // Критическая температура, открытие навредит
    SENSOR_FAILURE      // Отказ датчиков
};

enum class WindowMode {
    MANUAL,
    AUTO,
    BINARY,
    SHORT_TERM,
    EMERGENCY
};

struct RecentData {
    float temperature;          // Последняя измеренная температура
    float co2;                  // Последний измеренный CO2
    float temperatureMetric;    // Рассчитанная метрика температуры
    float co2Metric;            // Рассчитанная метрика CO2
    float totalMetric;          // Общая метрика
    int windowPosition;         // Текущая позиция окна (0-9)
    bool tempSensorError;       // Ошибка датчиков температуры
    bool co2SensorError;        // Ошибка датчика CO2
    unsigned long timestamp;    // Время последнего измерения
};

struct WindowConfig {
    // Основные параметры управления
    float metricTarget = 20.0f;
    float metricMargin = 10.0f;
    float predictionTime = 180.0f;

    // Параметры температурной метрики
    float tempIdeal = 22.0f;           // Идеальная температура
    float tempWeightMultiplier = 2.0f; // Множитель для метрики температуры
    float tempErrorFallback = 50.0f;   // Значение при ошибке датчиков

    // Параметры CO2 метрики
    int co2Ideal = 600;                // Идеальный CO2
    float co2WeightDivisor = 20.0f;    // Делитель для метрики CO2
    float co2ErrorFallback = 30.0f;    // Значение при ошибке датчика

    // Веса метрик
    float tempWeight = 0.7f;
    float co2Weight = 0.3f;

    // Аварийные параметры
    float tempCriticalHigh = 30.0f;
    float tempCriticalLow = 5.0f;
    int co2CriticalHigh = 2000;
    unsigned long emergencyCheckInterval = 10000;

     WindowMode defaultMode = WindowMode::AUTO;

    // Параметры для BINARY режима
    float binaryOpenThreshold = 30.0f;
    float binaryCloseThreshold = 10.0f;

    // Параметры для SHORT_TERM режима
    unsigned short shortTermHistorySize = 6; // 1 минута при 10-секундном интервале
    float shortTermSensitivity = 2.0f;
};

class WindowController {
private:
    WindowMode currentMode = WindowMode::AUTO;
    RecentData recentData;
    WindowConfig config;

    std::deque<float> shortTermMetrics;

    static const int POSITION_LEVELS = 10;
    static const int HISTORY_SIZE = 180;
    static const unsigned long DECISION_INTERVAL = 60000;
    static const unsigned long DATA_COLLECTION_INTERVAL = 60000;
    static constexpr float MIN_WEIGHT_THRESHOLD = 0.1f;

    struct MetricRecord {
        float metric;
        unsigned long timestamp;
    };

    struct PositionHistory {
        MetricRecord records[HISTORY_SIZE];
        int head = 0;
        int count = 0;

        void addRecord(float metric, unsigned long timestamp);
        float getWeightedMetric(unsigned long currentTime) const;
        float getTotalWeight(unsigned long currentTime) const;
    };

    PositionHistory positionHistories[POSITION_LEVELS];
    unsigned long lastDataCollectionTime = 0;
    unsigned long lastDecisionTime = 0;

    // Private methods
    void collectData(unsigned long currentTime);
    void makeDecisionAuto(unsigned long currentTime, float currentMetric, float predictedMetric);
    void makeDecisionBinary(float currentMetric);
    void makeDecisionShortTerm(float currentMetric);
    void handleManualMode();

    void takeActionAuto(unsigned long currentTime, float currentMetric, float predictedMetric);
    void takeActionBinary(float currentMetric);
    void takeActionShortTerm(float currentMetric);

    int findBestPosition(unsigned long currentTime, bool needToImprove) const;
    float calculateMetricTrend(unsigned long currentTime) const;
    float calculateTotalMetric();
    float calculateTemperatureMetric();
    float calculateCO2Metric();

    // emergencies ==============================================================================================================//
    struct EmergencyConfig {
        float tempCriticalHigh = 30.0f;
        float tempCriticalLow = 5.0f;
        int co2CriticalHigh = 2000;
        unsigned long emergencyCheckInterval = 10000; // Проверка каждые 10 секунд
    } emergencyConfig;

    EmergencyType currentEmergency = EmergencyType::NONE;
    unsigned long emergencyStartTime = 0;
    const unsigned long CO2_EMERGENCY_DURATION = 300000; // 5 минут для CO2
    const unsigned long TEMP_EMERGENCY_DURATION = 600000; // 10 минут для температуры
    unsigned long lastEmergencyCheckTime = 0;

    bool shouldExitEmergencyMode(unsigned long currentTime);
    EmergencyType checkEmergencyConditions();
    void handleEmergency(EmergencyType emergencyType);
    void handleCo2Emergency();
    void handleTempEmergency(bool willHelp);
    void handleSensorFailure();
    void emergencyFullOpen();

public:
    void setMode(WindowMode newMode);
    void setManualPosition(int position);
    void updateRecentData();
    WindowController() = default;
    void update();
    float getCurrentPosition() const;

    RecentData getRecentData() const { return recentData; }
    void setConfig(const WindowConfig& newConfig) {
        config = newConfig;
    }

    const WindowConfig& getConfig() const {
        return config;
    }
};

#pragma once
#include "sensors.h"
#include "motor_impl.h"

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
};

class WindowController {
private:
    RecentData recentData;
    WindowConfig config;
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
    void makeDecision(unsigned long currentTime);
    void takeAction(unsigned long currentTime, bool needToImprove);
    int findBestPosition(unsigned long currentTime, bool needToImprove) const;
    float calculateMetricTrend(unsigned long currentTime) const;
    float calculateTotalMetric();
    float calculateTemperatureMetric();
    float calculateCO2Metric();

    struct EmergencyConfig {
        float tempCriticalHigh = 30.0f;
        float tempCriticalLow = 5.0f;
        int co2CriticalHigh = 2000;
        unsigned long emergencyCheckInterval = 10000; // Проверка каждые 10 секунд
    } emergencyConfig;

    unsigned long lastEmergencyCheckTime = 0;

    bool checkEmergencyConditions();
    void handleEmergency();
    void emergencyFullOpen();

public:
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

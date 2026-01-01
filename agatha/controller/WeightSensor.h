#pragma once
#include <Arduino.h>
#include "HX711.h"

class WeightSensor {
public:
    WeightSensor(uint8_t dtPin, uint8_t sckPin);

    void begin(float calibrationFactor);
    float getWeight(uint8_t samples = 5);
    void tare();
    void calibrate(float knownWeight);  // калибровка по известному грузу

    float getCalibrationFactor() const { return _calibrationFactor; }
    bool isReady() const { return _isReady; }

private:
    HX711 _scale;
    float _calibrationFactor = 1.0f;
    bool _isReady = false;
};

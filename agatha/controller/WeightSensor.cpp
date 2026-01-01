#include "WeightSensor.h"

WeightSensor::WeightSensor(uint8_t dtPin, uint8_t sckPin) {
    _scale.begin(dtPin, sckPin);
}

void WeightSensor::begin(float calibrationFactor) {
    _calibrationFactor = calibrationFactor;
    _scale.set_scale(_calibrationFactor);
    _scale.tare();
    _isReady = true;
}

float WeightSensor::getWeight(uint8_t samples) {
    if (!_isReady || samples == 0) return 0.0f;
    return _scale.get_units(samples);
}

void WeightSensor::tare() {
    if (_isReady) {
        _scale.tare();
    }
}

void WeightSensor::calibrate(float knownWeight) {
    if (!_isReady || knownWeight <= 0) return;

    _scale.tare();  // сначала обнулили
    delay(100);

    float rawValue = _scale.get_units(10);  // усреднили 10 измерений
    if (rawValue > 0) {
        _calibrationFactor = rawValue / knownWeight;
        _scale.set_scale(_calibrationFactor);
    }
}

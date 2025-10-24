#pragma once

const int SENSORS_COUNT = 3;    // общее количество датчиков температуры

void temp_sensors_setup();
void temperature_sensors_update();

float get_sensor_recent_temp(int sensor_ind);
bool  get_sensor_error(int sensor_ind);

void co2_sensor_setup();
void co2_sensor_update();

int get_last_co2_ppm();
bool get_co2_read_error();
int get_optimal_co2_ppm();

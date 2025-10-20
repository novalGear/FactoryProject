#pragma once

void OLED_screen_setup();

void display_update();
void clear_rect(int x, int y, int width, int height);

void display_temperature(const int sensor_ind, const float temperatureC);
void display_temp_err(const int sensor_ind, bool is_valid);
void display_CO2(const int co2, const int co2_optimal);

void handleMenu(int button_index);
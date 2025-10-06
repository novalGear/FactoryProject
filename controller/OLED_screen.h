#pragma once

void OLED_screen_setup();
void clear_rect(int x, int y, int width, int height);

void display_temperature(const float temperatureC);
void display_temp_err();

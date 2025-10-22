#pragma once

void OLED_screen_setup();

void print_screen(String strings[], unsigned int count);
void print_line(String str, unsigned int line_ind);
void display_sensors();
void display_regular_update();
void handleMenu(int button_index);

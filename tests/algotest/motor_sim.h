#pragma once

// Имитация функций управления мотором
void change_pos(int pos);
int get_current_position_index();

// Функции для симуляции
void motor_simulation_setup();
void motor_simulation_update(unsigned long current_time);
int get_target_position();
int get_current_position();
bool is_motor_moving();
float get_position_progress(); // 0.0 - 1.0

// Конфигурация симуляции
void set_motor_simulation_speed(float moves_per_second); // Скорость изменения позиции

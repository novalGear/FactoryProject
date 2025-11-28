#pragma once

const int SENSORS_COUNT = 3;
// Управление симуляцией
void set_simulation_single_run(bool single_run);
bool is_simulation_finished();
void reset_simulation();

// Глобальные переменные для управления симуляцией
extern bool simulation_finished;
extern bool single_run_mode;

// Функции для симуляции датчиков
float get_room_temp();
bool get_room_sensor_error();
float get_outside_temp();
bool get_outside_sensor_error();
int get_last_co2_ppm();
bool get_co2_read_error();

// Управление симуляцией
void setup_simulation();
void update_simulation(unsigned long current_time);
void set_simulation_data_index(int index);
int get_simulation_data_index();

// Структура для тестовых данных
struct SimulationData {
    float room_temp;
    float outside_temp;
    int co2;
    unsigned long duration_ms; // Длительность этого состояния в мс
};

extern const SimulationData test_scenario[];
extern const int test_scenario_length;

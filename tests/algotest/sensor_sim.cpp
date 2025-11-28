#include "sensor_sim.h"
#include <Arduino.h>

// Тестовый сценарий - можно расширить по необходимости
const SimulationData test_scenario[] = {
    // Начальное состояние - комфортные условия
    {22.0f, 18.0f, 450, 30000},   // 30 сек

    // CO2 растет (люди в комнате)
    {22.5f, 18.0f, 800, 60000},   // 1 минута
    {23.0f, 18.0f, 1200, 60000},  // 1 минута - высокий CO2

    // Температура растет
    {24.0f, 20.0f, 1000, 60000},  // 1 минута
    {25.0f, 22.0f, 900, 60000},   // 1 минута - жарко

    // Идеальные условия снаружи - можно проветрить
    {26.0f, 22.0f, 800, 60000},   // 1 минута
    {24.0f, 20.0f, 600, 60000},   // 1 минута - стало лучше

    // Холодно снаружи
    {22.0f, 10.0f, 500, 60000},   // 1 минута
    {21.0f, 8.0f, 600, 60000},    // 1 минута

    // Критические условия
    {28.0f, 25.0f, 1500, 30000},  // 30 сек - перегрев
    {15.0f, 5.0f, 1800, 30000},   // 30 сек - холод + высокий CO2

    // Возврат к нормальным условиям
    {22.0f, 18.0f, 500, 30000}    // 30 сек
};

const int test_scenario_length = sizeof(test_scenario) / sizeof(test_scenario[0]);

// Текущее состояние симуляции
static int current_data_index = 0;
static unsigned long scenario_start_time = 0;
static unsigned long current_scenario_duration = 0;
// Глобальные переменные для управления симуляцией
bool simulation_finished = false;
bool single_run_mode = false;
float simulation_speed = 1.0f;

// Реализации функций
void set_simulation_single_run(bool single_run) {
    single_run_mode = single_run;
    Serial.print("Single run mode: ");
    Serial.println(single_run ? "ON" : "OFF");
}

bool is_simulation_finished() {
    return simulation_finished;
}

void reset_simulation() {
    current_data_index = 0;
    simulation_finished = false;
    Serial.println("Simulation reset");
}
void setup_simulation() {
    current_data_index = 0;
    scenario_start_time = millis();
    current_scenario_duration = test_scenario[0].duration_ms;
    Serial.println("Simulation setup complete");
    Serial.print("Test scenario length: ");
    Serial.println(test_scenario_length);
}

void update_simulation(unsigned long current_time) {
    if (simulation_finished) return;

    // Мгновенно переходим к следующему состоянию при каждом вызове
    current_data_index++;

    if (current_data_index >= test_scenario_length) {
        if (single_run_mode) {
            simulation_finished = true;
            Serial.println("=== SIMULATION FINISHED ===");
            return;
        } else {
            current_data_index = 0;
        }
    }

    // Логируем каждый переход
    Serial.print("Simulation step ");
    Serial.print(current_data_index);
    Serial.print("/");
    Serial.print(test_scenario_length - 1);
    Serial.print(": Room=");
    Serial.print(test_scenario[current_data_index].room_temp, 1);
    Serial.print("°C, Outside=");
    Serial.print(test_scenario[current_data_index].outside_temp, 1);
    Serial.print("°C, CO2=");
    Serial.print(test_scenario[current_data_index].co2);
    Serial.println("ppm");
}

void set_simulation_data_index(int index) {
    if (index >= 0 && index < test_scenario_length) {
        current_data_index = index;
        scenario_start_time = millis();
        current_scenario_duration = test_scenario[current_data_index].duration_ms;
    }
}

int get_simulation_data_index() {
    return current_data_index;
}

// Реализации функций датчиков
float get_room_temp() {
    return test_scenario[current_data_index].room_temp;
}

bool get_room_sensor_error() {
    return false; // В симуляции ошибок нет
}

float get_outside_temp() {
    return test_scenario[current_data_index].outside_temp;
}

bool get_outside_sensor_error() {
    return false; // В симуляции ошибок нет
}

int get_last_co2_ppm() {
    return test_scenario[current_data_index].co2;
}

bool get_co2_read_error() {
    return false; // В симуляции ошибок нет
}

#include "sensor_sim.h"
#include <Arduino.h>

const SimulationData test_scenario[] = {
    // Сценарий 1: Постепенное накопление CO2 в закрытом помещении
    // Серия 1 - начальное состояние
    {22.0f, 18.0f, 450, 0}, {22.1f, 18.0f, 470, 0}, {22.2f, 18.1f, 490, 0},
    {22.3f, 18.1f, 510, 0}, {22.4f, 18.2f, 530, 0}, {22.5f, 18.2f, 550, 0},

    // Серия 2 - CO2 продолжает расти
    {22.6f, 18.2f, 600, 0}, {22.7f, 18.3f, 650, 0}, {22.8f, 18.3f, 700, 0},
    {22.9f, 18.4f, 750, 0}, {23.0f, 18.4f, 800, 0}, {23.1f, 18.5f, 850, 0},

    // Серия 3 - система должна открыть окно для вентиляции
    {23.2f, 18.5f, 820, 0}, {23.1f, 18.5f, 790, 0}, {23.0f, 18.6f, 760, 0},
    {22.9f, 18.6f, 730, 0}, {22.8f, 18.7f, 700, 0}, {22.7f, 18.7f, 670, 0},

    // Серия 4 - нормализация после вентиляции
    {22.6f, 18.7f, 640, 0}, {22.5f, 18.8f, 610, 0}, {22.4f, 18.8f, 580, 0},
    {22.3f, 18.9f, 550, 0}, {22.2f, 18.9f, 520, 0}, {22.1f, 19.0f, 490, 0},

    // Сценарий 2: Перегрев в солнечный день
    // Серия 5 - постепенный нагрев
    {23.0f, 25.0f, 500, 0}, {23.5f, 25.2f, 510, 0}, {24.0f, 25.4f, 520, 0},
    {24.5f, 25.6f, 530, 0}, {25.0f, 25.8f, 540, 0}, {25.5f, 26.0f, 550, 0},

    // Серия 6 - критический перегрев, снаружи прохладнее
    {26.0f, 25.8f, 560, 0}, {26.5f, 25.6f, 570, 0}, {27.0f, 25.4f, 580, 0},
    {27.5f, 25.2f, 590, 0}, {28.0f, 25.0f, 600, 0}, {28.5f, 24.8f, 610, 0},

    // Серия 7 - система открывает окно для охлаждения
    {28.0f, 24.8f, 600, 0}, {27.5f, 24.8f, 590, 0}, {27.0f, 24.9f, 580, 0},
    {26.5f, 24.9f, 570, 0}, {26.0f, 25.0f, 560, 0}, {25.5f, 25.0f, 550, 0},

    // Сценарий 3: Вечернее похолодание + активность людей
    // Серия 8 - похолодание и рост CO2
    {22.0f, 15.0f, 600, 0}, {21.5f, 14.8f, 650, 0}, {21.0f, 14.6f, 700, 0},
    {20.5f, 14.4f, 750, 0}, {20.0f, 14.2f, 800, 0}, {19.5f, 14.0f, 850, 0},

    // Серия 9 - конфликт условий: холодно но высокий CO2
    {19.0f, 13.8f, 900, 0}, {18.5f, 13.6f, 950, 0}, {18.0f, 13.4f, 1000, 0},
    {17.5f, 13.2f, 1050, 0}, {17.0f, 13.0f, 1100, 0}, {16.5f, 12.8f, 1150, 0},

    // Серия 10 - система должна найти баланс
    {16.0f, 12.6f, 1100, 0}, {16.2f, 12.6f, 1050, 0}, {16.4f, 12.7f, 1000, 0},
    {16.6f, 12.7f, 950, 0}, {16.8f, 12.8f, 900, 0}, {17.0f, 12.8f, 850, 0},

    // Сценарий 4: Резкие изменения (проверка стабильности)
    // Серия 11 - быстрые колебания
    {22.0f, 20.0f, 500, 0}, {24.0f, 20.0f, 800, 0}, {22.0f, 20.0f, 500, 0},
    {26.0f, 20.0f, 1200, 0}, {22.0f, 20.0f, 500, 0}, {28.0f, 20.0f, 1500, 0},

    // Серия 12 - стабилизация
    {26.0f, 20.0f, 1200, 0}, {24.0f, 20.0f, 900, 0}, {22.0f, 20.0f, 600, 0},
    {22.0f, 20.0f, 550, 0}, {22.0f, 20.0f, 500, 0}, {22.0f, 20.0f, 480, 0},

    // Сценарий 5: Идеальные условия (проверка что система не "дергается")
    // Серия 13 - стабильные хорошие условия
    {22.0f, 20.0f, 450, 0}, {22.1f, 20.1f, 460, 0}, {21.9f, 20.0f, 470, 0},
    {22.0f, 20.2f, 460, 0}, {22.1f, 20.1f, 450, 0}, {21.9f, 20.0f, 440, 0},

    // Серия 14 - продолжение идеальных условий
    {22.0f, 20.1f, 430, 0}, {22.1f, 20.2f, 420, 0}, {21.9f, 20.1f, 410, 0},
    {22.0f, 20.0f, 400, 0}, {22.1f, 20.1f, 390, 0}, {21.9f, 20.2f, 380, 0}
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
    // Serial.print("Simulation step ");
    // Serial.print(current_data_index);
    // Serial.print("/");
    // Serial.print(test_scenario_length - 1);
    // Serial.print(": Room=");
    // Serial.print(test_scenario[current_data_index].room_temp, 1);
    // Serial.print("°C, Outside=");
    // Serial.print(test_scenario[current_data_index].outside_temp, 1);
    // Serial.print("°C, CO2=");
    // Serial.print(test_scenario[current_data_index].co2);
    // Serial.print("ppm  ");
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

#include "motor_sim.h"
#include <Arduino.h>

// Состояние симуляции мотора
static int current_position = 0;
static int target_position = 0;
static unsigned long move_start_time = 0;
static unsigned long move_duration = 2000; // 2 секунды на перемещение по умолчанию
static bool is_moving = false;

// Конфигурация
static float simulation_speed = 0.5f; // позиций в секунду

void motor_simulation_setup() {
    current_position = 0;
    target_position = 0;
    is_moving = false;
    Serial.println("Motor simulation setup complete");
}

void motor_simulation_update(unsigned long current_time) {
    if (!is_moving) return;

    // Мгновенное перемещение
    current_position = target_position;
    is_moving = false;

    Serial.print("Motor instantly moved to position: ");
    Serial.println(current_position);
}

void change_pos(int pos) {
    if (pos < 0 || pos > 9) {
        Serial.print("Invalid position: ");
        Serial.println(pos);
        return;
    }

    if (pos == current_position) {
        Serial.print("Already at position: ");
        Serial.println(pos);
        return;
    }

    target_position = pos;
    is_moving = true;

    Serial.print("Motor moving from ");
    Serial.print(current_position);
    Serial.print(" to ");
    Serial.println(target_position);

    // Сразу обновляем позицию
    motor_simulation_update(millis());
}

int get_current_position_index() {
    return current_position;
}

int get_target_position() {
    return target_position;
}

int get_current_position() {
    return current_position;
}

bool is_motor_moving() {
    return is_moving;
}

float get_position_progress() {
    if (!is_moving) return 1.0f;

    unsigned long elapsed = millis() - move_start_time;
    float progress = (float)elapsed / move_duration;
    return constrain(progress, 0.0f, 1.0f);
}

void set_motor_simulation_speed(float moves_per_second) {
    simulation_speed = moves_per_second;
    Serial.print("Motor simulation speed set to: ");
    Serial.print(simulation_speed);
    Serial.println(" moves/sec");
}

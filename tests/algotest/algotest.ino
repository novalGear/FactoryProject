#include "window_controller.h"
#include "sensor_sim.h"
#include "motor_sim.h"

WindowController windowController;
void setup() {
    Serial.begin(115200);
    Serial.println("=== INSTANT Window Controller Simulation ===");

    // Настройка мгновенной симуляции
    set_simulation_single_run(true);
    setup_simulation();
    motor_simulation_setup();

    // Настройка контроллера
    WindowConfig config;
    config.tempIdeal = 22.0f;
    config.tempWeightMultiplier = 2.0f;
    config.co2Ideal = 600;
    config.co2WeightDivisor = 20.0f;
    config.tempWeight = 0.7f;
    config.co2Weight = 0.3f;
    config.metricTarget = 20.0f;
    config.metricMargin = 10.0f;

    windowController.setConfig(config);
    windowController.setMode(WindowMode::AUTO);

    Serial.println("Instant simulation ready. Running full scenario...");
    Serial.println("=================================================");
}

void loop() {
    if (is_simulation_finished()) {
        // Симуляция завершена
        static bool finished_reported = false;
        if (!finished_reported) {
            Serial.println("=== FULL SIMULATION COMPLETED ===");
            Serial.println("All test scenarios executed.");
            finished_reported = true;
        }
        return;
    }

    // Один вызов = один шаг симуляции
    update_simulation(millis());
    motor_simulation_update(millis());
    windowController.update();

    // Короткая пауза для читаемости логов
    delay(100);
}

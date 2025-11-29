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

    windowController.setConfig(config);
    windowController.setMode(WindowMode::AUTO);

    Serial.println("Instant simulation ready. Running full scenario...");
    Serial.println("=================================================");
}

void loop() {
    if (is_simulation_finished()) {
        static bool finished_reported = false;
        if (!finished_reported) {
            Serial.println("=== FULL TEST COMPLETED ===");
            Serial.println("Total steps: " + String(test_scenario_length));
            Serial.println("Check if system:");
            Serial.println("1. Reacted to high CO2 by opening window");
            Serial.println("2. Reacted to overheating when outside is cooler");
            Serial.println("3. Found balance between temperature and CO2");
            Serial.println("4. Remained stable during ideal conditions");
            Serial.println("5. Handled rapid changes without oscillations");
            finished_reported = true;
        }
        return;
    }

    // Компактный вывод с группировкой по сериям
    int currentStep = get_simulation_data_index();
    
    Serial.println();
    Serial.print("Step");
    Serial.print(currentStep % 6);
    Serial.print(": ");

    // Один вызов = один шаг симуляции
    update_simulation(millis());
    motor_simulation_update(millis());
    windowController.tick();
    
    RecentData data = windowController.getRecentData();

    // Выводим заголовок для каждой новой серии из 6 значений
    if (currentStep % 6 == 0) {
        Serial.println();
        Serial.print("=== SERIES ");
        Serial.print(currentStep / 6 + 1);
        Serial.println(" ===");
    }

    // Компактный вывод для каждого шага

    // Показываем решение если было движение
    static int lastPosition = -1;
    if (data.windowPosition != lastPosition) {
        Serial.print(" -> MOVED to ");
        Serial.print(data.windowPosition);
        lastPosition = data.windowPosition;
    }
    Serial.println();

    delay(30); // Еще быстрее для больших тестов
}

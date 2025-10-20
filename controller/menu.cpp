#include <Arduino.h>
#include <array>

// --- Enum для состояний (последовательные значения) ---
enum MenuState {
    PAGE_PREVIEW,

    PAGE_MENU_DEFAULT,

    PAGE_MENU_MODE,
    PAGE_MENU_MODE_DEFAULT,
    PAGE_MENU_MODE_MANUAL,
    PAGE_MENU_MODE_ENERGY_SAVING,

    PAGE_MENU_PARAMS,
    PAGE_MENU_PARAMS_SET_TEMP,
    PAGE_MENU_PARAMS_SET_CO2,
    PAGE_MENU_PARAMS_SET_POS,

    PAGE_MENU_PARAMS_SET_POS_MIN,
    PAGE_MENU_PARAMS_SET_POS_MAX,

    MENU_STATE_COUNT
};

// --- Кнопки как int (0, 1, 2, 3) ---
const int NUM_BUTTONS = 4;

// --- Функции-обработчики обновления дисплея ---
void preview_upd() { Serial.println("Updating Preview Page..."); }
void menu_dflt_upd() { Serial.println("Updating Default Menu Page..."); }
void show_mode_selection() { Serial.println("Showing Mode Selection Screen..."); }
void set_mode_default_upd() { Serial.println("Showing Mode: Default Screen..."); }
void set_mode_manual_upd() { Serial.println("Showing Mode: Manual Screen..."); }
void set_mode_energy_saving_upd() { Serial.println("Showing Mode: Energy Saving Screen..."); }
void show_params_list() { Serial.println("Showing Params List Screen..."); } // Обновление для PAGE_MENU_PARAMS
void set_temp_upd() { Serial.println("Showing Temperature Settings Screen..."); }
void set_co2_upd() { Serial.println("Showing CO2 Settings Screen..."); }
void set_pos_upd() { Serial.println("Showing Position Settings Screen..."); }
void set_pos_min_upd() { Serial.println("Showing Position Min Settings Screen..."); }
void set_pos_max_upd() { Serial.println("Showing Position Max Settings Screen..."); }

// --- Функции-обработчики второстепенной логики (например, изменение значений) ---
// Принимают номер нажатой кнопки.
void preview_actions(int buttonIndex) { Serial.print("Preview Actions - Button: "); Serial.println(buttonIndex); }
void menu_dflt_actions(int buttonIndex) { Serial.print("Default Menu Actions - Button: "); Serial.println(buttonIndex); }
void mode_selection_actions(int buttonIndex) { Serial.print("Mode Selection Actions - Button: "); Serial.println(buttonIndex); }
void mode_default_actions(int buttonIndex) { Serial.print("Mode Default Actions - Button: "); Serial.println(buttonIndex); }
void mode_manual_actions(int buttonIndex) { Serial.print("Mode Manual Actions - Button: "); Serial.println(buttonIndex); }
void mode_energy_saving_actions(int buttonIndex) { Serial.print("Mode Energy Saving Actions - Button: "); Serial.println(buttonIndex); }
void params_list_actions(int buttonIndex) { Serial.print("Params List Actions - Button: "); Serial.println(buttonIndex); } // Действия для PAGE_MENU_PARAMS
void temp_actions(int buttonIndex) { Serial.print("Temp Actions (e.g., adjust value) - Button: "); Serial.println(buttonIndex); }
void co2_actions(int buttonIndex) { Serial.print("CO2 Actions (e.g., adjust value) - Button: "); Serial.println(buttonIndex); }
void pos_actions(int buttonIndex) { Serial.print("Pos Actions (e.g., adjust value) - Button: "); Serial.println(buttonIndex); }
void pos_min_actions(int buttonIndex) { Serial.print("Pos Min Actions (e.g., adjust value) - Button: "); Serial.println(buttonIndex); }
void pos_max_actions(int buttonIndex) { Serial.print("Pos Max Actions (e.g., adjust value) - Button: "); Serial.println(buttonIndex); }

// --- Типы для указателей на функции ---
using UpdateFunction = void(*)();
using ActionFunction = void(*)(int); // Функция, принимающая int

// --- 2D Массив сопоставления: [состояние][кнопка] -> новое_состояние ---
const MenuState NO_TRANSITION = static_cast<MenuState>(-1);
std::array<std::array<MenuState, NUM_BUTTONS>, MENU_STATE_COUNT> directNavigationArray;

// --- Массив функций обновления дисплея ---
std::array<UpdateFunction, MENU_STATE_COUNT> displayUpdateArray;

// --- Массив функций второстепенной обработки ---
std::array<ActionFunction, MENU_STATE_COUNT> secondaryActionArray;

void directNavigationArray_init() {
    directNavigationArray[PAGE_PREVIEW][0] = PAGE_MENU_DEFAULT;
    directNavigationArray[PAGE_PREVIEW][1] = PAGE_MENU_DEFAULT;
    directNavigationArray[PAGE_PREVIEW][2] = PAGE_MENU_DEFAULT;
    directNavigationArray[PAGE_PREVIEW][3] = PAGE_MENU_DEFAULT;

    // PAGE_MENU_DEFAULT [1]
    directNavigationArray[PAGE_MENU_DEFAULT][0] = PAGE_PREVIEW;                                 // Кнопка 0 -> Preview
    directNavigationArray[PAGE_MENU_DEFAULT][1] = PAGE_MENU_MODE;                               // Кнопка 1 -> Mode Selection
    directNavigationArray[PAGE_MENU_DEFAULT][2] = PAGE_MENU_PARAMS;                             // Кнопка 2 -> Params/Param1
    directNavigationArray[PAGE_MENU_DEFAULT][3] = PAGE_MENU_DEFAULT;                            // Кнопка 3 -> Stay/Oops

    // PAGE_MENU_MODE (Экран выбора) [2]
    directNavigationArray[PAGE_MENU_MODE][0] = PAGE_MENU_DEFAULT;                               // Кнопка 0 -> Mode Default & Back to Default
    directNavigationArray[PAGE_MENU_MODE][1] = PAGE_MENU_DEFAULT;                               // Кнопка 1 -> Mode Manual & Back to Default
    directNavigationArray[PAGE_MENU_MODE][2] = PAGE_MENU_DEFAULT;                               // Кнопка 2 -> Mode Energy Saving & Back to Default
    directNavigationArray[PAGE_MENU_MODE][3] = PAGE_MENU_DEFAULT;                               // Кнопка 3 -> Back to Default

    // PAGE_MENU_PARAMS_SET_PARAM1 [6]
    directNavigationArray[PAGE_MENU_PARAMS][0] = PAGE_MENU_PARAMS_SET_TEMP;                     // Кнопка 0 -> Param2
    directNavigationArray[PAGE_MENU_PARAMS][1] = PAGE_MENU_PARAMS_SET_CO2;                      // Кнопка 1 -> Param3
    directNavigationArray[PAGE_MENU_PARAMS][2] = PAGE_MENU_PARAMS_SET_POS;                      // Кнопка 2 -> Param4
    directNavigationArray[PAGE_MENU_PARAMS][3] = PAGE_MENU_DEFAULT;                             // Кнопка 3 -> Back to Default

    // PAGE_MENU_PARAMS_SET_TEMP [7]
    directNavigationArray[PAGE_MENU_PARAMS_SET_TEMP][0] = PAGE_MENU_PARAMS_SET_TEMP;            // Кнопка 0 -> inc 0.5 C
    directNavigationArray[PAGE_MENU_PARAMS_SET_TEMP][1] = PAGE_MENU_PARAMS_SET_TEMP;            // Кнопка 1 -> dec 0.5 C
    directNavigationArray[PAGE_MENU_PARAMS_SET_TEMP][2] = PAGE_MENU_PARAMS;                     // Кнопка 2 -> OK
    directNavigationArray[PAGE_MENU_PARAMS_SET_TEMP][3] = PAGE_MENU_PARAMS;                     // Кнопка 3 -> cancel

    // PAGE_MENU_PARAMS_SET_CO2 [8]
    directNavigationArray[PAGE_MENU_PARAMS_SET_CO2][0] = PAGE_MENU_PARAMS_SET_CO2;              // Кнопка 0 -> inc 20 ppm
    directNavigationArray[PAGE_MENU_PARAMS_SET_CO2][1] = PAGE_MENU_PARAMS_SET_CO2;              // Кнопка 1 -> dec 20 ppm
    directNavigationArray[PAGE_MENU_PARAMS_SET_CO2][2] = PAGE_MENU_PARAMS;                      // Кнопка 2 -> OK
    directNavigationArray[PAGE_MENU_PARAMS_SET_CO2][3] = PAGE_MENU_PARAMS;                      // Кнопка 3 -> cancel

    // PAGE_MENU_PARAMS_SET_POS [9]
    directNavigationArray[PAGE_MENU_PARAMS_SET_POS][0] = PAGE_MENU_PARAMS_SET_POS_MIN;          // Кнопка 0 -> set min pos
    directNavigationArray[PAGE_MENU_PARAMS_SET_POS][1] = PAGE_MENU_PARAMS_SET_POS_MAX;          // Кнопка 1 -> set max pos
    directNavigationArray[PAGE_MENU_PARAMS_SET_POS][2] = PAGE_MENU_PARAMS;                      // Кнопка 2 -> Back to params
    directNavigationArray[PAGE_MENU_PARAMS_SET_POS][3] = PAGE_MENU_PARAMS;                      // Кнопка 3 -> Back to params

    directNavigationArray[PAGE_MENU_PARAMS_SET_POS_MIN][0] = PAGE_MENU_PARAMS_SET_POS_MIN;      // Кнопка 0 -> inc a
    directNavigationArray[PAGE_MENU_PARAMS_SET_POS_MIN][1] = PAGE_MENU_PARAMS_SET_POS_MIN;      // Кнопка 1 -> dec a
    directNavigationArray[PAGE_MENU_PARAMS_SET_POS_MIN][2] = PAGE_MENU_PARAMS_SET_POS;          // Кнопка 2 -> OK
    directNavigationArray[PAGE_MENU_PARAMS_SET_POS_MIN][3] = PAGE_MENU_PARAMS_SET_POS;          // Кнопка 3 -> cancel

    directNavigationArray[PAGE_MENU_PARAMS_SET_POS_MAX][0] = PAGE_MENU_PARAMS_SET_POS_MAX;      // Кнопка 0 -> inc a
    directNavigationArray[PAGE_MENU_PARAMS_SET_POS_MAX][1] = PAGE_MENU_PARAMS_SET_POS_MAX;      // Кнопка 1 -> dec a
    directNavigationArray[PAGE_MENU_PARAMS_SET_POS_MAX][2] = PAGE_MENU_PARAMS_SET_POS;          // Кнопка 2 -> OK
    directNavigationArray[PAGE_MENU_PARAMS_SET_POS_MAX][3] = PAGE_MENU_PARAMS_SET_POS;          // Кнопка 3 -> cancel


}

// --- Функция для инициализации массива функций обновления дисплея ---
void initializeDisplayUpdateArray() {
    displayUpdateArray[PAGE_PREVIEW] = preview_upd;
    displayUpdateArray[PAGE_MENU_DEFAULT] = menu_dflt_upd;
    displayUpdateArray[PAGE_MENU_MODE] = show_mode_selection;
    displayUpdateArray[PAGE_MENU_MODE_DEFAULT] = set_mode_default_upd;
    displayUpdateArray[PAGE_MENU_MODE_MANUAL] = set_mode_manual_upd;
    displayUpdateArray[PAGE_MENU_MODE_ENERGY_SAVING] = set_mode_energy_saving_upd;
    displayUpdateArray[PAGE_MENU_PARAMS] = show_params_list; // Обновление для PAGE_MENU_PARAMS
    displayUpdateArray[PAGE_MENU_PARAMS_SET_TEMP] = set_temp_upd;
    displayUpdateArray[PAGE_MENU_PARAMS_SET_CO2] = set_co2_upd;
    displayUpdateArray[PAGE_MENU_PARAMS_SET_POS] = set_pos_upd;
    displayUpdateArray[PAGE_MENU_PARAMS_SET_POS_MIN] = set_pos_min_upd;
    displayUpdateArray[PAGE_MENU_PARAMS_SET_POS_MAX] = set_pos_max_upd;
}

// --- Функция для инициализации массива функций второстепенной обработки ---
void initializeSecondaryActionArray() {
    secondaryActionArray[PAGE_PREVIEW] = preview_actions;
    secondaryActionArray[PAGE_MENU_DEFAULT] = menu_dflt_actions;
    secondaryActionArray[PAGE_MENU_MODE] = mode_selection_actions;
    secondaryActionArray[PAGE_MENU_MODE_DEFAULT] = mode_default_actions;
    secondaryActionArray[PAGE_MENU_MODE_MANUAL] = mode_manual_actions;
    secondaryActionArray[PAGE_MENU_MODE_ENERGY_SAVING] = mode_energy_saving_actions;
    secondaryActionArray[PAGE_MENU_PARAMS] = params_list_actions; // Действия для PAGE_MENU_PARAMS
    secondaryActionArray[PAGE_MENU_PARAMS_SET_TEMP] = temp_actions;
    secondaryActionArray[PAGE_MENU_PARAMS_SET_CO2] = co2_actions;
    secondaryActionArray[PAGE_MENU_PARAMS_SET_POS] = pos_actions;
    secondaryActionArray[PAGE_MENU_PARAMS_SET_POS_MIN] = pos_min_actions;
    secondaryActionArray[PAGE_MENU_PARAMS_SET_POS_MAX] = pos_max_actions;
}

// --- Глобальное состояние ---
MenuState currentState = PAGE_MENU_DEFAULT;

// --- Функция обработки нажатия кнопки ---
void processButtonPress(int buttonIndex) {
    if (buttonIndex < 0 || buttonIndex >= NUM_BUTTONS) {
        Serial.println("Invalid button index!");
        return;
    }

    int stateIdx = static_cast<int>(currentState);
    if (stateIdx < 0 || stateIdx >= MENU_STATE_COUNT) {
        Serial.println("Invalid current state!");
        return;
    }

    MenuState newState = directNavigationArray[stateIdx][buttonIndex];

    if (newState != NO_TRANSITION) {
        if (newState != currentState) {
            currentState = newState;
            Serial.print("State changed to: ");
            Serial.println(static_cast<int>(currentState));
        }
        // Обновление дисплея произойдёт в updateDisplay
    } else {
        Serial.print("No mapping defined for button ");
        Serial.print(buttonIndex);
        Serial.print(" in state ");
        Serial.println(static_cast<int>(currentState));
    }
}

// --- Функция обновления отображения в зависимости от состояния ---
void updateDisplay() {
    int stateIdx = static_cast<int>(currentState);
    if (stateIdx >= 0 && stateIdx < MENU_STATE_COUNT) {
        displayUpdateArray[stateIdx](); // Вызываем функцию обновления для текущего состояния
    } else {
        Serial.println("Warning: Invalid state for display update!");
    }
}

void loop() {
    // Здесь вы будете получать реальные события кнопок
    // и вызывать processButtonPress с номером нажатой кнопки (0-3)
    // Пример симуляции:
    static unsigned long lastSimTime = millis();
    static int simStep = 0;
    if (millis() - lastSimTime > 4000) { // Каждые 4 секунды
        Serial.println("\n--- Simulation Step ---");
        // Имитируем нажатие кнопок в зависимости от состояния
        int simulatedButton = -1;
        switch (currentState) {
            case PAGE_MENU_DEFAULT:
                simulatedButton = 1; // В меню MODE
                break;
            case PAGE_MENU_MODE:
                simulatedButton = 0; // Выбрать MODE_DEFAULT
                break;
            case PAGE_MENU_MODE_DEFAULT:
            case PAGE_MENU_MODE_MANUAL:
            case PAGE_MENU_MODE_ENERGY_SAVING:
                simulatedButton = 3; // Назад к DEFAULT
                break;
            case PAGE_MENU_PARAMS_SET_PARAM1:
                simulatedButton = 0; // К следующему параметру
                break;
            case PAGE_MENU_PARAMS_SET_PARAM2:
                simulatedButton = 1; // К следующему параметру
                break;
            case PAGE_MENU_PARAMS_SET_PARAM3:
                simulatedButton = 2; // К следующему параметру
                break;
            case PAGE_MENU_PARAMS_SET_PARAM4:
                simulatedButton = 3; // Назад
                break;
            default:
                simulatedButton = 3; // Назад
        }
        Serial.print("Simulating button press: ");
        Serial.println(simulatedButton);
        processButtonPress(simulatedButton);
        updateDisplay(); // Обновляем дисплей после перехода
        simStep++;
        lastSimTime = millis();
    }

    delay(100); // Имитация работы loop
}

void preview_upd() {
    display_temperature();
    display_CO2();
}

void menu_upd() {
    switch (menu_ctx.option) {
        case OPT_NONE:      handle
        case OPT_MODE:
        case OPT_PARAMS:
    }
}

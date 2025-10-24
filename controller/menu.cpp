#include <Arduino.h>
#include <array>
#include "OLED_screen.h"
// #include "metric_control.h"

#define DBG_PRINT() Serial.println(String(__FILE__) + ":" + String(__LINE__) + " (" + String(__PRETTY_FUNCTION__) + ")")

const float TARGET_TEMP_INIT = 23.0;
const float TARGET_TEMP_STEP = 0.5;
const int TARGET_CO2_INIT  = 800; // ppm
const int TARGET_CO2_STEP  = 20;  // ppm

const float MIN_POS_INIT = 1.0;
const float MAX_POS_INIT = 10.0;
const float POS_STEP     = 0.2;

enum MenuState {
    PAGE_PREVIEW,

    PAGE_MENU_DEFAULT,

    PAGE_MENU_MODE,
    // PAGE_MENU_MODE_DEFAULT,
    // PAGE_MENU_MODE_MANUAL,
    // PAGE_MENU_MODE_ENERGY_SAVING,

    PAGE_MENU_PARAMS,
    PAGE_MENU_PARAMS_SET_TEMP,
    PAGE_MENU_PARAMS_SET_CO2,
    PAGE_MENU_PARAMS_SET_POS,

    PAGE_MENU_PARAMS_SET_POS_MIN,
    PAGE_MENU_PARAMS_SET_POS_MAX,

    MENU_STATE_COUNT
};

enum MODE {
  MODE_DEFAULT        = 0,
  MODE_MANUAL         = 1,
  MODE_ENERGY_SAVING  = 2
};

struct menu_context {
    MenuState   state;
    MODE        mode;
    float       temp_target;
    int         co2_target_ppm;
    float       min_pos;
    float       max_pos;
};

struct menu_context menu_ctx = {
    .state  = PAGE_PREVIEW,
    .mode   = MODE_DEFAULT,
    .temp_target    = TARGET_TEMP_INIT,
    .co2_target_ppm = TARGET_CO2_INIT,
    .min_pos = MIN_POS_INIT,
    .max_pos = MAX_POS_INIT
};

const int NUM_BUTTONS = 4;

// forward declaration функций графики и второстепенных обработчиков

void preview_upd();
void menu_dflt_upd();
void show_mode_selection();
void show_params_list();
void set_pos_upd();
void set_temp_upd();
void set_co2_upd();
void set_pos_min_upd();
void set_pos_max_upd();

// --- Функции-обработчики второстепенной логики (например, изменение значений) ---
// Принимают номер нажатой кнопки.
void preview_actions(int button_index) { Serial.print("Preview Actions - Button: "); Serial.println(button_index); }
void menu_dflt_actions(int button_index) { Serial.print("Default Menu Actions - Button: "); Serial.println(button_index); }
void mode_selection_actions(int button_index);
void params_list_actions(int button_index) { Serial.print("Params List Actions - Button: "); Serial.println(button_index); } // Действия для PAGE_MENU_PARAMS
void temp_actions(int button_index);
void co2_actions(int button_index);
void pos_actions(int button_index) { Serial.print("Pos Actions (e.g., adjust value) - Button: "); Serial.println(button_index); }
void pos_min_actions(int button_index);
void pos_max_actions(int button_index);

using UpdateFunction = void(*)();
using ActionFunction = void(*)(int);

// --- 2D Массив сопоставления: [состояние][кнопка] -> новое_состояние ---
const MenuState NO_TRANSITION = static_cast<MenuState>(-1);
std::array<std::array<MenuState, NUM_BUTTONS>, MENU_STATE_COUNT> directNavigationArray;

// --- Массив функций обновления дисплея ---
std::array<UpdateFunction, MENU_STATE_COUNT> displayUpdateArray;

// --- Массив функций второстепенной обработки ---
std::array<ActionFunction, MENU_STATE_COUNT> secondaryActionArray;

void navigation_init() {
    DBG_PRINT();
    directNavigationArray[PAGE_PREVIEW][0] = PAGE_MENU_DEFAULT;
    directNavigationArray[PAGE_PREVIEW][1] = PAGE_MENU_DEFAULT;
    directNavigationArray[PAGE_PREVIEW][2] = PAGE_MENU_DEFAULT;
    directNavigationArray[PAGE_PREVIEW][3] = PAGE_MENU_DEFAULT;

    // PAGE_MENU_DEFAULT [1]
    directNavigationArray[PAGE_MENU_DEFAULT][0] = PAGE_MENU_MODE;                               // Кнопка 0 -> mode selection
    directNavigationArray[PAGE_MENU_DEFAULT][1] = PAGE_MENU_PARAMS;                             // Кнопка 1 -> params setting
    directNavigationArray[PAGE_MENU_DEFAULT][2] = PAGE_PREVIEW;                                 // Кнопка 2 -> back to preview
    directNavigationArray[PAGE_MENU_DEFAULT][3] = PAGE_PREVIEW;                                 // Кнопка 3 -> back to preview

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

void initializeDisplayUpdateArray() {
    DBG_PRINT();

    displayUpdateArray[PAGE_PREVIEW] = preview_upd;

    displayUpdateArray[PAGE_MENU_DEFAULT] = menu_dflt_upd;
    displayUpdateArray[PAGE_MENU_MODE] = show_mode_selection;
    displayUpdateArray[PAGE_MENU_PARAMS] = show_params_list;

    displayUpdateArray[PAGE_MENU_PARAMS_SET_TEMP] = set_temp_upd;
    displayUpdateArray[PAGE_MENU_PARAMS_SET_CO2] = set_co2_upd;
    displayUpdateArray[PAGE_MENU_PARAMS_SET_POS] = set_pos_upd;
    displayUpdateArray[PAGE_MENU_PARAMS_SET_POS_MIN] = set_pos_min_upd;
    displayUpdateArray[PAGE_MENU_PARAMS_SET_POS_MAX] = set_pos_max_upd;
}

void initializeSecondaryActionArray() {
    DBG_PRINT();
    secondaryActionArray[PAGE_PREVIEW] = preview_actions;

    secondaryActionArray[PAGE_MENU_DEFAULT] = menu_dflt_actions;
    secondaryActionArray[PAGE_MENU_MODE] = mode_selection_actions;
    secondaryActionArray[PAGE_MENU_PARAMS] = params_list_actions;

    secondaryActionArray[PAGE_MENU_PARAMS_SET_TEMP] = temp_actions;
    secondaryActionArray[PAGE_MENU_PARAMS_SET_CO2] = co2_actions;
    secondaryActionArray[PAGE_MENU_PARAMS_SET_POS] = pos_actions;
    secondaryActionArray[PAGE_MENU_PARAMS_SET_POS_MIN] = pos_min_actions;
    secondaryActionArray[PAGE_MENU_PARAMS_SET_POS_MAX] = pos_max_actions;
}

void updateDisplay() {
    DBG_PRINT();
    int stateIdx = static_cast<int>(menu_ctx.state);
    if (stateIdx >= 0 && stateIdx < MENU_STATE_COUNT) {
        displayUpdateArray[stateIdx]();
    } else {
        Serial.println("Warning: Invalid state for display update!");
    }
}

void processButtonPress(int buttonIndex) {
    DBG_PRINT();
    if (buttonIndex < 0 || buttonIndex >= NUM_BUTTONS) {
        Serial.println("Invalid button index!");
        return;
    }

    int stateIdx = static_cast<int>(menu_ctx.state);
    if (stateIdx < 0 || stateIdx >= MENU_STATE_COUNT) {
        Serial.println("Invalid current state!");
        return;
    }

    secondaryActionArray[menu_ctx.state](buttonIndex);

    MenuState newState = directNavigationArray[stateIdx][buttonIndex];
    if (newState != NO_TRANSITION) {
        if (newState != menu_ctx.state) {
            menu_ctx.state = newState;
            Serial.print("State changed to: ");
            Serial.println(static_cast<int>(menu_ctx.state));
        }

    } else {
        Serial.print("No mapping defined for button ");
        Serial.print(buttonIndex);
        Serial.print(" in state ");
        Serial.println(static_cast<int>(menu_ctx.state));
    }

    updateDisplay();
}

void menu_setup() {
    DBG_PRINT();
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting menu system with secondary actions...");
    navigation_init();
    initializeDisplayUpdateArray();
    initializeSecondaryActionArray();
    updateDisplay();

    Serial.println("Menu setup completed");
}

// графика для меню ===========================================================================================================//

void preview_upd() {
    DBG_PRINT();

    display_sensors();
    print_line("1-4: menu", 4);
}

String menu_dflt_text[] = {
    "1: mode",
    "2: params",
    "3: leave"
};

String mode_selection_text[] = {
    "1: default",
    "2: manual",
    "3: energy-saving",
    "4: leave"
};

String params_list_text[] = {
    "1: temp",
    "2: co2",
    "3: motor",
    "4: leave",
};

String pos_upd_text[] = {
    "1: min position",
    "2: max position",
    "3: leave"
};

void menu_dflt_upd()        { DBG_PRINT(); print_screen(menu_dflt_text,      3); }
void show_mode_selection()  { DBG_PRINT(); print_screen(mode_selection_text, 4); }
void show_params_list()     { DBG_PRINT(); print_screen(params_list_text,    4); }
void set_pos_upd()          { DBG_PRINT(); print_screen(pos_upd_text,        3); }

template<typename T>
void print_upd_param(T param, T param_step, String comment) {
    DBG_PRINT();

    String upd_param_text[] = {
        "  " + String(param) + " " + comment,
        "1: inc " + String(param_step),
        "2: dec " + String(param_step),
        "3: OK",
        "4: cancel"
    };
    print_screen(upd_param_text, 5);
}

void set_temp_upd()     { DBG_PRINT(); print_upd_param(menu_ctx.temp_target,     TARGET_TEMP_STEP,   "C"); }
void set_co2_upd()      { DBG_PRINT(); print_upd_param(menu_ctx.co2_target_ppm,  TARGET_CO2_STEP,    "ppm"); }
void set_pos_min_upd()  { DBG_PRINT(); print_upd_param(menu_ctx.min_pos,         POS_STEP,           "?"); }
void set_pos_max_upd()  { DBG_PRINT(); print_upd_param(menu_ctx.max_pos,         POS_STEP,           "?"); }

// действия при нажатии на кнопку в меню ======================================================================================//

void mode_selection_actions(int button_index) {
    DBG_PRINT();

    Serial.print("Mode Selection Actions - Button: "); Serial.println(button_index);
    if (button_index < 0 || button_index >= NUM_BUTTONS) {
        Serial.println("Invalid button index!");
        return;
    }
    menu_ctx.mode = (MODE)button_index;
}

void temp_actions(int button_index) {
    DBG_PRINT();

    Serial.print("Temp Actions (e.g., adjust value) - Button: "); Serial.println(button_index);
    if (button_index < 0 || button_index >= NUM_BUTTONS) {
        Serial.println("Invalid button index!");
        return;
    }

    static float prev_target_temp = TARGET_TEMP_INIT;

    switch(button_index) {
        case 0: menu_ctx.temp_target += TARGET_TEMP_STEP;   break;
        case 1: menu_ctx.temp_target -= TARGET_TEMP_STEP;   break;
        case 2: prev_target_temp = menu_ctx.temp_target;    break;
        case 3: menu_ctx.temp_target = prev_target_temp;    break;
        default: assert(0);
    }
}

void co2_actions(int button_index) {
    DBG_PRINT();

    Serial.print("CO2 Actions (e.g., adjust value) - Button: "); Serial.println(button_index);
    if (button_index < 0 || button_index >= NUM_BUTTONS) {
        Serial.println("Invalid button index!");
        return;
    }

    static int prev_target_co2 = TARGET_CO2_INIT;

    switch(button_index) {
        case 0: menu_ctx.co2_target_ppm += TARGET_CO2_STEP;   break;
        case 1: menu_ctx.co2_target_ppm -= TARGET_CO2_STEP;   break;
        case 2: prev_target_co2 = menu_ctx.co2_target_ppm;    break;
        case 3: menu_ctx.co2_target_ppm = prev_target_co2;    break;
        default: assert(0);
    }
}

void pos_min_actions(int button_index) {
    DBG_PRINT();

    Serial.print("Pos Min Actions (e.g., adjust value) - Button: "); Serial.println(button_index);
    if (button_index < 0 || button_index >= NUM_BUTTONS) {
        Serial.println("Invalid button index!");
        return;
    }

    static float prev_min_pos = MIN_POS_INIT;

    switch(button_index) {
        case 0: menu_ctx.min_pos += POS_STEP;       break;
        case 1: menu_ctx.min_pos -= POS_STEP;       break;
        case 2: prev_min_pos = menu_ctx.min_pos;    break;
        case 3: menu_ctx.min_pos = prev_min_pos;    break;
        default: assert(0);
    }
}

void pos_max_actions(int button_index) {
    DBG_PRINT();

    Serial.print("Pos Max Actions (e.g., adjust value) - Button: "); Serial.println(button_index);
    if (button_index < 0 || button_index >= NUM_BUTTONS) {
        Serial.println("Invalid button index!");
        return;
    }

    static float prev_max_pos = MAX_POS_INIT;

    switch(button_index) {
        case 0: menu_ctx.max_pos += POS_STEP;       break;
        case 1: menu_ctx.max_pos -= POS_STEP;       break;
        case 2: prev_max_pos = menu_ctx.max_pos;    break;
        case 3: menu_ctx.max_pos = prev_max_pos;    break;
        default: assert(0);
    }
}



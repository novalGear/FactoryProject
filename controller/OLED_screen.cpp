#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "sensors.h"

#define DBG_PRINT() Serial.println(String(__FILE__) + ":" + String(__LINE__) + " (" + String(__PRETTY_FUNCTION__) + ")")

extern const int SENSORS_COUNT;

struct rect {
    int x;
    int y;
    int width;
    int height;
};

const int DIGIT_WIDTH  = 6;
const int DIGIT_HEIGHT = 8;

const int SCREEN_WIDTH  = 128;
const int SCREEN_HEIGHT = 64;

const unsigned long DISPLAY_UPD_PERIOD_MS = 100;

const unsigned int STRINGS_IN_SCREEN = 5;

struct rect screen[] = {
    {.x = 0, .y = DIGIT_HEIGHT * 0, .width = SCREEN_WIDTH, .height = SCREEN_HEIGHT},
    {.x = 0, .y = DIGIT_HEIGHT * 1, .width = SCREEN_WIDTH, .height = SCREEN_HEIGHT},
    {.x = 0, .y = DIGIT_HEIGHT * 2, .width = SCREEN_WIDTH, .height = SCREEN_HEIGHT},
    {.x = 0, .y = DIGIT_HEIGHT * 3, .width = SCREEN_WIDTH, .height = SCREEN_HEIGHT},
    {.x = 0, .y = DIGIT_HEIGHT * 4, .width = SCREEN_WIDTH, .height = SCREEN_HEIGHT}
};

const struct rect TEMP_DATA[] = {
    {0, DIGIT_HEIGHT * 0, 8 * DIGIT_WIDTH, DIGIT_HEIGHT},
    {0, DIGIT_HEIGHT * 1, 8 * DIGIT_WIDTH, DIGIT_HEIGHT},
    {0, DIGIT_HEIGHT * 2, 8 * DIGIT_WIDTH, DIGIT_HEIGHT}
};

const struct rect TEMP_ERROR[] = {
    {TEMP_DATA[0].width + DIGIT_WIDTH, TEMP_DATA[0].y, max(SCREEN_WIDTH - (TEMP_DATA[0].width + DIGIT_WIDTH), 0), DIGIT_HEIGHT},
    {TEMP_DATA[1].width + DIGIT_WIDTH, TEMP_DATA[1].y, max(SCREEN_WIDTH - (TEMP_DATA[1].width + DIGIT_WIDTH), 0), DIGIT_HEIGHT},
    {TEMP_DATA[2].width + DIGIT_WIDTH, TEMP_DATA[2].y, max(SCREEN_WIDTH - (TEMP_DATA[2].width + DIGIT_WIDTH), 0), DIGIT_HEIGHT}
};

const struct rect CO2_DATA = {0, DIGIT_HEIGHT * 3, SCREEN_WIDTH, DIGIT_HEIGHT};

const int OLED_RESET    = -1;  // Не используем сброс (если не подключен)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void OLED_screen_setup() {
    const int SDA_PIN = 21; // замените на ваш пин, если другой
    const int SCL_PIN = 22; // замените на ваш пин, если другой

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000); // 100 кГц — надёжно

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED не найден или не отвечает!");
        while (1);
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
}

void handleMenu(int button_index) {
    String msg = "button " + String(button_index) + " clicked";
    Serial.println(msg);
}

void display_regular_update() {
    static unsigned long last_upd_ms = 0;
    unsigned long now = millis();
    if (now - last_upd_ms > DISPLAY_UPD_PERIOD_MS) {
        display.display();
        last_upd_ms = now;
    }
}

void prepare_rect(const struct rect* Rect) {
    display.setCursor(Rect->x, Rect->y);
    display.fillRect(Rect->x, Rect->y, Rect->width, Rect->height, SSD1306_BLACK);
}

void print_screen(String strings[], unsigned int count) {
    assert(strings);
    unsigned int n_strings = min(STRINGS_IN_SCREEN, count);
    for (int i = 0; i < n_strings; i++) {
        prepare_rect(&(screen[i]));
        display.println(strings[i]);
    }
}

void print_line(String str, unsigned int line_ind) {
    assert(line_ind < STRINGS_IN_SCREEN);
    prepare_rect(&(screen[line_ind]));
    display.println(str);
}

void display_temperature() {
    DBG_PRINT();
    for (int sensor_ind = 0; sensor_ind < SENSORS_COUNT; sensor_ind++) {
        float temperatureC = get_sensor_recent_temp(sensor_ind);
        prepare_rect(&TEMP_DATA[sensor_ind]);
        display.print(temperatureC);
        display.println(" C ");
    }
}

void display_temp_err() {
    DBG_PRINT();
    for (int sensor_ind = 0; sensor_ind < SENSORS_COUNT; sensor_ind++) {
        bool error = get_sensor_error(sensor_ind);
        prepare_rect(&TEMP_ERROR[sensor_ind]);
        String msg;
        if (!error) {
            msg = " valid";
        } else {
            msg = " lost";
        }
        display.println(msg);
    }
}

void display_CO2() {
    DBG_PRINT();
    int co2 = get_last_co2_ppm();
    int co2_optimal = get_optimal_co2_ppm();

    prepare_rect(&CO2_DATA);

    String msg = "CO2:" + String(co2) + "ppm" + "(" + String((int)round((float)co2 / co2_optimal * 100)) +"%)";
    display.println(msg);
}


void display_sensors() {
    DBG_PRINT();
    display_temperature();
    display_temp_err();
    display_CO2();
}

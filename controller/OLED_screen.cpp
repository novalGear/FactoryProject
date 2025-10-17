#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>


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

const struct rect TEMP_DATA[] = {
    {0, DIGIT_HEIGHT * 1, 7 * DIGIT_WIDTH, DIGIT_HEIGHT},
    {0, DIGIT_HEIGHT * 2, 7 * DIGIT_WIDTH, DIGIT_HEIGHT},
    {0, DIGIT_HEIGHT * 3, 7 * DIGIT_WIDTH, DIGIT_HEIGHT}
};

const struct rect TEMP_ERROR[] = {
    {TEMP_DATA[0].width + DIGIT_WIDTH, TEMP_DATA[0].y, max(SCREEN_WIDTH - (TEMP_DATA[0].width + DIGIT_WIDTH), 0), DIGIT_HEIGHT},
    {TEMP_DATA[1].width + DIGIT_WIDTH, TEMP_DATA[1].y, max(SCREEN_WIDTH - (TEMP_DATA[1].width + DIGIT_WIDTH), 0), DIGIT_HEIGHT},
    {TEMP_DATA[2].width + DIGIT_WIDTH, TEMP_DATA[2].y, max(SCREEN_WIDTH - (TEMP_DATA[2].width + DIGIT_WIDTH), 0), DIGIT_HEIGHT}
};

const struct rect CO2_DATA = {0, DIGIT_HEIGHT * 4, SCREEN_WIDTH, DIGIT_HEIGHT};

const int OLED_RESET    = -1;  // Не используем сброс (если не подключен)

// Инициализация дисплея по I2C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void prepare_rect(const struct rect* Rect) {
    display.setCursor(Rect->x, Rect->y);
    display.fillRect(Rect->x, Rect->y, Rect->width, Rect->height, SSD1306_BLACK);
}

void OLED_screen_setup() {
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED не найден или не отвечает!");
        while(1); // Остановка, если дисплей не обнаружен
    }

    display.clearDisplay();
    // Настройка текста
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.display();
}

void display_temperature(const int sensor_ind, const float temperatureC) {
    prepare_rect(&TEMP_DATA[sensor_ind]);
    display.print(temperatureC);
    display.println(" C");
}

void display_temp_err(const int sensor_ind, bool error) {
    prepare_rect(&TEMP_ERROR[sensor_ind]);
    String msg;
    if (!error) {
        msg = " valid";
    } else {
        msg = " lost";
    }
    display.println(msg);
    display.display();
}

void display_CO2(const int co2, const int co2_optimal) {
    prepare_rect(&CO2_DATA);

    String msg = "CO2:" + String(co2) + "ppm" + "(" + String((int)round((float)co2 / co2_optimal * 100)) +"%)";
    display.println(msg);
    display.display();
}
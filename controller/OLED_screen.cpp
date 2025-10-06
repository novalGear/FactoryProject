#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

const int SCREEN_WIDTH  = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET    = -1;  // Не используем сброс (если не подключен)

// Инициализация дисплея по I2C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


void OLED_screen_setup() {
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED не найден или не отвечает!");
        while(1); // Остановка, если дисплей не обнаружен
    }

    display.clearDisplay();

    // Настройка текста
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("DST-015 OLED init");
    display.display();
}

void clear_rect(int x, int y, int width, int height) {
    display.fillRect(x, y, width, height, SSD1306_BLACK);
}

void display_temperature(const float temperatureC) {
    display.setCursor(0, 10);
    clear_rect(0, 10, SCREEN_WIDTH, 10);
    display.print(temperatureC);
    display.println(" C");
    display.display();
}

void display_temp_err() {
    display.setCursor(0, 10);
    clear_rect(0, 10, SCREEN_WIDTH, 10);

    display.println("temp sensor not found");
    display.display();

}

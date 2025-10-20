#include <Arduino.h>

const int MOTOR_PWM_PIN = 18;  // Пин ШИМ -> на вход ШИМ драйвера BLDC
const int MOTOR_DIR_PIN = 19;  // Пин направления -> на вход DIR драйвера BLDC
const int ENCODER_A_PIN = 25;  // Канал A энкодера
const int ENCODER_B_PIN = 26;  // Канал B энкодера

const int PWM_FREQ = 20000;         // Частота ШИМ, например 20 кГц
const int PWM_RESOLUTION_BITS = 8;  // Разрешение ШИМ (8 бит = 0-255)

// --- Переменные для энкодера ---
volatile long encoderPos = 0;      // volatile: изменяется в прерывании
volatile byte lastEncoded = 0;     // volatile: состояние в прерывании

// --- Переменная для состояния LEDC ---
bool ledcAttached = false; // Объявляем переменную ledcAttached

unsigned long lastSerialPrint = 0;
const unsigned long serialPrintInterval = 500; // Печатать раз в 500 мс

// --- Функция-обработчик прерывания для энкодера ---
void IRAM_ATTR encoderISR() {
    // Считываем текущее состояние каналов A и B
    byte m = 0;
    if (digitalRead(ENCODER_A_PIN)) m |= 0x02; // Бит 1
    if (digitalRead(ENCODER_B_PIN)) m |= 0x01; // Бит 0

    // Формируем 4-битное значение (предыдущее, текущее)
    byte encoded = (lastEncoded << 2) | m;

    // Обновляем счётчик на основе квадратурного кода
    // 0b00 -> 0b01 -> 0b11 -> 0b10 -> 0b00 (CW) -> +1
    // 0b00 -> 0b10 -> 0b11 -> 0b01 -> 0b00 (CCW) -> -1
    if (encoded == 0b0001 || encoded == 0b0111 || encoded == 0b1110 || encoded == 0b1000) {
        encoderPos++; // CW
    } else if (encoded == 0b0010 || encoded == 0b1011 || encoded == 0b1101 || encoded == 0b0100) {
        encoderPos--; // CCW
    }
    // 0b0000, 0b0101, 0b1010, 0b1111 - это недопустимые переходы (дребезг/ошибка)

    // Сохраняем текущее состояние как последнее
    lastEncoded = m;
}

void motor_setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // Ждём открытия Serial Monitor
    }
    Serial.println("BLDC Motor Control & Encoder Reader Started (Initial stop) (LEDC API 3.0+)");

    pinMode(MOTOR_PWM_PIN, OUTPUT);
    pinMode(MOTOR_DIR_PIN, OUTPUT);
    pinMode(ENCODER_A_PIN, INPUT_PULLUP);
    pinMode(ENCODER_B_PIN, INPUT_PULLUP);

    digitalWrite(MOTOR_DIR_PIN, HIGH);

    if (!ledcAttach(MOTOR_PWM_PIN, PWM_FREQ, PWM_RESOLUTION_BITS)) {
        Serial.println("Error: Failed to setup LEDC for pin " + String(MOTOR_PWM_PIN));
        ledcAttached = false;
        digitalWrite(MOTOR_PWM_PIN, HIGH);
    } else {
        Serial.println("LEDC setup successful for pin " + String(MOTOR_PWM_PIN));
        ledcAttached = true;
        // ledcDetach(MOTOR_PWM_PIN); // Отключаем ШИМ от пина
        // ledcAttached = false;
        digitalWrite(MOTOR_PWM_PIN, HIGH);
        Serial.println("LEDC detached after setup, motor initially stopped (pin HIGH).");
    }

    attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), encoderISR, CHANGE);

    Serial.println("Motor setup complete. Initially stopped.");
}

/*
void loop() {
  unsigned long currentTime = millis(); // Получаем текущее время один раз за цикл

  // --- Логика смены направления ---
  if (!motorIsStopping) { // Выполняем проверку интервала, только если мотор не остановлен
    if (currentTime - lastDirectionChange >= directionChangeInterval) {
      // Пора останавливать мотор перед сменой направления
      motorIsStopping = true; // Устанавливаем флаг остановки
      stopStartTime = currentTime; // Запоминаем время начала остановки

      // --- НАДЁЖНО ОСТАНАВЛИВАЕМ МОТОР ---
      if (ledcAttached) { // Проверяем, был ли ШИМ прикреплён
        ledcDetach(MOTOR_PWM_PIN); // Отключаем ШИМ от пина (НОВАЯ ФУНКЦИЯ)
        ledcAttached = false; // Обновляем флаг
        Serial.println("LEDC detached from pin, motor should stop.");
      }
      // Убедимся, что пин в HIGH (ВАШЕ ИСПРАВЛЕНИЕ)
      // Это нужно сделать *после* ledcDetach, чтобы не было конфликта
      digitalWrite(MOTOR_PWM_PIN, HIGH); // Устанавливаем пин в HIGH Вольт (остановка) - ПО ВАШЕМУ СЛУЧАЮ
      Serial.println("Motor stopped (PWM detached + pin HIGH). Waiting 2 seconds before changing direction...");
    }
  } else { // Мотор в состоянии остановки перед сменой
    if (currentTime - stopStartTime >= stopDuration) {
      // Прошло достаточно времени для остановки, меняем направление
      motorDirection = !motorDirection; // Переключить направление
      digitalWrite(MOTOR_DIR_PIN, motorDirection);
      Serial.print("Direction changed to ");
      Serial.println(motorDirection ? "FORWARD" : "BACKWARD");

      // --- ЗАПУСКАЕМ МОТОР СНОВА ---
      // Повторно настраиваем ШИМ перед запуском
      if (!ledcAttached) { // Проверяем, нужно ли заново прикреплять ШИМ
         if (ledcAttach(MOTOR_PWM_PIN, PWM_FREQ, PWM_RESOLUTION_BITS)) {
            Serial.println("LEDC re-attached to pin.");
            ledcAttached = true; // Обновляем флаг
         } else {
            Serial.println("Error: Failed to re-attach LEDC for pin " + String(MOTOR_PWM_PIN));
            // Можно оставить мотор остановленным или обработать ошибку иначе
            // motorIsStopping = false; // Не сбрасываем флаг, если ошибка
            // lastDirectionChange = currentTime;
         }
      }

      if (ledcAttached) { // Только если ШИМ успешно прикреплён
          ledcWrite(MOTOR_PWM_PIN, motorSpeed); // <-- ИСПОЛЬЗУЕМ НОВЫЙ ledcWrite
          Serial.println("Motor restarted with new direction.");
      } else {
          Serial.println("Cannot restart motor: LEDC not attached.");
      }


      // Сбрасываем флаг остановки
      motorIsStopping = false;
      // Обновляем время последней смены направления (считаем, что смена произошла сейчас)
      lastDirectionChange = currentTime;
    }
    // Если прошло меньше времени stopDuration, мотор остаётся остановленным, и мы ждём дальше.
  }

  // Периодическая печать позиции в Serial
  if (currentTime - lastSerialPrint >= serialPrintInterval) {
    Serial.print("Encoder Position: ");
    Serial.print(encoderPos);
    Serial.print(" | Speed PWM (LEDC 3.0+): ");
    // Выводим 0, если мотор остановлен, или реальную скорость, или NA если ошибка
    // Используем String() для приведения чисел к строке
    Serial.print(motorIsStopping ? "0" : (ledcAttached ? String(motorSpeed) : "NA"));
    Serial.print(" | Direction: ");
    Serial.print(motorDirection ? "FORWARD" : "BACKWARD");
    Serial.print(" | State: ");
    Serial.print(motorIsStopping ? "STOPPING" : "RUNNING");
    Serial.print(" | LEDC: ");
    Serial.println(ledcAttached ? "Attached" : "Detached");
    lastSerialPrint = currentTime; // Обновляем время последней печати
  }

  // Здесь можно добавить другую логику
  // Например, изменение motorSpeed в зависимости от encoderPos для позиционирования
}
*/

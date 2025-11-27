#include <Arduino.h>

#define DBG_PRINT() Serial.println(String(__PRETTY_FUNCTION__) + ":" + String(__LINE__))

const int MOTOR_PWM_PIN = 18;   // Пин ШИМ -> на вход ШИМ драйвера BLDC
const int MOTOR_DIR_PIN = 19;   // Пин направления -> на вход DIR драйвера BLDC
const int ENCODER_OUTPUT_PIN = 26;     // encoder pin

const int PWM_FREQ = 20000;         // Частота ШИМ
const int PWM_RESOLUTION_BITS = 8;  // Разрешение ШИМ (8 бит = 0-255)

volatile long encoderCount = 0;
volatile byte lastEncoded = 0;
volatile int current_motor_dir = 0; // 1 - вперед, -1 - назад, 0 - остановлен

bool ledcAttached = false;

unsigned long lastSerialPrint = 0;
const unsigned long serialPrintInterval = 500;

// Глобальная константа для разрешения энкодера (установите ваше значение)
const int ENCODER_RESOLUTION = 100; // Пример: 2000 тиков на оборот

// Флаг, указывающий, что выполняется задача поворота
static bool motorMoveTaskActive = false;
// Целевое количество тиков для поворота (относительно начальной позиции)
static long targetTickCount = 0;
// Начальная позиция энкодера в начале задачи поворота
static long initialencoderCount = 0;
// Требуемое количество оборотов (может быть дробным)
static float requiredRevolutions = 0.0;
// Требуемое направление (-1 или +1)
static int requiredDirection = 1; // 1 для FORWARD, -1 для BACKWARD
// Требуемая скорость (0-1023 для 10-битного PWM)
static int requiredSpeed = 512; // Пример: 50% скважности

const unsigned long DFLT_TIMEOUT = 10000;

volatile unsigned long isrCallCount = 0;

void IRAM_ATTR encoderISR() {
  encoderCount += current_motor_dir;
}

// basic motor management =======================================================================================================//

void stop_motor() {
  if (ledcAttached) {
    ledcWrite(MOTOR_PWM_PIN, 0);
  } else {
    digitalWrite(MOTOR_PWM_PIN, LOW);
  }
  current_motor_dir = 0;
  Serial.println("Motor STOPPED");
}


void set_motor_speed(int speed, int direction) {
    // speed: 0-255
    // direction: 0 или 1
    assert(speed >= 0 || speed < 255);
    assert(direction > 0 || direction < 2);
    // direction [1, 0] -> [1, -1]
    current_motor_dir = 2 * direction - 1;

    digitalWrite(MOTOR_DIR_PIN, direction);

    if (!ledcAttached) {
        if (ledcAttach(MOTOR_PWM_PIN, PWM_FREQ, PWM_RESOLUTION_BITS)) {
        ledcAttached = true;
        Serial.println("LEDC attached");
        }
    }

    if (ledcAttached) {
        ledcWrite(MOTOR_PWM_PIN, speed);
    }
    //
    //   Serial.print("Motor: Speed=");
    //   Serial.print(speed);
    //   Serial.print(" (");
    //   Serial.print((speed * 100) / 255);
    //   Serial.print("%), DIR=");
    //   Serial.println(direction ? "HIGH" : "LOW");
}

void detach_motor_pwm() {
  if (ledcAttached) {
    ledcDetach(MOTOR_PWM_PIN);
    ledcAttached = false;
    Serial.println("LEDC detached from motor pin.");
  }
}

// setup ========================================================================================================================//

void motor_setup() {
    Serial.println("Pins - ENC:" + String(ENCODER_OUTPUT_PIN) +
                ", PWM:" + String(MOTOR_PWM_PIN) +
                ", DIR:" + String(MOTOR_DIR_PIN));

    // Настройка пинов
    pinMode(MOTOR_DIR_PIN, OUTPUT);
    pinMode(MOTOR_PWM_PIN, OUTPUT);
    pinMode(ENCODER_OUTPUT_PIN, INPUT_PULLUP);

    // Инициализация LEDC
    if (ledcAttach(MOTOR_PWM_PIN, PWM_FREQ, PWM_RESOLUTION_BITS)) {
        ledcAttached = true;
        Serial.println("LEDC initialized successfully");
    } else {
        Serial.println("LEDC initialization failed - using digital writes");
    }

    // Подключение прерывания
    attachInterrupt(digitalPinToInterrupt(ENCODER_OUTPUT_PIN), encoderISR, RISING);

    Serial.println("Setup complete. Starting motor test...");
    Serial.println("---");

    // Начальная остановка мотора
    stop_motor();
}

// checks =======================================================================================================================//

bool isMotorMoveTaskActive() {
    return motorMoveTaskActive;
}

// usable motor utils ===========================================================================================================//



void setMotorMoveTask(float revolutions, int direction, int speed) {
    if (revolutions < 0) {
        revolutions = -revolutions;
        direction = 1 - direction;
    }

    // Защита от нулевых оборотов
    if (revolutions <= 0) {
        stop_motor();
        return;
    }

    requiredRevolutions = revolutions;
    requiredDirection = (direction > 0) ? 1 : 0; // фикс для бинарного направления
    requiredSpeed = constrain(speed, 0, 1023);
    targetTickCount = (long)(requiredRevolutions * ENCODER_RESOLUTION);
    initialencoderCount = encoderCount;
    motorMoveTaskActive = true;

    Serial.print("Move Task: ");
    Serial.print(requiredRevolutions, 3);
    Serial.print(" revs, dir=");
    Serial.print(requiredDirection);
    Serial.print(", speed=");
    Serial.print(requiredSpeed);
    Serial.print(", target=");
    Serial.print(targetTickCount);
    Serial.print(" ticks (current: ");
    Serial.print(encoderCount);
    Serial.println(")");
}

// Улучшенная версия MotorExecMoveTask
bool MotorExecMoveTask() {
    if (!motorMoveTaskActive) {
        return false;
    }

    long currentTickCount = abs(encoderCount - initialencoderCount);

    Serial.println("ticks: encoderCount=" + String(encoderCount) + ", initial=" + String(initialencoderCount) + ", current=" +String(currentTickCount));
    // Добавим защиту от переполнения и проверку прогресса
    if (currentTickCount >= targetTickCount) {
        stop_motor();
        Serial.print("Move completed. Final ticks: ");
        Serial.println(currentTickCount);
        return false;
    }

    // Плавное торможение при приближении к цели (опционально)
    long remainingTicks = targetTickCount - currentTickCount;
    if (remainingTicks < 100) { // На последних 100 тиках
        int slowSpeed = map(remainingTicks, 0, 100, 0, requiredSpeed);
        set_motor_speed(slowSpeed, requiredDirection);
    } else {
        set_motor_speed(requiredSpeed, requiredDirection);
    }

    return true;
}

/**
 * @brief Блокирующее перемещение мотора на заданное число оборотов
 *
 * @param revolutions Количество оборотов (отрицательные = обратное направление)
 * @param direction Направление (0 или 1)
 * @param speed Скорость ШИМ (0-1023)
 * @param timeout_ms Таймаут в миллисекундах (0 = без таймаута)
 * @return true - успешное выполнение, false - таймаут или ошибка
 */
bool unint_motor_move(float revolutions, int direction, int speed, unsigned long timeout_ms = DFLT_TIMEOUT) {
    setMotorMoveTask(revolutions, direction, speed);
    unsigned long startTime = millis();

    while(MotorExecMoveTask()) {
        // Проверка таймаута
        if (timeout_ms > 0 && (millis() - startTime) > timeout_ms) {
            stop_motor();
            Serial.println("Motor move TIMEOUT!");
            return false;
        }

        // Короткая задержка для стабильности
        delay(1);
    }

    // Короткая пауза для стабилизации после остановки
    delay(50);
    return true;
}

void cancelMotorMoveTask() {
    motorMoveTaskActive = false;
    Serial.println("Motor Move Task Cancelled.");
}

// Testing ======================================================================================================================//

void motor_test() {
  // DBG_PRINT();
  unsigned long currentTime = millis();

  static bool motorIsStopping = false;
  static unsigned int lastDirectionChange = 0;
  unsigned int stopDuration = 2000;
  unsigned int directionChangeInterval = 5000;
  unsigned int motorSpeed = 200;
  static bool motorDirection = false;
  static unsigned int stopStartTime = 0;

  if (!motorIsStopping) {
    if (currentTime - lastDirectionChange >= directionChangeInterval) {
      motorIsStopping = true;
      stopStartTime = currentTime;

      // if (ledcAttached) {
      //   ledcDetach(MOTOR_PWM_PIN);
      //   ledcAttached = false;
      //   Serial.println("LEDC detached from pin, motor should stop.");
      // }
      // digitalWrite(MOTOR_PWM_PIN, HIGH);
      // Serial.println("Motor stopped (PWM detached + pin HIGH). Waiting 2 seconds before changing direction...");

      if (ledcAttached) {
        ledcWrite(MOTOR_PWM_PIN, 0); // Остановка через PWM
        Serial.println("Motor stopped (PWM set to 0).");
      } else {
        // Если уже detach, установите пин в состояние, соответствующее остановке
        digitalWrite(MOTOR_PWM_PIN, LOW); // или HIGH, в зависимости от схемы
        Serial.println("Motor stopped (pin state set, LEDC not attached).");
      }
    }
  } else {
    if (currentTime - stopStartTime >= stopDuration) {
      motorDirection = !motorDirection;
      digitalWrite(MOTOR_DIR_PIN, motorDirection);
      Serial.print("Direction changed to ");
      Serial.println(motorDirection ? "FORWARD" : "BACKWARD");

      if (!ledcAttached) {
         if (ledcAttach(MOTOR_PWM_PIN, PWM_FREQ, PWM_RESOLUTION_BITS)) {
            Serial.println("LEDC re-attached to pin.");
            ledcAttached = true;
         } else {
            Serial.println("Error: Failed to re-attach LEDC for pin " + String(MOTOR_PWM_PIN));
         }
      }

      if (ledcAttached) {
          ledcWrite(MOTOR_PWM_PIN, motorSpeed);
          Serial.println("Motor restarted with new direction.");
      } else {
          Serial.println("Cannot restart motor: LEDC not attached.");
      }


      motorIsStopping = false;
      lastDirectionChange = currentTime;
    }
  }

  if (currentTime - lastSerialPrint >= serialPrintInterval) {
    Serial.print("Encoder Position: ");
    Serial.print(encoderCount);
    Serial.print(motorIsStopping ? "0" : (ledcAttached ? String(motorSpeed) : "NA"));
    Serial.print(" | Direction: ");
    Serial.print(motorDirection ? "FORWARD" : "BACKWARD");
    Serial.print(" | State: ");
    Serial.print(motorIsStopping ? "STOPPING" : "RUNNING");
    Serial.print(" | LEDC: ");
    Serial.println(ledcAttached ? "Attached" : "Detached");
    lastSerialPrint = currentTime;
  }

}


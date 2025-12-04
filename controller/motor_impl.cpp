#include <Arduino.h>
#include "motor_impl.h"

#define DBG_PRINT() Serial.println(String(__PRETTY_FUNCTION__) + ":" + String(__LINE__))

const int MOTOR_PWM_PIN = 18;           // Пин ШИМ -> на вход ШИМ драйвера BLDC
const int MOTOR_DIR_PIN = 19;           // Пин направления -> на вход DIR драйвера BLDC
const int ENCODER_OUTPUT_PIN = 26;      // encoder pin

const int PWM_FREQ = 20000;         // Частота ШИМ
const int PWM_RESOLUTION_BITS = 8;  // Разрешение ШИМ (8 бит = 0-255)

volatile long encoderCount = 0;
long lastStoppedEncoderCount = 0;
volatile int current_motor_dir = 0; // 1 - вперед, -1 - назад

bool ledcAttached = false;

unsigned long lastSerialPrint = 0;
const unsigned long serialPrintInterval = 500;

const int ENCODER_RESOLUTION = 256;                         // Глобальная константа для разрешения энкодера
const unsigned long CRIT_THRESHOLD  = 10;                   // Критическое значение внешнего поворота
const unsigned long MAX_MOTOR_POS   = 2000;                 // максимальное значение в ед. изм. энкодера
const unsigned long MAX_POS         = 10;                   // число возможных позиций мотора

static bool motorMoveTaskActive = false;                    // Флаг, указывающий, что выполняется задача поворота
static long targetTickCount = 0;                            // Целевое количество тиков для поворота (относительно начальной позиции)
static long initialencoderCount = 0;                        // Начальная позиция энкодера в начале задачи поворота

// static float requiredRevolutions = 0.0;                     // Требуемое количество оборотов (может быть дробным)
static int requiredDirection = 1;                           // Требуемое направление: 1 для FORWARD, -1 для BACKWARD
static int requiredSpeed = 0;                               // Требуемая скорость (0-1023 для 10-битного PWM)

int curr_pos_ind = 0;

volatile unsigned long isrCallCount = 0;

void IRAM_ATTR encoderISR() {
  encoderCount += current_motor_dir;
}

void updateStoppedPosition() {
    lastStoppedEncoderCount = encoderCount;
}

long get_encoder() {
    return encoderCount;
}

// Forward declaration

void stop_motor();
void set_motor_speed(int speed, int direction);

// basic motor management =======================================================================================================//

void stop_motor() {
    set_motor_speed(0, 0);
    motorMoveTaskActive = false;
    resetEncoderVelocityCalculation();
    Serial.println("Motor STOPPED (direction preserved)");
}

void set_motor_speed(int speed, int direction) {
    assert(speed >= 0 && speed <= 255);
    assert(direction == 0 || direction == 1);

    if (speed > 0) {
        current_motor_dir = (direction == 1) ? 1 : -1;
    }
    if (!ledcAttached) {
        if (ledcAttach(MOTOR_PWM_PIN, PWM_FREQ, PWM_RESOLUTION_BITS)) {
            ledcAttached = true;
        }
    }

    digitalWrite(MOTOR_DIR_PIN, direction);
    ledcWrite(MOTOR_PWM_PIN, speed);
//
//     Serial.print("Motor: Speed=");
//     Serial.print(speed);
//     Serial.print(", DIR=");
//     Serial.print(direction ? "HIGH" : "LOW");
//     Serial.print(", CurrentDir=");
//     Serial.println(current_motor_dir);
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


/**
 * @brief Проверяет, было ли внешнее воздействие на остановленный мотор
 * @param threshold Пороговое значение тиков для обнаружения движения
 * @return true если обнаружено значительное внешнее воздействие
 */
bool checkExternalMovement(long threshold = CRIT_THRESHOLD) {
    // Если мотор сейчас движется - не проверяем
    if (isMotorMoveTaskActive()) {
        return false;
    }
    // Проверяем разницу с последней зафиксированной позицией
    long positionDiff = abs(encoderCount - lastStoppedEncoderCount);

    if (positionDiff >= threshold) {
        Serial.print("EXTERNAL MOVEMENT DETECTED! Diff: ");
        Serial.print(positionDiff);
        Serial.print(" ticks (");
        Serial.print(encoderCount);
        Serial.print(" vs ");
        Serial.print(lastStoppedEncoderCount);
        Serial.println(")");

        // Обновляем эталонную позицию на текущую
        updateStoppedPosition();
        return true;
    }

    return false;
}

// usable motor utils ===========================================================================================================//

void setMotorMoveTask(unsigned long ticks, int direction, int speed) {
    assert(direction == 0 || direction == 1);
    if (ticks == 0) {
        stop_motor();
        return;
    }
//
//     if (checkExternalMovement) {
//         Serial.println("External movement caught");
//         reset_motor();
//     }

    // проверка на значения direction

    // requiredRevolutions = ticks;
    requiredDirection = direction;
    requiredSpeed = constrain(speed, 0, 1023);
    targetTickCount = ticks; // (long)(requiredRevolutions * ENCODER_RESOLUTION);
    initialencoderCount = encoderCount;
    motorMoveTaskActive = true;

    Serial.print("Move Task: ");
    Serial.print("dir=");
    Serial.print(requiredDirection);
    Serial.print(", speed=");
    Serial.print(requiredSpeed);
    Serial.print(", target=");
    Serial.print(targetTickCount);
    Serial.print(" ticks (current: ");
    Serial.print(encoderCount);
    Serial.println(")");
}

bool MotorExecMoveTask() {
    if (!motorMoveTaskActive) {
        return false;
    }

    long currentDisplacement = encoderCount - initialencoderCount;
    long absoluteDisplacement = abs(currentDisplacement);

    // // ДЛЯ ОТЛАДКИ - выводим реальные значения
    // Serial.println("DEBUG: curr=" + String(encoderCount) +
    //               ", init=" + String(initialencoderCount) +
    //               ", disp=" + String(currentDisplacement) +
    //               ", target=" + String(targetTickCount));

    if (absoluteDisplacement >= targetTickCount) {
        stop_motor();
        lastStoppedEncoderCount = encoderCount;
        Serial.println("Move COMPLETED. Ticks: " + String(absoluteDisplacement));
        return false;
    }

    // Продолжаем движение
    set_motor_speed(requiredSpeed, requiredDirection);
    return true;
}

/**
 * @brief Блокирующее перемещение мотора на заданное число оборотов
 *
 * @param ticks Поворот в тиках энкодера
 * @param direction Направление (0 или 1)
 * @param speed Скорость ШИМ (0-1023)
 * @param timeout_ms Таймаут в миллисекундах (0 = без таймаута)
 * @return true - успешное выполнение, false - таймаут или ошибка
 */
bool unint_motor_move(unsigned long ticks, int direction, int speed, unsigned long timeout_ms) {
    setMotorMoveTask(ticks, direction, speed);

    unsigned long startTime = millis();
    Serial.println("motor init encoder position: " + String(encoderCount));

    while (MotorExecMoveTask()) {
        // Кормим WDT
        vTaskDelay(1); // корректно для FreeRTOS, лучше чем delay()

        // Проверка таймаута
        if (timeout_ms > 0 && (millis() - startTime) > timeout_ms) {
            Serial.println("Motor move TIMEOUT!");
            stop_motor();
            // lastStoppedEncoderCount = encoderCount;
            return false;
        }
    }

    stop_motor();
    // lastStoppedEncoderCount = encoderCount;
    delay(30); // небольшая стабилизация
    return true;
}

void cancelMotorMoveTask() {
    motorMoveTaskActive = false;
    Serial.println("Motor Move Task Cancelled.");
}

// discrete control =============================================================================================================//

unsigned long pos2ticks(int pos) {
    const unsigned long step = MAX_MOTOR_POS / MAX_POS;
    return step * abs(pos - curr_pos_ind);
}

int dir2pos(int pos) {
    return (pos > curr_pos_ind) ? 0 : 1;
}

int change_pos(int pos) {
    if (pos == curr_pos_ind) {
        Serial.println("pos = curr");
        return 0;
    }
    if (pos > MAX_POS) {
        Serial.println("pos out of range");
        return -1;
    }

    // вычисляем количество тиков
    unsigned long ticks = pos2ticks(pos);

    // направление 0/1
    int direction = dir2pos(pos);

    Serial.print("change_pos(): curr=");
    Serial.print(curr_pos_ind);
    Serial.print(" -> ");
    Serial.print(pos);
    Serial.print(" | ticks=");
    Serial.print(ticks);
    Serial.print(" dir=");
    Serial.println(direction);

    // выполняем движение
    bool ok = unint_motor_move(ticks, direction);

    if (!ok) {
        Serial.println("Error: movement failed.");
        return -1;
    }

    // обновляем состояние
    curr_pos_ind = pos;

    return 0;
}


int get_current_position_index() {
    return curr_pos_ind;
}

// HOMING =======================================================================================================================//

// Добавьте эти константы в начало файла после других констант
const int HOMING_SPEED = 150;            // Скорость хоуминга (0-255)
const int HOMING_MIN_VELOCITY = 100;     // Минимальная скорость энкодера для остановки хоуминга
const unsigned long HOMING_TIMEOUT_MS = 10000;  // Таймаут хоуминга 10 секунд
const unsigned long VELOCITY_SAMPLE_PERIOD = 100;  // Период измерения скорости (мс)

// Добавьте глобальные переменные для расчета скорости
unsigned long lastEncoderSampleTime = 0;
long lastEncoderCount = 0;
float currentEncoderVelocity = 0.0;      // Текущая скорость энкодера в тиках/секунду

// ===================================================================
// Функция calculateEncoderVelocity - расчет скорости энкодера
// ===================================================================
/**
 * @brief Рассчитывает текущую скорость энкодера
 * @return Скорость в тиках/секунду
 */
float calculateEncoderVelocity() {
    unsigned long currentTime = millis();

    if (lastEncoderSampleTime == 0) {
        // Первый вызов
        lastEncoderSampleTime = currentTime;
        return HOMING_MIN_VELOCITY + 1;
    }

    // Вычисляем дельту времени
    unsigned long deltaTime = currentTime - lastEncoderSampleTime;

    if (deltaTime < VELOCITY_SAMPLE_PERIOD) {
        // Не прошло достаточно времени для измерения
        return currentEncoderVelocity;
    }

    // Вычисляем дельту тиков
    long deltaTicks = encoderCount - lastEncoderCount;

    // Рассчитываем скорость (тиков/секунду)
    currentEncoderVelocity = (deltaTicks * 1000.0) / deltaTime;

    // Обновляем значения для следующего расчета
    lastEncoderSampleTime = currentTime;
    lastEncoderCount = encoderCount;

    return currentEncoderVelocity;
}

// ===================================================================
// Функция resetEncoderVelocityCalculation - сброс расчета скорости
// ===================================================================
/**
 * @brief Сбрасывает расчет скорости энкодера
 */
void resetEncoderVelocityCalculation() {
    lastEncoderSampleTime = 0;
    lastEncoderCount = encoderCount;
    currentEncoderVelocity = 0.0;
}// ===================================================================
// Функция performHoming - выполнение процедуры хоуминга
// ===================================================================
/**
 * @brief Выполняет процедуру хоуминга мотора
 * @param homingDirection Направление хоуминга (0 или 1)
 * @return true - хоуминг успешен, false - таймаут или ошибка
 *
 * Алгоритм:
 * 1. Двигает мотор в указанном направлении с HOMING_SPEED
 * 2. Постоянно отслеживает скорость энкодера
 * 3. Когда скорость падает ниже HOMING_MIN_VELOCITY, останавливаем мотор
 * 4. Сбрасывает encoderCount в 0
 * 5. Имеет таймаут HOMING_TIMEOUT_MS
 */
int performHoming(int homingDirection) {
    assert(homingDirection == 0 || homingDirection == 1);

    Serial.println("=== STARTING HOMING PROCEDURE ===");
    Serial.print("Direction: ");
    Serial.println(homingDirection == 1 ? "FORWARD" : "BACKWARD");
    Serial.println("Moving until encoder velocity drops below " + String(HOMING_MIN_VELOCITY) + " ticks/sec");

    cancelMotorMoveTask();
    resetEncoderVelocityCalculation();

    unsigned long startTime = millis();
    unsigned long lastPrintTime = 0;
    unsigned long motorStartTime = millis();

    // Запускаем мотор
    Serial.println("Starting motor...");
    set_motor_speed(HOMING_SPEED, homingDirection);

    // Даем мотору больше времени на разгон в закрепленной конструкции
    Serial.println("Waiting for motor to start...");
    delay(1000); // Увеличиваем до 1 секунды для разгона

    bool homingSuccessful = false;
    bool timeoutReached = false;

    // Счетчик для пропуска первых измерений скорости
    int measurementsToSkip = 3; // Пропустить первые 3 измерения
    int measurementCount = 0;

    Serial.println("Beginning velocity monitoring...");

    while (!homingSuccessful && !timeoutReached) {
        if (millis() - startTime > HOMING_TIMEOUT_MS) {
            Serial.println("HOMING TIMEOUT after " + String(HOMING_TIMEOUT_MS) + "ms");
            timeoutReached = true;
            break;
        }

        float velocity = calculateEncoderVelocity();

        // Пропускаем первые несколько измерений
        measurementCount++;
        if (measurementCount <= measurementsToSkip) {
            if (millis() - lastPrintTime > 500) {
                Serial.println("Skipping initial measurement " + String(measurementCount) +
                               "/" + String(measurementsToSkip) + "...");
                lastPrintTime = millis();
            }
            vTaskDelay(100); // Даем больше времени между измерениями
            continue;
        }

        // Выводим информацию о скорости
        if (millis() - lastPrintTime > 500) {
            Serial.print("Homing... Velocity: ");
            Serial.print(velocity);
            Serial.print(" ticks/sec, Encoder: ");
            Serial.print(encoderCount);
            Serial.print(", Time: ");
            Serial.print((millis() - startTime) / 1000.0, 1);
            Serial.println("s");
            lastPrintTime = millis();
        }

        // Если скорость все еще 0 после ожидания разгона, проверяем мотор
        if (measurementCount > measurementsToSkip + 5 && fabs(velocity) < 10) {
            Serial.println("WARNING: Motor may not be moving. Checking...");

            // Даем мотору немного больше времени
            delay(500);
            velocity = calculateEncoderVelocity();

            if (fabs(velocity) < 10) {
                Serial.println("ERROR: Motor not moving. Trying with higher speed...");

                // Пробуем увеличить скорость
                set_motor_speed(HOMING_SPEED + 50, homingDirection);
                delay(1000);
                velocity = calculateEncoderVelocity();

                if (fabs(velocity) < 10) {
                    Serial.println("FATAL: Motor still not moving. Aborting homing.");
                    stop_motor();
                    return -2;
                }
            }
        }

        // Проверяем скорость (учитываем как положительные, так и отрицательные скорости)
        float absVelocity = fabs(velocity);

        if (absVelocity < HOMING_MIN_VELOCITY && absVelocity > 0) {
            // Скорость низкая, но не нулевая - возможное достижение упора

            // Ждем немного для подтверждения
            delay(200);
            velocity = calculateEncoderVelocity();
            absVelocity = fabs(velocity);

            if (absVelocity < HOMING_MIN_VELOCITY && absVelocity > 0) {
                // Скорость действительно низкая и не нулевая - достигли упора
                Serial.println("HOMING COMPLETE - Encoder velocity dropped to " + String(velocity) + " ticks/sec");

                // Продолжаем двигаться еще немного, чтобы убедиться в упоре
                delay(300);

                // Проверяем еще раз
                velocity = calculateEncoderVelocity();
                absVelocity = fabs(velocity);

                if (absVelocity < HOMING_MIN_VELOCITY) {
                    stop_motor();
                    delay(300); // Ждем полной остановки

                    // Сбрасываем счетчик энкодера в 0
                    encoderCount = 0;
                    lastStoppedEncoderCount = 0;
                    initialencoderCount = 0;

                    // Обновляем текущую позицию
                    curr_pos_ind = 0;

                    Serial.println("Encoder counter RESET to 0");
                    homingSuccessful = true;

                    // Делаем небольшой отъезд от упора
                    Serial.println("Moving away from homing position...");
                    int oppositeDirection = homingDirection == 1 ? 0 : 1;
                    set_motor_speed(HOMING_SPEED / 2, oppositeDirection);
                    delay(400);
                    stop_motor();
                    delay(200);
                }
            }
        }

        // Короткая задержка для стабильности
        vTaskDelay(50); // Увеличиваем задержку
    }

    // Останавливаем мотор на всякий случай
    stop_motor();

    // Сбрасываем расчет скорости
    resetEncoderVelocityCalculation();

    if (timeoutReached) {
        Serial.println("HOMING FAILED - Timeout reached");
        return -1;
    }

    if (homingSuccessful) {
        Serial.println("=== HOMING PROCEDURE COMPLETED SUCCESSFULLY ===");
        Serial.println("Current position: " + String(curr_pos_ind));
        Serial.println("Encoder value: " + String(encoderCount));
        return 0;
    }

    Serial.println("HOMING FAILED - Unknown error");
    return -4;
}

// Testing ======================================================================================================================//

void motor_test() {
    unint_motor_move(300, 1, 200);
    delay(2000);
    unint_motor_move(300, 0, 200);
    delay(2000);
}


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

// Forward declaration

void stop_motor();
void set_motor_speed(int speed, int direction);

// basic motor management =======================================================================================================//

void stop_motor() {
    set_motor_speed(0, 0);
    motorMoveTaskActive = false;
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
    while(MotorExecMoveTask()) {
        // Проверка таймаута
        if (timeout_ms > 0 && (millis() - startTime) > timeout_ms) {
            stop_motor();
            long currentDisplacement = encoderCount - initialencoderCount;
            long absoluteDisplacement = abs(currentDisplacement);
            lastStoppedEncoderCount = encoderCount;
            Serial.println("Move COMPLETED. Ticks: " + String(absoluteDisplacement));
            Serial.println("Motor move TIMEOUT!");
            return false;
        }

        // Короткая задержка для стабильности
        delay(1);
    }
    stop_motor();
    // Короткая пауза для стабилизации после остановки
    delay(50);
    return true;
}

void cancelMotorMoveTask() {
    motorMoveTaskActive = false;
    Serial.println("Motor Move Task Cancelled.");
}

// discrete control =============================================================================================================//

unsigned long pos2ticks(int pos) {
    if (pos == curr_pos_ind)    { return 0;  }
    if (pos > MAX_POS)          { return -1; }
    return (MAX_MOTOR_POS / MAX_POS) * pos;
}

int dir2pos(int pos) {
    return ((pos - curr_pos_ind) > 0 ) ? 1 : 0;
}

int change_pos(int pos) {
    if (pos == curr_pos_ind)    { return 0;  }
    if (pos > MAX_POS)          { return -1; }

    unsigned long ticks = pos2ticks(pos);
    int direction = dir2pos(pos);
    unint_motor_move(ticks, direction);
}

int get_current_position_index() {
    return curr_pos_ind;
}

// Testing ======================================================================================================================//

void motor_test() {
    unint_motor_move(300, 1, 200);
    delay(2000);
    unint_motor_move(300, -1, 200);
    delay(2000);
}


// --- Определения пинов ---
const int ENCODER_OUTPUT_PIN = 26;
const int MOTOR_PWM_PIN = 18;
const int MOTOR_DIR_PIN = 19;

// --- Константы для LEDC ---
const int PWM_FREQ = 5000; // Частота ШИМ
const int PWM_RESOLUTION_BITS = 8; // 8 бит (0-255)
const int PWM_CHANNEL = 0; // Канал LEDC

// --- Глобальные переменные ---
volatile long encoderCount = 0;
bool ledcAttached = false;

// --- Функции управления мотором ---
void setMotorSpeed(int speed, int direction) {
  // speed: 0-255
  // direction: 0 или 1
  
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
  
  Serial.print("Motor: Speed=");
  Serial.print(speed);
  Serial.print(" (");
  Serial.print((speed * 100) / 255);
  Serial.print("%), DIR=");
  Serial.println(direction ? "HIGH" : "LOW");
}

// --- Обработчик прерывания ---
void IRAM_ATTR encoderISR() {
  encoderCount++;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Motor Test - Compatible Version");
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
  stopMotor();
}

void loop() {
  static int testPhase = 0;
  static unsigned long phaseStartTime = millis();
  static unsigned long lastEncoderPrint = 0;

  unsigned long currentTime = millis();

  // Смена состояния каждые 3 секунды
  if (currentTime - phaseStartTime >= 3000) {
    testPhase = (testPhase + 1) % 6;
    phaseStartTime = currentTime;
    
    Serial.println("---");
    Serial.print("Phase ");
    Serial.println(testPhase + 1);
    
    switch(testPhase) {
      case 0: // Медленно вперед
        setMotorSpeed(100, LOW);
        Serial.println("Slow FORWARD");
        break;
      case 1: // Быстро вперед
        setMotorSpeed(200, LOW);
        Serial.println("Fast FORWARD");
        break;
      case 2: // Остановка
        stopMotor();
        Serial.println("STOP");
        break;
      case 3: // Медленно назад
        setMotorSpeed(100, HIGH);
        Serial.println("Slow BACKWARD");
        break;
      case 4: // Быстро назад
        setMotorSpeed(200, HIGH);
        Serial.println("Fast BACKWARD");
        break;
      case 5: // Остановка
        stopMotor();
        Serial.println("STOP");
        break;
    }
  }

  // Вывод показаний энкодера каждые 500мс
  if (currentTime - lastEncoderPrint >= 500) {
    Serial.println("Encoder: " + String(encoderCount));
    lastEncoderPrint = currentTime;
  }

  delay(10);
}
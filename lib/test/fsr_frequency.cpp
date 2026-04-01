#include <Arduino.h>

const int FORCE_SENSOR_PIN = 10;
const int LED_CALM = 7;
const int LED_REG = 5;
const int LED_ACT = 3;
const int LED_OVE = 1;

const int BIG_SQUEEZE_ON = 2500;
const int BIG_SQUEEZE_OFF = 2000;

int squeezeCount = 0;
bool isSqueezing = false;

unsigned long lastTime = 0;
const int interval = 1000; // 1 second

unsigned long lastSqueezeTime = 0;
const int debounceDelay = 200;

void setup() {
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);

  pinMode(FORCE_SENSOR_PIN, INPUT);
  pinMode(LED_CALM, OUTPUT);
  pinMode(LED_REG, OUTPUT);
  pinMode(LED_ACT, OUTPUT);
  pinMode(LED_OVE, OUTPUT);
}

// smoothing
int readSmoothed() {
  int sum = 0;
  for (int i = 0; i < 5; i++) {
    sum += analogRead(FORCE_SENSOR_PIN);
    delay(2);
  }
  return sum / 5;
}

void loop() {
  int analogReading = readSmoothed();

  // detect squeeze
  if (!isSqueezing && analogReading >= BIG_SQUEEZE_ON) {
    if (millis() - lastSqueezeTime > debounceDelay) {
      squeezeCount++;
      isSqueezing = true;
      lastSqueezeTime = millis();
    }
  }

  if (isSqueezing && analogReading <= BIG_SQUEEZE_OFF) {
    isSqueezing = false;
  }

  // every 1 second → determine state
  if (millis() - lastTime >= interval) {

    Serial.print("Frequency: ");
    Serial.print(squeezeCount);
    Serial.print(" -> ");
    digitalWrite(LED_CALM, LOW);
    digitalWrite(LED_REG, LOW);
    digitalWrite(LED_ACT, LOW);
    digitalWrite(LED_OVE, LOW);

    // 🧠 STATE LOGIC
    if (squeezeCount <= 1) {
      Serial.println("Calm");
      digitalWrite(LED_CALM, HIGH); // ON
    }
    else if (squeezeCount <= 2) {
      Serial.println("Self-regulating");
      digitalWrite(LED_REG, HIGH); // ON
    }
    else if (squeezeCount <= 3) {
      Serial.println("Active");
      digitalWrite(LED_ACT, HIGH); // ON
    }
    else {
      Serial.println("Overwhelmed");
      digitalWrite(LED_OVE, HIGH); // ON
    }

    squeezeCount = 0;
    lastTime = millis();
  }
}
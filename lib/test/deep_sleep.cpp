const int REED_PIN = 6;   // Use RTC-capable pin (e.g. 4)
const int LED = 13;

void setup() {
  Serial.begin(115200);

  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
}

void loop() {
  // Nothing needed here — logic handled in setup()
  int state = digitalRead(REED_PIN);

  if (state == LOW) {
    // Reed switch triggered → go to sleep
    Serial.println("Triggered → Going to deep sleep");
    digitalWrite(LED, LOW);

    delay(1000); // allow message to print

    // Wake up when pin goes HIGH (not triggered)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)REED_PIN, 1);

    esp_deep_sleep_start();
  } 
  else {
    // Not triggered → stay awake
    Serial.println("Not triggered → Awake");
    delay(100);
    digitalWrite(LED, HIGH);
  }
}

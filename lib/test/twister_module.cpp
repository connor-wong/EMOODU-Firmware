#include <Arduino.h>

// Define the pin connected to the middle leg of the potentiometer
const int potPin = 9;
const int threshold = 100;    // Minimum "jump" to count as a movement
const int activityLimit1 = 0;  // How many jumps in a window count as "Calm"
const int activityLimit2 = 15;  // How many jumps in a window count as "Self-regulating"
const int activityLimit3 = 30;  // How many jumps in a window count as "Active"
const int activityLimit4 = 45;  // How many jumps in a window count as "Overstimulated"
int lastValue = 0;
int activityCount = 0;
unsigned long lastCheckTime = 0;
const int windowMs = 1000;     // Look at activity every 1000ms

void setup() {
  // Initialize Serial Monitor at 115200 baud
  Serial.begin(115200);
  
  // Configure the ADC resolution (0-4095)
  analogReadResolution(4095); 
  
  pinMode(potPin, INPUT);
}

void loop() {
  // Read the raw ADC value (0 - 4095)
  int rawValue = analogRead(potPin);
  
  // Convert the raw value to Voltage (0 - 3.3V)
  float voltage = rawValue * (3.3 / 4095.0);

  // Print results to Serial Monitor
  //Serial.print("ADC Value: ");
  //Serial.print(rawValue);
  //Serial.print(" | Voltage: ");
  //Serial.print(voltage);
  //Serial.println("V");

  //delay(100); // Small delay for readability

  // 1. Calculate how much the knob moved since the last loop
  int delta = abs(rawValue - lastValue);

  // 2. If it moved significantly, increment activity
  if (delta > threshold) {
    activityCount++;
  }
  lastValue = rawValue;

  // 3. Every 'windowMs', evaluate the "Frequency"
  if (millis() - lastCheckTime > windowMs) {
    if (activityCount >= activityLimit4) {
      Serial.println("Overstimulated");
      // Trigger your state change here (e.g., turn on an LED)
      neopixelWrite(48, 255, 0, 0); // Red
    } 
    else if (activityCount >= activityLimit3) {
      Serial.println("Active");
      // Trigger your state change here (e.g., turn on an LED)
      neopixelWrite(48, 255, 0, 255); // Purple
    }
    else if (activityCount >= activityLimit2) {
      Serial.println("Self-regulating");
      // Trigger your state change here (e.g., turn on an LED)
      neopixelWrite(48, 255, 150, 0); // Yellow
    }
    else if (activityCount >= activityLimit1) {
      Serial.println("Calm");
      // Trigger your state change here (e.g., turn on an LED)
      neopixelWrite(48, 0, 255, 0); // Green
    } 

    Serial.println(activityCount);
    // Reset for next window
    activityCount = 0;
    lastCheckTime = millis();
  }

  delay(10); // Fast sampling

}

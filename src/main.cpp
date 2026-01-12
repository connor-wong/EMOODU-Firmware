#include <TFT_eSPI.h>
#include <Arduino.h>
#include <image_960x240.h>

// CS pins for each display
#define TFT_DUMMY 10
#define TFT_CS1 4
#define TFT_CS2 5
#define TFT_CS3 6
#define TFT_CS4 7

TFT_eSPI tft = TFT_eSPI(); // Single TFT instance we'll reuse

void selectDisplay(int displayNum);
void deselectAll(void);
void displayImageAcrossScreens(void);

void setup() {
  Serial.begin(115200);

  // Set all CS pins as outputs and deselect all displays
  pinMode(TFT_CS1, OUTPUT);
  pinMode(TFT_CS2, OUTPUT);
  pinMode(TFT_CS3, OUTPUT);
  pinMode(TFT_CS4, OUTPUT);
  
  digitalWrite(TFT_CS1, HIGH);
  digitalWrite(TFT_CS2, HIGH);
  digitalWrite(TFT_CS3, HIGH);
  digitalWrite(TFT_CS4, HIGH);
  
  // Initialize each display one by one
  selectDisplay(0);
  tft.init();
  tft.setRotation(0);

  selectDisplay(1);
  tft.init();
  tft.setRotation(0);
  
  selectDisplay(2);
  tft.init();
  tft.setRotation(0);
  
  selectDisplay(3);
  tft.init();
  tft.setRotation(0);
  
  selectDisplay(4);
  tft.init();
  tft.setRotation(0);

  displayImageAcrossScreens();
}

void loop() {
  // Nothing to do here
}

void selectDisplay(int displayNum) {
  // Deselect all first
  deselectAll();
  
  // Select the requested display (CS is active LOW)
  switch(displayNum) {
    case 0: digitalWrite(TFT_CS1, LOW); break;
    case 1: digitalWrite(TFT_CS1, LOW); break;
    case 2: digitalWrite(TFT_CS2, LOW); break;
    case 3: digitalWrite(TFT_CS3, LOW); break;
    case 4: digitalWrite(TFT_CS4, LOW); break;
  }
}

void deselectAll(void) {
  #ifdef TFT_CS
    digitalWrite(TFT_CS, HIGH);
  #endif
  
  digitalWrite(TFT_CS1, HIGH);
  digitalWrite(TFT_CS2, HIGH);
  digitalWrite(TFT_CS3, HIGH);
  digitalWrite(TFT_CS4, HIGH);
}

void displayImageAcrossScreens(void) {
  uint16_t lineBuffer[240];
  
  for (int displayIdx = 0; displayIdx < 4; displayIdx++) {
    selectDisplay(displayIdx + 1);
    tft.setAddrWindow(0, 0, 240, 240);
    
    for (int y = 0; y < 240; y++) {
      int xOffset = displayIdx * 240;
      
      // Read from PROGMEM into line buffer
      for (int x = 0; x < 240; x++) {
        int pixelIndex = y * IMG_WIDTH + x + xOffset;
        lineBuffer[x] = pgm_read_word(&imgArray[pixelIndex]);
      }
      
      // Push the line to the display
      tft.pushColors(lineBuffer, 240);
    }
    
    deselectAll();
  }
}
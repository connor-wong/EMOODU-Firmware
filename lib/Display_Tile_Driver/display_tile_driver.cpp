#include "display_tile_driver.h"
#include <pgmspace.h>

DisplayTileDriver::DisplayTileDriver(uint8_t cs1, uint8_t cs2, uint8_t cs3, uint8_t cs4, uint8_t dummy)
    : _cs1(cs1), _cs2(cs2), _cs3(cs3), _cs4(cs4), _dummy(dummy), _rotation(0) {
}

void DisplayTileDriver::begin() {
    // Configure all CS pins as outputs
    pinMode(_dummy, OUTPUT);
    pinMode(_cs1, OUTPUT);
    pinMode(_cs2, OUTPUT);
    pinMode(_cs3, OUTPUT);
    pinMode(_cs4, OUTPUT);
    
    // Deselect all displays
    deselectAll();
    delay(100);
    
    // Initialize dummy display first (workaround for ESP32-S3)
    selectTile(0);
    _tft.init();
    _tft.setRotation(_rotation);
    deselectAll();
    delay(50);
    
    // Initialize each tile
    for (uint8_t i = 1; i <= NUM_TILES; i++) {
        selectTile(i);
        delay(10);
        _tft.init();
        _tft.setRotation(_rotation);
        _tft.fillScreen(TFT_BLACK);
        deselectAll();
        delay(50);
    }
}

void DisplayTileDriver::selectTile(uint8_t tileNum) {
    deselectAll();
    delayMicroseconds(10);
    
    if (tileNum == 0) {
        digitalWrite(_dummy, LOW);
    } else {
        uint8_t csPin = getTileCSPin(tileNum);
        if (csPin != 0) {
            digitalWrite(csPin, LOW);
        }
    }
    
    delayMicroseconds(10);
}

void DisplayTileDriver::deselectAll() {
    digitalWrite(_dummy, HIGH);
    digitalWrite(_cs1, HIGH);
    digitalWrite(_cs2, HIGH);
    digitalWrite(_cs3, HIGH);
    digitalWrite(_cs4, HIGH);
}

uint8_t DisplayTileDriver::getTileCSPin(uint8_t tileNum) {
    switch(tileNum) {
        case 1: return _cs1;
        case 2: return _cs2;
        case 3: return _cs3;
        case 4: return _cs4;
        default: return 0;
    }
}

void DisplayTileDriver::displayImage(const uint16_t* imageArray, uint16_t imageWidth, uint16_t imageHeight) {
    uint16_t lineBuffer[TILE_WIDTH];
    
    for (uint8_t tileIdx = 0; tileIdx < NUM_TILES; tileIdx++) {
        selectTile(tileIdx + 1);
        
        _tft.setAddrWindow(0, 0, TILE_WIDTH, TILE_HEIGHT);
        
        for (uint16_t y = 0; y < TILE_HEIGHT && y < imageHeight; y++) {
            uint16_t xOffset = tileIdx * TILE_WIDTH;
            
            // Copy line from image array
            for (uint16_t x = 0; x < TILE_WIDTH; x++) {
                if (x + xOffset < imageWidth) {
                    uint32_t pixelIndex = y * imageWidth + x + xOffset;
                    lineBuffer[x] = imageArray[pixelIndex];
                } else {
                    lineBuffer[x] = 0; // Black for out of bounds
                }
            }
            
            _tft.pushColors(lineBuffer, TILE_WIDTH);
        }
        
        deselectAll();
        delay(5);
    }
}

void DisplayTileDriver::displayImagePROGMEM(const uint16_t* imageArray, uint16_t imageWidth, uint16_t imageHeight) {
    uint16_t lineBuffer[TILE_WIDTH];
    
    for (uint8_t tileIdx = 0; tileIdx < NUM_TILES; tileIdx++) {
        selectTile(tileIdx + 1);
        
        _tft.setAddrWindow(0, 0, TILE_WIDTH, TILE_HEIGHT);
        
        for (uint16_t y = 0; y < TILE_HEIGHT && y < imageHeight; y++) {
            uint16_t xOffset = tileIdx * TILE_WIDTH;
            
            // Read from PROGMEM into line buffer
            for (uint16_t x = 0; x < TILE_WIDTH; x++) {
                if (x + xOffset < imageWidth) {
                    uint32_t pixelIndex = y * imageWidth + x + xOffset;
                    lineBuffer[x] = pgm_read_word(&imageArray[pixelIndex]);
                } else {
                    lineBuffer[x] = 0; // Black for out of bounds
                }
            }
            
            _tft.pushColors(lineBuffer, TILE_WIDTH);
        }
        
        deselectAll();
        delay(5);
    }
}

void DisplayTileDriver::clearAll(uint16_t color) {
    for (uint8_t i = 1; i <= NUM_TILES; i++) {
        selectTile(i);
        _tft.fillScreen(color);
        deselectAll();
    }
}

void DisplayTileDriver::setRotation(uint8_t rotation) {
    _rotation = rotation;
    for (uint8_t i = 1; i <= NUM_TILES; i++) {
        selectTile(i);
        _tft.setRotation(_rotation);
        deselectAll();
    }
}

void DisplayTileDriver::displayTestPattern() {
    for (uint8_t i = 1; i <= NUM_TILES; i++) {
        selectTile(i);
        
        // Different color for each tile
        uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW};
        _tft.fillScreen(colors[i - 1]);
        
        // Display tile number
        _tft.setCursor(30, 100);
        _tft.setTextColor(TFT_WHITE);
        _tft.setTextSize(5);
        _tft.printf("Tile %d", i);
        
        deselectAll();
    }
}

TFT_eSPI& DisplayTileDriver::getTFT() {
    return _tft;
}
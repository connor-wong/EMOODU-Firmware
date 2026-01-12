#ifndef DISPLAY_TILE_DRIVER_H
#define DISPLAY_TILE_DRIVER_H

#include <TFT_eSPI.h>
#include <Arduino.h>

// Image Array
#include <image_240x240.h>

// Display configuration
#define TILE_WIDTH 240
#define TILE_HEIGHT 240
#define NUM_TILES 4
#define TOTAL_DISPLAY_WIDTH (TILE_WIDTH * NUM_TILES)  // 960
#define TOTAL_DISPLAY_HEIGHT TILE_HEIGHT               // 240

/* 
ST7789 Display Module Pins
- TFT_MOSI 1  // SDA
- TFT_SCLK 2  // SCL
- TFT_CS   -1  
- TFT_DC   9
- TFT_RST  -1  // Or -1 if tied to 3.3V
*/

class DisplayTileDriver {
    public:
        // Constructor
        DisplayTileDriver(uint8_t cs1, uint8_t cs2, uint8_t cs3, uint8_t cs4, uint8_t dummy = 38);
        
        // Initialize all displays
        void begin();
        
        // Display a full-width image (960x240) across all tiles
        void displayImage(const uint16_t* imageArray, uint16_t imageWidth, uint16_t imageHeight);
        
        // Display image from PROGMEM
        void displayImagePROGMEM(const uint16_t* imageArray, uint16_t imageWidth, uint16_t imageHeight);
        
        // Clear all displays
        void clearAll(uint16_t color = TFT_BLACK);
        
        // Select a specific tile (1-4) for direct drawing
        void selectTile(uint8_t tileNum);
        
        // Deselect all tiles
        void deselectAll();
        
        // Get access to TFT_eSPI object for advanced operations
        TFT_eSPI& getTFT();
        
        // Set rotation for all displays
        void setRotation(uint8_t rotation);
        
        // Display test pattern to identify each tile
        void displayTestPattern();
        
    private:
        uint8_t _cs1, _cs2, _cs3, _cs4, _dummy;
        TFT_eSPI _tft;
        uint8_t _rotation;
        
        // Internal helper to get CS pin for a tile number
        uint8_t getTileCSPin(uint8_t tileNum);
};

#endif // DISPLAY_TILE_DRIVER_H
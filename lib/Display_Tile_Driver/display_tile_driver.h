#ifndef DISPLAY_TILE_DRIVER_H
#define DISPLAY_TILE_DRIVER_H


// Image Array
#include <image_240x240.h>


/* 

ST7789 Display Pins

- TFT_MOSI 6  // SDA
- TFT_SCLK 7  // SCL
- TFT_CS   8   // Or -1 if no CS
- TFT_DC   9
- TFT_RST  10  // Or -1 if tied to 3.3V
- TFT_BL   -1  // If tied to 3.3V

*/

#define TILE_CS_1   4
#define TILE_CS_2   5
#define TILE_CS_3   6
#define TILE_CS_4   7

#define TILE_DC     15
#define TILE_RES    -1 // Unused
#define TILE_SDA   35  //  11 MOSI
#define TILE_SCL   36  //  12 CLK

void display_tile_setup(void);
// void single_tile_test (Adafruit_ST7735 &tft);
// void dual_tile_test (void);

#endif
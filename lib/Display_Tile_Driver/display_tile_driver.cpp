#include <display_tile_driver.h>

Adafruit_ST7735 tft1 = Adafruit_ST7735(TILE_CS_1, TILE_DC, TILE_SDA, TILE_SCL, TILE_RES);
Adafruit_ST7735 tft2 = Adafruit_ST7735(TILE_CS_2, TILE_DC, TILE_SDA, TILE_SCL, TILE_RES);
Adafruit_ST7735 tft3 = Adafruit_ST7735(TILE_CS_3, TILE_DC, TILE_SDA, TILE_SCL, TILE_RES);
Adafruit_ST7735 tft4 = Adafruit_ST7735(TILE_CS_4, TILE_DC, TILE_SDA, TILE_SCL, TILE_RES);

const uint16_t* const IMAGES[] = {
  image_256x256_playground,
  image_256x256_food,
  image_256x256_singer,
  image_256x256_dog
};

void display_tile_setup(void) {
  display_tile_init(tft1);
  display_tile_init(tft2);
  display_tile_init(tft3);
  display_tile_init(tft4);
}

void display_tile_init(Adafruit_ST7735 &tft) {
  tft.initR(INITR_144GREENTAB); // for 1.44" ST7735 (128x128)
  tft.setRotation(0);            
  tft.fillScreen(ST77XX_BLACK);
}


/*

For testing purpose

void single_tile_test (Adafruit_ST7735 &tft) {
  const int w = 128, h = 128;

  tft.initR(INITR_144GREENTAB);
  tft.fillScreen(ST77XX_BLACK);

  drawImage(tft, 0, 0, w, h, 0, 0, image_128x128, IMG_WIDTH);
}

void dual_tile_test (void) {
  const int w = 128, h = 128;

  display_tile_init(tft1);
  display_tile_init(tft2);
  display_tile_init(tft3);
  display_tile_init(tft4);

  // Top Left: src window (0,0) → (128x128), draw at (0,0)
  drawImage(tft1, 0, 0, w, h, 0,   0, image_256x128, IMG_WIDTH);

  // Top Right: src window (128,0) → (128x128), draw at (0,0)
  drawImage(tft2, 0, 0, w, h, 128, 0, image_256x128, IMG_WIDTH);

  // Bottom Left: src window (0,0) → (128x128), draw at (0,0)
  drawImage(tft3, 0, 0, w, h, 0,   0, image_256x128, IMG_WIDTH);

  // Bottom Right: src window (128,0) → (128x128), draw at (0,0)
  drawImage(tft4, 0, 0, w, h, 128, 0, image_256x128, IMG_WIDTH);
}

*/

void full_tile_test (void) {
  const int w = 128, h = 128;
  
  // display_tile_init(tft1);
  // display_tile_init(tft2);
  // display_tile_init(tft3);
  // display_tile_init(tft4);

  // 1: src window (0, 0) → (128x128), draw at (0,0)
  drawImage(tft1, 0, 0, w, h, 0, 0, image_256x256_dog, IMG_WIDTH);

  // 2: src window (128, 0) → (128x128), draw at (0,0)
  drawImage(tft2, 0, 0, w, h, 128, 0, image_256x256_dog, IMG_WIDTH);

  // 3: src window (0, 128) → (128x128), draw at (0,0)
  drawImage(tft3, 0, 0, w, h, 0, 128, image_256x256_dog, IMG_WIDTH);

  // 4: src window (128, 128) → (128x128), draw at (0,0)
  drawImage(tft4, 0, 0, w, h, 128, 128, image_256x256_dog, IMG_WIDTH);
}

void drawAll(uint8_t index) {
  const uint16_t* src = IMAGES[index];
  const int W = 128, H = 128;

  drawImage(tft1, 0, 0, W, H,   0,   0, src, IMG_WIDTH);
  drawImage(tft2, 0, 0, W, H, 128,   0, src, IMG_WIDTH);
  drawImage(tft3, 0, 0, W, H,   0, 128, src, IMG_WIDTH);
  drawImage(tft4, 0, 0, W, H, 128, 128, src, IMG_WIDTH);
}

static void drawImage(Adafruit_ST7735 &tft, int16_t dstX, int16_t dstY, int16_t w, int16_t h, int16_t srcX, int16_t srcY, const uint16_t *src, int16_t srcStride) {
  uint16_t line[128];  // buffer for one row

  tft.startWrite();

  for (int16_t row = 0; row < h; row++) {
    // Read one line from PROGMEM
    for (int16_t col = 0; col < w; col++) {
      line[col] = pgm_read_word(&src[(srcY + row) * srcStride + (srcX + col)]);
    }

    tft.setAddrWindow(dstX, dstY + row, w, 1);
    tft.writePixels(line, w);
  }
  tft.endWrite();
}
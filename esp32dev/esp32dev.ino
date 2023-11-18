//
// intended for: ESP32-2432S028R
//    ESP32 Arduino LVGL WIFI & Bluetooth Development Board 2.8"
//    240 * 320 Smart Display Screen 2.8 inch LCD TFT Module With Touch WROOM
//
//          from: http://www.jczn1688.com/
//  purchased at: https://www.aliexpress.com/item/1005004502250619.html
//     resources: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display
//
//                    Arduino IDE 2.2.1
// additional boards:
// https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
//    install boards: esp32 by Espressif 2.0.14
//   install library: TFT_eSPI by Bodmer 2.5.31
//                    XPT2046_Touchscreen by Paul Stoffregen 1.4.0
//     setup library: replace User_Setup.h in libraries/TFT_eSPI/ with provided
//                    file
//
//   Arduino IDE, Tools menu and select:
//                                Board: ESP32 Dev Module
//                        CPU Frequency: 240MHz (WiFi/BT)
//                     Core Debug Level: None
// Erase All Flash Before Sketch Upload: Disabled
//                       Events Runs On: Core 1
//                      Flash Frequency: 80MHz
//                           Flash Mode: DIO
//                           Flash Size: 4MB (32Mb)
//                         JTAG Adapter: Disabled
//                      Arduino Runs On: Core 1
//                     Partition Scheme: Default 4MB with spiffs (1.2MB
//                                       APP/1.5MB SPIFFS)
//                                PSRAM: Disabled
//                         Upload Speed: 921600
//                           Programmer: Esptool
//
//  Visual Code:
//    * install plug-in Arduino v0.6.0
//    * task ">Arduino: Initialize"
//    * task ">Arduino: Board Manager"
//        install "esp32 by Espressif Systems Version 2.0.14"
//    * task ">Arduino: Board Config"
//        select "ESP32 Dev Module (esp32)"
//    * install libraries (plug-in installs libraries in ~/Arduino/libraries)

#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// #define USE_WIFI
#ifdef USE_WIFI
#include "WiFi.h"
#include "secrets.h"
#endif

// ldr (light dependant resistor)
// analog read of pin gives: 0 for full brightness, higher values is darker
#define LDR_PIN 34

// rgb led
#define CYD_LED_BLUE 17
#define CYD_LED_RED 4
#define CYD_LED_GREEN 16

// setting up screen and touch from:
// https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/Examples/Basics/2-TouchTest/2-TouchTest.ino
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32 // Master Out Slave In
#define XPT2046_MISO 39 // Master In Slave Out
#define XPT2046_SCK 25  // Serial Clock
#define XPT2046_SS 33   // Slave Select

static SPIClass spi{HSPI};
static XPT2046_Touchscreen ts{XPT2046_SS, XPT2046_IRQ};
static TFT_eSPI tft{};

class fps {
  unsigned interval_ms_ = 5000;
  unsigned frames_rendered_in_interval_ = 0;
  unsigned long last_update_ms_ = 0;
  unsigned long now_ms_ = 0;
  unsigned long prv_now_ms_ = 0;
  unsigned current_fps_ = 0;
  float dt_s_ = 0;

public:
  void init(const unsigned long now_ms) {
    last_update_ms_ = prv_now_ms_ = now_ms;
  }

  auto on_frame(const unsigned long now_ms) -> bool {
    now_ms_ = now_ms;
    dt_s_ = (now_ms - prv_now_ms_) / 1000.0f;
    prv_now_ms_ = now_ms;
    frames_rendered_in_interval_++;
    const unsigned long dt_ms = now_ms - last_update_ms_;
    if (dt_ms >= interval_ms_) {
      current_fps_ = frames_rendered_in_interval_ * 1000 / dt_ms;
      frames_rendered_in_interval_ = 0;
      last_update_ms_ = now_ms;
      return true;
    }
    return false;
  }

  inline auto get() -> unsigned { return current_fps_; }

  inline auto now_ms() -> unsigned long { return now_ms_; }

  inline auto dt_s() -> float { return dt_s_; }

} static fps{};

// palette used to convert uint8_t to uint16_t rgb 565
// lower and higher byte swapped (red being the highest bits)
static constexpr uint16_t palette[256]{
    0b0000000000000000, // black
    0b0000000011111000, // red
    0b1110000000000111, // green
    0b0001111100000000, // blue
    0b1111111111111111, // white
};

static constexpr unsigned frame_width = 320;
static constexpr unsigned frame_height = 240;

static constexpr unsigned tile_width = 16;
static constexpr unsigned tile_height = 16;

// the right shift of 'x' to get the x in tiles map
static constexpr unsigned tile_width_shift = 4;

// the bits that are the partial tile position between 0 and not including
// 'tile_width'
static constexpr unsigned tile_width_and = 15;

// the right shift of 'y' to get the y in tiles map
static constexpr unsigned tile_height_shift = 4;

// the bits that are the partial tile position between 0 and not including
// 'tile_height'
static constexpr unsigned tile_height_and = 15;

static constexpr unsigned tile_count = 256;
using tile_ix = uint8_t;

struct tile {
  const uint8_t data[tile_width * tile_height];
} static constexpr tiles[tile_count]{
#include "tiles.h"
};

static constexpr unsigned tiles_map_width = 320;
static constexpr unsigned tiles_map_height = 17;

struct tiles_map {
  tile_ix cell[tiles_map_height][tiles_map_width];
} static constexpr tiles_map{{
#include "tiles_map.h"
}};

static constexpr unsigned sprite_width = 16;
static constexpr unsigned sprite_height = 16;
static constexpr unsigned sprite_count = 256;
using sprite_ix = uint8_t;

// used when rendering
static constexpr int16_t sprite_width_neg = -int16_t(sprite_width);

static constexpr uint8_t sprite1_data[sprite_width * sprite_height]{
    // clang-format off
  0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,
    // clang-format on
};

struct sprite {
  float x;
  float y;
  float dx;
  float dy;
  const uint8_t *data;
  int16_t scr_x;
  int16_t scr_y;
  sprite_ix collision_with;
} static sprites[sprite_count];

// pixel precision collision detection between sprites on screen
static sprite_ix collision_map[frame_height][frame_width];

// buffers for rendering a chunk while the other is transferred to the screen
// using DMA. allocated in setup.
static uint16_t *dma_buf_1;
static uint16_t *dma_buf_2;
static constexpr unsigned dma_buf_size =
    sizeof(uint16_t) * frame_width * tile_height;

// scrolling from right to left / down up
static float x = tiles_map_width * tile_width - frame_width;
static float dx_per_s = -16;
static float y = 1;
static float dy_per_s = 1;

static void render_scanline(uint16_t *render_buf_ptr, const int16_t scanline_y,
                            const unsigned tile_x, const unsigned tile_dx,
                            const unsigned tile_width_minus_dx,
                            const tile_ix *tiles_map_row_ptr,
                            const unsigned tile_sub_y,
                            const unsigned tile_sub_y_times_tile_width) {
  // used later by sprite renderer to overwrite tiles pixels
  uint16_t *scanline_ptr = render_buf_ptr;

  if (tile_width_minus_dx) {
    // render first partial tile
    const tile_ix tile_index = *(tiles_map_row_ptr + tile_x);
    const uint8_t *tile_data_ptr =
        tiles[tile_index].data + tile_sub_y_times_tile_width + tile_dx;
    for (unsigned i = tile_dx; i < tile_width; i++) {
      *render_buf_ptr++ = palette[*tile_data_ptr++];
    }
  }
  // render full tiles
  const unsigned tx_max = tile_x + (frame_width / tile_width);
  for (unsigned tx = tile_x + 1; tx < tx_max; tx++) {
    const tile_ix tile_index = *(tiles_map_row_ptr + tx);
    const uint8_t *tile_data_ptr =
        tiles[tile_index].data + tile_sub_y_times_tile_width;
    for (unsigned i = 0; i < tile_width; i++) {
      *render_buf_ptr++ = palette[*tile_data_ptr++];
    }
  }
  if (tile_dx) {
    // render last partial tile
    const tile_ix tile_index = *(tiles_map_row_ptr + tx_max);
    const uint8_t *tile_data_ptr =
        tiles[tile_index].data + tile_sub_y_times_tile_width;
    for (unsigned i = 0; i < tile_dx; i++) {
      *render_buf_ptr++ = palette[*tile_data_ptr++];
    }
  }

  // render sprites
  // note. although grossly inefficient algorithm the DMA is busy while
  // rendering one tile height of sprites and tiles. core 0 will do graphics
  // and core 1 will do game logic.

  // i not from 0 because sprite[0] is unused and represents 'no sprite'
  // in collision map
  sprite *spr = &sprites[1];
  for (unsigned i = 1; i < sprite_count; i++, spr++) {
    if (spr->scr_y > scanline_y or
        spr->scr_y + int16_t(sprite_height) <= scanline_y or
        spr->scr_x <= sprite_width_neg or spr->scr_x > int16_t(frame_width) or
        spr->data == nullptr) {
      // not within scan line or
      // is outside the screen x-wise or
      // sprite has no data
      // Serial.printf("skipped sprite %d\n", i);
      continue;
    }
    const uint8_t *spr_data_ptr =
        spr->data + (scanline_y - spr->scr_y) * sprite_width;
    uint16_t *scanline_dst_ptr = scanline_ptr + spr->scr_x;
    unsigned render_width = sprite_width;
    sprite_ix *collision_pixel = &collision_map[scanline_y][spr->scr_x];
    if (spr->scr_x < 0) {
      // adjustment if x is negative
      spr_data_ptr -= spr->scr_x;
      scanline_dst_ptr -= spr->scr_x;
      render_width = sprite_width + spr->scr_x;
      collision_pixel -= spr->scr_x;
    } else if (spr->scr_x + sprite_width > frame_width) {
      // adjustment if sprite partially outside screen (x-wise)
      render_width = frame_width - spr->scr_x;
    }
    // render line of sprite
    for (unsigned j = 0; j < render_width; j++) {
      if (*collision_pixel) {
        // collision
        spr->collision_with = *collision_pixel;
        sprites[*collision_pixel].collision_with = i;
      }
      // set pixel collision value to sprite index
      *collision_pixel++ = i;
      // write pixel from sprite data or skip if 0
      const uint8_t color_ix = *spr_data_ptr++;
      if (color_ix) {
        *scanline_dst_ptr++ = palette[color_ix];
      } else {
        scanline_dst_ptr++;
      }
    }
  }
}

// one tile height buffer, palette, 8-bit tiles from tiles map, 8-bit sprites
// 31 fps
static void render(const unsigned x, const unsigned y) {
  const unsigned tile_x = x >> tile_width_shift;
  const unsigned tile_dx = x & tile_width_and;
  const unsigned tile_width_minus_dx = tile_width - tile_dx;
  unsigned tile_y = y >> tile_height_shift;
  const unsigned tile_dy = y & tile_height_and;
  const unsigned tile_y_max = tile_y + (frame_height / tile_height);
  const unsigned tile_height_minus_dy = tile_height - tile_dy;

  // selects buffer to write while DMA reads the other buffer
  bool dma_buf_use_first = true;
  // y in frame for current tiles row to be copied by DMA
  unsigned frame_y = 0;
  // current line y on screen
  int16_t scanline_y = 0;
  // pointer to start of current row of tiles
  const tile_ix *tiles_map_row_ptr = tiles_map.cell[tile_y];
  if (tile_dy) {
    // render the partial top tile
    // swap between two rendering buffers to not overwrite DMA accessed buffer
    uint16_t *render_buf_ptr = dma_buf_use_first ? dma_buf_1 : dma_buf_2;
    dma_buf_use_first = not dma_buf_use_first;
    // pointer to the buffer that the DMA will copy to screen
    uint16_t *dma_buf = render_buf_ptr;
    // render scanlines of first partial tile
    for (unsigned tile_sub_y = tile_dy,
                  tile_sub_y_times_tile_width = tile_dy * tile_width;
         tile_sub_y < tile_height;
         tile_sub_y++, tile_sub_y_times_tile_width += tile_width,
                  render_buf_ptr += frame_width) {

      render_scanline(render_buf_ptr, scanline_y, tile_x, tile_dx,
                      tile_width_minus_dx, tiles_map_row_ptr, tile_sub_y,
                      tile_sub_y_times_tile_width);
      scanline_y++;
    }
    tft.setAddrWindow(0, frame_y, frame_width, tile_height_minus_dy);
    tft.pushPixelsDMA(dma_buf, frame_width * tile_height_minus_dy);
    tile_y++;
    frame_y += tile_height_minus_dy;
  }
  // for each row of full tiles
  for (; tile_y < tile_y_max;
       tile_y++, tiles_map_row_ptr += tiles_map_width, frame_y += tile_height) {
    // swap between two rendering buffers to not overwrite DMA accessed buffer
    uint16_t *render_buf_ptr = dma_buf_use_first ? dma_buf_1 : dma_buf_2;
    dma_buf_use_first = not dma_buf_use_first;
    // pointer to the buffer that the DMA will copy to screen
    uint16_t *dma_buf = render_buf_ptr;
    // render one tile height of pixels from tiles map and sprites to the
    // 'render_buf_ptr'
    for (unsigned tile_sub_y = 0, tile_sub_y_times_tile_width = 0;
         tile_sub_y < tile_height;
         tile_sub_y++, tile_sub_y_times_tile_width += tile_height,
                  render_buf_ptr += frame_width) {

      render_scanline(render_buf_ptr, scanline_y, tile_x, tile_dx,
                      tile_width_minus_dx, tiles_map_row_ptr, tile_sub_y,
                      tile_sub_y_times_tile_width);
      scanline_y++;
    }
    tft.setAddrWindow(0, frame_y, frame_width, tile_height);
    tft.pushPixelsDMA(dma_buf, frame_width * tile_height);
  }
  if (tile_dy) {
    // render last partial tile
    // swap between two rendering buffers to not overwrite DMA accessed buffer
    uint16_t *render_buf_ptr = dma_buf_use_first ? dma_buf_1 : dma_buf_2;
    dma_buf_use_first = not dma_buf_use_first;
    // pointer to the buffer that the DMA will copy to screen
    uint16_t *dma_buf = render_buf_ptr;
    // render the partial last tile row
    for (unsigned tile_sub_y = 0, tile_sub_y_times_tile_width = 0;
         tile_sub_y < tile_dy;
         tile_sub_y++, tile_sub_y_times_tile_width += tile_width,
                  render_buf_ptr += frame_width) {

      render_scanline(render_buf_ptr, scanline_y, tile_x, tile_dx,
                      tile_width_minus_dx, tiles_map_row_ptr, tile_sub_y,
                      tile_sub_y_times_tile_width);
      scanline_y++;
    }
    tft.setAddrWindow(0, frame_y, frame_width, tile_dy);
    tft.pushPixelsDMA(dma_buf, frame_width * tile_dy);
  }
}

void setup(void) {
  Serial.begin(115200);
  sleep(1); // arbitrary wait 1 second for serial to connect
  while (!Serial)
    ; // wait for serial port to connect. needed for native usb port only
  Serial.printf("\n------------------- info -----------------------------------"
                "------------------\n");
  Serial.printf("        chip model: %s\n", ESP.getChipModel());
  Serial.printf("            screen: %d x %d px\n", frame_width, frame_height);
  Serial.printf("     free heap mem: %d B\n", ESP.getFreeHeap());
  Serial.printf("largest free block: %d B\n", ESP.getMaxAllocHeap());
#ifdef USE_WIFI
  Serial.printf("using WiFi\n");
#endif

  // allocate rendering buffers
  dma_buf_1 = (uint16_t *)malloc(dma_buf_size);
  dma_buf_2 = (uint16_t *)malloc(dma_buf_size);
  if (dma_buf_1 == nullptr or dma_buf_2 == nullptr) {
    Serial.printf("!!! could not allocate DMA buffers");
    while (true) {
      sleep(60);
    }
  }

  Serial.printf("------------------- after init -------------------------------"
                "----------------\n");
  Serial.printf("          free mem: %zu B\n", ESP.getFreeHeap());
  Serial.printf("largest free block: %zu B\n", ESP.getMaxAllocHeap());
  Serial.printf("------------------- buffers ----------------------------------"
                "----------------\n");
  Serial.printf("   DMA buf 1 and 2: %zu B\n", 2 * dma_buf_size);
  Serial.printf("           sprites: %zu B\n", sizeof(sprites));
  Serial.printf("     collision_map: %zu B\n", sizeof(collision_map));
  Serial.printf("         tiles_map: %zu B\n", sizeof(tiles_map));
  Serial.printf("             tiles: %zu B\n", sizeof(tiles));
  Serial.printf("------------------- instances --------------------------------"
                "----------------\n");
  Serial.printf("            sprite: %zu B\n", sizeof(sprite));
  Serial.printf("      sprite image: %zu B\n", sizeof(sprite1_data));
  Serial.printf("              tile: %zu B\n", sizeof(tile));
  Serial.printf("--------------------------------------------------------------"
                "----------------\n");

  // setup rgb led
  pinMode(CYD_LED_RED, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE, OUTPUT);

  // set rgb led to green
  digitalWrite(CYD_LED_RED, HIGH);
  digitalWrite(CYD_LED_GREEN, LOW);
  digitalWrite(CYD_LED_BLUE, HIGH);

  // setup ldr
  pinMode(LDR_PIN, INPUT);

  // start the SPI for the touch screen and init the TS library
  spi.begin(XPT2046_SCK, XPT2046_MISO, XPT2046_MOSI, XPT2046_SS);
  ts.begin(spi);
  ts.setRotation(1);

  tft.init();
  tft.setRotation(1);
  tft.initDMA(true);

#ifdef USE_WIFI
  WiFi.begin(SECRET_WIFI_NETWORK, SECRET_WIFI_PASSWORD);
  WiFi.setAutoReconnect(true);
  Serial.print("Connecting to ");
  Serial.print(SECRET_WIFI_NETWORK);
  Serial.flush();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    sleep(1);
  }
  Serial.print("\nConnected\nIP: ");
  Serial.println(WiFi.localIP());
#endif

  // set random seed to get same random every time
  randomSeed(0);

  // initiate sprites
  {
    float spr_x = -24, spr_y = -24;
    // sprite[0] is unused / reserved. start from sprite[1]
    sprite *spr = &sprites[1];
    for (unsigned i = 1; i < sprite_count; i++, spr++) {
      spr->data = sprite1_data;
      spr->x = spr_x;
      spr->y = spr_y;
      spr->dx = 0.5f;
      spr->dy = 2.0f - float(rand() % 4);
      spr_x += 24;
      if (spr_x > (frame_width + sprite_width)) {
        spr_x = -24;
        spr_y += 24;
      }
    }
  }

  fps.init(millis());
}

void loop() {
  // frames per second update
  if (fps.on_frame(millis())) {
    Serial.printf("t=%lu  fps=%u  ldr=%u\n", fps.now_ms(), fps.get(),
                  analogRead(LDR_PIN));
  }

  // check touch screen
  if (ts.tirqTouched() and ts.touched()) {
    const TS_Point pt = ts.getPoint();
    const int x_relative_center = pt.x - 4096 / 2;
    constexpr float dx_factor = 200.0f / (4096 / 2);
    dx_per_s = dx_factor * x_relative_center;
    Serial.printf("touch x=%d  y=%d  z=%d  dx=%f\n", pt.x, pt.y, pt.z,
                  dx_per_s);
  }

  // update physics (todo. do on other core)
  {
    const float dt_s = fps.dt_s();
    sprite *spr = &sprites[1]; // 1 because sprite[0] is reserved
    for (unsigned i = 1; i < sprite_count; i++, spr++) {
      // update physics
      spr->x += spr->dx * dt_s;
      spr->y += spr->dy * dt_s;
      // set rendering info
      spr->scr_x = int16_t(spr->x);
      spr->scr_y = int16_t(spr->y);
      // clear collision info
      spr->collision_with = 0;
    }
  }

  // clear collisions map
  memset(collision_map, 0, sizeof(collision_map));

  // render tiles, sprites and collision map
  tft.startWrite();
  render(unsigned(x), unsigned(y));
  tft.endWrite();

  // handle collisions
  {
    sprite *spr = &sprites[1]; // 1 because sprite[0] is reserved
    for (unsigned i = 1; i < sprite_count; i++, spr++) {
      if (spr->collision_with) {
        Serial.printf("sprite %d collision with %d\n", i, spr->collision_with);
        spr->data = nullptr; // hide sprite
      }
    }
  }

  // update x position in pixels in the tile map
  x += dx_per_s * fps.dt_s();
  if (x < 0) {
    x = 0;
    dx_per_s = -dx_per_s;
  } else if (x > (tiles_map_width * tile_width - frame_width)) {
    x = tiles_map_width * tile_width - frame_width;
    dx_per_s = -dx_per_s;
  }
  // update y position in pixels in the tile map
  y += dy_per_s * fps.dt_s();
  // Serial.printf("x=%f  y=%f  dy=%f\n", x, y, dy_per_s);
  if (y < 0) {
    Serial.printf("y < 0\n");
    y = 0;
    dy_per_s = -dy_per_s;
  } else if (y > (tiles_map_height * tile_height - frame_height)) {
    y = tiles_map_height * tile_height - frame_height;
    dy_per_s = -dy_per_s;
  }
}

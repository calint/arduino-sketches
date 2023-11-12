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
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

static SPIClass spi{HSPI};
static XPT2046_Touchscreen ts{XPT2046_CS, XPT2046_IRQ};
static TFT_eSPI tft{};

// #define USE_WIFI

#ifdef USE_WIFI
#include "WiFi.h"
#include "secrets.h"
#endif

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

static constexpr unsigned frame_width = 320;
static constexpr unsigned frame_height = 240;

static constexpr unsigned tile_width = 8;
static constexpr unsigned tile_height = 8;

// the right shift of 'x' to get the x in tiles map
static constexpr unsigned tile_width_shift = 3;

// the bits that are the partial tile position between 0 and not including
// 'tile_width'
static constexpr unsigned tile_width_and = 7;

struct tile {
  const uint8_t data[tile_width * tile_height];
} static constexpr tiles[]{
#include "tiles.h"
};

static constexpr unsigned tiles_map_width = 80;
static constexpr unsigned tiles_map_height = 30;
struct tiles_map {
  uint8_t cell[tiles_map_height][tiles_map_width];
} static constexpr tiles_map{{
#include "tiles_map.h"
}};

static constexpr uint16_t palette[256]{
    0b0000000000000000, // black
    0b0000000000011111, // green
    0b0000011111100000, // red
    0b0000011111111111, // yellow
    0b1111100000000000, // blue
    0b1111100000011111, // cyan
    0b1111111111100000, // magenta
    0b1111111111111111, // white
};

static constexpr unsigned sprite_width = 8;
static constexpr unsigned sprite_height = 8;
static constexpr unsigned sprite_count = 1024;

// clang-format off
uint8_t sprite1_data[]{
  0,0,2,2,2,2,0,0,
  0,2,2,2,2,2,2,0,
  2,2,2,2,2,2,2,2,
  2,2,2,0,0,2,2,2,
  2,2,2,0,0,2,2,2,
  2,2,2,2,2,2,2,2,
  0,2,2,2,2,2,2,0,
  0,0,2,2,2,2,0,0,
};
// clang-format on

// clang-format off
uint8_t sprite2_data[]{
  0,3,3,3,3,3,3,0,
  3,2,2,2,2,2,2,3,
  3,2,2,2,2,2,2,3,
  3,2,2,0,0,2,2,3,
  3,2,2,0,0,2,2,3,
  3,2,2,2,2,2,2,3,
  3,2,2,2,2,2,2,3,
  0,3,3,3,3,3,3,0,
};
// clang-format on

struct sprite {
  uint8_t *data;
  int16_t x;
  int16_t y;
} static sprites[sprite_count];

void setup(void) {
  Serial.begin(115200);
  sleep(1); // arbitrary wait 1 second for serial to connect
  while (!Serial)
    ; // wait for serial port to connect. needed for native usb port only
  Serial.printf("\n------------------------------------------------------------"
                "------------------\n");
  Serial.printf("        chip model: %s\n", ESP.getChipModel());
  Serial.printf("            screen: %d x %d px\n", frame_width, frame_height);
  Serial.printf("largest free block: %d B\n", ESP.getMaxAllocHeap());
#ifdef USE_WIFI
  Serial.printf("using WiFi\n");
#endif
  Serial.printf("--------------------------------------------------------------"
                "----------------\n");

  // setup rgb led
  pinMode(CYD_LED_RED, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE, OUTPUT);

  // setup ldr
  pinMode(LDR_PIN, INPUT);

  // Start the SPI for the touch screen and init the TS library
  spi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
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

  fps.init(millis());

  // set rgb led to green
  digitalWrite(CYD_LED_RED, HIGH);
  digitalWrite(CYD_LED_GREEN, LOW);
  digitalWrite(CYD_LED_BLUE, HIGH);

  // initiate sprites
  unsigned i = 0;
  for (unsigned x = 0; x < 32; x++) {
    for (unsigned y = 0; y < 16; y++) {
      sprite *spr = &sprites[i++];
      spr->x = x * 10;
      spr->y = y * 10;
      spr->data = sprite1_data;
    }
  }
  for (unsigned x = 0; x < 32; x++) {
    for (unsigned y = 0; y < 16; y++) {
      sprite *spr = &sprites[i++];
      spr->x = x * 10;
      spr->y = y * 10;
      spr->data = sprite2_data;
    }
  }
  // for (unsigned x = 0; x < 32; x++) {
  //   for (unsigned y = 0; y < 16; y++) {
  //     sprite *spr = &sprites[i++];
  //     spr->x = x * 10;
  //     spr->y = y * 10;
  //     spr->data = sprite1_data;
  //   }
  // }
  // for (unsigned x = 0; x < 32; x++) {
  //   for (unsigned y = 0; y < 16; y++) {
  //     sprite *spr = &sprites[i++];
  //     spr->x = x * 10;
  //     spr->y = y * 10;
  //     spr->data = sprite2_data;
  //   }
  // }
}

// one tile height buffer, palette, 8-bit tiles from tiles map
// 31 fps
static void tiles_map_render(const unsigned x) {
  static uint16_t line_buf_1[frame_width * tile_height];
  static uint16_t line_buf_2[frame_width * tile_height];

  const unsigned tile_x = x >> tile_width_shift;
  const unsigned tile_dx = x & tile_width_and;
  const unsigned tile_width_minus_dx = tile_width - tile_dx;

  // selects buffer to write while DMA reads the other buffer
  bool line_buf_first = true;
  // pointer to start of current row of tiles
  const uint8_t *tiles_map_row_ptr = tiles_map.cell[0];
  // y in frame for current tiles row
  unsigned frame_y = 0;
  int spr_width_neg = -(int)sprite_width;
  // for each row of tiles
  for (unsigned tile_y = 0; tile_y < tiles_map_height;
       tile_y++, tiles_map_row_ptr += tiles_map_width, frame_y += tile_height) {
    // swap between two line buffers to not overwrite DMA accessed buffer
    uint16_t *line_buf_ptr = line_buf_first ? line_buf_1 : line_buf_2;
    uint16_t *line_buf_ptr_dma = line_buf_ptr;
    line_buf_first = not line_buf_first;
    memset(line_buf_ptr, 0, frame_width * tile_height);
    // render one tile height of data to the 'line_buf_ptr' starting and ending
    // with possible partial tiles
    for (unsigned ty = 0, ty_times_tile_height = 0; ty < tile_height;
         ty++, ty_times_tile_height += tile_height) {
      if (tile_width_minus_dx) {
        // render first partial tile
        const uint8_t tile_ix = *(tiles_map_row_ptr + tile_x);
        const uint8_t *tile_data_ptr =
            tiles[tile_ix].data + ty_times_tile_height + tile_dx;
        for (unsigned i = tile_dx; i < tile_width; i++) {
          *line_buf_ptr++ = palette[*tile_data_ptr++];
        }
      }
      // render full tiles
      const unsigned tx_max = tile_x + (frame_width / tile_width);
      for (unsigned tx = tile_x + 1; tx < tx_max; tx++) {
        const uint8_t tile_ix = *(tiles_map_row_ptr + tx);
        const uint8_t *tile_data_ptr =
            tiles[tile_ix].data + ty_times_tile_height;
        for (unsigned i = 0; i < tile_width; i++) {
          *line_buf_ptr++ = palette[*tile_data_ptr++];
        }
      }
      if (tile_dx) {
        // render last partial tile
        const uint8_t tile_ix = *(tiles_map_row_ptr + tx_max);
        const uint8_t *tile_data_ptr =
            tiles[tile_ix].data + ty_times_tile_height;
        for (unsigned i = 0; i < tile_dx; i++) {
          *line_buf_ptr++ = palette[*tile_data_ptr++];
        }
      }

      // render sprites
      // although grossly inefficient method the DMA is busy while rendering
      // 1024 sprites and tiles. core 0 will do graphics and core 1 will do game
      // logic
      const unsigned y = frame_y + ty;
      for (unsigned i = 0; i < sizeof(sprites) / sizeof(struct sprite); i++) {
        const sprite *spr = &sprites[i];
        // spr->x > spr_width_neg and spr->x < frame_width and
        if (spr->y <= y and (spr->y + sprite_height) > y) {
          const unsigned sprite_data_row_num = y - spr->y;
          uint16_t *px_ptr = line_buf_ptr_dma + frame_width * ty + spr->x;
          uint8_t *spr_data_ptr =
              spr->data + sprite_data_row_num * sprite_width;
          // Serial.printf("spr.y=%d  y=%d  i=%d  data=%p\n", spr->y, y, i,
          //               (void *)spr_data_ptr);
          for (unsigned j = 0; j < sprite_width; j++) {
            const uint8_t color_ix = *spr_data_ptr++;
            if (color_ix) {
              *px_ptr++ = palette[color_ix];
            } else {
              px_ptr++;
            }
            // *px_ptr++ = palette[*spr_data_ptr++];
          }
        }
      }
    }
    // write buffer to screen
    // if (tft.dmaBusy()) {
    //   Serial.printf("dma busy\n");
    // }
    tft.setAddrWindow(0, frame_y, frame_width, tile_height);
    tft.pushPixelsDMA(line_buf_ptr_dma, frame_width * tile_height);
  }
}

static float x = 0;
static float dx_per_s = 100;

void loop() {
  if (fps.on_frame(millis())) {
    Serial.printf("t=%lu  fps=%u  ldr=%u\n", fps.now_ms(), fps.get(),
                  analogRead(LDR_PIN));
  }

  if (ts.tirqTouched() and ts.touched()) {
    const TS_Point pt = ts.getPoint();
    const int x_relative_center = pt.x - 4096 / 2;
    constexpr float dx_factor = 200.0f / (4096 / 2);
    dx_per_s = dx_factor * x_relative_center;
    Serial.printf("touch x=%d  y=%d  z=%d  dx=%f\n", pt.x, pt.y, pt.z,
                  dx_per_s);
  }

  tft.startWrite();

  tiles_map_render(unsigned(x));

  // for (unsigned i = 0; i < sizeof(sprites) / sizeof(struct sprite); i++) {
  //   const struct sprite *spr = &sprites[i];
  //   tft.setAddrWindow(spr->x, spr->y, sprite_width, sprite_height);
  //   tft.pushPixelsDMA(spr->data, sprite_width * sprite_height);
  // }

  tft.endWrite();

  x += dx_per_s * fps.dt_s();
  if (x < 0) {
    x = 0;
    dx_per_s = -dx_per_s;
  } else if (x > (tiles_map_width * tile_width - frame_width)) {
    x = tiles_map_width * tile_width - frame_width;
    dx_per_s = -dx_per_s;
  }
}

//
// intended for:
//    ESP32 Arduino LVGL WIFI & Bluetooth Development Board 2.8"
//    240 * 320 Smart Display Screen 2.8 inch LCD TFT Module With Touch WROOM
//
//          from: http://www.jczn1688.com/
//  purchased at: https://www.aliexpress.com/item/1005004502250619.html
//
//                    Arduino IDE 2.2.1
// additional boards: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
//    install boards: esp32 by Espressif 2.0.14
//   install library: TFT_eSPI by Bodmer 2.5.31
//     setup library: replace User_Setup.h in libraries/TFT_eSPI/ with provided file
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
//                     Partition Scheme: Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)
//                                PSRAM: Disabled
//                         Upload Speed: 921600
//                           Programmer: Esptool
//

#include <TFT_eSPI.h>  // Graphics and font library for ST7735 driver chip
#include <SPI.h>

// #define USE_WIFI

#ifdef USE_WIFI
#include "WiFi.h"
#include "secrets.h"
#endif

class fps {
  unsigned interval_ms_ = 5000;
  unsigned frames_rendered_in_interval_ = 0;
  unsigned long last_update_ms_ = 0;
  unsigned long prv_now_ms_ = 0;
  unsigned current_fps_ = 0;
  unsigned long now_ms_ = 0;
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

  inline auto get() -> unsigned {
    return current_fps_;
  }

  inline auto now_ms() -> unsigned long {
    return now_ms_;
  }

  inline auto dt_s() -> float {
    return dt_s_;
  }

} static fps{};

static constexpr uint16_t frame_width = 320;
static constexpr uint16_t frame_height = 240;

static constexpr uint8_t tile_width = 8;
static constexpr uint8_t tile_height = 8;

// the right shift of 'x' to get the x in tiles map
static constexpr uint8_t tile_width_shift = 3;

// the bits that are the partial tile position between 0 and not including 'tile_width'
static constexpr uint8_t tile_width_and = 7;

struct tile {
  const uint8_t data[tile_width * tile_height];
} static constexpr tiles[] PROGMEM{
#include "tiles.h"
};

static constexpr unsigned tiles_map_width = 80;
static constexpr unsigned tiles_map_height = 30;
struct tiles_map {
  uint8_t cell[tiles_map_height][tiles_map_width];
} static constexpr tiles_map{
  {
#include "tiles_map.h"
  }
};

static constexpr uint16_t palette[256]{
  0b0000000000000000,  // black
  0b0000000000011111,  // green
  0b0000011111100000,  // red
  0b0000011111111111,  // yellow
  0b1111100000000000,  // blue
  0b1111100000011111,  // cyan
  0b1111111111100000,  // magenta
  0b1111111111111111,  // white
};

static TFT_eSPI tft;  // invoke library, pins defined in User_Setup.h

void setup(void) {
  Serial.begin(115200);
  sleep(1);  // arbitrary wait 1 second for serial to connect
  while (!Serial)
    ;  // wait for serial port to connect. needed for native usb port only
  Serial.printf("\n------------------------------------------------------------------------------\n");
  Serial.printf("        chip model: %s\n", ESP.getChipModel());
  Serial.printf("            screen: %d x %d px\n", frame_width, frame_height);
  Serial.printf("largest free block: %d B\n", ESP.getMaxAllocHeap());
#ifdef USE_WIFI
  Serial.printf("using WiFi\n");
#endif
  Serial.printf("------------------------------------------------------------------------------\n");

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
}

// one tile height buffer, paletted 8 bit tiles in tiles map
// 31 fps
static void tiles_map_render(const unsigned x) {
  static uint16_t line_buf_1[frame_width * tile_height];
  static uint16_t line_buf_2[frame_width * tile_height];

  const unsigned tile_x = x >> tile_width_shift;
  const unsigned tile_dx = x & tile_width_and;
  const unsigned tile_width_minus_dx = tile_width - tile_dx;

  bool line_buf_first = true;  // selects buffer to write while dma reads the other
  for (unsigned tile_y = 0; tile_y < tiles_map_height; tile_y++) {
    // swap between two line buffers to not overwrite DMA accessed buffer
    uint16_t* line_buf_ptr = line_buf_first ? line_buf_1 : line_buf_2;
    uint16_t* line_buf_ptr_dma = line_buf_ptr;
    line_buf_first = not line_buf_first;
    for (unsigned ty = 0; ty < tile_height; ty++) {
      if (tile_width_minus_dx) {
        // render first partial tile
        const uint8_t tile_ix = tiles_map.cell[tile_y][tile_x];
        const uint8_t* tile_data_ptr = tiles[tile_ix].data + (ty * tile_height) + tile_dx;
        for (unsigned i = tile_dx; i < tile_width; i++) {
          *line_buf_ptr++ = palette[*tile_data_ptr++];
        }
      }
      // render full tiles
      for (unsigned tx = 1; tx < frame_width / tile_width; tx++) {
        const uint8_t tile_ix = tiles_map.cell[tile_y][tile_x + tx];
        const uint8_t* tile_data_ptr = tiles[tile_ix].data + (ty * tile_height);
        for (unsigned i = 0; i < tile_width; i++) {
          *line_buf_ptr++ = palette[*tile_data_ptr++];
        }
      }
      if (tile_dx) {
        // render last partial tile
        const uint8_t tile_ix = tiles_map.cell[tile_y][tile_x + frame_width / tile_width];
        const uint8_t* tile_data_ptr = tiles[tile_ix].data + (ty * tile_height);
        for (unsigned i = 0; i < tile_dx; i++) {
          *line_buf_ptr++ = palette[*tile_data_ptr++];
        }
      }
    }
    tft.setAddrWindow(0, tile_y * tile_height, frame_width, tile_height);
    tft.pushPixelsDMA(line_buf_ptr_dma, frame_width * tile_height);
  }
}

static float x = 0;
static float dx_per_s = 80;

void loop() {
  if (fps.on_frame(millis())) {
    Serial.printf("t=%lu  fps=%d\n", fps.now_ms(), fps.get());
  }

  tft.startWrite();
  tiles_map_render(unsigned(x));
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

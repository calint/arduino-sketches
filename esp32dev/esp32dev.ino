//
// intended for:
//    ESP32 Arduino LVGL WIFI&Bluetooth Development Board 2.8"'
//    240*320 Smart Display Screen 2.8inch LCD TFT Module With Touch WROOM
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
#define USE_DMA

#ifdef USE_WIFI
#include "WiFi.h"
#include "secrets.h"
#endif

static TFT_eSPI tft;  // Invoke library, pins defined in User_Setup.h

class fps {
  unsigned interval_ms = 5000;
  unsigned frames_rendered_in_interval = 0;
  unsigned long last_update_ms = 0;
  unsigned current_fps = 0;

public:
  auto on_frame(const unsigned long now_ms) -> bool {
    frames_rendered_in_interval++;
    const unsigned long dt_ms = now_ms - last_update_ms;
    if (dt_ms >= interval_ms) {
      current_fps = frames_rendered_in_interval * 1000 / dt_ms;
      frames_rendered_in_interval = 0;
      last_update_ms = now_ms;
      return true;
    }
    return false;
  }

  inline auto get() -> unsigned {
    return current_fps;
  }

} fps{};

static constexpr uint16_t frame_width = 320;
static constexpr uint16_t frame_height = 240;

static constexpr uint8_t tile_width = 8;
static constexpr uint8_t tile_height = 8;
struct tile {
  uint16_t data[tile_width * tile_height];
} tiles[4]{
  { 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff },

  { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff,
    0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff,
    0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff,
    0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff,
    0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff,
    0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff },

  { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff,
    0xffff, 0x0000, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff,
    0xffff, 0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 0x0000, 0xffff,
    0xffff, 0x0000, 0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 0xffff,
    0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0x0000, 0xffff,
    0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff },

  { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff,
    0xffff, 0x0000, 0xffff, 0x0000, 0x0000, 0xffff, 0x0000, 0xffff,
    0xffff, 0x0000, 0x0000, 0xffff, 0xffff, 0x0000, 0x0000, 0xffff,
    0xffff, 0x0000, 0x0000, 0xffff, 0xffff, 0x0000, 0x0000, 0xffff,
    0xffff, 0x0000, 0xffff, 0x0000, 0x0000, 0xffff, 0x0000, 0xffff,
    0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff },
};

// static constexpr uint8_t tile_map_width = 4;
// static constexpr uint8_t tile_map_height = 4;

// struct tile_map {
//   uint8_t tiles[tile_map_width * tile_map_height];
// } tile_map{
//   {
//     0,
//     1,
//     2,
//     3,
//     1,
//     2,
//     3,
//     0,
//     2,
//     3,
//     0,
//     1,
//     3,
//     0,
//     1,
//     2,
//   }
// };

static int32_t viewport_x;
static int32_t viewport_y;
static int32_t viewport_w;
static int32_t viewport_h;

void print_heap_info(Stream& os) {
  os.print("used: ");
  os.print(ESP.getHeapSize() - ESP.getFreeHeap());
  os.println(" B");
  os.print("free: ");
  os.print(ESP.getFreeHeap());
  os.println(" B");
  os.print("total: ");
  os.print(ESP.getHeapSize());
  os.println(" B");
}

void setup(void) {
  Serial.begin(115200);
  sleep(1);  // arbitrary wait 1 second for serial to connect
  while (!Serial)
    ;  // wait for serial port to connect. needed for native usb port only
  Serial.printf("\n------------------------------------------------------------------------------\n");
  Serial.printf("        chip model: %s\n", ESP.getChipModel());
  Serial.printf("largest free block: %d B\n", ESP.getMaxAllocHeap());
#ifdef USE_DMA
  Serial.printf("using DMA\n");
#endif
#ifdef USE_WIFI
  Serial.printf("using WiFi\n");
#endif
  // Serial.printf("largest free block: %d B\n", heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
  Serial.printf("------------------------------------------------------------------------------\n");

  // heap_caps_print_heap_info(MALLOC_CAP_8BIT);

  // uint32_t total = 0;
  // while (true) {
  //   // heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
  //   const uint32_t n = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
  //   if (n == 0) {
  //     break;
  //   }
  //   void* buf = malloc(n);
  //   if (buf == nullptr) {
  //     break;
  //   }
  //   total += n;
  //   Serial.printf("address: %p   allocated: %d\n", buf, n);
  // }
  // Serial.printf("total allocated bytes: %d\n", total);

  // Serial.printf("heap info:\n");
  // print_heap_info(Serial);

  tft.init();
  tft.setRotation(1);
#ifdef USE_DMA
  tft.initDMA(true);
#endif

  viewport_x = tft.getViewportX();
  viewport_y = tft.getViewportY();
  viewport_w = tft.getViewportWidth();
  viewport_h = tft.getViewportHeight();

  Serial.printf("viewport: x=%d, y=%d, w=%d, h=%d\n", viewport_x, viewport_y, viewport_w, viewport_h);

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
}

static uint16_t line_buf_1[frame_width * tile_height];
static uint16_t line_buf_2[frame_width * tile_height];

void loop() {
  const unsigned long now_ms = millis();
  if (fps.on_frame(now_ms)) {
    Serial.printf("t=%lu  fps=%d\n", now_ms, fps.get());
  }

  tft.startWrite();

  // fps 31
  bool line_buf_first = true;
  for (unsigned y = 0; y < frame_height; y += tile_height) {
    // swap between two line buffers to not overwrite DMA accessed buffer
    uint16_t* line_buf_ptr = line_buf_first ? line_buf_1 : line_buf_2;
    uint16_t* line_buf_ptr_dma = line_buf_ptr;
    line_buf_first = !line_buf_first;
    for (unsigned ty = 0; ty < tile_height; ty++) {
      for (unsigned tx = 0; tx < frame_width / tile_width; tx++) {
        uint16_t* tile_data_ptr = tiles[2].data + (ty * tile_height);
        memcpy(line_buf_ptr, tile_data_ptr, tile_width * sizeof(uint16_t));
        line_buf_ptr += tile_width;
      }
    }
    tft.setAddrWindow(viewport_x, viewport_y + y, viewport_w, tile_height);
    tft.pushPixelsDMA(line_buf_ptr_dma, viewport_w * tile_height);
  }

  // 25 fps
  // uint16_t line_buf_1[viewport_w];
  // uint16_t line_buf_2[viewport_w];
  // bool line_buf_first = true;
  // uint8_t tile_y = 0;
  // for (unsigned y = 0; y < frame_height; y++) {
  //   // swap between two line buffers to not overwrite DMA accessed buffer
  //   uint16_t* line_buf_ptr = line_buf_first ? line_buf_1 : line_buf_2;
  //   uint16_t* line_buf_ptr_dma = line_buf_ptr;
  //   line_buf_first = !line_buf_first;
  //   for (unsigned x = 0; x < frame_width / tile_width; x++) {
  //     uint16_t* tile_data_ptr = tiles[2].data + (tile_y * tile_height);
  //     memcpy(line_buf_ptr, tile_data_ptr, tile_width * sizeof(uint16_t));
  //     line_buf_ptr += tile_width;
  //   }
  //   tile_y++;
  //   if (tile_y >= tile_height) {
  //     tile_y = 0;
  //   }
  //   tft.setAddrWindow(viewport_x, viewport_y + y, viewport_w, 1);
  //   tft.pushPixelsDMA(line_buf_ptr_dma, viewport_w);
  // }

  // 25 fps
  // uint8_t tile_id = 0;
  // for (unsigned y = 0; y < frame_height; y += tile_height) {
  //   for (unsigned x = 0; x < frame_width; x += tile_width) {
  //     tft.setAddrWindow(viewport_x + x, viewport_y + y, tile_width, tile_height);
  //     tft.pushPixels(tiles[tile_id].data, tile_width * tile_height);
  //     tile_id++;
  //     if (tile_id > 3) {
  //       tile_id = 0;
  //     }
  //   }
  // }
  tft.endWrite();
}

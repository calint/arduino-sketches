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

static constexpr uint16_t frame_width = 320;
static constexpr uint16_t frame_height = 120;
// static uint16_t* frame_buf = nullptr;
static uint16_t frame_buf[frame_width * frame_height];  // RGB565
static uint16_t color;

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

void loop() {
  const unsigned long now_ms = millis();
  if (fps.on_frame(now_ms)) {
    Serial.printf("t=%lu  fps=%d\n", now_ms, fps.get());
  }

  uint16_t color_px = color;
  uint16_t* bufptr = frame_buf;
  constexpr unsigned n = frame_width * frame_height;
  for (unsigned i = 0; i < n; i++) {
    *bufptr++ = color_px++;
  }
  color++;

  tft.startWrite();
  tft.setAddrWindow(viewport_x, viewport_y, viewport_w, viewport_h);
#ifdef USE_DMA
  tft.pushPixelsDMA(frame_buf, frame_width * frame_height);
#else
  tft.pushPixels(frame_buf, frame_width * frame_height);
#endif
  tft.endWrite();
}

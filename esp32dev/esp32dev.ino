//
// intended for:
//    ESP32 Arduino LVGL WIFI&Bluetooth Development Board 2.8"'
//    240*320 Smart Display Screen 2.8inch LCD TFT Module With Touch WROOM
//
//          from: http://http://www.jczn1688.com/
//  purchased at: https://www.aliexpress.com/item/1005004502250619.html
//
// additional boards: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
//    install boards: esp32 by Espressif
//   install library: TFT_eSPI by Bodmer
//     setup library: replace User_Setup.h in libraries/TFT_eSPI/ with provided file
//
// in arduino ide, in Tools menu select:
//             Board: ESP32 Dev Module
//      Upload Speed: 921600
//   Flash Frequency: 80MHz
//        Flash Mode: DIO
//        Flash Size: 4MB (32Mb)
//  Partition Scheme: Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)
//        Programmer: Esptool
//

#include <TFT_eSPI.h>  // Graphics and font library for ST7735 driver chip
#include <SPI.h>

static TFT_eSPI tft;  // Invoke library, pins defined in User_Setup.h

static constexpr uint16_t frame_width = 320;
static constexpr uint16_t frame_height = 160;
static uint16_t frame_buf[frame_width * frame_height];  // RGB565
static uint16_t color;

class fps {
  unsigned frames_rendered = 0;
  unsigned long last_update_ms = 0;
  unsigned current_fps = 0;

public:
  auto on_frame(const unsigned long now) -> bool {
    frames_rendered++;
    const unsigned long dt = now - last_update_ms;
    if (dt > 1000) {
      current_fps = frames_rendered * 1000 / dt;
      frames_rendered = 0;
      last_update_ms = now;
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
  while (!Serial)
    ;  // wait for serial port to connect. needed for native usb port only

  Serial.printf("------------------------------------------------------------------------------\n");
  Serial.printf("        chip model: %s\n", ESP.getChipModel());
  Serial.printf("largest free block: %d B\n", ESP.getMaxAllocHeap());
  // Serial.printf("largest free block: %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
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

  viewport_x = tft.getViewportX();
  viewport_y = tft.getViewportY();
  viewport_w = tft.getViewportWidth();
  viewport_h = tft.getViewportHeight();

  Serial.printf("viewport: x=%d, y=%d, w=%d, h=%d\n", viewport_x, viewport_y, viewport_w, viewport_h);
}

void loop() {
  const unsigned long now = millis();
  if (fps.on_frame(now)) {
    // Serial.printf("t=%lu  fps=%d\n", now, fps.get());
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
  tft.pushPixels(frame_buf, frame_width * frame_height);
  tft.endWrite();
}

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

static TFT_eSPI tft{};  // Invoke library, pins defined in User_Setup.h

static constexpr uint16_t frame_width = 320;
static constexpr uint16_t frame_height = 160;
static uint16_t frame[frame_width * frame_height];
static uint16_t color = 0;

static int32_t viewport_x;
static int32_t viewport_y;
static int32_t viewport_w;
static int32_t viewport_h;

auto print_heap_info(Stream& os) -> void {
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
    ;  // wait for serial port to connect. Needed for native USB port only

  print_heap_info(Serial);
  tft.init();
  tft.setRotation(1);
  viewport_x = tft.getViewportX();
  viewport_y = tft.getViewportY();
  viewport_w = tft.getViewportWidth();
  viewport_h = tft.getViewportHeight();
  Serial.printf("viewport: x=%d, y=%d, w=%d, h=%d\n", viewport_x, viewport_y, viewport_w, viewport_h);
}

void loop() {
  uint16_t* frame_ptr = frame;
  const unsigned n = frame_width * frame_height;
  for (unsigned i = 0; i < n; i++) {
    *frame_ptr++ = color;
  }

  unsigned long t0 = millis();
  tft.startWrite();
  tft.setAddrWindow(viewport_x, viewport_y, viewport_w, viewport_h);
  tft.pushPixels(frame, frame_width * frame_height);
  tft.endWrite();
  unsigned long t1 = millis();
  // delay(2000);
  color += 32;
  Serial.printf("blit: %lu ms color: %x\n", t1 - t0, color);
}

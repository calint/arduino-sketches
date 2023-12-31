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
//    install boards: esp32 by Espressif 2.0.13
//   install library: TFT_eSPI by Bodmer 2.5.34
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
//    * install plug-in Arduino by Microsoft v0.6.0
//    * task ">Arduino: Initialize"
//    * task ">Arduino: Board Manager"
//        install "esp32 by Espressif Systems Version 2.0.14"
//    * task ">Arduino: Board Config"
//        select "ESP32 Dev Module (esp32)"
//    * install libraries:
//        TFT_eSPI by Bodmer 2.5.34
//        XPT2046_Touchscreen by Paul Stoffregen 1.4.0
//    * replace User_Setup.h in ~/Arduino/libraries/TFT_eSPI/ with provided file
//

// note. after upgrading library TFT_eSPI replace the User_Setup.h and make a
// clean build

// note. why some buffers are allocated at 'setup'
// Due to a technical limitation, the maximum statically allocated DRAM usage is
// 160KB. The remaining 160KB (for a total of 320KB of DRAM) can only be
// allocated at runtime as heap.
// -- https://stackoverflow.com/questions/71085927/how-to-extend-esp32-heap-size

// note. design decision of 'hpp' source files
// * the program is one file split into logical sections using includes
// * increases opportunities for optimization by the compiler
// * directory 'game' contains the user code that interfaces with 'engine.hpp'
// * order of include and content of 'defs.hpp', 'game.hpp', 'main.hpp' solves
//   circular references and gives user the necessary callbacks to interface
//   with engine

// main entry file to user code
#include "game/main.hpp"

// platform specific definitions and objects
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// #define USE_WIFI
#ifdef USE_WIFI
#include "WiFi.h"
#include "secrets.h"
#endif

// rgb led
static constexpr uint8_t cyd_led_blue = 17;
static constexpr uint8_t cyd_led_red = 4;
static constexpr uint8_t cyd_led_green = 16;

// ldr (light dependant resistor)
// analog read of pin gives: 0 for full brightness, higher values is darker
static constexpr uint8_t cyd_ldr_pin = 34;

// setup touch screen
// https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/Examples/Basics/2-TouchTest/2-TouchTest.ino
static constexpr uint8_t xpt2046_irq = 36;  // Interrupt Request
static constexpr uint8_t xpt2046_mosi = 32; // Master Out Slave In
static constexpr uint8_t xpt2046_miso = 39; // Master In Slave Out
static constexpr uint8_t xpt2046_clk = 25;  // Clock
static constexpr uint8_t xpt2046_cs = 33;   // Chip Select

static SPIClass spi{HSPI};
static XPT2046_Touchscreen touch_screen{xpt2046_cs, xpt2046_irq};
static TFT_eSPI display{};

// buffers for rendering a chunk while the other is transferred to the screen
// using DMA. allocated in setup
static uint16_t *dma_buf_1;
static uint16_t *dma_buf_2;
static constexpr unsigned dma_buf_size =
    sizeof(uint16_t) * display_width * tile_height;

// clang-format off
// note. not formatted because compiler gets confused and issues invalid error
static void render_scanline(
    uint16_t *render_buf_ptr,
    sprite_ix *collision_map_scanline_ptr,
    const int16_t scanline_y,
    const unsigned tile_x,
    const unsigned tile_dx,
    const unsigned tile_width_minus_dx,
    const tile_ix *tiles_map_row_ptr,
    const unsigned tile_sub_y,
    const unsigned tile_sub_y_times_tile_width
) {
  // clang-format on
  // used later by sprite renderer to overwrite tiles pixels
  uint16_t *scanline_ptr = render_buf_ptr;

  if (tile_width_minus_dx) {
    // render first partial tile
    const tile_ix tile_index = *(tiles_map_row_ptr + tile_x);
    const uint8_t *tile_data_ptr =
        tiles[tile_index].data + tile_sub_y_times_tile_width + tile_dx;
    for (unsigned i = tile_dx; i < tile_width; i++) {
      *render_buf_ptr++ = palette_tiles[*tile_data_ptr++];
    }
  }
  // render full tiles
  const unsigned tx_max = tile_x + (display_width / tile_width);
  for (unsigned tx = tile_x + 1; tx < tx_max; tx++) {
    const tile_ix tile_index = *(tiles_map_row_ptr + tx);
    const uint8_t *tile_data_ptr =
        tiles[tile_index].data + tile_sub_y_times_tile_width;
    for (unsigned i = 0; i < tile_width; i++) {
      *render_buf_ptr++ = palette_tiles[*tile_data_ptr++];
    }
  }
  if (tile_dx) {
    // render last partial tile
    const tile_ix tile_index = *(tiles_map_row_ptr + tx_max);
    const uint8_t *tile_data_ptr =
        tiles[tile_index].data + tile_sub_y_times_tile_width;
    for (unsigned i = 0; i < tile_dx; i++) {
      *render_buf_ptr++ = palette_tiles[*tile_data_ptr++];
    }
  }

  // render sprites
  // note. although grossly inefficient algorithm the DMA is busy while
  // rendering one tile height of sprites and tiles

  sprite *spr = sprites.all_list();
  const unsigned len = sprites.all_list_len();
  for (unsigned i = 0; i < len; i++, spr++) {
    if (!spr->img or spr->scr_y > scanline_y or
        spr->scr_y + int16_t(sprite_height) <= scanline_y or
        spr->scr_x <= sprite_width_neg or spr->scr_x > int16_t(display_width)) {
      // sprite has no image or
      // not within scanline or
      // is outside the screen x-wise
      continue;
    }
    const uint8_t *spr_data_ptr =
        spr->img + (scanline_y - spr->scr_y) * sprite_width;
    uint16_t *scanline_dst_ptr = scanline_ptr + spr->scr_x;
    unsigned render_width = sprite_width;
    sprite_ix *collision_pixel = collision_map_scanline_ptr + spr->scr_x;
    if (spr->scr_x < 0) {
      // adjustment if x is negative
      spr_data_ptr -= spr->scr_x;
      scanline_dst_ptr -= spr->scr_x;
      render_width = sprite_width + spr->scr_x;
      collision_pixel -= spr->scr_x;
    } else if (spr->scr_x + sprite_width > display_width) {
      // adjustment if sprite partially outside screen (x-wise)
      render_width = display_width - spr->scr_x;
    }
    // render scanline of sprite
    object *obj = spr->obj;
    for (unsigned j = 0; j < render_width;
         j++, collision_pixel++, scanline_dst_ptr++) {
      // write pixel from sprite data or skip if 0
      const uint8_t color_ix = *spr_data_ptr++;
      if (color_ix) {
        *scanline_dst_ptr = palette_sprites[color_ix];
        if (*collision_pixel != sprite_ix_reserved) {
          sprite *spr2 = sprites.instance(*collision_pixel);
          object *other_obj = spr2->obj;
          if (obj->col_mask & other_obj->col_bits) {
            obj->col_with = other_obj;
          }
          if (other_obj->col_mask & obj->col_bits) {
            other_obj->col_with = obj;
          }
        }
        // set pixel collision value to sprite index
        *collision_pixel = i;
      }
    }
  }
}

// buffer: one tile height, palette, 8-bit tiles from tiles map, 8-bit sprites
// 31 fps with DMA, 22 fps without
static void render(const unsigned x, const unsigned y) {
  display.startWrite();

  const unsigned tile_x = x >> tile_width_shift;
  const unsigned tile_dx = x & tile_width_and;
  const unsigned tile_width_minus_dx = tile_width - tile_dx;
  unsigned tile_y = y >> tile_height_shift;
  const unsigned tile_dy = y & tile_height_and;
  const unsigned tile_y_max = tile_y + (display_height / tile_height);
  const unsigned tile_height_minus_dy = tile_height - tile_dy;

  // selects buffer to write while DMA reads the other buffer
  bool dma_buf_use_first = true;
  // y in frame for current tiles row to be copied by DMA
  unsigned frame_y = 0;
  // current line y on screen
  int16_t scanline_y = 0;
  // pointer to start of current row of tiles
  const tile_ix *tiles_map_row_ptr = tile_map.cell[tile_y];
  // pointer to collision map starting at top left of screen
  sprite_ix *collision_map_scanline_ptr = collision_map;
  if (tile_dy) {
    // render the partial top tile
    // swap between two rendering buffers to not overwrite DMA accessed
    // buffer
    uint16_t *render_buf_ptr = dma_buf_use_first ? dma_buf_1 : dma_buf_2;
    dma_buf_use_first = not dma_buf_use_first;
    // pointer to the buffer that the DMA will copy to screen
    uint16_t *dma_buf = render_buf_ptr;
    // render scanlines of first partial tile
    for (unsigned tile_sub_y = tile_dy,
                  tile_sub_y_times_tile_width = tile_dy * tile_width;
         tile_sub_y < tile_height;
         tile_sub_y++, tile_sub_y_times_tile_width += tile_width,
                  render_buf_ptr += display_width,
                  collision_map_scanline_ptr += display_width, scanline_y++) {

      render_scanline(render_buf_ptr, collision_map_scanline_ptr, scanline_y,
                      tile_x, tile_dx, tile_width_minus_dx, tiles_map_row_ptr,
                      tile_sub_y, tile_sub_y_times_tile_width);
    }

    display.setAddrWindow(0, frame_y, display_width, tile_height_minus_dy);
    display.pushPixelsDMA(dma_buf, display_width * tile_height_minus_dy);

    tile_y++;
    tiles_map_row_ptr += tile_map_width;
    frame_y += tile_height_minus_dy;
  }
  // for each row of full tiles
  for (; tile_y < tile_y_max;
       tile_y++, frame_y += tile_height, tiles_map_row_ptr += tile_map_width) {
    // swap between two rendering buffers to not overwrite DMA accessed
    // buffer
    uint16_t *render_buf_ptr = dma_buf_use_first ? dma_buf_1 : dma_buf_2;
    dma_buf_use_first = not dma_buf_use_first;
    // pointer to the buffer that the DMA will copy to screen
    uint16_t *dma_buf = render_buf_ptr;
    // render one tile height of pixels from tiles map and sprites to the
    // 'render_buf_ptr'
    for (unsigned tile_sub_y = 0, tile_sub_y_times_tile_width = 0;
         tile_sub_y < tile_height;
         tile_sub_y++, tile_sub_y_times_tile_width += tile_height,
                  render_buf_ptr += display_width,
                  collision_map_scanline_ptr += display_width, scanline_y++) {

      render_scanline(render_buf_ptr, collision_map_scanline_ptr, scanline_y,
                      tile_x, tile_dx, tile_width_minus_dx, tiles_map_row_ptr,
                      tile_sub_y, tile_sub_y_times_tile_width);
    }

    display.setAddrWindow(0, frame_y, display_width, tile_height);
    display.pushPixelsDMA(dma_buf, display_width * tile_height);
  }
  if (tile_dy) {
    // render last partial tile
    // swap between two rendering buffers to not overwrite DMA accessed
    // buffer
    uint16_t *render_buf_ptr = dma_buf_use_first ? dma_buf_1 : dma_buf_2;
    dma_buf_use_first = not dma_buf_use_first;
    // pointer to the buffer that the DMA will copy to screen
    uint16_t *dma_buf = render_buf_ptr;
    // render the partial last tile row
    for (unsigned tile_sub_y = 0, tile_sub_y_times_tile_width = 0;
         tile_sub_y < tile_dy;
         tile_sub_y++, tile_sub_y_times_tile_width += tile_width,
                  render_buf_ptr += display_width,
                  collision_map_scanline_ptr += display_width, scanline_y++) {

      render_scanline(render_buf_ptr, collision_map_scanline_ptr, scanline_y,
                      tile_x, tile_dx, tile_width_minus_dx, tiles_map_row_ptr,
                      tile_sub_y, tile_sub_y_times_tile_width);
    }

    display.setAddrWindow(0, frame_y, display_width, tile_dy);
    display.pushPixelsDMA(dma_buf, display_width * tile_dy);
  }

  display.endWrite();
}

void setup(void) {
  Serial.begin(115200);
  sleep(1); // arbitrary wait 1 second for serial to connect
  while (!Serial)
    ; // wait for serial port to connect. needed for native usb port only

  // heap_caps_dump_all();
  Serial.printf("\n\n");
  Serial.printf("------------------- platform -----------------------------\n");
  Serial.printf("        chip model: %s\n", ESP.getChipModel());
  Serial.printf("            screen: %u x %u px\n", display_width,
                display_height);
  Serial.printf("     free heap mem: %u B\n", ESP.getFreeHeap());
  Serial.printf("largest free block: %u B\n", ESP.getMaxAllocHeap());
#ifdef USE_WIFI
  Serial.printf("              WiFi: on\n");
#endif
  Serial.printf("------------------- type sizes ---------------------------\n");
  Serial.printf("              bool: %zu B\n", sizeof(bool));
  Serial.printf("              char: %zu B\n", sizeof(char));
  Serial.printf("               int: %zu B\n", sizeof(int));
  Serial.printf("              long: %zu B\n", sizeof(long));
  Serial.printf("         long long: %zu B\n", sizeof(long long));
  Serial.printf("             float: %zu B\n", sizeof(float));
  Serial.printf("            double: %zu B\n", sizeof(double));
  Serial.printf("             void*: %zu B\n", sizeof(void *));

  // allocate DMA buffers
  dma_buf_1 = (uint16_t *)malloc(dma_buf_size);
  dma_buf_2 = (uint16_t *)malloc(dma_buf_size);
  if (!dma_buf_1 or !dma_buf_2) {
    Serial.printf("!!! could not allocate DMA buffers");
    while (true)
      ;
  }

  engine_setup();

  Serial.printf("------------------- after init ---------------------------\n");
  Serial.printf("     free heap mem: %zu B\n", ESP.getFreeHeap());
  Serial.printf("largest free block: %zu B\n", ESP.getMaxAllocHeap());
  Serial.printf("------------------- in program memory --------------------\n");
  Serial.printf("     sprite images: %zu B\n", sizeof(sprite_imgs));
  Serial.printf("             tiles: %zu B\n", sizeof(tiles));
  Serial.printf("          tile map: %zu B\n", sizeof(tile_map));
  Serial.printf("------------------- globals ------------------------------\n");
  Serial.printf("           sprites: %zu B\n", sizeof(sprites));
  Serial.printf("           objects: %zu B\n", sizeof(objects));
  Serial.printf("------------------- on heap ------------------------------\n");
  Serial.printf("      sprites data: %zu B\n", sprites.allocated_data_size_B());
  Serial.printf("      objects data: %zu B\n", objects.allocated_data_size_B());
  Serial.printf("     collision map: %zu B\n", collision_map_size);
  Serial.printf("   DMA buf 1 and 2: %zu B\n", 2 * dma_buf_size);
  Serial.printf("------------------- object sizes -------------------------\n");
  Serial.printf("            sprite: %zu B\n", sizeof(sprite));
  Serial.printf("            object: %zu B\n", sizeof(object));
  Serial.printf("              tile: %zu B\n", sizeof(tile));
  Serial.printf("----------------------------------------------------------\n");

  // setup rgb led
  pinMode(cyd_led_red, OUTPUT);
  pinMode(cyd_led_green, OUTPUT);
  pinMode(cyd_led_blue, OUTPUT);

  // set rgb led to green
  digitalWrite(cyd_led_red, HIGH);
  digitalWrite(cyd_led_green, LOW);
  digitalWrite(cyd_led_blue, HIGH);

  // setup ldr
  pinMode(cyd_ldr_pin, INPUT);

  // start the spi for the touch screen and init the library
  spi.begin(xpt2046_clk, xpt2046_miso, xpt2046_mosi, xpt2046_cs);
  touch_screen.begin(spi);
  touch_screen.setRotation(display_orientation);

  // initiate display
  display.init();
  display.setRotation(display_orientation);
  display.initDMA(true);

#ifdef USE_WIFI
  WiFi.begin(secret_wifi_network, secret_wifi_password);
  WiFi.setAutoReconnect(true);
  Serial.print("Connecting to ");
  Serial.print(secret_wifi_network);
  Serial.flush();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    sleep(1);
  }
  Serial.print("\nConnected\nIP: ");
  Serial.println(WiFi.localIP());
#endif

  // set random seed for deterministic behavior
  randomSeed(0);

  // initiate clock to current time and frames-per-second calculation every 2
  // seconds
  clk.init(millis(), 2000);

  main_setup();
}

void loop() {
  if (clk.on_frame(millis())) {
    Serial.printf("t=%lu  fps=%u  ldr=%u  objs=%u  sprs=%u\n", clk.ms, clk.fps,
                  analogRead(cyd_ldr_pin), objects.allocated_list_len(),
                  sprites.allocated_list_len());
  }

  if (touch_screen.tirqTouched() and touch_screen.touched()) {
    const TS_Point pt = touch_screen.getPoint();
    main_on_touch_screen(pt.x, pt.y, pt.z);
  }

  engine_loop();
}

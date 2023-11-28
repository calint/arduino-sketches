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
//    * replace User_Setup.h in ~/Arduino/libraries/TFT_eSPI/ with provided file
//

// note. why some buffers are allocated at 'setup'
// Due to a technical limitation, the maximum statically allocated DRAM usage is
// 160KB. The remaining 160KB (for a total of 320KB of DRAM) can only be
// allocated at runtime as heap.
// -- https://stackoverflow.com/questions/71085927/how-to-extend-esp32-heap-size

#include "game/main.hpp"

// pixel precision collision detection between on screen sprites
// allocated in setup
static sprite_ix *collision_map;
static constexpr unsigned collision_map_size =
    sizeof(sprite_ix) * display_width * display_height;

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
      *render_buf_ptr++ = palette[*tile_data_ptr++];
    }
  }
  // render full tiles
  const unsigned tx_max = tile_x + (display_width / tile_width);
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

  sprite *spr = sprites.all_list();
  for (unsigned i = 0; i < sprites.all_list_len(); i++, spr++) {
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
        *scanline_dst_ptr = palette[color_ix];
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

// one tile height buffer, palette, 8-bit tiles from tiles map, 8-bit sprites
// 31 fps
static void render(const unsigned x, const unsigned y) {
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
}

void setup(void) {
  Serial.begin(115200);
  sleep(1); // arbitrary wait 1 second for serial to connect
  while (!Serial)
    ; // wait for serial port to connect. needed for native usb port only

  // heap_caps_dump_all();
  Serial.printf("\n\n");
  Serial.printf("------------------- info ---------------------------------\n");
  Serial.printf("        chip model: %s\n", ESP.getChipModel());
  Serial.printf("            screen: %d x %d px\n", display_width,
                display_height);
  Serial.printf("     free heap mem: %d B\n", ESP.getFreeHeap());
  Serial.printf("largest free block: %d B\n", ESP.getMaxAllocHeap());
#ifdef USE_WIFI
  Serial.printf("              WiFi: on\n");
#endif

  // allocate dma buffers
  dma_buf_1 = (uint16_t *)malloc(dma_buf_size);
  dma_buf_2 = (uint16_t *)malloc(dma_buf_size);
  if (!dma_buf_1 or !dma_buf_2) {
    Serial.printf("!!! could not allocate DMA buffers");
    while (true) {
      sleep(60);
    }
  }

  // allocate collision map
  collision_map = (sprite_ix *)malloc(collision_map_size);
  if (!collision_map) {
    Serial.printf("!!! could not allocate collision map");
    while (true) {
      sleep(60);
    }
  }

  Serial.printf("------------------- after init ---------------------------\n");
  Serial.printf("          free mem: %zu B\n", ESP.getFreeHeap());
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
  pinMode(ldr_pin, INPUT);

  // start the SPI for the touch screen and init the TS library
  spi.begin(xpt2046_clk, xpt2046_miso, xpt2046_mosi, xpt2046_cs);
  touch_screen.begin(spi);
  touch_screen.setRotation(1);

  display.init();
  display.setRotation(1);
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

  // set random seed to get same random every time
  randomSeed(0);

  // initiate frames-per-second and dt keeper
  clk.init(millis());

  setup_scene();
}

void loop() {
  // frames per second update
  if (clk.on_frame(millis())) {
    Serial.printf("t=%lu  fps=%u  ldr=%u  objs=%u  sprs=%u\n", clk.now_ms(),
                  clk.fps(), analogRead(ldr_pin), objects.allocated_list_len(),
                  sprites.allocated_list_len());
  }

  // check touch screen
  if (touch_screen.tirqTouched() and touch_screen.touched()) {
    const TS_Point pt = touch_screen.getPoint();
    controller.on_touch(pt.x, pt.y, pt.z);
  }

  objects.update();

  // apply freed sprites during 'objects.update()'
  sprites.apply_free();

  // clear collisions map
  memset(collision_map, sprite_ix_reserved, collision_map_size);

  // render tiles, sprites and collision map
  display.startWrite();
  render(unsigned(tile_map_x), unsigned(tile_map_y));
  display.endWrite();

  // game logic hook
  on_after_frame();
}

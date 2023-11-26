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

#include "o1store.hpp"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <limits>

// #define USE_WIFI
#ifdef USE_WIFI
#include "WiFi.h"
#include "secrets.h"
#endif

// ldr (light dependant resistor)
// analog read of pin gives: 0 for full brightness, higher values is darker
static constexpr uint8_t ldr_pin = 34;

// rgb led
static constexpr uint8_t cyd_led_blue = 17;
static constexpr uint8_t cyd_led_red = 4;
static constexpr uint8_t cyd_led_green = 16;

// setting up screen and touch from:
// https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/Examples/Basics/2-TouchTest/2-TouchTest.ino
static constexpr uint8_t xpt2046_irq = 36;
static constexpr uint8_t xpt2046_mosi = 32; // Master Out Slave In
static constexpr uint8_t xpt2046_miso = 39; // Master In Slave Out
static constexpr uint8_t xpt2046_clk = 25;  // Clock
static constexpr uint8_t xpt2046_cs = 33;   // Chip Select

static SPIClass spi{HSPI};
static XPT2046_Touchscreen touch_screen{xpt2046_cs, xpt2046_irq};
static TFT_eSPI display{};

// palette used to convert uint8_t to uint16_t rgb 565
// lower and higher byte swapped (red being the highest bits)
static constexpr uint16_t palette[256]{
    0b0000000000000000, // black
    0b0000000011111000, // red
    0b1110000000000111, // green
    0b0001111100000000, // blue
    0b1111111111111111, // white
};

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

class tile {
public:
  const uint8_t data[tile_width * tile_height];
} static constexpr tiles[tile_count]{
#include "tiles.h"
};

static constexpr unsigned tiles_map_width = 320;
static constexpr unsigned tiles_map_height = 17;

class tiles_map {
public:
  tile_ix cell[tiles_map_height][tiles_map_width];
} static constexpr tiles_map{{
#include "tiles_map.h"
}};

static constexpr unsigned sprite_width = 16;
static constexpr unsigned sprite_height = 16;

// used when rendering
static constexpr int16_t sprite_width_neg = -int16_t(sprite_width);

// images used by sprites
static constexpr uint8_t sprite_imgs[256][sprite_width * sprite_height]{
#include "sprite_imgs.h"
};

using sprite_ix = uint8_t;
// data type used to index a sprite
// note. for 'collision_map' to fit in heap it must be 8-bit

// the reserved 'sprite_ix' in 'collision_map' representing 'no sprite pixel'
static constexpr sprite_ix sprite_ix_reserved =
    std::numeric_limits<sprite_ix>::max();

// forward declaration of type
class object;

class sprite {
public:
  const uint8_t *img = nullptr;
  int16_t scr_x = 0;
  int16_t scr_y = 0;
  sprite_ix collision_with = sprite_ix_reserved;
  sprite_ix alloc_ix = sprite_ix_reserved;
  object *obj = nullptr;

  inline auto get_collision_with_object() -> object *;
  // note. implementation needs access to 'sprites' declared later. circular
  // reference

  inline void clear_collision_with_object() {
    collision_with = sprite_ix_reserved;
  }
};

using sprites_store = o1store<sprite, 255, sprite_ix, 1>;
// note. 255 because sprite_ix a.k.a. uint8_t max size is 255
// note. sprite 255 is reserved which gives 255 [0:254] usable sprites

static sprites_store sprites{};

// implementation needs access to 'sprites'. solves circular reference.
auto sprite::get_collision_with_object() -> object * {
  if (collision_with == sprite_ix_reserved) {
    return nullptr;
  }
  return sprites.instance(collision_with)->obj;
}

// display dimensions
static constexpr unsigned display_width = 320;
static constexpr unsigned display_height = 240;

using object_ix = uint8_t;
// data type used to index an object in 'o1store'

// enumeration of classes of object
enum object_class : uint8_t { object_cls, hero_cls, bullet_cls, dummy_cls };

class object {
public:
  object_ix alloc_ix;
  // note. no default value since it would overwrite the 'o1store' assigned
  // value at 'allocate_instance()'

  object_class cls = object_cls;
  float x = 0;
  float y = 0;
  float dx = 0;
  float dy = 0;
  float ddx = 0;
  float ddy = 0;
  sprite *spr = nullptr;

  object() {}
  // note. constructor must be defined because the default constructor
  // overwrites the 'o1store' assigned 'alloc_ix' at the 'new in place'
  //
  // note. after constructor of inheriting class 'spr' must be in valid state.

  virtual ~object() {
    // Serial.printf("object destructor. alloc_ix=%u\n", alloc_ix);
    // turn off sprite
    spr->img = nullptr;
    // free sprite instance
    sprites.free_instance(spr);
  }

  // returns true if object has died
  virtual auto update(const float dt_s) -> bool {
    dx += ddx * dt_s;
    dy += ddy * dt_s;
    x += dx * dt_s;
    y += dy * dt_s;

    // update rendering info
    spr->scr_x = int16_t(x);
    spr->scr_y = int16_t(y);

    return false;
  }
};

class bullet final : public object {
public:
  int8_t damage = 1;

  bullet() {
    cls = bullet_cls;
    spr = sprites.allocate_instance();
    spr->obj = this;
    spr->img = sprite_imgs[1];
    spr->clear_collision_with_object();
  }

  auto update(const float dt_s) -> bool override {
    if (object::update(dt_s)) {
      return true;
    }
    if (x <= -float(sprite_width) or x > display_width or
        y <= -float(sprite_height) or y > display_height) {
      return true;
    }
    if (spr->get_collision_with_object()) {
      Serial.printf("bullet collided\n");
      return true;
    }

    // spr->clear_collision_with();

    return false;
  }
};

class hero final : public object {
  int16_t health = 10;
  sprite *spr_left;
  sprite *spr_right;

  // returns true if dead
  auto apply_damage_from_collision(sprite *spr) -> bool {
    if (object *obj = spr->get_collision_with_object()) {
      if (obj->cls == bullet_cls) {
        bullet *blt = static_cast<bullet *>(obj);
        health -= blt->damage;
        Serial.printf("hero health: %d\n", health);
        if (health <= 0) {
          return true;
        }
      }
    }
    return false;
  }

public:
  hero() {
    cls = hero_cls;
    spr = sprites.allocate_instance();
    spr->obj = this;
    spr->img = sprite_imgs[0];
    spr->clear_collision_with_object();

    spr_left = sprites.allocate_instance();
    spr_left->obj = this;
    spr_left->img = sprite_imgs[0];
    spr_left->clear_collision_with_object();

    spr_right = sprites.allocate_instance();
    spr_right->obj = this;
    spr_right->img = sprite_imgs[0];
    spr_right->clear_collision_with_object();
  }

  ~hero() override {
    // Serial.printf("hero destructor. alloc_ix=%u\n", alloc_ix);
    // turn off sprites
    spr_left->img = nullptr;
    spr_right->img = nullptr;
    // free sprite instances
    sprites.free_instance(spr_left);
    sprites.free_instance(spr_right);
  }

  auto update(const float dt_s) -> bool override {
    if (object::update(dt_s)) {
      return true;
    }

    // apply damage from maybe collision with bullets
    if (apply_damage_from_collision(spr) or
        apply_damage_from_collision(spr_left) or
        apply_damage_from_collision(spr_right)) {
      return true;
    }

    // reset collision information
    spr->clear_collision_with_object();
    spr_left->clear_collision_with_object();
    spr_right->clear_collision_with_object();

    // set position of additional sprites
    spr_left->scr_x = spr->scr_x;
    spr_left->scr_y = spr->scr_y + sprite_height;

    spr_right->scr_x = spr->scr_x;
    spr_right->scr_y = spr->scr_y - sprite_height;

    return false;
  }
};

class dummy final : public object {
public:
  dummy() { cls = dummy_cls; }

  auto update(const float dt_s) -> bool override {
    if (object::update(dt_s)) {
      return true;
    }
    if (spr->get_collision_with_object()) {
      return true;
    }
    if (x > display_width) {
      return true;
    }

    // spr->clear_collision_with();I

    return false;
  }
};

using object_store = o1store<object, 255, object_ix, 2, 256>;
// note. 255 because object_ix a.k.a. uint8_t max size is 255
//       instance size 256 to fit largest sub-class of 'object'

class objects : public object_store {
public:
  void update(const float dt_s) {
    object_ix *it = allocated_list();
    const object_ix len = allocated_list_len();
    for (object_ix i = 0; i < len; i++, it++) {
      object *obj = instance(*it);
      if (obj->update(dt_s)) {
        obj->~object();
        free_instance(obj);
      }
    }

    // apply free of objects that have died during update
    apply_free();
  }
} static objects{};

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

// scrolling from right to left / down up
static float x = tiles_map_width * tile_width - display_width;
static float dx_per_s = -16;
static float y = 1;
static float dy_per_s = 1;
static unsigned long last_fire_ms = 0;

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

  inline auto get() const -> unsigned { return current_fps_; }

  inline auto now_ms() const -> unsigned long { return now_ms_; }

  inline auto dt_s() const -> float { return dt_s_; }

} static fps{};

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
    // render line of sprite
    for (unsigned j = 0; j < render_width;
         j++, collision_pixel++, scanline_dst_ptr++) {
      // write pixel from sprite data or skip if 0
      const uint8_t color_ix = *spr_data_ptr++;
      if (color_ix) {
        *scanline_dst_ptr = palette[color_ix];
        if (*collision_pixel != sprite_ix_reserved) {
          spr->collision_with = *collision_pixel;
          sprites.instance(*collision_pixel)->collision_with = i;
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
  const tile_ix *tiles_map_row_ptr = tiles_map.cell[tile_y];
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
       tile_y++, frame_y += tile_height, tiles_map_row_ptr += tiles_map_width) {
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

// void setup_scene() {
//   hero *hro = new (objects.allocate_instance()) hero{};
//   // Serial.printf("hero alloc_ix %u\n", hro.alloc_ix);
//   hro->x = 250;
//   hro->y = 100;

//   bullet *blt = new (objects.allocate_instance()) bullet{};
//   // Serial.printf("bullet alloc_ix %u\n", blt.alloc_ix);
//   blt->x = 50;
//   blt->y = 100;
//   blt->dx = 40;
// }

void setup_scene() {
  float x = -24, y = -24;
  for (object_ix i = 0; i < objects.all_list_len(); i++) {
    dummy *obj = new (objects.allocate_instance()) dummy{};
    sprite *spr = sprites.allocate_instance();
    spr->clear_collision_with_object();
    spr->img = sprite_imgs[i % 2];
    spr->obj = obj;
    obj->spr = spr;
    obj->x = x;
    obj->y = y;
    obj->dx = 0.5f;
    obj->dy = 2.0f - float(rand() % 4);
    x += 24;
    if (x > display_width) {
      x = -24;
      y += 24;
    }
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
  Serial.printf("         tiles map: %zu B\n", sizeof(tiles_map));
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

  setup_scene();

  // initiate frames-per-second and dt keeper
  fps.init(millis());
}

void loop() {
  // frames per second update
  if (fps.on_frame(millis())) {
    Serial.printf("t=%lu  fps=%u  ldr=%u  objs=%u  sprs=%u\n", fps.now_ms(),
                  fps.get(), analogRead(ldr_pin), objects.allocated_list_len(),
                  sprites.allocated_list_len());
  }

  // check touch screen
  if (touch_screen.tirqTouched() and touch_screen.touched()) {
    const TS_Point pt = touch_screen.getPoint();
    const int x_relative_center = pt.x - 4096 / 2;
    constexpr float dx_factor = 200.0f / (4096 / 2);
    dx_per_s = dx_factor * x_relative_center;
    const float click_y = pt.y * display_height / 4096;
    Serial.printf("touch x=%d  y=%d  z=%d  click_y=%f  dx=%f\n", pt.x, pt.y,
                  pt.z, click_y, dx_per_s);

    // fire four times a second
    if (fps.now_ms() - last_fire_ms > 250) {
      last_fire_ms = fps.now_ms();
      if (objects.can_allocate()) {
        bullet *blt = new (objects.allocate_instance()) bullet{};
        // Serial.printf("bullet alloc_ix %u\n", blt.alloc_ix);
        blt->x = 50;
        blt->y = click_y;
        blt->dx = 40;
      }
    }
  }

  // Serial.printf("t=%lu\n", millis());

  objects.update(fps.dt_s());

  sprites.apply_free();

  // clear collisions map
  memset(collision_map, sprite_ix_reserved, collision_map_size);

  // render tiles, sprites and collision map
  display.startWrite();
  render(unsigned(x), unsigned(y));
  display.endWrite();

  // update x position in pixels in the tile map
  x += dx_per_s * fps.dt_s();
  if (x < 0) {
    x = 0;
    dx_per_s = -dx_per_s;
  } else if (x > (tiles_map_width * tile_width - display_width)) {
    x = tiles_map_width * tile_width - display_width;
    dx_per_s = -dx_per_s;
  }
  // update y position in pixels in the tile map
  y += dy_per_s * fps.dt_s();
  // Serial.printf("x=%f  y=%f  dy=%f\n", x, y, dy_per_s);
  if (y < 0) {
    Serial.printf("y < 0\n");
    y = 0;
    dy_per_s = -dy_per_s;
  } else if (y > (tiles_map_height * tile_height - display_height)) {
    y = tiles_map_height * tile_height - display_height;
    dy_per_s = -dy_per_s;
  }
}

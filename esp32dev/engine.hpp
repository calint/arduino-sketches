#pragma once
#include "o1store.hpp"

using collision_bits = unsigned;
// used by 'object' for collision detection interest flags

// including defs for engine and game
#include "game/defs.hpp"

// palette used to convert uint8_t to uint16_t rgb 565
// lower and higher byte swapped (red being the highest bits)
static constexpr uint16_t palette_tiles[256]{
#include "game/resources/palette_tiles.hpp"
};

static constexpr uint16_t palette_sprites[256]{
#include "game/resources/palette_sprites.hpp"
};

// tile dimensions
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

using tile_ix = uint8_t;

class tile {
public:
  const uint8_t data[tile_width * tile_height];
} static constexpr tiles[tile_count]{
#include "game/resources/tile_imgs.hpp"
};

class tile_map {
public:
  tile_ix cell[tile_map_height][tile_map_width];
} static constexpr tile_map{{
#include "game/resources/tile_map.hpp"
}};

// tile map controls
static float tile_map_x = 0;
static float tile_map_dx = 0;
static float tile_map_y = 0;
static float tile_map_dy = 0;

// sprite dimensions
static constexpr unsigned sprite_width = 16;
static constexpr unsigned sprite_height = 16;

static constexpr int16_t sprite_width_neg = -int16_t(sprite_width);
// used when rendering

// images used by sprites
static constexpr uint8_t sprite_imgs[sprite_imgs_count]
                                    [sprite_width * sprite_height]{
#include "game/resources/sprite_imgs.hpp"
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
  object *obj = nullptr;
  uint8_t const *img = nullptr;
  int16_t scr_x = 0;
  int16_t scr_y = 0;
  sprite_ix alloc_ix = sprite_ix_reserved;
};

using sprites_store = o1store<sprite, 255, sprite_ix, 1>;
// note. 255 because sprite_ix a.k.a. uint8_t max size is 255
// note. sprite 255 is reserved which gives 255 [0:254] usable sprites

static sprites_store sprites{};

// display dimensions
static constexpr unsigned display_width = display_orientation == 0 ? 240 : 320;
static constexpr unsigned display_height = display_orientation == 0 ? 320 : 240;

// pixel precision collision detection between on screen sprites
// allocated in setup
static sprite_ix *collision_map;
static constexpr unsigned collision_map_size =
    sizeof(sprite_ix) * display_width * display_height;

// helper class managing current frame time, dt, frames per second calculation
class clk {
  unsigned interval_ms_ = 5000;
  unsigned frames_rendered_in_interval_ = 0;
  unsigned long last_update_ms_ = 0;
  unsigned long now_ms_ = 0;
  unsigned long prv_now_ms_ = 0;
  unsigned current_fps_ = 0;
  float dt_s_ = 0;

public:
  // called at setup with current time and frames per seconds calculation
  // interval
  void init(const unsigned long now_ms,
            const unsigned interval_of_fps_calculations_ms) {
    last_update_ms_ = prv_now_ms_ = now_ms;
    interval_ms_ = interval_of_fps_calculations_ms;
  }

  // called before every frame to update state
  // returns true if new frames per second calculation was done
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

  // returns current frames per second calculated at interval specified at
  // 'init'
  inline auto fps() const -> unsigned { return current_fps_; }

  // returns current time since boot in milliseconds
  inline auto now_ms() const -> unsigned long { return now_ms_; }

  // returns frame delta time in seconds
  inline auto dt_s() const -> float { return dt_s_; }

  // returns argument multiplied by delta time in seconds for frame
  inline auto dt(float f) const -> float { return f * dt_s_; }

} static clk{};

using object_ix = uint8_t;
// data type used to index an 'object' in 'o1store'

enum object_class : uint8_t;
// enum declared in "game/defs.hpp" defining the game objects

class object {
public:
  sprite *spr = nullptr;
  object *col_with = nullptr;
  collision_bits col_bits = 0;
  collision_bits col_mask = 0;
  // note: used to declare interest in collisions with objects whose
  // 'col_bits' bitwise AND with this 'col_mask' is not 0

  float x = 0;
  float y = 0;
  float dx = 0;
  float dy = 0;
  float ddx = 0;
  float ddy = 0;

  object_ix alloc_ix;
  // note. no default value since it would overwrite the 'o1store' assigned
  // value at 'allocate_instance()'

  const object_class cls;

  object(object_class c) : cls{c} {}
  // note. constructor must be defined because the default constructor
  // overwrites the 'o1store' assigned 'alloc_ix' at the 'new in place'
  //
  // note. after constructor of inheriting class 'spr' must be in valid state.

  virtual ~object() {
    // turn off sprite
    spr->img = nullptr;
    // free sprite instance
    sprites.free_instance(spr);
  }

  // returns true if object has died
  // note. regarding classes overriding 'update(...)'
  // after 'update(...)' 'col_with' should be 'nullptr'
  virtual auto update() -> bool {
    dx += clk.dt(ddx);
    dy += clk.dt(ddy);
    x += clk.dt(dx);
    y += clk.dt(dy);

    // update rendering info
    spr->scr_x = int16_t(x);
    spr->scr_y = int16_t(y);

    return false;
  }
};

using object_store =
    o1store<object, 255, object_ix, 2, object_instance_max_size_B>;
// note. 255 because object_ix a.k.a. uint8_t max size is 255
//       instance size 256 to fit largest sub-class of 'object'

class objects : public object_store {
public:
  void update() {
    object_ix *it = allocated_list();
    const object_ix len = allocated_list_len();
    for (object_ix i = 0; i < len; i++, it++) {
      object *obj = instance(*it);
      if (obj->update()) {
        obj->~object();
        free_instance(obj);
      }
    }

    // apply free of objects that have died during update
    apply_free();
  }
} static objects{};

// forward declaration of platform specific function
static void render(const unsigned x, const unsigned y);

// forward declaration of user provided callback
static void main_on_frame_completed();

// update and render the state of the engine
static void engine_loop() {
  objects.update();

  // apply freed sprites during 'objects.update()'
  sprites.apply_free();

  // clear collisions map
  memset(collision_map, sprite_ix_reserved, collision_map_size);

  // render tiles, sprites and collision map
  render(unsigned(tile_map_x), unsigned(tile_map_y));

  // game logic hook
  main_on_frame_completed();
}
#pragma once
#include "o1store.hpp"

#include "game/resources/palette.hpp"

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

// number of different tiles
static constexpr unsigned tile_count = 256;
using tile_ix = uint8_t;

class tile {
public:
  const uint8_t data[tile_width * tile_height];
} static constexpr tiles[tile_count]{
#include "game/resources/tile_imgs.hpp"
};

// tile map dimension
static constexpr unsigned tile_map_width = 320;
static constexpr unsigned tile_map_height = 17;

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
static constexpr uint8_t sprite_imgs[256][sprite_width * sprite_height]{
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
static constexpr unsigned display_width = 320;
static constexpr unsigned display_height = 240;

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

  inline auto fps() const -> unsigned { return current_fps_; }

  inline auto now_ms() const -> unsigned long { return now_ms_; }

  inline auto dt_s() const -> float { return dt_s_; }

  inline auto dt(float f) const -> float { return f * dt_s_; }

} static clk{};

using object_ix = uint8_t;
// data type used to index an 'object' in 'o1store'

using collision_bits = unsigned;
// used by 'object' for collision detection interest flags

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
    // Serial.printf("object destructor. alloc_ix=%u\n", alloc_ix);
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

#include "game/defs.hpp"

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

#pragma once
// setup code, touch screen callback, frame completed callback
// solves circular references between 'game' and game objects

#include "../engine.hpp"

#include "game.hpp"

#include "objects/bullet.hpp"
#include "objects/hero.hpp"
#include "objects/ship1.hpp"

static void main_setup() {
  // scrolling vertically from bottom up
  tile_map_x = 0;
  tile_map_y = tile_map_height * tile_height - display_height;
  tile_map_dy = -16;
  // tile_map_y = 0;
  // tile_map_dy = 1;

  hero *hro = new (objects.allocate_instance()) hero{};
  hro->x = 100;
  hro->y = 30;

  bullet *blt = new (objects.allocate_instance()) bullet{};
  blt->x = 100;
  blt->y = 300;
  blt->dy = -100;
}

// calibration of touch screen
constexpr int16_t touch_screen_value_min = 300;
constexpr int16_t touch_screen_value_max = 3600;
constexpr int16_t touch_screen_value_range =
    touch_screen_value_max - touch_screen_value_min;

unsigned long last_fire_ms = 0;
// keeps track of when the previous bullet was fired

// callback when screen is touched, happens before 'update'
static void main_on_touch_screen(int16_t x, int16_t y, int16_t z) {
  // const int y_relative_center =
  //     y - touch_screen_value_min - touch_screen_value_range / 2;
  // constexpr float dy_factor = 200.0f / (touch_screen_value_range / 2);
  // tile_map_dy = dy_factor * y_relative_center;

  // fire eight times a second
  if (clk.now_ms() - last_fire_ms > 125) {
    // Serial.printf("touch  x=%u  y=%u\n", x, y);
    last_fire_ms = clk.now_ms();
    if (objects.can_allocate()) {
      bullet *blt = new (objects.allocate_instance()) bullet{};
      blt->x = (x - touch_screen_value_min) * display_width /
               touch_screen_value_range;
      blt->y = 300;
      blt->dy = -100;
    }
  }
}

// callback after frame has been rendered, happens after 'update'
static void main_on_frame_completed() {
  // update x position in pixels in the tile map
  tile_map_x += clk.dt(tile_map_dx);
  if (tile_map_x < 0) {
    tile_map_x = 0;
    tile_map_dx = -tile_map_dx;
  } else if (tile_map_x > (tile_map_width * tile_width - display_width)) {
    tile_map_x = tile_map_width * tile_width - display_width;
    tile_map_dx = -tile_map_dx;
  }
  // update y position in pixels in the tile map
  tile_map_y += clk.dt(tile_map_dy);
  if (tile_map_y < 0) {
    tile_map_y = 0;
    tile_map_dy = -tile_map_dy;
  } else if (tile_map_y > (tile_map_height * tile_height - display_height)) {
    tile_map_y = tile_map_height * tile_height - display_height;
    tile_map_dy = -tile_map_dy;
  }

  if (not game.hero_is_alive) {
    hero *hro = new (objects.allocate_instance()) hero{};
    hro->x = float(rand()) * display_width / RAND_MAX;
    hro->y = 30;
    hro->dx = float(rand()) * 64 / RAND_MAX;
  }

  if (game.wave_objects_alive == 0) {
    if (not game.wave_done_waiting) {
      game.wave_done_ms = clk.now_ms();
      game.wave_done_waiting = true;
    } else if (clk.now_ms() - game.wave_done_ms > 3000) {
      // last wave was terminated more than 3 seconds ago
      game.wave_done_waiting = false;
      float x = 24;
      float y = -float(sprite_height);
      for (unsigned i = 0; i < 7; i++) {
        if (!objects.can_allocate()) {
          break;
        }
        ship1 *enm = new (objects.allocate_instance()) ship1{};
        enm->x = x;
        enm->y = y;
        enm->dy = 30;
        x += 32;
        y -= 8;
        Serial.printf("enemy wave. %u  x=%f  y=%f\n", i, x, y);
      }
    }
  }
}

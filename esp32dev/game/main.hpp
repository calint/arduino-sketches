#pragma once
// contains setup code, controller abstraction and callback for logic after a
// frame has been rendered solving circular references between 'game' and
// game objects

// include first section of the program
#include "../engine.hpp"
// include game logic
#include "game.hpp"
// include objects
#include "objects/bullet.hpp"
#include "objects/dummy.hpp"
#include "objects/hero.hpp"

static void setup_scene() {
  // scrolling from right to left / down up
  tile_map_x = tile_map_width * tile_width - display_width;
  // tile_map_dx = -16;
  tile_map_y = 1;
  tile_map_dy = 1;

  hero *hro = new (objects.allocate_instance()) hero{};
  hro->x = 300;
  hro->y = 100;

  bullet *blt = new (objects.allocate_instance()) bullet{};
  blt->x = 50;
  blt->y = 100;
  blt->dx = 40;
}

unsigned long last_fire_ms = 0;
// keeps track of when the previous bullet was fired

// callback when screen is touched, happens before 'update'
static void main_on_touch_screen(int16_t x, int16_t y, int16_t z) {
  const int x_relative_center = x - 4096 / 2;
  constexpr float dx_factor = 200.0f / (4096 / 2);
  tile_map_dx = dx_factor * x_relative_center;
  const float click_y = y * display_height / 4096;

  // fire eight times a second
  if (clk.now_ms() - last_fire_ms > 125) {
    last_fire_ms = clk.now_ms();
    if (objects.can_allocate()) {
      bullet *blt = new (objects.allocate_instance()) bullet{};
      blt->x = 50;
      blt->y = click_y;
      blt->dx = 100;
    }
  }
}

// callback after frame has been rendered, happens after 'update'
static void main_on_after_frame() {
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
    hro->x = 300;
    hro->y = float(rand()) * display_height / RAND_MAX;
  }
}

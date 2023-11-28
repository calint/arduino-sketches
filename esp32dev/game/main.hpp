#pragma once
#include "../preamble.hpp"
#include "objects/bullet.hpp"
#include "objects/dummy.hpp"
#include "objects/hero.hpp"

static void setup_scene() {
  // scrolling from right to left / down up
  tile_map_x = tile_map_width * tile_width - display_width;
  tile_map_dx = -16;
  tile_map_y = 1;
  tile_map_dy = 1;

  hero *hro = new (objects.allocate_instance()) hero{};
  hro->x = 250;
  hro->y = 100;

  bullet *blt = new (objects.allocate_instance()) bullet{};
  blt->x = 50;
  blt->y = 100;
  blt->dx = 40;
}

// void setup_scene() {
//   float x = -24, y = -24;
//   for (object_ix i = 0; i < objects.all_list_len(); i++) {
//     dummy *obj = new (objects.allocate_instance()) dummy{};
//     sprite *spr = sprites.allocate_instance();
//     const object_ix type = i % 2;
//     spr->img = sprite_imgs[type];
//     spr->obj = obj;
//     obj->spr = spr;
//     if (type == 0) {
//       // if square
//       // set collision bit 1
//       obj->col_bits = 1;
//       // interested in collision with
//       // collision bit 1 or 2 set
//       obj->col_mask = 3;
//       // 'squares' react to collisions with 'squares' and 'bullets'
//     } else {
//       // if square
//       // set collision bit 2
//       obj->col_bits = 2;
//       // interested in collision with
//       // collision bit 2 set
//       obj->col_mask = 2;
//       // 'bullets' react to collisions with 'bullets'
//     }
//     obj->x = x;
//     obj->y = y;
//     obj->dx = 0.5f;
//     obj->dy = 2.0f - float(rand() % 4);
//     x += 24;
//     if (x > display_width) {
//       x = -24;
//       y += 24;
//     }
//   }
// }

class controller {
  unsigned long last_fire_ms = 0;

public:
  void on_touch(int16_t x, int16_t y, int16_t z) {
    const int x_relative_center = x - 4096 / 2;
    constexpr float dx_factor = 200.0f / (4096 / 2);
    tile_map_dx = dx_factor * x_relative_center;
    const float click_y = y * display_height / 4096;
    // Serial.printf("touch x=%d  y=%d  z=%d  click_y=%f  dx=%f\n", pt.x, pt.y,
    //               pt.z, click_y, dx_per_s);

    // fire eight times a second
    if (clk.now_ms() - last_fire_ms > 125) {
      last_fire_ms = clk.now_ms();
      if (objects.can_allocate()) {
        bullet *blt = new (objects.allocate_instance()) bullet{};
        // Serial.printf("bullet alloc_ix %u\n", blt.alloc_ix);
        blt->x = 50;
        blt->y = click_y;
        blt->dx = 40;
      }
    }
  }
} static controller{};

// callback after frame has been rendered
static void on_after_frame() {
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
  // Serial.printf("x=%f  y=%f  dy=%f\n", x, y, dy_per_s);
  if (tile_map_y < 0) {
    Serial.printf("y < 0\n");
    tile_map_y = 0;
    tile_map_dy = -tile_map_dy;
  } else if (tile_map_y > (tile_map_height * tile_height - display_height)) {
    tile_map_y = tile_map_height * tile_height - display_height;
    tile_map_dy = -tile_map_dy;
  }
}

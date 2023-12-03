#pragma once
#include "../../engine.hpp"

class ship2 final : public object {
  static constexpr unsigned animation_rate_ms = 250;
  clk_time_ms animation_frame_ms = 0;
  uint8_t animation_frame_ix = 0;

public:
  ship2() : object{ship2_cls}, animation_frame_ms{clk.ms} {
    col_bits = cb_hero;
    col_mask = cb_enemy_bullet;

    spr = sprites.allocate_instance();
    spr->obj = this;
    spr->img = sprite_imgs[6];
  }

  // returns true if object died
  auto update() -> bool override {
    if (object::update()) {
      return true;
    }

    if (y > display_height) {
      return true;
    }

    if (col_with) {
      return true;
    }

    // animate sprite
    const unsigned ms_since_last_update = clk.ms - animation_frame_ms;
    if (ms_since_last_update > animation_rate_ms) {
      animation_frame_ms = clk.ms;
      switch (animation_frame_ix) {
      case 0:
        animation_frame_ix = 1;
        spr->img = sprite_imgs[7];
        break;
      case 1:
        animation_frame_ix = 0;
        spr->img = sprite_imgs[6];
        break;
      default:
        animation_frame_ix = 0;
        spr->img = sprite_imgs[6];
      }
    }

    return false;
  }
};

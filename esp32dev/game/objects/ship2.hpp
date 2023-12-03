#pragma once
#include "../../engine.hpp"

class ship2 final : public object {
  // animation definition
  inline static constexpr sprite_ix animation_frames[]{6, 7};
  static constexpr sprite_ix animation_frames_len =
      sizeof(animation_frames) / sizeof(sprite_ix);
  static constexpr unsigned animation_rate_ms = 500;

  // animation state
  uint8_t animation_frames_ix = 0;
  clk_time_ms animation_frame_ms = 0;

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

    // animation logic
    const unsigned ms_since_last_update = clk.ms - animation_frame_ms;
    if (ms_since_last_update > animation_rate_ms) {
      animation_frame_ms = clk.ms;
      animation_frames_ix++;
      if (animation_frames_ix >= animation_frames_len) {
        animation_frames_ix = 0;
      }
      spr->img = sprite_imgs[animation_frames[animation_frames_ix]];
    }

    return false;
  }
};

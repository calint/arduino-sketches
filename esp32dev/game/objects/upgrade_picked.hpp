#pragma once
#include "../../engine.hpp"

#include "game_object.hpp"

class upgrade_picked final : public game_object {
  clk::time death_at_ms = 0;

public:
  upgrade_picked() : game_object{upgrade_picked_cls} {
    col_bits = cb_none;
    col_mask = cb_none;

    spr = sprites.allocate_instance();
    spr->obj = this;
    spr->img = sprite_imgs[9];

    death_at_ms = clk.ms + 1000;
  }

  // returns true if object died
  auto update() -> bool override {
    if (clk.ms >= death_at_ms) {
      return true;
    }
    return false;
  }
};

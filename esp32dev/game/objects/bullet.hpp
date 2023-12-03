#pragma once
#include "../../engine.hpp"

#include "fragment.hpp"

class bullet final : public object {
public:
  int8_t damage = 1;

  bullet() : object{bullet_cls} {
    col_bits = cb_enemy_bullet;
    col_mask = cb_hero;

    spr = sprites.allocate_instance();
    spr->obj = this;
    spr->img = sprite_imgs[1];
  }

  // returns true if object died
  auto update() -> bool override {
    if (object::update()) {
      return true;
    }
    if (x <= -float(sprite_width) or x >= display_width or
        y <= -float(sprite_height) or y >= display_height) {
      return true;
    }
    if (col_with) {
      // Serial.printf("bullet collided\n");
      if (not objects.can_allocate()) {
        return true;
      }
      fragment *frg = new (objects.allocate_instance()) fragment{};
      frg->die_at_ms = clk.ms + 250;
      frg->x = x;
      frg->y = y;
      return true;
    }
    return false;
  }
};

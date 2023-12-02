#pragma once
#include "../../engine.hpp"
// include game logic
#include "../game.hpp"

class ship1 final : public object {
public:
  ship1() : object{ship1_cls} {
    col_bits = cb_hero;
    col_mask = cb_enemy_bullet;

    spr = sprites.allocate_instance();
    spr->obj = this;
    spr->img = sprite_imgs[5];
    game.wave_objects_alive++;
  }

  ~ship1() override { game.wave_objects_alive--; }

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

    return false;
  }
};

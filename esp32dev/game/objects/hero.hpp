#pragma once
// include first section of the program
#include "../../engine.hpp"
// include definitions shared by all objects and game engine
#include "../defs.hpp"
// include game logic
#include "../game.hpp"
// include dependencies
#include "bullet.hpp"
#include "fragment.hpp"

class hero final : public object {
  sprite *spr_left;
  sprite *spr_right;
  int16_t health = 10;

public:
  hero() : object{hero_cls} {
    col_bits = cb_hero;
    col_mask = cb_enemy | cb_enemy_bullet;

    spr = sprites.allocate_instance();
    spr->obj = this;
    spr->img = sprite_imgs[0];

    spr_left = sprites.allocate_instance();
    spr_left->obj = this;
    spr_left->img = sprite_imgs[0];

    spr_right = sprites.allocate_instance();
    spr_right->obj = this;
    spr_right->img = sprite_imgs[0];

    game.hero_is_alive = true;
  }

  ~hero() override {
    // turn off sprites
    spr_left->img = nullptr;
    spr_right->img = nullptr;
    // free sprite instances
    sprites.free_instance(spr_left);
    sprites.free_instance(spr_right);

    game.hero_is_alive = false;
  }

  // returns true if object died
  auto update() -> bool override {
    if (object::update()) {
      return true;
    }

    if (col_with) {
      if (col_with->cls == bullet_cls) {
        bullet *blt = static_cast<bullet *>(col_with);
        health -= blt->damage;
        Serial.printf("hero health: %d\n", health);
        if (health <= 0) {
          create_fragments();
          return true;
        }
      }
      // reset collision information
      col_with = nullptr;
    }

    // set position of additional sprites
    spr_left->scr_x = spr->scr_x - sprite_width;
    spr_left->scr_y = spr->scr_y;

    spr_right->scr_x = spr->scr_x + sprite_width;
    spr_right->scr_y = spr->scr_y;

    return false;
  }

private:
  static constexpr float frag_speed = 200;
  static constexpr unsigned frag_count = 16;

  void create_fragments() {
    for (unsigned i = 0; i < frag_count; i++) {
      if (not objects.can_allocate()) {
        break;
      }
      fragment *frg = new (objects.allocate_instance()) fragment{};
      frg->die_at_ms = clk.now_ms() + 500;
      frg->x = x;
      frg->y = y;
      frg->dx = frag_speed * rand() / RAND_MAX - frag_speed / 2;
      frg->dy = frag_speed * rand() / RAND_MAX - frag_speed / 2;
      frg->ddx = frag_speed * rand() / RAND_MAX - frag_speed / 2;
      frg->ddy = frag_speed * rand() / RAND_MAX - frag_speed / 2;
    }
  }
};

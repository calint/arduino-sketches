#pragma once
#include "../../preamble.hpp"
#include "../defs.hpp"

#include "bullet.hpp"
#include "fragment.hpp"

class hero final : public object {
  sprite *spr_left;
  sprite *spr_right;
  int16_t health = 10;

public:
  hero() {
    cls = hero_cls;

    col_bits = 1;
    col_mask = 3;

    spr = sprites.allocate_instance();
    spr->obj = this;
    spr->img = sprite_imgs[0];

    spr_left = sprites.allocate_instance();
    spr_left->obj = this;
    spr_left->img = sprite_imgs[0];

    spr_right = sprites.allocate_instance();
    spr_right->obj = this;
    spr_right->img = sprite_imgs[0];
  }

  ~hero() override {
    // Serial.printf("hero destructor. alloc_ix=%u\n", alloc_ix);
    // turn off sprites
    spr_left->img = nullptr;
    spr_right->img = nullptr;
    // free sprite instances
    sprites.free_instance(spr_left);
    sprites.free_instance(spr_right);
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
    spr_left->scr_x = spr->scr_x;
    spr_left->scr_y = spr->scr_y + sprite_height;

    spr_right->scr_x = spr->scr_x;
    spr_right->scr_y = spr->scr_y - sprite_height;

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

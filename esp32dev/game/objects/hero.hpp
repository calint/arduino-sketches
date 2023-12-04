#pragma once
#include "../../engine.hpp"
// include game logic
#include "../game.hpp"
// include dependencies
#include "bullet.hpp"
#include "fragment.hpp"
#include "game_object.hpp"

class hero final : public game_object {
  sprite *spr_left;
  sprite *spr_right;

public:
  hero() : game_object{hero_cls} {
    col_bits = cb_hero;
    col_mask = cb_enemy | cb_enemy_bullet;

    hlth = 10;

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
    if (game_object::update()) {
      return true;
    }

    if (x > display_width) {
      dx = -dx;
      x = display_width;
    } else if (x < sprite_width_neg) {
      dx = -dx;
      x = sprite_width_neg;
    }

    return false;
  }

  void on_death_by_collision() override { create_fragments(); }

  void pre_render() override {
    game_object::pre_render();

    // set position of additional sprites
    spr_left->scr_x = spr->scr_x - sprite_width;
    spr_left->scr_y = spr->scr_y;

    spr_right->scr_x = spr->scr_x + sprite_width;
    spr_right->scr_y = spr->scr_y;
  }

private:
  static constexpr float frag_speed = 300;
  static constexpr unsigned frag_count = 16;

  void create_fragments() {
    for (unsigned i = 0; i < frag_count; i++) {
      if (not objects.can_allocate()) {
        break;
      }
      fragment *frg = new (objects.allocate_instance()) fragment{};
      frg->die_at_ms = clk.ms + 500;
      frg->x = x;
      frg->y = y;
      frg->dx = frag_speed * rand() / RAND_MAX - frag_speed / 2;
      frg->dy = frag_speed * rand() / RAND_MAX - frag_speed / 2;
      frg->ddx = frag_speed * rand() / RAND_MAX - frag_speed / 2;
      frg->ddy = frag_speed * rand() / RAND_MAX - frag_speed / 2;
    }
  }
};

#pragma once
#include "../../engine.hpp"

class game_object : public object {
public:
  sprite *spr = nullptr;

  float x = 0;
  float y = 0;
  float dx = 0;
  float dy = 0;
  float ddx = 0;
  float ddy = 0;

  uint16_t hlth = 0;
  uint16_t dmg = 0;

  object_class cls;

  game_object(object_class c) : cls{c} {}
  // note. constructor must be defined because the default constructor
  // overwrites the 'o1store' assigned 'alloc_ix' at the 'new in place'
  //
  // note. after constructor of inheriting class 'spr' must be in valid state.

  virtual ~game_object() {
    // turn off sprite
    spr->img = nullptr;
    // free sprite instance
    sprites.free_instance(spr);
  }

  // returns true if object has died
  // note. regarding classes overriding 'update(...)'
  // after 'update(...)' 'col_with' should be 'nullptr'
  virtual auto update() -> bool override {
    if (col_with) {
      if (on_collision(static_cast<game_object *>(col_with))) {
        return true;
      }
      col_with = nullptr;
    }

    dx += ddx * clk.dt;
    dy += ddy * clk.dt;
    x += dx * clk.dt;
    y += dy * clk.dt;

    return false;
  }

  // sets sprite screen position prior to render
  virtual void pre_render() override {
    // update rendering info
    spr->scr_x = int16_t(x);
    spr->scr_y = int16_t(y);
  }

  // called from 'update' if object in collision
  // returns true if object has died
  virtual auto on_collision(game_object *obj) -> bool {
    if (obj->dmg >= hlth) {
      on_death_by_collision();
      return true;
    }
    hlth -= obj->dmg;
    return false;
  }

  // called from 'on_collision' if object has died due to collision
  virtual void on_death_by_collision() {}
};

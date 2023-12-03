#pragma once
#include "../../engine.hpp"

class fragment final : public object {
public:
  int8_t damage = 1;
  clk::time die_at_ms;

  fragment() : object{fragment_cls} {
    col_bits = cb_fragment;
    col_mask = cb_none;

    spr = sprites.allocate_instance();
    spr->obj = this;
    spr->img = sprite_imgs[2];
  }

  // returns true if object died
  auto update() -> bool override {
    if (object::update()) {
      return true;
    }
    if (clk.ms > die_at_ms) {
      return true;
    }
    return false;
  }
};

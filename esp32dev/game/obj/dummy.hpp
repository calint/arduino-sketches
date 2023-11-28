#pragma once
#include "../../preamble.hpp"
#include "../defs.hpp"

class dummy final : public object {
public:
  dummy() { cls = dummy_cls; }

  auto update() -> bool override {
    if (object::update()) {
      return true;
    }
    if (col_with) {
      return true;
    }
    if (x > display_width) {
      return true;
    }
    return false;
  }
};

#pragma once
#include "../../engine.hpp"

class dummy final : public object {
public:
  dummy() : object{dummy_cls} {}

  auto update() -> bool override {
    if (object::update()) {
      return true;
    }
    if (x > display_width) {
      return true;
    }
    return false;
  }
};

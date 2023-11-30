#pragma once
// include first section of the program
#include "../../engine.hpp"
// include definitions shared by all objects and game engine
#include "../defs.hpp"

class dummy final : public object {
public:
  dummy() : object{dummy_cls} {}

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

#pragma once
// imported by objects that access the game state

class game final {
public:
  bool hero_is_alive = false;
  uint8_t wave_objects_alive = 0;
  bool wave_done_waiting = false;
  unsigned long wave_done_ms = 0;
} game{};
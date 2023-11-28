#pragma once

enum object_class : uint8_t { hero_cls, bullet_cls, dummy_cls, fragment_cls };
// enumeration of game object classes

// collision bits
static constexpr unsigned cb_none = 0;
static constexpr unsigned cb_hero = 1 << 0;
static constexpr unsigned cb_hero_bullet = 1 << 1;
static constexpr unsigned cb_fragment = 1 << 2;
static constexpr unsigned cb_enemy = 1 << 3;
static constexpr unsigned cb_enemy_bullet = 1 << 4;

static constexpr unsigned object_instance_max_size_B = 256;
// enough to fit any instance of game object

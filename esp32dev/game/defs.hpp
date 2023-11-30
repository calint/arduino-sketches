#pragma once
// constants and definitions used by the engine and game objects

enum object_class : uint8_t { hero_cls, bullet_cls, dummy_cls, fragment_cls };
// enumeration of game object classes

// collision bits
static constexpr collision_bits cb_none = 0;
static constexpr collision_bits cb_hero = 1 << 0;
static constexpr collision_bits cb_hero_bullet = 1 << 1;
static constexpr collision_bits cb_fragment = 1 << 2;
static constexpr collision_bits cb_enemy = 1 << 3;
static constexpr collision_bits cb_enemy_bullet = 1 << 4;

static constexpr unsigned object_instance_max_size_B = 256;
// enough to fit any instance of game object

// number of different tiles (maximum 256)
static constexpr unsigned tile_count = 256;

// tile map dimension
static constexpr unsigned tile_map_width = 320;
static constexpr unsigned tile_map_height = 17;

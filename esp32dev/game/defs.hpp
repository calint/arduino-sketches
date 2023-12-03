#pragma once
// constants and definitions used by the engine and game objects

// number of sprite images
// defined in 'resources/sprite_imgs.hpp'
static constexpr unsigned sprite_imgs_count = 256;

// number of tile images (maximum 256)
// defined in 'resources/tile_imgs.hpp'
static constexpr unsigned tile_count = 256;

// tile map dimension
// defined in 'resources/tile_map.hpp'
static constexpr unsigned tile_map_width = 15;
static constexpr unsigned tile_map_height = 320;

// size that fits any instance of game object
static constexpr unsigned object_instance_max_size_B = 256;

// enumeration of game object classes
// defined in 'objects/*'
enum object_class : uint8_t {
  hero_cls,
  bullet_cls,
  dummy_cls,
  fragment_cls,
  ship1_cls
};

// collision bits
static constexpr collision_bits cb_none = 0;
static constexpr collision_bits cb_hero = 1 << 0;
static constexpr collision_bits cb_hero_bullet = 1 << 1;
static constexpr collision_bits cb_fragment = 1 << 2;
static constexpr collision_bits cb_enemy = 1 << 3;
static constexpr collision_bits cb_enemy_bullet = 1 << 4;

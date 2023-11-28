#pragma once

enum object_class : uint8_t { hero_cls, bullet_cls, dummy_cls, fragment_cls };
// enumeration of game object classes

static constexpr unsigned object_instance_max_size_B = 256;
// enough to fit any instance of game object

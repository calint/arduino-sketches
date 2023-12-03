#!/bin/bash
set -e
cd $(dirname "$0")

./read-palette sprites.png > ../../game/resources/palette_sprites.hpp
./read-sprites sprites.png > ../../game/resources/sprite_imgs.hpp

./read-palette tiles.png > ../../game/resources/palette_tiles.hpp
./read-sprites tiles.png > ../../game/resources/tile_imgs.hpp
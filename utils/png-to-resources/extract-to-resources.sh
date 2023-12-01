#!/bin/bash
set -e
cd $(dirname "$0")

./read-palette sprites.png > ../../esp32dev/game/resources/palette_sprites.hpp
./read-sprites sprites.png > ../../esp32dev/game/resources/sprite_imgs.hpp

./read-palette tiles.png > ../../esp32dev/game/resources/palette_tiles.hpp
./read-sprites tiles.png > ../../esp32dev/game/resources/tile_imgs.hpp
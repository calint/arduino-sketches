#!/bin/bash
set -e
cd $(dirname "$0")

./read-palette.py sprites.png > ../../game/resources/palette_sprites.hpp
./read-sprites.py sprites.png > ../../game/resources/sprite_imgs.hpp

./read-palette.py tiles.png > ../../game/resources/palette_tiles.hpp
./read-sprites.py tiles.png > ../../game/resources/tile_imgs.hpp
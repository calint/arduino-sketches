#!/bin/bash
set -e

./read-palette sprites.png > ../../esp32dev/game/resources/palette.hpp
./read-sprites sprites.png > ../../esp32dev/game/resources/sprite_imgs.hpp
./read-sprites tiles.png > ../../esp32dev/game/resources/tile_imgs.hpp
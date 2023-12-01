#!/bin/bash
set -e

clang++ -o read-palette read-palette.cpp -lpng
clang++ -o read-sprite read-sprite.cpp -lpng
#!/bin/bash
set -e

clang++ -o read-palette read-palette.cpp -lpng
clang++ -o read-sprites read-sprites.cpp -lpng
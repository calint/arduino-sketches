# game code

intention:
* developing a toy game using platform independent engine

table of contents (in include order by program file):
* `/defs.hpp` constants used by engine and game objects
* `/resources/*` partial files defining tiles, sprites, palettes and tile map
* `/game.hpp` game state used by objects
* `/objects/*` game objects
* `/main.hpp` setup initial game state, callbacks from engine, game logic

# anatomy of a game

## main.hpp
### function `main_setup`
* initiates the game by creating initial objects and sets tile map position and velocity
### function `main_on_touch_screen`
* handles user interaction with touch screen
### function `main_on_frame_completed`
* implements game logic

## objects/*
* implements game objects

## game.hpp
* contains game state
* included by objects that access game state
* game state used in `main_on_frame_completed` solving circular reference problems

## resources/*
* separate palettes for tiles and sprites
* sprites
* tiles
* tile map

## defs.hpp
### `enum object_class`
* each game object class has an entry named with `_cls` suffix
### `collision_bits`
* naming bits with constants
* constants used to define sprite collision bits and mask

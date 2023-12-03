### game code

intention:
* developing a toy game using platform independent engine

table of contents (in include order by program file):
* `/defs.hpp` constants used by engine and game objects
* `/resources/*` partial files defining tiles, sprites, palettes and tile map
* `/game.hpp` game state used by objects
* `/objects/*` game objects
* `/main.hpp` setup initial game state, callbacks from engine, game logic

## anatomy of a game
### main.hpp
#### function `main_setup`
* initiates the game by creating initial objects and set tile map position and velocity
#### function `main_on_touch_screen`
* handle user interaction with touch screen
#### function `main_on_frame_completed`
* implement game logic
### objects/*
### game.hpp
* included by objects to modify game state
* game state used in `main_on_frame_completed` solving circular references
### resources/*
* palettes
  - tiles
  - sprites
* sprites
* tiles
* tile map
### defs.hpp
#### `enum object_class`
* add an entry for each game object class
#### `collision_bits`

### game code

intention:
* developing a toy game using platform independent engine

layout (in order of inclusion in the program file):
* `/def.hpp` constants used by the engine and game objects
* `/resources/*` partial files defining tiles, sprites, palettes, tile map
* `/game.hpp` imported by objects using the game state
* `/objects/*` game objects
* `/main.hpp` setup, callbacks from the engine, game logic

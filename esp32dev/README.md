### toy game for esp32-2432s028r a.k.a. cheap-yellow-display (cyd)

intention:
* exploring the device by developing a toy game
* developing a toy game engine using tiles and sprites with on screen collision detection between sprites
* decent performance, ~30 frames per second on the device

layout:
* `/esp32dev.ino` platform dependent code for booting and rendering
* `/engine.hpp` platform independent code of game engine
* `/game/*` game code using `engine.hpp`

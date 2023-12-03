### toy game for esp32-2432s028r a.k.a. cheap-yellow-display (cyd)

intention:
* exploring the device by developing a toy game
* decent performance, ~30 frames per second

layout:
* `/esp32dev.ino` platform dependent code for booting and rendering
* `/engine.hpp` platform independent code of game engine
* `/game/*` game code using `engine.hpp`

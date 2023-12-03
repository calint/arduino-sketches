### toy game for ESP32-2432S028R a.k.a. cheap-yellow-display (cyd)

intention:
* developing arduino sketch using visual code
* exploring the device by developing a toy game
* developing a toy game engine
  - smooth scrolling tile map
  - sprites with on screen collision detection
  - intuitive definition of game objects and logic
  - decent performance, ~30 frames per second on the device

layout:
* `/esp32dev.ino` platform dependent code for booting and rendering
* `/engine.hpp` platform independent code of game engine
* `/game/*` game code using `engine.hpp`

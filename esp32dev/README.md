### toy game for ESP32-2432S028R a.k.a. cheap-yellow-display (cyd)

intention:
* developing arduino sketch using visual code targeting esp32 boards
* exploring the device by developing a toy game
* developing a toy game engine
  - smooth scrolling tile map
  - sprites with pixel precision on screen collision detection
  - intuitive definition of game objects and logic
  - decent performance, ~30 frames per second on the device

table of contents:
* `/esp32dev.ino` code for booting and rendering
* `/platform.hpp` platform constants used by engine and game
* `/engine.hpp` platform independent code of game engine
* `/game/*` game code using `engine.hpp`
* `/utils/png-to-resources` util for extracting game engine resources from files exported by gimp

about the device:
* [manufacturer](http://www.jczn1688.com/)
* [purchased at](https://www.aliexpress.com/item/1005004502250619.html)
* [community](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)

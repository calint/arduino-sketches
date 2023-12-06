### toy game for ESP32-2432S028R a.k.a. cheap-yellow-display (cyd)

intention:
* developing arduino sketch using visual code targeting esp32 boards
* exploring the device by developing a toy game
* developing a platform independent toy game engine featuring:
  - smooth scrolling tile map
  - sprites with pixel precision on screen collision detection
  - intuitive definition of game objects and logic
  - decent performance, ~30 frames per second on the device

table of contents:
* `esp32dev.ino` booting and rendering
* `platform.hpp` platform constants used by engine and game
* `engine.hpp` platform independent game engine code
* `game/*` game code using `engine.hpp`
* `utils/png-to-resources` tools for extracting resources from png files

important:
* `User_Setup.h` configuration for display ILI9341
  - copy to directory of library TFT_eSPI
  - on linux using visual code, library is at `~/Arduino/library/TFT_eSPI`

### about the device
* [community](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
* [purchased at](https://www.aliexpress.com/item/1005004502250619.html)
* [manufacturer](http://www.jczn1688.com/)

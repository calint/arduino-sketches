### howto: raspberry pico w in arduino ide 2.1.1
File -> Preferences

Additional boards manager URLs:
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

Tools -> Board -> Board Manager

install "Raspberry Pi Pico/RP2040 by Earle F. Philhower, III" (3.3.2)

compile

first time uploading:
* select board "Raspberry Pi Pico W"
* boot pico with "bootsel" pressed then release it
* select Tools -> Port -> UF2 Board

subsequent uploads select port e.g. "/dev/ttyACM0"

connect with serial terminal to e.g. /dev/ttyACM0 at 115200 baud

type "help" to get started

## howto: esp32-2432s028r in visual code 1.84.2
* install plug-in Arduino v0.6.0
* task ">Arduino: Initialize"
* task ">Arduino: Board Manager"
  - install "esp32 by Espressif Systems Version 2.0.14"
* task ">Arduino: Board Config"
  - select "ESP32 Dev Module (esp32)"
* install libraries (plug-in installs libraries in ~/Arduino/libraries)
  - TFT_eSPI by Bodmer 2.5.31
  - XPT2046_Touchscreen by Paul Stoffregen 1.4.0
* replace User_Setup.h in ~/Arduino/libraries/TFT_eSPI/ with provided file
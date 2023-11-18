# arduino-sketches
experiments with arduino sketches for raspberry pico w, arduino nano esp32 and ESP32-2432S028R a.k.a. cheap-yellow-display

### howto: raspberry pico w in arduino ide (2.1.1)
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

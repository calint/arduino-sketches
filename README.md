# arduino-sketches
experiments with arduino sketches for raspberry pico w

### howto in arduino ide (2.1.1)
File -> Preferences

Additional boards manager URLs:
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

Tools -> Board -> Board Manager

install "Raspberry Pi Pico/RP2040 by Earle F. Philhower, III" (3.3.2)

compile

first time uploading:
* select board "Raspberry Pi Pico W"
* boot pico with "bootsel" pressed
* enable "Show all ports"
* select board port "UF2 Board UF2 Devices"

subsequent uploads select port e.g. "/dev/ttyACM0"

connect with serial terminal to e.g. /dev/ttyACM0 at 115200 baud

type "help" to get started

## clipboard
```
git config --global credential.helper cache
git config --global credential.helper 'cache --timeout=36000'

git add . && git commit -m "." && git push
TAG=2023-08-19--1 && git tag $TAG && git push origin $TAG
```

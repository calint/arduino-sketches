# arduino-sketches
experiments with arduino sketches for raspberry pico w

## arduino ide library with support for raspberry pico w
menu
File->Preferences
additional boards manager URLs:
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

install library

select board "Raspberry Pi Pico W"

compile and upload

connect with serial terminal to e.g. /dev/ttyACM0 at 115200 baud

type "help" to get started

## clipboard
```
git config --global credential.helper cache
git config --global credential.helper 'cache --timeout=36000'

git add . && git commit -m "." && git push
TAG=2023-08-19--1 && git tag $TAG && git push origin $TAG
```

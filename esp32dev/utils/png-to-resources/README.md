### tools for extracting resources from png files
* edit gimp files "sprites.xcf" and "tiles.xcf"
* create a layer for each sprite / tile

#### exporting to png for processing
* enable all layers

![layers](readme-1.png)

* File -> Export As...

![export to png](readme-2.png)

#### extracting resources
script `./extract-to-resources.sh` will overwrite `game/resources/*` files

note. make sure transparency pixel is palette index 0

#### current resources
tiles:

![tiles](tiles.png)

sprites:

![sprites](sprites.png)

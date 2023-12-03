# anatomy of a game object

base class `object` in engine has attributes common to most objects and provides overridable functions for custom logic

## attributes

### related to run time information
* object class: `cls` is mandatory to initiate an object and is defined in `defs.hpp` by game code where each object class has an entry

### related to position and motion
* position: `x`, `y`
* velocity: `dx`, `dy`
* acceleration: `ddx`, `ddy`

### related to display
* sprite: `spr`

### related to collision detection
* engine performs collision detection between sprites on screen if a bitwise AND operation involving `col_bits` from an object and `col_mask` from a another object is non-zero
* example: if `col_bits` of object A bitwise AND with `col_mask` of object B is non-zero then object B `col_with` pointer is set to object A
* same procedure is done with A and B swapped
* the definition of the 32 available bits and their meaning is custom depending on the game
* example:
  - bit 1 - 'enemy fire' - meaning that all enemy fire classes enable bit 1 in `col_bits`
  - hero `col_mask` would enable bit 1 to get informed when collision with any 'enemy fire' object occurs
* this scheme enables objects to collide with each other without triggering collision detection handling such as enemy ships rendered on each other

## overridable functions

### constructor
* base constructor sets mandatory `cls` to provide run time information
  - object classes are defined in `enum object_class` in `defs.hpp`
* allocate and initiate `sprite`
  - set `spr->obj` to current object
  - set `spr->img` to image data, usually defined in `sprite_imgs[...]`
* object may be composed of several sprites
  - declare additional sprite pointers as class attributes
  - initiate in the constructor in same manner as `spr`

### destructor
* object de-allocates sprite
* additional clean-up implemented by inheriting class

### update
* game loop calls `update()` on allocated objects
* default implementation is to update position and motion attributes
* user object should call base class `update()` before implementing custom logic
* common custom logic is collision handling
  - check `col_with`, if not `nullptr` handle collision then reset to `nullptr`
* return `true` if object has 'died' and should be de-allocated by the engine

### update_sprite
* game loop calls `update_sprite()` before rendering
* default implementation sets sprite screen position using object position
* objects composed of several sprites override this function to set screen position on the additional sprites

# examples
* `ship1.hpp` basic object with typical implementation
* `ship2.hpp` ad-hoc implementation of animated sprite
* `hero.hpp` composed of several sprites

#pragma once
// platform constants used by engine and game

static constexpr uint8_t display_orientation = 0;
// 0: vertical, 1: horizontal

// display dimensions of screen ILI9341 depending on orientation
static constexpr unsigned display_width = display_orientation == 0 ? 240 : 320;
static constexpr unsigned display_height = display_orientation == 0 ? 320 : 240;

// calibration of touch screen
constexpr int16_t touch_screen_min_x = 400;
constexpr int16_t touch_screen_max_x = 3700;
constexpr int16_t touch_screen_range_x =
    touch_screen_max_x - touch_screen_min_x;
constexpr int16_t touch_screen_min_y = 300;
constexpr int16_t touch_screen_max_y = 3750;
constexpr int16_t touch_screen_range_y =
    touch_screen_max_y - touch_screen_min_y;
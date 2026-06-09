#pragma once

#include <lvgl.h>

// A 0-F keypad: a 0-9 number group above a tinted A-F letter group. Invokes
// on_key(value) with 0..15 on each press. One keypad is expected on screen at a
// time (modal use), which keeps the callback wiring simple.
typedef void (*HexKeyCb)(int value);
lv_obj_t *hexKeypad_create(lv_obj_t *parent, HexKeyCb on_key);

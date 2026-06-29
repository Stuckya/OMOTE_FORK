#pragma once

#include <lvgl.h>
#include <cstdint>
#include <string>

// A centered row of PIN segment boxes. pinSegments_render fills entered chars
// and marks the next empty slot active; it reads structure from the row itself,
// so no external state is needed.
lv_obj_t *pinSegments_create(lv_obj_t *parent, uint32_t count);
void pinSegments_render(lv_obj_t *segments, const std::string &code);

#pragma once

#include <lvgl.h>
#include <cstdint>

// A drag-around-the-ring value picker: the knob follows the finger, the value
// snaps to whole steps, and the centre reads it back. Built on lv_arc, so the
// drag tracking is the widget's own.
namespace RingPicker {

typedef void (*ReleasedCallback)(uint16_t value);

struct Config {
  // The ring is drawn from zero so the smallest selectable value still reads as
  // progress; minValue is the floor the snap enforces, not where the arc starts.
  uint16_t minValue;
  uint16_t maxValue;
  uint16_t step;
  uint16_t initialValue;
  const char *unit;
  ReleasedCallback onReleased;
};

lv_obj_t *create(lv_obj_t *parent, const Config &config);
uint16_t value(lv_obj_t *ring);

}  // namespace RingPicker

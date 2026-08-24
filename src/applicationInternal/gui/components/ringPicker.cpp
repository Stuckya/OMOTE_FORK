#include "applicationInternal/gui/components/ringPicker.h"

#include <cmath>
#include <cstdio>

#include "applicationInternal/gui/guiTheme.h"

namespace RingPicker {

namespace {

const lv_coord_t kSize = 150;
const lv_coord_t kArcWidth = 10;
const lv_coord_t kKnobPad = 6;
// The knob overhangs the track, and a full sweep parks it back at twelve
// o'clock -- so the ring keeps clearance for whatever sits above it.
const lv_coord_t kTopMargin = 12;
// One notch per step, marking where the value can land; passed ones light up.
// The last step is not drawn: a full sweep lands it back at twelve o'clock,
// where a dot would read as a selectable zero.
const int kNotchCount = 11;
const lv_coord_t kNotchSize = 4;
const float kNotchRadiusRatio = 0.66f;
const uint32_t kNotchLit = 0x4ea1ff;

struct State {
  Config config;
  lv_obj_t *arc;
  lv_obj_t *valueLabel;
  lv_obj_t *notches[kNotchCount];
};

uint16_t snapped(const Config &config, int16_t raw) {
  const int steps = (raw + config.step / 2) / config.step;
  int value = steps * config.step;
  if (value < config.minValue) value = config.minValue;
  if (value > config.maxValue) value = config.maxValue;
  return (uint16_t)value;
}

void render(State *state) {
  const uint16_t value = (uint16_t)lv_arc_get_value(state->arc);
  char text[8];
  snprintf(text, sizeof(text), "%u", (unsigned)value);
  lv_label_set_text(state->valueLabel, text);

  const uint16_t step = state->config.step;
  for (int i = 0; i < kNotchCount; i++) {
    const uint16_t notchValue = (uint16_t)((i + 1) * step);
    const bool lit = value >= notchValue;
    lv_obj_set_style_bg_color(state->notches[i],
                              GuiTheme::color(lit ? kNotchLit : GuiTheme::kSurface3),
                              LV_PART_MAIN);
  }
}

void arc_changed_cb(lv_event_t *event) {
  State *state = (State *)lv_event_get_user_data(event);
  const uint16_t value = snapped(state->config, lv_arc_get_value(state->arc));
  // Write the snap back so the knob rests on a step, not between two.
  if (value != lv_arc_get_value(state->arc)) {
    lv_arc_set_value(state->arc, value);
  }
  render(state);
}

void arc_deleted_cb(lv_event_t *event) {
  delete (State *)lv_event_get_user_data(event);
}

}  // namespace

lv_obj_t *create(lv_obj_t *parent, const Config &config) {
  lv_obj_t *wrap = lv_obj_create(parent);
  lv_obj_remove_style_all(wrap);
  lv_obj_set_size(wrap, kSize, kSize + kTopMargin);
  lv_obj_set_style_pad_top(wrap, kTopMargin, LV_PART_MAIN);
  lv_obj_clear_flag(wrap, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *arc = lv_arc_create(wrap);
  lv_obj_set_size(arc, kSize, kSize);
  lv_obj_center(arc);
  lv_arc_set_rotation(arc, 270);
  lv_arc_set_bg_angles(arc, 0, 360);
  lv_arc_set_range(arc, 0, config.maxValue);
  lv_arc_set_value(arc, config.initialValue);
  lv_obj_set_style_arc_width(arc, kArcWidth, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc, kArcWidth, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arc, GuiTheme::color(GuiTheme::kSurface3),
                             LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc, GuiTheme::color(GuiTheme::kBlue),
                             LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(arc, GuiTheme::color(GuiTheme::kWhite),
                            LV_PART_KNOB);
  lv_obj_set_style_pad_all(arc, kKnobPad, LV_PART_KNOB);

  State *state = new State();
  state->config = config;
  state->arc = arc;
  for (int i = 0; i < kNotchCount; i++) {
    lv_obj_t *notch = lv_obj_create(wrap);
    lv_obj_remove_style_all(notch);
    lv_obj_set_size(notch, kNotchSize, kNotchSize);
    lv_obj_set_style_radius(notch, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(notch, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(notch, LV_OBJ_FLAG_CLICKABLE);
    const double angle = 2.0 * M_PI * (i + 1) * config.step / config.maxValue;
    const double radius = (kSize / 2.0) * kNotchRadiusRatio;
    lv_obj_set_pos(notch,
                   (lv_coord_t)(kSize / 2.0 + radius * sin(angle) - kNotchSize / 2.0),
                   (lv_coord_t)(kSize / 2.0 - radius * cos(angle) - kNotchSize / 2.0));
    // Behind the knob: the knob is the thing being aimed, not the scale.
    lv_obj_move_background(notch);
    state->notches[i] = notch;
  }

  // The readout sits over the arc's middle; flexRow/Column leave it
  // non-interactive so it cannot eat the drag.
  lv_obj_t *center = GuiTheme::flexColumn(wrap, 0);
  lv_obj_center(center);

  lv_obj_t *valueLabel = lv_label_create(center);
  lv_obj_set_style_text_font(valueLabel, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(valueLabel, GuiTheme::color(GuiTheme::kWhite),
                              LV_PART_MAIN);

  lv_obj_t *unitLabel = lv_label_create(center);
  lv_label_set_text(unitLabel, config.unit);
  lv_obj_set_style_text_font(unitLabel, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_set_style_text_color(unitLabel, GuiTheme::color(GuiTheme::kTextMute),
                              LV_PART_MAIN);

  state->valueLabel = valueLabel;
  lv_obj_add_event_cb(arc, arc_changed_cb, LV_EVENT_VALUE_CHANGED, state);
  lv_obj_add_event_cb(arc, arc_deleted_cb, LV_EVENT_DELETE, state);
  render(state);
  return arc;
}

uint16_t value(lv_obj_t *ring) { return (uint16_t)lv_arc_get_value(ring); }

}  // namespace RingPicker

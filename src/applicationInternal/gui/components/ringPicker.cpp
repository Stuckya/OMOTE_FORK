#include "applicationInternal/gui/components/ringPicker.h"

#include <cstdio>

#include "applicationInternal/gui/guiTheme.h"

namespace RingPicker {

namespace {

const lv_coord_t kSize = 150;
const lv_coord_t kArcWidth = 10;
const lv_coord_t kKnobPad = 6;

struct State {
  Config config;
  lv_obj_t *arc;
  lv_obj_t *valueLabel;
};

uint16_t snapped(const Config &config, int16_t raw) {
  const int steps = (raw - config.minValue + config.step / 2) / config.step;
  int value = config.minValue + steps * config.step;
  if (value < config.minValue) value = config.minValue;
  if (value > config.maxValue) value = config.maxValue;
  return (uint16_t)value;
}

void render(State *state) {
  char text[8];
  snprintf(text, sizeof(text), "%u", (unsigned)lv_arc_get_value(state->arc));
  lv_label_set_text(state->valueLabel, text);
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

void arc_released_cb(lv_event_t *event) {
  State *state = (State *)lv_event_get_user_data(event);
  if (state->config.onReleased != nullptr) {
    state->config.onReleased((uint16_t)lv_arc_get_value(state->arc));
  }
}

void arc_deleted_cb(lv_event_t *event) {
  delete (State *)lv_event_get_user_data(event);
}

}  // namespace

lv_obj_t *create(lv_obj_t *parent, const Config &config) {
  lv_obj_t *wrap = lv_obj_create(parent);
  lv_obj_remove_style_all(wrap);
  lv_obj_set_size(wrap, kSize, kSize);
  lv_obj_clear_flag(wrap, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *arc = lv_arc_create(wrap);
  lv_obj_set_size(arc, kSize, kSize);
  lv_obj_center(arc);
  lv_arc_set_rotation(arc, 270);
  lv_arc_set_bg_angles(arc, 0, 360);
  lv_arc_set_range(arc, config.minValue, config.maxValue);
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

  lv_obj_t *center = lv_obj_create(wrap);
  lv_obj_remove_style_all(center);
  lv_obj_set_size(center, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_center(center);
  lv_obj_set_flex_flow(center, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(center, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  // The readout must not eat the drag: it sits over the arc's middle.
  lv_obj_clear_flag(center, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *valueLabel = lv_label_create(center);
  lv_obj_set_style_text_font(valueLabel, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(valueLabel, GuiTheme::color(GuiTheme::kWhite),
                              LV_PART_MAIN);

  lv_obj_t *unitLabel = lv_label_create(center);
  lv_label_set_text(unitLabel, config.unit);
  lv_obj_set_style_text_font(unitLabel, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_set_style_text_color(unitLabel, GuiTheme::color(GuiTheme::kTextMute),
                              LV_PART_MAIN);

  State *state = new State{config, arc, valueLabel};
  lv_obj_add_event_cb(arc, arc_changed_cb, LV_EVENT_VALUE_CHANGED, state);
  lv_obj_add_event_cb(arc, arc_released_cb, LV_EVENT_RELEASED, state);
  lv_obj_add_event_cb(arc, arc_deleted_cb, LV_EVENT_DELETE, state);
  render(state);
  return arc;
}

uint16_t value(lv_obj_t *ring) { return (uint16_t)lv_arc_get_value(ring); }

}  // namespace RingPicker

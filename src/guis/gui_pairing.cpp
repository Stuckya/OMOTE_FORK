#include "guis/gui_pairing.h"

#include <cstdint>
#include <string>
#include <vector>

#include "applicationInternal/hardware/hardwarePresenter.h"
#include "applicationInternal/hub/pairingManager.h"

namespace {

constexpr uint32_t COL_SURFACE_1 = 0x1c1c1e;  // segment idle
constexpr uint32_t COL_SURFACE_2 = 0x2c2c2e;  // keys / filled segment
constexpr uint32_t COL_BLUE = 0x007aff;       // submit / active segment
constexpr uint32_t COL_RED = 0xff3b30;        // cancel / error
constexpr uint32_t COL_GREEN = 0x34c759;      // paired
constexpr uint32_t COL_LTR_BG = 0x22262e;     // tinted A-F keys
constexpr uint32_t COL_LTR_TXT = 0x9fb4d8;
constexpr uint32_t COL_MUTE = 0x8a8a8d;

lv_obj_t *overlay = nullptr;
lv_obj_t *submit_btn = nullptr;
lv_obj_t *submit_label = nullptr;
std::vector<lv_obj_t *> segment_labels;

std::string pin_buffer;
std::string submitted_code;
std::string device_name;
uint32_t expected_len = 4;
bool requires_pin = true;

void refresh_awaiting() {
  for (size_t i = 0; i < segment_labels.size(); ++i) {
    lv_obj_t *seg = lv_obj_get_parent(segment_labels[i]);
    const bool filled = i < pin_buffer.size();
    const bool active = i == pin_buffer.size();
    lv_label_set_text(segment_labels[i],
                      filled ? std::string(1, pin_buffer[i]).c_str() : "");
    lv_obj_set_style_bg_color(
        seg, lv_color_hex(filled ? COL_SURFACE_2 : COL_SURFACE_1), LV_PART_MAIN);
    lv_obj_set_style_border_width(seg, active ? 2 : 0, LV_PART_MAIN);
    lv_obj_set_style_border_color(seg, lv_color_hex(COL_BLUE), LV_PART_MAIN);
  }
  const bool complete = pin_buffer.size() == expected_len;
  lv_obj_set_style_bg_color(
      submit_btn, lv_color_hex(complete ? COL_BLUE : COL_SURFACE_2), LV_PART_MAIN);
  lv_obj_set_style_text_color(
      submit_label, complete ? lv_color_white() : lv_color_hex(COL_MUTE),
      LV_PART_MAIN);
}

void hex_key_cb(lv_event_t *e) {
  if (pin_buffer.size() >= expected_len) return;
  const int v = static_cast<int>(reinterpret_cast<intptr_t>(
      lv_obj_get_user_data(lv_event_get_target(e))));
  pin_buffer += static_cast<char>(v < 10 ? '0' + v : 'A' + (v - 10));
  refresh_awaiting();
}

void backspace_cb(lv_event_t *e) {
  LV_UNUSED(e);
  if (!pin_buffer.empty()) {
    pin_buffer.pop_back();
    refresh_awaiting();
  }
}

void submit_cb(lv_event_t *e) {
  LV_UNUSED(e);
  if (pin_buffer.size() != expected_len) return;
  submitted_code = pin_buffer;
  Hub::PairingManager::getInstance().submitPin(submitted_code.c_str());
  gui_pairing_show_verifying();
}

void cancel_cb(lv_event_t *e) {
  LV_UNUSED(e);
  Hub::PairingManager::getInstance().cancel();
  gui_pairing_hide();
}

void done_cb(lv_event_t *e) {
  LV_UNUSED(e);
  gui_pairing_hide();
}

void retry_cb(lv_event_t *e) {
  LV_UNUSED(e);
  gui_pairing_show_awaiting(device_name, "", expected_len, requires_pin);
}

void style_key(lv_obj_t *btn, uint32_t bg, uint32_t txt, const char *text) {
  lv_obj_remove_style_all(btn);
  lv_obj_set_style_bg_color(btn, lv_color_hex(bg), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BLUE), LV_STATE_PRESSED);
  lv_obj_t *lbl = lv_label_create(btn);
  lv_label_set_text(lbl, text);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(lbl, lv_color_hex(txt), LV_PART_MAIN);
  lv_obj_center(lbl);
}

// col_dsc/row_dsc must outlive the object — LVGL keeps the pointer, not a copy,
// so each grid needs its own persistent descriptor arrays.
void make_key_grid(lv_obj_t *parent, int first, int last, int cols,
                   lv_coord_t *col_dsc, lv_coord_t *row_dsc) {
  lv_obj_t *grid = lv_obj_create(parent);
  lv_obj_remove_style_all(grid);
  lv_obj_set_width(grid, lv_pct(100));
  lv_obj_set_height(grid, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_column(grid, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_row(grid, 6, LV_PART_MAIN);
  lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
  lv_obj_set_layout(grid, LV_LAYOUT_GRID);

  for (int v = first; v <= last; ++v) {
    const int idx = v - first;
    const bool letter = v >= 10;
    lv_obj_t *btn = lv_btn_create(grid);
    const char text[2] = {static_cast<char>(letter ? 'A' + (v - 10) : '0' + v),
                          '\0'};
    style_key(btn, letter ? COL_LTR_BG : COL_SURFACE_2,
              letter ? COL_LTR_TXT : 0xffffff, text);
    lv_obj_set_user_data(btn, reinterpret_cast<void *>(static_cast<intptr_t>(v)));
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, idx % cols, 1,
                         LV_GRID_ALIGN_STRETCH, idx / cols, 1);
    lv_obj_add_event_cb(btn, hex_key_cb, LV_EVENT_CLICKED, nullptr);
  }
}

lv_obj_t *make_overlay() {
  gui_pairing_hide();
  overlay = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(overlay);
  lv_obj_set_size(overlay, SCR_WIDTH, SCR_HEIGHT);
  lv_obj_set_pos(overlay, 0, 0);
  lv_obj_set_style_bg_color(overlay, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_pad_all(overlay, 12, LV_PART_MAIN);
  lv_obj_set_flex_flow(overlay, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(overlay, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(overlay, 10, LV_PART_MAIN);
  lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
  return overlay;
}

void add_header(const std::string &name, bool with_cancel) {
  lv_obj_t *header = lv_obj_create(overlay);
  lv_obj_remove_style_all(header);
  lv_obj_set_width(header, lv_pct(100));
  lv_obj_set_height(header, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  lv_obj_t *label = lv_label_create(header);
  lv_label_set_text(label, name.c_str());
  lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_flex_grow(label, 1);

  if (!with_cancel) return;
  lv_obj_t *cancel = lv_btn_create(header);
  lv_obj_remove_style_all(cancel);
  lv_obj_set_size(cancel, 26, 26);
  lv_obj_set_style_radius(cancel, 13, LV_PART_MAIN);
  lv_obj_set_style_bg_color(cancel, lv_color_hex(COL_RED), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(cancel, LV_OPA_30, LV_PART_MAIN);
  lv_obj_add_event_cb(cancel, cancel_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *x = lv_label_create(cancel);
  lv_label_set_text(x, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_color(x, lv_color_hex(COL_RED), LV_PART_MAIN);
  lv_obj_center(x);
}

lv_obj_t *add_core() {
  lv_obj_t *core = lv_obj_create(overlay);
  lv_obj_remove_style_all(core);
  lv_obj_set_width(core, lv_pct(100));
  lv_obj_set_flex_grow(core, 1);
  lv_obj_set_flex_flow(core, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(core, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(core, 12, LV_PART_MAIN);
  return core;
}

void add_badge(lv_obj_t *parent, uint32_t color, const char *symbol) {
  lv_obj_t *c = lv_obj_create(parent);
  lv_obj_remove_style_all(c);
  lv_obj_set_size(c, 60, 60);
  lv_obj_set_style_radius(c, 30, LV_PART_MAIN);
  lv_obj_set_style_bg_color(c, lv_color_hex(color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(c, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_t *s = lv_label_create(c);
  lv_label_set_text(s, symbol);
  lv_obj_set_style_text_font(s, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(s, lv_color_white(), LV_PART_MAIN);
  lv_obj_center(s);
}

void add_title(lv_obj_t *parent, const char *text, const lv_font_t *font,
               uint32_t color) {
  lv_obj_t *l = lv_label_create(parent);
  lv_label_set_text(l, text);
  lv_obj_set_style_text_font(l, font, LV_PART_MAIN);
  lv_obj_set_style_text_color(l, lv_color_hex(color), LV_PART_MAIN);
}

void add_action(lv_obj_t *parent, const char *text, uint32_t bg,
                lv_event_cb_t cb) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_remove_style_all(btn);
  lv_obj_set_width(btn, lv_pct(100));
  lv_obj_set_height(btn, 44);
  lv_obj_set_style_radius(btn, 12, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(bg), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *l = lv_label_create(btn);
  lv_label_set_text(l, text);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(l, lv_color_white(), LV_PART_MAIN);
  lv_obj_center(l);
}

void add_text_btn(lv_obj_t *parent, const char *text, uint32_t color,
                  lv_event_cb_t cb) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_remove_style_all(btn);
  lv_obj_set_width(btn, lv_pct(100));
  lv_obj_set_height(btn, 30);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *l = lv_label_create(btn);
  lv_label_set_text(l, text);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(l, lv_color_hex(color), LV_PART_MAIN);
  lv_obj_center(l);
}

void build_awaiting() {
  make_overlay();
  add_header(device_name, true);

  lv_obj_t *segs = lv_obj_create(overlay);
  lv_obj_remove_style_all(segs);
  lv_obj_set_width(segs, lv_pct(100));
  lv_obj_set_height(segs, 50);
  lv_obj_set_flex_flow(segs, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(segs, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(segs, 8, LV_PART_MAIN);

  const lv_coord_t seg_w = expected_len > 4 ? 28 : 42;
  segment_labels.clear();
  for (uint32_t i = 0; i < expected_len; ++i) {
    lv_obj_t *seg = lv_obj_create(segs);
    lv_obj_remove_style_all(seg);
    lv_obj_set_size(seg, seg_w, 48);
    lv_obj_set_style_radius(seg, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(seg, lv_color_hex(COL_SURFACE_1), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_t *lbl = lv_label_create(seg);
    lv_label_set_text(lbl, "");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(lbl);
    segment_labels.push_back(lbl);
  }

  lv_obj_t *keypad = lv_obj_create(overlay);
  lv_obj_remove_style_all(keypad);
  lv_obj_set_width(keypad, lv_pct(100));
  lv_obj_set_height(keypad, LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(keypad, 1);
  lv_obj_set_flex_flow(keypad, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(keypad, 8, LV_PART_MAIN);
  static lv_coord_t num_col[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
                                 LV_GRID_FR(1), LV_GRID_FR(1),
                                 LV_GRID_TEMPLATE_LAST};
  static lv_coord_t num_row[] = {28, 28, LV_GRID_TEMPLATE_LAST};
  static lv_coord_t ltr_col[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
                                 LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
                                 LV_GRID_TEMPLATE_LAST};
  static lv_coord_t ltr_row[] = {28, LV_GRID_TEMPLATE_LAST};
  make_key_grid(keypad, 0, 9, 5, num_col, num_row);
  make_key_grid(keypad, 10, 15, 6, ltr_col, ltr_row);

  lv_obj_t *actions = lv_obj_create(overlay);
  lv_obj_remove_style_all(actions);
  lv_obj_set_width(actions, lv_pct(100));
  lv_obj_set_height(actions, 44);
  lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(actions, 8, LV_PART_MAIN);

  lv_obj_t *back = lv_btn_create(actions);
  lv_obj_set_size(back, 54, 44);
  style_key(back, COL_SURFACE_2, 0xffffff, LV_SYMBOL_BACKSPACE);
  lv_obj_add_event_cb(back, backspace_cb, LV_EVENT_CLICKED, nullptr);

  submit_btn = lv_btn_create(actions);
  lv_obj_remove_style_all(submit_btn);
  lv_obj_set_height(submit_btn, 44);
  lv_obj_set_flex_grow(submit_btn, 1);
  lv_obj_set_style_radius(submit_btn, 12, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(submit_btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_add_event_cb(submit_btn, submit_cb, LV_EVENT_CLICKED, nullptr);
  submit_label = lv_label_create(submit_btn);
  lv_label_set_text(submit_label, "Submit");
  lv_obj_set_style_text_font(submit_label, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_center(submit_label);

  refresh_awaiting();
}

}  // namespace

void gui_pairing_show_awaiting(const std::string &deviceName,
                               const std::string &message,
                               uint32_t expectedPinLength, bool requiresPin) {
  LV_UNUSED(message);
  device_name = deviceName;
  expected_len = expectedPinLength > 0 ? expectedPinLength : 4;
  requires_pin = requiresPin;
  pin_buffer.clear();
  build_awaiting();
}

void gui_pairing_show_verifying() {
  make_overlay();
  add_header(device_name, false);
  lv_obj_t *core = add_core();
  lv_obj_t *spinner = lv_spinner_create(core, 1000, 60);
  lv_obj_set_size(spinner, 56, 56);
  lv_obj_set_style_arc_color(spinner, lv_color_hex(COL_SURFACE_2), LV_PART_MAIN);
  lv_obj_set_style_arc_color(spinner, lv_color_hex(COL_BLUE), LV_PART_INDICATOR);
  add_title(core, "Verifying code...", &lv_font_montserrat_16, 0xffffff);
  if (!submitted_code.empty()) {
    add_title(core, submitted_code.c_str(), &lv_font_montserrat_16, COL_MUTE);
  }
}

void gui_pairing_show_success(const std::string &deviceName) {
  device_name = deviceName;
  make_overlay();
  add_header(device_name, false);
  lv_obj_t *core = add_core();
  add_badge(core, COL_GREEN, LV_SYMBOL_OK);
  add_title(core, "Paired", &lv_font_montserrat_16, 0xffffff);
  add_action(overlay, "Done", COL_BLUE, done_cb);
}

void gui_pairing_show_failed(const std::string &message) {
  make_overlay();
  add_header(device_name, false);
  lv_obj_t *core = add_core();
  add_badge(core, COL_RED, LV_SYMBOL_CLOSE);
  add_title(core, "Incorrect code", &lv_font_montserrat_16, 0xffffff);
  if (!message.empty()) {
    add_title(core, message.c_str(), &lv_font_montserrat_12, COL_MUTE);
  }
  add_action(overlay, "Try Again", COL_BLUE, retry_cb);
  add_text_btn(overlay, "Cancel", COL_RED, cancel_cb);
}

void gui_pairing_hide() {
  if (overlay != nullptr) {
    lv_obj_del(overlay);
    overlay = nullptr;
    submit_btn = nullptr;
    submit_label = nullptr;
    segment_labels.clear();
  }
  pin_buffer.clear();
}

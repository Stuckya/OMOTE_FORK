#include "applicationInternal/gui/components/hexKeypad.h"

#include "applicationInternal/gui/guiTheme.h"

namespace {

HexKeyCb s_on_key = nullptr;

int char_to_value(const char *txt) {
  if (txt == nullptr || txt[0] == '\0') return -1;
  const char c = txt[0];
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  return -1;
}

void key_pressed(lv_event_t *e) {
  lv_obj_t *matrix = lv_event_get_target(e);
  const uint32_t id = lv_btnmatrix_get_selected_btn(matrix);
  const int value = char_to_value(lv_btnmatrix_get_btn_text(matrix, id));
  if (value >= 0 && s_on_key != nullptr) s_on_key(value);
}

// One lv_btnmatrix draws a whole grid of keys from a static map, replacing what
// used to be one lv_btn + lv_label per digit (16 keys -> 2 objects total). The
// map pointer is retained by LVGL, so it must outlive the widget.
lv_obj_t *make_matrix(lv_obj_t *parent, const char **map, lv_coord_t height,
                      uint32_t key_bg, uint32_t key_txt) {
  lv_obj_t *matrix = lv_btnmatrix_create(parent);
  lv_btnmatrix_set_map(matrix, map);
  lv_obj_set_width(matrix, lv_pct(100));
  lv_obj_set_height(matrix, height);

  lv_obj_set_style_bg_opa(matrix, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(matrix, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(matrix, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(matrix, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_column(matrix, 6, LV_PART_MAIN);

  lv_obj_set_style_bg_color(matrix, GuiTheme::color(key_bg), LV_PART_ITEMS);
  lv_obj_set_style_bg_opa(matrix, LV_OPA_COVER, LV_PART_ITEMS);
  lv_obj_set_style_radius(matrix, 8, LV_PART_ITEMS);
  lv_obj_set_style_text_color(matrix, GuiTheme::color(key_txt), LV_PART_ITEMS);
  lv_obj_set_style_text_font(matrix, &lv_font_montserrat_16, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(matrix, GuiTheme::color(GuiTheme::kBlue),
                            LV_PART_ITEMS | LV_STATE_PRESSED);

  lv_obj_add_event_cb(matrix, key_pressed, LV_EVENT_VALUE_CHANGED, nullptr);
  return matrix;
}

}  // namespace

lv_obj_t *hexKeypad_create(lv_obj_t *parent, HexKeyCb on_key) {
  s_on_key = on_key;

  lv_obj_t *keypad = lv_obj_create(parent);
  lv_obj_remove_style_all(keypad);
  lv_obj_set_width(keypad, lv_pct(100));
  lv_obj_set_height(keypad, LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(keypad, 1);
  lv_obj_set_flex_flow(keypad, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(keypad, 8, LV_PART_MAIN);

  static const char *num_map[] = {"0", "1", "2", "3", "4", "\n",
                                  "5", "6", "7", "8", "9", ""};
  static const char *ltr_map[] = {"A", "B", "C", "D", "E", "F", ""};
  make_matrix(keypad, num_map, 62, GuiTheme::kSurface2, GuiTheme::kWhite);
  make_matrix(keypad, ltr_map, 28, GuiTheme::kKeyTinted, GuiTheme::kKeyTintedTxt);
  return keypad;
}

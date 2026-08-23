#include "applicationInternal/gui/guiTheme.h"

namespace GuiTheme {

lv_obj_t *primaryButton(lv_obj_t *parent, const char *text, uint32_t bg,
                        lv_event_cb_t cb) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_remove_style_all(btn);
  lv_obj_set_width(btn, lv_pct(100));
  lv_obj_set_height(btn, 44);
  lv_obj_set_style_radius(btn, 12, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, color(bg), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *l = lv_label_create(btn);
  lv_label_set_text(l, text);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(l, color(kWhite), LV_PART_MAIN);
  lv_obj_center(l);
  return btn;
}

lv_obj_t *textButton(lv_obj_t *parent, const char *text, uint32_t txt_color,
                     lv_event_cb_t cb) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_remove_style_all(btn);
  lv_obj_set_width(btn, lv_pct(100));
  lv_obj_set_height(btn, 30);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *l = lv_label_create(btn);
  lv_label_set_text(l, text);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(l, color(txt_color), LV_PART_MAIN);
  lv_obj_center(l);
  return btn;
}

lv_obj_t *tintedButton(lv_obj_t *parent, const char *text, uint32_t bg,
                       lv_opa_t bg_opa, uint32_t txt_color, lv_event_cb_t cb) {
  lv_obj_t *btn = primaryButton(parent, text, bg, cb);
  lv_obj_set_style_bg_opa(btn, bg_opa, LV_PART_MAIN);
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (label != nullptr) {
    lv_obj_set_style_text_color(label, color(txt_color), LV_PART_MAIN);
  }
  return btn;
}

static lv_obj_t *flexBox(lv_obj_t *parent, lv_flex_flow_t flow, lv_coord_t gap) {
  lv_obj_t *box = lv_obj_create(parent);
  lv_obj_remove_style_all(box);
  lv_obj_set_size(box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(box, flow);
  lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(box, gap, LV_PART_MAIN);
  lv_obj_set_style_pad_row(box, gap, LV_PART_MAIN);
  // A bare container is clickable and scrollable by default, and v8 does not
  // bubble: left alone it swallows the taps meant for what it holds.
  lv_obj_clear_flag(box, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
  return box;
}

lv_obj_t *flexRow(lv_obj_t *parent, lv_coord_t gap) {
  return flexBox(parent, LV_FLEX_FLOW_ROW, gap);
}

lv_obj_t *flexColumn(lv_obj_t *parent, lv_coord_t gap) {
  return flexBox(parent, LV_FLEX_FLOW_COLUMN, gap);
}

lv_obj_t *spacer(lv_obj_t *parent) {
  lv_obj_t *s = lv_obj_create(parent);
  lv_obj_remove_style_all(s);
  lv_obj_set_width(s, lv_pct(100));
  lv_obj_set_flex_grow(s, 1);
  return s;
}

lv_obj_t *splitActionRow(lv_obj_t *parent, lv_coord_t height) {
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_remove_style_all(row);
  lv_obj_set_width(row, lv_pct(100));
  lv_obj_set_height(row, height);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_border_color(row, color(kSurface3), LV_PART_MAIN);
  lv_obj_set_style_border_side(row, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
  lv_obj_set_style_border_width(row, 1, LV_PART_MAIN);
  // The row must not swallow the taps meant for its actions: v8 does not bubble.
  lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  return row;
}

lv_obj_t *splitAction(lv_obj_t *row, const char *text, uint32_t txt_color,
                      lv_event_cb_t cb, void *user_data) {
  const bool first = lv_obj_get_child_cnt(row) == 0;
  lv_obj_t *btn = lv_btn_create(row);
  lv_obj_remove_style_all(btn);
  lv_obj_set_height(btn, lv_pct(100));
  lv_obj_set_flex_grow(btn, 1);
  if (!first) {
    lv_obj_set_style_border_color(btn, color(kSurface3), LV_PART_MAIN);
    lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_LEFT, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
  }
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);
  lv_obj_t *l = lv_label_create(btn);
  lv_label_set_text(l, text);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_set_style_text_color(l, color(txt_color), LV_PART_MAIN);
  lv_obj_center(l);
  return btn;
}

}  // namespace GuiTheme

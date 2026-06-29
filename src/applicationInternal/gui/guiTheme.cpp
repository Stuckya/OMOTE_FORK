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

}  // namespace GuiTheme

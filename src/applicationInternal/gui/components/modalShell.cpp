#include "applicationInternal/gui/components/modalShell.h"

#include "applicationInternal/gui/guiTheme.h"
// SCR_WIDTH / SCR_HEIGHT come from build defines (-D), not the HAL.

namespace {

constexpr lv_coord_t kStatusBarH = 20;

void add_header(lv_obj_t *overlay, const std::string &name,
                const std::string &sub, lv_event_cb_t on_cancel) {
  lv_obj_t *header = lv_obj_create(overlay);
  lv_obj_remove_style_all(header);
  lv_obj_set_width(header, lv_pct(100));
  lv_obj_set_height(header, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(header, 9, LV_PART_MAIN);

  lv_obj_t *icon = lv_obj_create(header);
  lv_obj_remove_style_all(icon);
  lv_obj_set_size(icon, 24, 24);
  lv_obj_set_style_radius(icon, 7, LV_PART_MAIN);
  lv_obj_set_style_bg_color(icon, GuiTheme::color(GuiTheme::kSurface2),
                            LV_PART_MAIN);
  lv_obj_set_style_bg_opa(icon, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_t *isym = lv_label_create(icon);
  lv_label_set_text(isym, LV_SYMBOL_VIDEO);
  lv_obj_set_style_text_font(isym, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_set_style_text_color(isym, GuiTheme::color(GuiTheme::kWhite),
                              LV_PART_MAIN);
  lv_obj_center(isym);

  lv_obj_t *col = lv_obj_create(header);
  lv_obj_remove_style_all(col);
  lv_obj_set_height(col, LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(col, 1);
  lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
  lv_obj_t *label = lv_label_create(col);
  lv_obj_set_width(label, lv_pct(100));
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_label_set_text(label, name.c_str());
  lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(label, GuiTheme::color(GuiTheme::kWhite),
                              LV_PART_MAIN);
  if (!sub.empty()) {
    lv_obj_t *sl = lv_label_create(col);
    lv_label_set_text(sl, sub.c_str());
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(sl, GuiTheme::color(GuiTheme::kTextMute),
                                LV_PART_MAIN);
  }

  if (on_cancel == nullptr) return;
  lv_obj_t *cancel = lv_btn_create(header);
  lv_obj_remove_style_all(cancel);
  lv_obj_set_size(cancel, 26, 26);
  lv_obj_set_style_radius(cancel, 13, LV_PART_MAIN);
  lv_obj_set_style_bg_color(cancel, GuiTheme::color(GuiTheme::kRed),
                            LV_PART_MAIN);
  lv_obj_set_style_bg_opa(cancel, LV_OPA_20, LV_PART_MAIN);
  lv_obj_add_event_cb(cancel, on_cancel, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *x = lv_label_create(cancel);
  lv_label_set_text(x, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_font(x, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_set_style_text_color(x, GuiTheme::color(GuiTheme::kRed), LV_PART_MAIN);
  lv_obj_center(x);
}

}  // namespace

lv_obj_t *modalShell_create(const std::string &name, const std::string &sub,
                            lv_event_cb_t on_cancel) {
  lv_obj_t *overlay = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(overlay);
  lv_obj_set_size(overlay, SCR_WIDTH, SCR_HEIGHT - kStatusBarH);
  lv_obj_set_pos(overlay, 0, kStatusBarH);
  lv_obj_set_style_bg_color(overlay, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_pad_all(overlay, 16, LV_PART_MAIN);
  lv_obj_set_flex_flow(overlay, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(overlay, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(overlay, 10, LV_PART_MAIN);
  lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

  add_header(overlay, name, sub, on_cancel);
  return overlay;
}

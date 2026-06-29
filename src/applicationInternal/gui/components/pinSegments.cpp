#include "applicationInternal/gui/components/pinSegments.h"

#include "applicationInternal/gui/guiTheme.h"

lv_obj_t *pinSegments_create(lv_obj_t *parent, uint32_t count) {
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_remove_style_all(row);
  lv_obj_set_width(row, lv_pct(100));
  lv_obj_set_height(row, 50);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(row, 8, LV_PART_MAIN);

  // Narrower segments past 4 so the longest real case (6, Android TV) stays on
  // one line in the content band.
  const lv_coord_t seg_w = count > 4 ? 28 : 42;
  for (uint32_t i = 0; i < count; ++i) {
    lv_obj_t *seg = lv_obj_create(row);
    lv_obj_remove_style_all(seg);
    lv_obj_set_size(seg, seg_w, 48);
    lv_obj_set_style_radius(seg, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(seg, GuiTheme::color(GuiTheme::kSurface1),
                              LV_PART_MAIN);
    lv_obj_set_style_border_color(seg, GuiTheme::color(GuiTheme::kBlue),
                                  LV_PART_MAIN);
    lv_obj_t *lbl = lv_label_create(seg);
    lv_label_set_text(lbl, "");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, GuiTheme::color(GuiTheme::kWhite),
                                LV_PART_MAIN);
    lv_obj_center(lbl);
  }
  return row;
}

void pinSegments_render(lv_obj_t *segments, const std::string &code) {
  const uint32_t n = lv_obj_get_child_cnt(segments);
  for (uint32_t i = 0; i < n; ++i) {
    lv_obj_t *seg = lv_obj_get_child(segments, i);
    lv_obj_t *lbl = lv_obj_get_child(seg, 0);
    const bool filled = i < code.size();
    const bool active = i == code.size();
    lv_label_set_text(lbl, filled ? std::string(1, code[i]).c_str() : "");
    const uint32_t bg = filled ? GuiTheme::kSurface2
                               : (active ? GuiTheme::kSegActive
                                         : GuiTheme::kSurface1);
    lv_obj_set_style_bg_color(seg, GuiTheme::color(bg), LV_PART_MAIN);
    lv_obj_set_style_border_width(seg, active ? 2 : 0, LV_PART_MAIN);
  }
}

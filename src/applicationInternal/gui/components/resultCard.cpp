#include "applicationInternal/gui/components/resultCard.h"

#include "applicationInternal/gui/guiTheme.h"

namespace {

struct KindStyle {
  uint32_t color;
  const char *symbol;
};

KindStyle style_for(ResultKind kind) {
  switch (kind) {
    case ResultKind::kSuccess:
      return {GuiTheme::kGreen, LV_SYMBOL_OK};
    case ResultKind::kFailed:
      return {GuiTheme::kRed, LV_SYMBOL_CLOSE};
    case ResultKind::kTimeout:
      return {GuiTheme::kAmber, LV_SYMBOL_WARNING};
  }
  return {GuiTheme::kRed, LV_SYMBOL_CLOSE};
}

}  // namespace

void resultCard_create(lv_obj_t *parent, ResultKind kind,
                       const std::string &title, const std::string &subtitle) {
  lv_obj_t *core = lv_obj_create(parent);
  lv_obj_remove_style_all(core);
  lv_obj_set_width(core, lv_pct(100));
  lv_obj_set_flex_grow(core, 1);
  lv_obj_set_flex_flow(core, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(core, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(core, 12, LV_PART_MAIN);

  const KindStyle ks = style_for(kind);

  lv_obj_t *badge = lv_obj_create(core);
  lv_obj_remove_style_all(badge);
  lv_obj_set_size(badge, 60, 60);
  lv_obj_set_style_radius(badge, 30, LV_PART_MAIN);
  lv_obj_set_style_bg_color(badge, GuiTheme::color(ks.color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_t *sym = lv_label_create(badge);
  lv_label_set_text(sym, ks.symbol);
  lv_obj_set_style_text_font(sym, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(sym, GuiTheme::color(GuiTheme::kWhite),
                              LV_PART_MAIN);
  lv_obj_center(sym);

  lv_obj_t *t = lv_label_create(core);
  lv_label_set_text(t, title.c_str());
  lv_obj_set_style_text_font(t, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(t, GuiTheme::color(GuiTheme::kWhite), LV_PART_MAIN);

  if (subtitle.empty()) return;
  lv_obj_t *s = lv_label_create(core);
  lv_label_set_long_mode(s, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(s, lv_pct(85));
  lv_label_set_text(s, subtitle.c_str());
  lv_obj_set_style_text_align(s, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_font(s, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_set_style_text_color(s, GuiTheme::color(GuiTheme::kTextMute),
                              LV_PART_MAIN);
}

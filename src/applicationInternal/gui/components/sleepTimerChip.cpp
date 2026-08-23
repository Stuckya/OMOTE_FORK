#include "applicationInternal/gui/components/sleepTimerChip.h"

#include "applicationInternal/gui/guiTheme.h"
#include "guis/gui_sleepTimer.h"

LV_IMG_DECLARE(sleep_moon_small);

namespace {

lv_obj_t *chip = nullptr;
lv_obj_t *chipLabel = nullptr;

void chip_event_cb(lv_event_t *) { gui_sleepTimer_show(); }

}  // namespace

void createSleepTimerChip(lv_obj_t *parent) {
  chip = GuiTheme::flexRow(parent, 3);
  lv_obj_add_flag(chip, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(chip, chip_event_cb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *moon = lv_img_create(chip);
  lv_img_set_src(moon, &sleep_moon_small);
  lv_obj_set_style_img_recolor(moon, GuiTheme::color(GuiTheme::kTextMute),
                               LV_PART_MAIN);
  lv_obj_set_style_img_recolor_opa(moon, LV_OPA_COVER, LV_PART_MAIN);

  chipLabel = lv_label_create(chip);
  lv_label_set_text(chipLabel, "");
  lv_obj_set_style_text_font(chipLabel, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_set_style_text_color(chipLabel, GuiTheme::color(GuiTheme::kTextMute),
                              LV_PART_MAIN);

  lv_obj_add_flag(chip, LV_OBJ_FLAG_HIDDEN);
}

void setLabelSleepTimer(const std::string &remaining) {
  if (chip == nullptr || chipLabel == nullptr) {
    return;
  }
  if (remaining.empty()) {
    lv_obj_add_flag(chip, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_label_set_text(chipLabel, remaining.c_str());
  lv_obj_clear_flag(chip, LV_OBJ_FLAG_HIDDEN);
}

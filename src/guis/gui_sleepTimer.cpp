#include "guis/gui_sleepTimer.h"

#if (ENABLE_HUB_COMMUNICATION > 0)

#include <ctime>
#include <hubSleepTimer.h>

#include "applicationInternal/gui/components/modalShell.h"
#include "applicationInternal/gui/components/ringPicker.h"
#include "applicationInternal/gui/guiBase.h"
#include "applicationInternal/gui/guiStatusUpdate.h"
#include "applicationInternal/gui/guiTheme.h"
#include "applicationInternal/hub/sleepTimer.h"
#include "applicationInternal/scenes/sceneHandler.h"

LV_IMG_DECLARE(sleep_moon);
LV_IMG_DECLARE(sleep_moon_medium);

namespace {


lv_obj_t *overlay = nullptr;
lv_obj_t *pickerRing = nullptr;
lv_obj_t *countdownLabel = nullptr;
lv_obj_t *offAtLabel = nullptr;

std::string subtitleForActiveScene() {
  const std::string scene = gui_memoryOptimizer_getActiveSceneName();
  return scene.empty() ? std::string("Powers off active devices")
                       : "Powers off " + scene + " scene";
}

// Empty until the hub has set the clock -- the same condition the status-bar
// clock shows as "--:--".
std::string offAtText(uint32_t remainingSeconds) {
  const std::string at = formatLocalClockTime(time(nullptr) + (time_t)remainingSeconds);
  return at.empty() ? std::string() : "Powers off at " + at;
}

void destroy_overlay() {
  if (overlay != nullptr) {
    lv_obj_del(overlay);
  }
  overlay = nullptr;
  pickerRing = nullptr;
  countdownLabel = nullptr;
  offAtLabel = nullptr;
  Hub::SleepTimer::setRepaintCallback(nullptr);
}

void close_event_cb(lv_event_t *) { gui_sleepTimer_hide(); }

void confirm_event_cb(lv_event_t *) {
  if (pickerRing == nullptr) {
    return;
  }
  Hub::SleepTimer::arm(RingPicker::value(pickerRing));
  gui_sleepTimer_hide();
}

void extend_event_cb(lv_event_t *) { Hub::SleepTimer::extendByDefault(); }

void cancel_event_cb(lv_event_t *) {
  Hub::SleepTimer::cancel();
  gui_sleepTimer_hide();
}

void repaint_countdown() {
  if (countdownLabel == nullptr) {
    return;
  }
  if (!Hub::SleepTimer::isArmed()) {
    gui_sleepTimer_show();  // it was cancelled or fired elsewhere: back to the ring
    return;
  }
  lv_label_set_text(countdownLabel, Hub::SleepTimer::countdownText().c_str());
  if (offAtLabel != nullptr) {
    lv_label_set_text(offAtLabel, offAtText(Hub::SleepTimer::remainingSeconds()).c_str());
  }
}

void build_picker(lv_obj_t *root) {
  RingPicker::Config config;
  config.minValue = HubSleepTimer::MIN_MINUTES;
  config.maxValue = HubSleepTimer::MAX_MINUTES;
  config.step = HubSleepTimer::STEP_MINUTES;
  config.initialValue = HubSleepTimer::DEFAULT_MINUTES;
  config.unit = "min";
  pickerRing = RingPicker::create(root, config);

  // Dragging only chooses; arming is its own deliberate tap, so a slip on the
  // ring cannot start a countdown you did not mean.
  GuiTheme::spacer(root);
  GuiTheme::primaryButton(root, "Confirm", GuiTheme::kBlue, confirm_event_cb);
}

void build_armed(lv_obj_t *root) {
  lv_obj_t *badge = lv_obj_create(root);
  lv_obj_remove_style_all(badge);
  lv_obj_set_size(badge, 60, 60);
  lv_obj_set_style_radius(badge, 30, LV_PART_MAIN);
  lv_obj_set_style_bg_color(badge, GuiTheme::color(GuiTheme::kBlue), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(badge, LV_OPA_20, LV_PART_MAIN);
  lv_obj_t *moon = lv_img_create(badge);
  lv_img_set_src(moon, &sleep_moon);
  lv_obj_set_style_img_recolor(moon, GuiTheme::color(GuiTheme::kBlue), LV_PART_MAIN);
  lv_obj_set_style_img_recolor_opa(moon, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_center(moon);

  countdownLabel = lv_label_create(root);
  lv_obj_set_style_text_font(countdownLabel, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(countdownLabel, GuiTheme::color(GuiTheme::kWhite),
                              LV_PART_MAIN);

  offAtLabel = lv_label_create(root);
  lv_obj_set_style_text_font(offAtLabel, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_set_style_text_color(offAtLabel, GuiTheme::color(GuiTheme::kTextMute),
                              LV_PART_MAIN);

  GuiTheme::spacer(root);

  lv_obj_t *actions = GuiTheme::flexRow(root, 8);
  lv_obj_set_width(actions, lv_pct(100));
  lv_obj_set_height(actions, 40);

  lv_obj_t *extend = GuiTheme::tintedButton(actions, "+15 min", GuiTheme::kSurface2,
                                            LV_OPA_COVER, GuiTheme::kBlue,
                                            extend_event_cb);
  lv_obj_set_flex_grow(extend, 1);
  lv_obj_set_height(extend, 40);
  // Red on red tint: it kills the timer, not the sheet.
  lv_obj_t *cancel = GuiTheme::tintedButton(actions, "Cancel", GuiTheme::kRed,
                                            LV_OPA_20, GuiTheme::kRed,
                                            cancel_event_cb);
  lv_obj_set_flex_grow(cancel, 1);
  lv_obj_set_height(cancel, 40);

  repaint_countdown();
  Hub::SleepTimer::setRepaintCallback(repaint_countdown);
}

}  // namespace

void gui_sleepTimer_show(void) {
  // Modals sit below the status bar, so the chip stays tappable while another
  // one owns the screen. Stacking a second overlay there would resume that
  // one's tabs underneath it when this sheet closed; leave it alone instead.
  if (overlay == nullptr && guis_tabsSuspended()) {
    return;
  }
  guis_suspendActiveTabs();
  destroy_overlay();

  overlay = modalShell_createWithIcon("Sleep Timer", subtitleForActiveScene(),
                                      close_event_cb, &sleep_moon_medium,
                                      GuiTheme::kBlue);
  if (Hub::SleepTimer::isArmed()) {
    build_armed(overlay);
    return;
  }
  build_picker(overlay);
}

void gui_sleepTimer_hide(void) {
  destroy_overlay();
  guis_resumeActiveTabs();
}

void gui_sleepTimer_dismiss_overlay(void) { destroy_overlay(); }

#else

// The countdown is the hub's; with no hub there is nothing to arm or show.
void gui_sleepTimer_show(void) {}
void gui_sleepTimer_hide(void) {}
void gui_sleepTimer_dismiss_overlay(void) {}

#endif

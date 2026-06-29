#include "guis/gui_pairing.h"

#include <cstdint>
#include <string>

#include "applicationInternal/gui/components/hexKeypad.h"
#include "applicationInternal/gui/components/modalShell.h"
#include "applicationInternal/gui/components/pinSegments.h"
#include "applicationInternal/gui/components/resultCard.h"
#include "applicationInternal/gui/guiBase.h"
#include "applicationInternal/gui/guiTheme.h"
#include "applicationInternal/hub/pairingManager.h"
#include "guis/gui_devices.h"

namespace {

lv_obj_t *overlay = nullptr;
lv_obj_t *segments = nullptr;
lv_obj_t *submit_btn = nullptr;
lv_obj_t *submit_label = nullptr;

std::string pin_buffer;
std::string submitted_code;
std::string device_name;
uint32_t expected_len = 4;
bool requires_pin = true;

void destroy_overlay() {
  if (overlay == nullptr) return;
  lv_obj_del(overlay);
  overlay = nullptr;
  segments = nullptr;
  submit_btn = nullptr;
  submit_label = nullptr;
}

// Each pairing screen frees the tab tree underneath (idempotent — only the
// first screen actually tears it down) and replaces the previous overlay. The
// tree is rebuilt once in gui_pairing_hide() when pairing ends.
void begin_screen() {
  guis_suspendActiveTabs();
  // Pairing can be entered from the device-scan results; both modals share
  // lv_layer_top, so the scan overlay must go before ours appears.
  gui_devices_dismiss_overlay();
  destroy_overlay();
}

void render() {
  pinSegments_render(segments, pin_buffer);
  const bool complete = pin_buffer.size() == expected_len;
  lv_obj_set_style_bg_color(
      submit_btn,
      GuiTheme::color(complete ? GuiTheme::kBlue : GuiTheme::kSurface2),
      LV_PART_MAIN);
  lv_obj_set_style_text_color(
      submit_label,
      GuiTheme::color(complete ? GuiTheme::kWhite : GuiTheme::kTextMute),
      LV_PART_MAIN);
}

void on_key(int v) {
  if (pin_buffer.size() >= expected_len) return;
  pin_buffer += static_cast<char>(v < 10 ? '0' + v : 'A' + (v - 10));
  render();
}

void backspace_cb(lv_event_t *e) {
  LV_UNUSED(e);
  if (!pin_buffer.empty()) {
    pin_buffer.pop_back();
    render();
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

// Backspace + completion-gated Submit — the one pairing-specific control row.
void add_pin_actions() {
  lv_obj_t *actions = lv_obj_create(overlay);
  lv_obj_remove_style_all(actions);
  lv_obj_set_width(actions, lv_pct(100));
  lv_obj_set_height(actions, 44);
  lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(actions, 8, LV_PART_MAIN);

  lv_obj_t *back = lv_btn_create(actions);
  lv_obj_remove_style_all(back);
  lv_obj_set_size(back, 54, 44);
  lv_obj_set_style_radius(back, 12, LV_PART_MAIN);
  lv_obj_set_style_bg_color(back, GuiTheme::color(GuiTheme::kSurface2),
                            LV_PART_MAIN);
  lv_obj_set_style_bg_opa(back, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(back, GuiTheme::color(GuiTheme::kBlue),
                            LV_STATE_PRESSED);
  lv_obj_add_event_cb(back, backspace_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *bsym = lv_label_create(back);
  lv_label_set_text(bsym, LV_SYMBOL_BACKSPACE);
  lv_obj_set_style_text_font(bsym, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(bsym, GuiTheme::color(GuiTheme::kTextDim),
                              LV_PART_MAIN);
  lv_obj_center(bsym);

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
}

}  // namespace

void gui_pairing_show_awaiting(const std::string &deviceName,
                               const std::string &message,
                               uint32_t expectedPinLength, bool requiresPin) {
  LV_UNUSED(message);
  begin_screen();
  device_name = deviceName;
  expected_len = expectedPinLength > 0 ? expectedPinLength : 4;
  requires_pin = requiresPin;
  pin_buffer.clear();

  overlay = modalShell_create(device_name, "Code from your TV", cancel_cb);
  segments = pinSegments_create(overlay, expected_len);
  hexKeypad_create(overlay, on_key);
  add_pin_actions();
  render();
}

void gui_pairing_show_verifying() {
  begin_screen();
  overlay = modalShell_create(device_name, "", nullptr);

  lv_obj_t *core = lv_obj_create(overlay);
  lv_obj_remove_style_all(core);
  lv_obj_set_width(core, lv_pct(100));
  lv_obj_set_flex_grow(core, 1);
  lv_obj_set_flex_flow(core, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(core, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(core, 12, LV_PART_MAIN);

  lv_obj_t *spinner = lv_spinner_create(core, 1000, 60);
  lv_obj_set_size(spinner, 56, 56);
  lv_obj_set_style_arc_color(spinner, GuiTheme::color(GuiTheme::kSurface2),
                             LV_PART_MAIN);
  lv_obj_set_style_arc_color(spinner, GuiTheme::color(GuiTheme::kBlue),
                             LV_PART_INDICATOR);

  lv_obj_t *t = lv_label_create(core);
  lv_label_set_text(t, "Verifying code...");
  lv_obj_set_style_text_font(t, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(t, GuiTheme::color(GuiTheme::kWhite), LV_PART_MAIN);

  if (!submitted_code.empty()) {
    lv_obj_t *chips = lv_obj_create(core);
    lv_obj_remove_style_all(chips);
    lv_obj_set_size(chips, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(chips, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(chips, 7, LV_PART_MAIN);
    for (char c : submitted_code) {
      lv_obj_t *chip = lv_obj_create(chips);
      lv_obj_remove_style_all(chip);
      lv_obj_set_size(chip, 28, 34);
      lv_obj_set_style_radius(chip, 8, LV_PART_MAIN);
      lv_obj_set_style_bg_color(chip, GuiTheme::color(GuiTheme::kSurface2),
                                LV_PART_MAIN);
      lv_obj_set_style_bg_opa(chip, LV_OPA_COVER, LV_PART_MAIN);
      lv_obj_t *l = lv_label_create(chip);
      const char text[2] = {c, '\0'};
      lv_label_set_text(l, text);
      lv_obj_set_style_text_font(l, &lv_font_montserrat_16, LV_PART_MAIN);
      lv_obj_set_style_text_color(l, GuiTheme::color(GuiTheme::kWhite),
                                  LV_PART_MAIN);
      lv_obj_center(l);
    }
  }

  GuiTheme::textButton(overlay, "Cancel", GuiTheme::kRed, cancel_cb);
}

void gui_pairing_show_success(const std::string &deviceName) {
  device_name = deviceName;
  begin_screen();
  overlay = modalShell_create(device_name, "", nullptr);
  resultCard_create(overlay, ResultKind::kSuccess, "Paired",
                    device_name + " is ready to control.");
  GuiTheme::primaryButton(overlay, "Done", GuiTheme::kBlue, done_cb);
}

void gui_pairing_show_failed(const std::string &message) {
  begin_screen();
  overlay = modalShell_create(device_name, "", nullptr);
  resultCard_create(overlay, ResultKind::kFailed, "Incorrect code", message);
  GuiTheme::primaryButton(overlay, "Try Again", GuiTheme::kBlue, retry_cb);
  GuiTheme::textButton(overlay, "Cancel", GuiTheme::kRed, cancel_cb);
}

void gui_pairing_hide() {
  destroy_overlay();
  pin_buffer.clear();
  guis_resumeActiveTabs();
}

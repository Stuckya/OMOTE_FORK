#include "guis/gui_devices.h"

#include <ctime>
#include <string>
#include <vector>

#include "applicationInternal/gui/components/modalShell.h"
#include "applicationInternal/gui/guiBase.h"
#include "applicationInternal/gui/guiTheme.h"
#include "applicationInternal/hub/deviceManager.h"
#include "applicationInternal/hub/pairingManager.h"

namespace {

lv_obj_t *overlay = nullptr;

// Detail/confirm need the subject device after the results list is gone.
Hub::DeviceEntry selected_device;

void destroy_overlay() {
  if (overlay == nullptr) return;
  lv_obj_del(overlay);
  overlay = nullptr;
}

// Same overlay discipline as gui_pairing: every screen frees the tab tree
// underneath (idempotent) and replaces the previous overlay.
void begin_screen() {
  guis_suspendActiveTabs();
  destroy_overlay();
}

void close_cb(lv_event_t *e) {
  LV_UNUSED(e);
  gui_devices_hide();
}

void cancel_scan_cb(lv_event_t *e) {
  LV_UNUSED(e);
  Hub::DeviceManager::getInstance().cancelScan();
  gui_devices_hide();
}

void scan_again_cb(lv_event_t *e) {
  LV_UNUSED(e);
  Hub::DeviceManager::getInstance().startScan();
  gui_devices_show_scanning();
}

// Tapping a pairable scan row hands the screen to the pairing modal; this
// overlay is dismissed without resuming tabs so the flows don't stack.
void pair_row_cb(lv_event_t *e) {
  auto *entry = static_cast<Hub::DeviceEntry *>(lv_event_get_user_data(e));
  const std::string device_id = entry->deviceId;
  gui_devices_dismiss_overlay();
  Hub::PairingManager::getInstance().startPairing(device_id);
}

void detail_row_cb(lv_event_t *e) {
  auto *entry = static_cast<Hub::DeviceEntry *>(lv_event_get_user_data(e));
  gui_devices_show_detail(*entry);
}

// Rows keep pointers into this storage; rebuilt with each results screen.
std::vector<Hub::DeviceEntry> row_entries;

lv_obj_t *make_card(lv_obj_t *parent) {
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_remove_style_all(card);
  lv_obj_set_width(card, lv_pct(100));
  lv_obj_set_height(card, LV_SIZE_CONTENT);
  lv_obj_set_style_radius(card, 12, LV_PART_MAIN);
  lv_obj_set_style_bg_color(card, GuiTheme::color(GuiTheme::kSurface1),
                            LV_PART_MAIN);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_pad_hor(card, 12, LV_PART_MAIN);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  // lv_obj is CLICKABLE by default and events don't bubble; the card must not
  // swallow taps meant for its rows.
  lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);
  return card;
}

lv_obj_t *make_row(lv_obj_t *card, int height) {
  lv_obj_t *row = lv_obj_create(card);
  lv_obj_remove_style_all(row);
  lv_obj_set_width(row, lv_pct(100));
  lv_obj_set_height(row, height);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  return row;
}

lv_obj_t *small_label(lv_obj_t *parent, const char *text, uint32_t color) {
  lv_obj_t *l = lv_label_create(parent);
  lv_label_set_text(l, text);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_set_style_text_color(l, GuiTheme::color(color), LV_PART_MAIN);
  return l;
}

std::string format_paired_date(uint64_t epoch) {
  if (epoch == 0) return "--";
  static const char *const kMonths[] = {"Jan", "Feb", "Mar", "Apr",
                                        "May", "Jun", "Jul", "Aug",
                                        "Sep", "Oct", "Nov", "Dec"};
  const time_t t = static_cast<time_t>(epoch);
  struct tm parts;
  if (gmtime_r(&t, &parts) == nullptr) return "--";
  return std::string(kMonths[parts.tm_mon]) + " " +
         std::to_string(parts.tm_mday);
}

void forget_confirmed_cb(lv_event_t *e) {
  LV_UNUSED(e);
  Hub::DeviceManager::getInstance().forgetDevice(selected_device.deviceId);
  gui_devices_hide();
}

void confirm_dismiss_cb(lv_event_t *e) {
  lv_obj_del(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}

// Dimmed scrim + centered alert over the detail screen; Cancel is the safe
// default, Forget is red.
void show_confirm_alert(lv_event_t *e) {
  LV_UNUSED(e);
  lv_obj_t *scrim = lv_obj_create(overlay);
  lv_obj_remove_style_all(scrim);
  lv_obj_set_size(scrim, lv_pct(100), lv_pct(100));
  lv_obj_add_flag(scrim, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_align(scrim, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(scrim, GuiTheme::color(GuiTheme::kBlack),
                            LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scrim, LV_OPA_60, LV_PART_MAIN);
  lv_obj_add_flag(scrim, LV_OBJ_FLAG_CLICKABLE);  // swallow taps underneath

  lv_obj_t *alert = lv_obj_create(scrim);
  lv_obj_remove_style_all(alert);
  lv_obj_set_size(alert, 196, LV_SIZE_CONTENT);
  lv_obj_center(alert);
  lv_obj_set_style_radius(alert, 14, LV_PART_MAIN);
  lv_obj_set_style_bg_color(alert, GuiTheme::color(GuiTheme::kSurface2),
                            LV_PART_MAIN);
  lv_obj_set_style_bg_opa(alert, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_flex_flow(alert, LV_FLEX_FLOW_COLUMN);

  lv_obj_t *top = lv_obj_create(alert);
  lv_obj_remove_style_all(top);
  lv_obj_set_width(top, lv_pct(100));
  lv_obj_set_height(top, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(top, 14, LV_PART_MAIN);
  lv_obj_set_flex_flow(top, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(top, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(top, 6, LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(top);
  lv_label_set_text_fmt(title, "Forget %s?", selected_device.name.c_str());
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(title, GuiTheme::color(GuiTheme::kWhite),
                              LV_PART_MAIN);
  lv_obj_set_width(title, lv_pct(100));
  lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

  lv_obj_t *msg = small_label(top, "You'll need to pair again to control it.",
                              GuiTheme::kTextDim);
  lv_obj_set_width(msg, lv_pct(100));
  lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

  lv_obj_t *btns = lv_obj_create(alert);
  lv_obj_remove_style_all(btns);
  lv_obj_set_width(btns, lv_pct(100));
  lv_obj_set_height(btns, 40);
  lv_obj_set_flex_flow(btns, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_border_color(btns, GuiTheme::color(GuiTheme::kSurface3),
                                LV_PART_MAIN);
  lv_obj_set_style_border_side(btns, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
  lv_obj_set_style_border_width(btns, 1, LV_PART_MAIN);

  auto make_alert_btn = [&](const char *text, uint32_t color,
                            lv_event_cb_t cb, void *user_data) {
    lv_obj_t *b = lv_btn_create(btns);
    lv_obj_remove_style_all(b);
    lv_obj_set_height(b, lv_pct(100));
    lv_obj_set_flex_grow(b, 1);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, user_data);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, GuiTheme::color(color), LV_PART_MAIN);
    lv_obj_center(l);
    return b;
  };

  lv_obj_t *cancel = make_alert_btn("Cancel", GuiTheme::kBlue,
                                    confirm_dismiss_cb, scrim);
  lv_obj_set_style_border_color(cancel, GuiTheme::color(GuiTheme::kSurface3),
                                LV_PART_MAIN);
  lv_obj_set_style_border_side(cancel, LV_BORDER_SIDE_RIGHT, LV_PART_MAIN);
  lv_obj_set_style_border_width(cancel, 1, LV_PART_MAIN);
  make_alert_btn("Forget", GuiTheme::kRed, forget_confirmed_cb, nullptr);
}

const char *requirement_text(omote_PairingRequirement req) {
  switch (req) {
    case omote_PairingRequirement_PAIRING_REQUIREMENT_DISABLED:
      return "Pairing disabled";
    case omote_PairingRequirement_PAIRING_REQUIREMENT_UNSUPPORTED:
      return "Unsupported";
    case omote_PairingRequirement_PAIRING_REQUIREMENT_NOT_NEEDED:
      return "No pairing needed";
    default:
      return "";
  }
}

}  // namespace

void gui_devices_show_scanning() {
  begin_screen();
  overlay = modalShell_create("Pair Apple TV", "", nullptr);

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
  lv_label_set_text(t, "Scanning...");
  lv_obj_set_style_text_font(t, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(t, GuiTheme::color(GuiTheme::kWhite),
                              LV_PART_MAIN);

  lv_obj_t *sub = small_label(core, "Looking for Apple TVs on your network.",
                              GuiTheme::kTextMute);
  lv_obj_set_width(sub, lv_pct(90));
  lv_label_set_long_mode(sub, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

  GuiTheme::textButton(overlay, "Cancel", GuiTheme::kTextDim, cancel_scan_cb);
}

void gui_devices_show_scan_results(
    const std::vector<Hub::DeviceEntry> &devices) {
  begin_screen();

  if (devices.empty()) {
    overlay = modalShell_create("Pair Apple TV", "", close_cb);

    // Neutral (not alarming) empty state: idle badge, title carries it.
    lv_obj_t *core = lv_obj_create(overlay);
    lv_obj_remove_style_all(core);
    lv_obj_set_width(core, lv_pct(100));
    lv_obj_set_flex_grow(core, 1);
    lv_obj_set_flex_flow(core, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(core, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(core, 12, LV_PART_MAIN);

    lv_obj_t *badge = lv_obj_create(core);
    lv_obj_remove_style_all(badge);
    lv_obj_set_size(badge, 60, 60);
    lv_obj_set_style_radius(badge, 30, LV_PART_MAIN);
    lv_obj_set_style_bg_color(badge, GuiTheme::color(GuiTheme::kSurface2),
                              LV_PART_MAIN);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_t *sym = lv_label_create(badge);
    lv_label_set_text(sym, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(sym, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(sym, GuiTheme::color(GuiTheme::kTextMute),
                                LV_PART_MAIN);
    lv_obj_center(sym);

    lv_obj_t *t = lv_label_create(core);
    lv_label_set_text(t, "No Apple TVs found");
    lv_obj_set_style_text_font(t, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(t, GuiTheme::color(GuiTheme::kWhite),
                                LV_PART_MAIN);

    GuiTheme::primaryButton(overlay, "Scan Again", GuiTheme::kBlue,
                            scan_again_cb);
    return;
  }

  row_entries = devices;
  overlay = modalShell_create(
      "Apple TVs", std::to_string(devices.size()) + " found", close_cb);

  lv_obj_t *card = make_card(overlay);
  for (Hub::DeviceEntry &entry : row_entries) {
    lv_obj_t *row = make_row(card, 44);

    lv_obj_t *names = lv_obj_create(row);
    lv_obj_remove_style_all(names);
    lv_obj_clear_flag(names, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_height(names, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(names, 1);
    lv_obj_set_flex_flow(names, LV_FLEX_FLOW_COLUMN);
    lv_obj_t *nm = lv_label_create(names);
    lv_label_set_text(nm, entry.name.c_str());
    lv_obj_set_style_text_font(nm, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_width(nm, lv_pct(100));
    lv_label_set_long_mode(nm, LV_LABEL_LONG_DOT);
    small_label(names, entry.address.c_str(), GuiTheme::kTextMute);

    const bool pairable =
        entry.pairing ==
            omote_PairingRequirement_PAIRING_REQUIREMENT_MANDATORY ||
        entry.pairing == omote_PairingRequirement_PAIRING_REQUIREMENT_OPTIONAL;

    if (entry.paired) {
      lv_obj_set_style_text_color(nm, GuiTheme::color(GuiTheme::kWhite),
                                  LV_PART_MAIN);
      small_label(row, LV_SYMBOL_OK " Paired", GuiTheme::kGreen);
      lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(row, detail_row_cb, LV_EVENT_CLICKED, &entry);
    } else if (pairable) {
      lv_obj_set_style_text_color(nm, GuiTheme::color(GuiTheme::kWhite),
                                  LV_PART_MAIN);
      small_label(row, LV_SYMBOL_RIGHT, GuiTheme::kTextMute);
      lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(row, pair_row_cb, LV_EVENT_CLICKED, &entry);
    } else {
      lv_obj_set_style_text_color(nm, GuiTheme::color(GuiTheme::kTextMute),
                                  LV_PART_MAIN);
      small_label(row, requirement_text(entry.pairing), GuiTheme::kTextMute);
    }
  }

  lv_obj_t *spacer = lv_obj_create(overlay);
  lv_obj_remove_style_all(spacer);
  lv_obj_set_width(spacer, lv_pct(100));
  lv_obj_set_flex_grow(spacer, 1);

  GuiTheme::textButton(overlay, "Scan Again", GuiTheme::kBlue, scan_again_cb);
}

void gui_devices_show_detail(const Hub::DeviceEntry &device) {
  selected_device = device;
  begin_screen();
  overlay = modalShell_create(device.name, device.model, close_cb);

  lv_obj_t *card = make_card(overlay);
  lv_obj_t *row = make_row(card, 36);
  small_label(row, "Status", GuiTheme::kTextMute);
  small_label(row, device.paired ? LV_SYMBOL_OK " Paired" : "Not paired",
              device.paired ? GuiTheme::kGreen : GuiTheme::kTextDim);

  row = make_row(card, 36);
  small_label(row, "Address", GuiTheme::kTextMute);
  small_label(row, device.address.c_str(), GuiTheme::kWhite);

  row = make_row(card, 36);
  small_label(row, "Paired", GuiTheme::kTextMute);
  small_label(row, format_paired_date(device.pairedAt).c_str(),
              GuiTheme::kWhite);

  lv_obj_t *spacer = lv_obj_create(overlay);
  lv_obj_remove_style_all(spacer);
  lv_obj_set_width(spacer, lv_pct(100));
  lv_obj_set_flex_grow(spacer, 1);

  // Destructive action: red-tinted background, red text, gated by the alert.
  lv_obj_t *forget = GuiTheme::primaryButton(overlay, "Forget This Device",
                                             GuiTheme::kRed, show_confirm_alert);
  lv_obj_set_style_bg_opa(forget, LV_OPA_20, LV_PART_MAIN);
  lv_obj_t *forget_label = lv_obj_get_child(forget, 0);
  if (forget_label != nullptr) {
    lv_obj_set_style_text_color(forget_label, GuiTheme::color(GuiTheme::kRed),
                                LV_PART_MAIN);
  }
}

void gui_devices_dismiss_overlay() {
  destroy_overlay();
}

void gui_devices_hide() {
  destroy_overlay();
  guis_resumeActiveTabs();
}

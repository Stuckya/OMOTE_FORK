#include <lvgl.h>
#include <string>
#include <hubNowPlaying.h>
#include "applicationInternal/hardware/hardwarePresenter.h"
#include "applicationInternal/gui/guiBase.h"
#include "applicationInternal/gui/guiRegistry.h"
#include "applicationInternal/omote_log.h"
#include "devices/mediaPlayer/device_appleTV/gui_appleTV.h"

#include "applicationInternal/commandHandler.h"
#include "devices/mediaPlayer/device_appleTV/device_appleTV.h"

// LVGL declarations
LV_IMG_DECLARE(appleTvIcon);
LV_IMG_DECLARE(appleDisplayIcon);
LV_IMG_DECLARE(appleBackIcon);

// Apple Key Event handler
static void appleKey_event_cb(lv_event_t* e) {
  // Send IR command based on the event user data  
  int user_data = *((int*)(&(e->user_data)));
  omote_log_v("appleKey_event_cb: Event Id: '%d'.\r\n", user_data);

  if (user_data == 2)
  {
    executeCommand(APPLETV_HOME);
  }
  else if (user_data == 1)
  {
    executeCommand(APPLETV_MENU);
  }
}

// The tab is deleted and rebuilt as the user swipes, so the latest playback
// lives here and each rebuild draws from it.
static HubNowPlaying nowPlaying;

static lv_obj_t* metadata_container = nullptr;
static lv_obj_t* title_label = nullptr;
static lv_obj_t* artist_label = nullptr;
static lv_obj_t* state_label = nullptr;
static lv_obj_t* progress_bar = nullptr;

static void render_now_playing();

void create_tab_content_appleTV(lv_obj_t* tab) {
  lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* appleImg = lv_img_create(tab);
  lv_img_set_src(appleImg, &appleTvIcon);
  lv_obj_align(appleImg, LV_ALIGN_CENTER, 0, -100);
  
  metadata_container = lv_obj_create(tab);
  lv_obj_set_size(metadata_container, SCR_WIDTH - 32, 140);
  lv_obj_align(metadata_container, LV_ALIGN_CENTER, 0, 25);
  lv_obj_clear_flag(metadata_container, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_set_style_bg_color(metadata_container, lv_color_hex(0x1C1C1E), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(metadata_container, LV_OPA_100, LV_PART_MAIN);
  lv_obj_set_style_border_width(metadata_container, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(metadata_container, 12, LV_PART_MAIN);
  lv_obj_set_style_pad_all(metadata_container, 16, LV_PART_MAIN);
  
  title_label = lv_label_create(metadata_container);
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_label_set_long_mode(title_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_width(title_label, SCR_WIDTH - 64);
  
  artist_label = lv_label_create(metadata_container);
  lv_obj_set_style_text_font(artist_label, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_set_style_text_color(artist_label, lv_color_hex(0xEBEBF5), LV_PART_MAIN);
  lv_obj_align(artist_label, LV_ALIGN_TOP_LEFT, 0, 28);
  lv_label_set_long_mode(artist_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_width(artist_label, SCR_WIDTH - 64);
  
  state_label = lv_label_create(metadata_container);
  lv_obj_set_style_text_font(state_label, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_set_style_text_color(state_label, lv_color_hex(0x8E8E93), LV_PART_MAIN);
  lv_obj_align(state_label, LV_ALIGN_BOTTOM_LEFT, 0, -24);
  
  progress_bar = lv_bar_create(metadata_container);
  lv_obj_set_size(progress_bar, SCR_WIDTH - 64, 3);
  lv_obj_align(progress_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
  
  lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x3A3A3C), LV_PART_MAIN);
  lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x007AFF), LV_PART_INDICATOR);
  lv_obj_set_style_radius(progress_bar, 2, LV_PART_MAIN);
  lv_obj_set_style_radius(progress_bar, 2, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(progress_bar, LV_OPA_100, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(progress_bar, LV_OPA_100, LV_PART_INDICATOR);
  
  lv_bar_set_range(progress_bar, 0, 100);

  render_now_playing();
}

static std::string replace_bullet_chars(const std::string& input) {
  std::string result = input;
  size_t pos = 0;
  while ((pos = result.find("·", pos)) != std::string::npos) {
    result.replace(pos, 3, "| ");
    pos += 2;
  }
  return result;
}

static std::string add_marquee_gap(const std::string& input) {
  if (input.empty()) return input;
  return input + "      ";
}

static std::string title_text() {
  const std::string title = nowPlaying.title();
  if (title.empty()) {
    return "No media playing";
  }
  return add_marquee_gap(replace_bullet_chars(title));
}

static std::string artist_text() {
  const std::string artist = nowPlaying.artist();
  if (artist.empty()) {
    return "";
  }
  std::string text = replace_bullet_chars(artist);
  const std::string album = nowPlaying.album();
  if (!album.empty()) {
    text += " • " + replace_bullet_chars(album);
  }
  return add_marquee_gap(text);
}

static const char* state_text() {
  switch (nowPlaying.state()) {
    case HubNowPlaying::State::PLAYING: return LV_SYMBOL_PLAY " Playing";
    case HubNowPlaying::State::PAUSED:  return LV_SYMBOL_PAUSE " Paused";
    case HubNowPlaying::State::STOPPED: return LV_SYMBOL_STOP " Stopped";
    case HubNowPlaying::State::LOADING: return "Loading";
    case HubNowPlaying::State::SEEKING: return "Seeking";
    case HubNowPlaying::State::NONE:    return "";
  }
  return "";
}

// Setting a scrolling label's text restarts its scroll, so leave unchanged text alone.
static void set_label_text_if_changed(lv_obj_t* label, const std::string& text) {
  if (text != lv_label_get_text(label)) {
    lv_label_set_text(label, text.c_str());
  }
}

static void render_now_playing() {
  const bool tabIsBuilt = metadata_container != nullptr;
  if (!tabIsBuilt) {
    return;
  }

  set_label_text_if_changed(title_label, title_text());
  set_label_text_if_changed(artist_label, artist_text());
  set_label_text_if_changed(state_label, state_text());

  if (nowPlaying.hasProgress()) {
    lv_obj_clear_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);
    lv_bar_set_value(progress_bar, nowPlaying.progressPercent(), LV_ANIM_OFF);
  } else {
    lv_obj_add_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);
  }
}

void show_appleTV_now_playing(const omote_DeviceState& snapshot) {
  nowPlaying.observe(snapshot);
  render_now_playing();
}

void notify_tab_before_delete_appleTV(void) {
  metadata_container = nullptr;
  title_label = nullptr;
  artist_label = nullptr;
  state_label = nullptr;
  progress_bar = nullptr;
}

void register_gui_appleTV(void){
  register_gui(std::string(tabName_appleTV), & create_tab_content_appleTV, & notify_tab_before_delete_appleTV);
}

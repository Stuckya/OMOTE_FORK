#include <lvgl.h>
#include <ctime>
#include <string>
#include <hubNowPlaying.h>
#include "applicationInternal/hardware/hardwarePresenter.h"
#include "applicationInternal/gui/guiBase.h"
#include "applicationInternal/gui/fonts/guiFonts.h"
#include "applicationInternal/gui/guiRegistry.h"
#include "applicationInternal/gui/guiShownListeners.h"
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

#if defined(ARDUINO)
static const bool HAS_ARTWORK_MEMORY = OMOTE_HARDWARE_REV >= 5;
#else
static const bool HAS_ARTWORK_MEMORY = true;
#endif

#if (ENABLE_HUB_COMMUNICATION == 1)
static const HubLink HUB_LINK = HubLink::ESP_NOW;
#elif (ENABLE_HUB_COMMUNICATION == 2)
static const HubLink HUB_LINK = HubLink::MQTT;
#elif (ENABLE_HUB_COMMUNICATION == 3)
static const HubLink HUB_LINK = HubLink::WEBSOCKET;
#else
static const HubLink HUB_LINK = HubLink::NONE;
#endif

static const bool FETCHES_ARTWORK = remoteFetchesArtwork(HAS_ARTWORK_MEMORY, HUB_LINK);

static const lv_coord_t CONTENT_WIDTH = SCR_WIDTH;
static const lv_coord_t STAGE_TOP = 12;
static const lv_coord_t STAGE_HEIGHT = 148;
static const lv_coord_t SIDE_PAD = 16;
static const lv_coord_t TEXT_FORWARD_PAD = 18;
static const lv_coord_t BOTTOM_PAD = 8;
static const lv_coord_t COVER_RADIUS = 11;
static const lv_coord_t PROGRESS_HEIGHT = 3;
static const lv_coord_t PROGRESS_RADIUS = 2;
static const lv_coord_t STATE_ICON_GAP = 4;
static const uint32_t TICK_MS = 1000;
static const int PROGRESS_RANGE = 1000;
static const lv_opa_t HAIRLINE_OPA = 18;

static const uint32_t COLOR_TEXT = 0xFFFFFF;
static const uint32_t COLOR_TEXT_SECONDARY = 0xEBEBF5;
static const uint32_t COLOR_TEXT_TERTIARY = 0x8E8E93;
static const uint32_t COLOR_PLACEHOLDER_TILE = 0x202024;
static const uint32_t COLOR_PLACEHOLDER_GLYPH = 0x6F6F76;
static const uint32_t COLOR_TRACK = 0x3A3A3C;
static const uint32_t COLOR_ACCENT = 0x007AFF;

struct MediaTypeLook {
  const char* glyph;
  const char* label;
  uint32_t accent;
};

static MediaTypeLook lookFor(omote_MediaType mediaType) {
  switch (mediaType) {
    case omote_MediaType_MEDIA_TYPE_MUSIC: return {LV_SYMBOL_AUDIO, "MUSIC", 0xFF7E9A};
    case omote_MediaType_MEDIA_TYPE_TV:    return {LV_SYMBOL_VIDEO, "TV", 0x6BA2FF};
    case omote_MediaType_MEDIA_TYPE_VIDEO: return {LV_SYMBOL_VIDEO, "VIDEO", 0x46CBBC};
    default:                               return {LV_SYMBOL_PLAY, "MEDIA", 0x9A9AA0};
  }
}

// The tab is deleted and rebuilt as the user swipes, so the latest playback
// lives here and each rebuild draws from it.
static HubNowPlaying nowPlaying;

struct ProgressRow {
  lv_obj_t* row;
  lv_obj_t* bar;
  lv_obj_t* stateIcon;
  lv_obj_t* elapsed;
  lv_obj_t* duration;
};

struct CoverView {
  lv_obj_t* root;
  lv_obj_t* cover;
  lv_obj_t* glyph;
  lv_obj_t* title;
  lv_obj_t* subtitle;
  lv_obj_t* caption;
  ProgressRow progress;
};

struct TextForwardView {
  lv_obj_t* root;
  lv_obj_t* type;
  lv_obj_t* state;
  lv_obj_t* title;
  lv_obj_t* subtitle;
  lv_obj_t* caption;
  ProgressRow progress;
};

static lv_obj_t* idleView = nullptr;
static CoverView coverView = {};
static TextForwardView textForwardView = {};
static lv_timer_t* tickTimer = nullptr;
static bool tabIsShown = false;

static void render_now_playing();

static lv_obj_t* create_plain_box(lv_obj_t* parent) {
  lv_obj_t* box = lv_obj_create(parent);
  lv_obj_remove_style_all(box);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
  return box;
}

static lv_obj_t* create_label(lv_obj_t* parent, const lv_font_t* font, uint32_t color) {
  lv_obj_t* label = lv_label_create(parent);
  lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
  lv_obj_set_style_text_color(label, lv_color_hex(color), LV_PART_MAIN);
  lv_label_set_text(label, "");
  return label;
}

static lv_obj_t* create_line_label(lv_obj_t* parent, const lv_font_t* font, uint32_t color,
                                   lv_coord_t width) {
  lv_obj_t* label = create_label(parent, font, color);
  lv_obj_set_width(label, width);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  return label;
}

static ProgressRow create_progress_row(lv_obj_t* parent, lv_coord_t width) {
  ProgressRow progress;
  progress.row = create_plain_box(parent);
  lv_obj_set_size(progress.row, width, 26);
  lv_obj_align(progress.row, LV_ALIGN_BOTTOM_MID, 0, -BOTTOM_PAD);

  progress.bar = lv_bar_create(progress.row);
  lv_obj_set_size(progress.bar, width, PROGRESS_HEIGHT);
  lv_obj_align(progress.bar, LV_ALIGN_TOP_MID, 0, 0);
  lv_bar_set_range(progress.bar, 0, PROGRESS_RANGE);
  lv_obj_set_style_bg_color(progress.bar, lv_color_hex(COLOR_TRACK), LV_PART_MAIN);
  lv_obj_set_style_bg_color(progress.bar, lv_color_hex(COLOR_ACCENT), LV_PART_INDICATOR);
  lv_obj_set_style_radius(progress.bar, PROGRESS_RADIUS, LV_PART_MAIN);
  lv_obj_set_style_radius(progress.bar, PROGRESS_RADIUS, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(progress.bar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(progress.bar, LV_OPA_COVER, LV_PART_INDICATOR);

  // Flex skips hidden children, so the elapsed time sits flush left whenever the icon is hidden.
  lv_obj_t* lead = create_plain_box(progress.row);
  lv_obj_set_size(lead, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_align(lead, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_flex_flow(lead, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(lead, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(lead, STATE_ICON_GAP, LV_PART_MAIN);
  progress.stateIcon = create_label(lead, &lv_font_montserrat_12, COLOR_TEXT_SECONDARY);
  progress.elapsed = create_label(lead, &lv_font_montserrat_12, COLOR_TEXT_TERTIARY);
  progress.duration = create_label(progress.row, &lv_font_montserrat_12, COLOR_TEXT_TERTIARY);
  lv_obj_align(progress.duration, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  return progress;
}

static void create_idle_view(lv_obj_t* tab) {
  idleView = create_plain_box(tab);
  lv_obj_set_size(idleView, CONTENT_WIDTH, LV_PCT(100));
  lv_obj_set_flex_flow(idleView, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(idleView, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t* logo = lv_img_create(idleView);
  lv_img_set_src(logo, &appleTvIcon);
  lv_obj_set_style_pad_bottom(logo, 22, LV_PART_MAIN);

  lv_obj_t* title = create_label(idleView, &lv_font_montserrat_16, COLOR_TEXT);
  lv_label_set_text(title, "Nothing playing");

  lv_obj_t* nudge = create_label(idleView, &lv_font_montserrat_12, COLOR_TEXT_TERTIARY);
  lv_obj_set_width(nudge, CONTENT_WIDTH - 48);
  lv_obj_set_style_pad_top(nudge, 7, LV_PART_MAIN);
  lv_obj_set_style_text_align(nudge, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_label_set_text(nudge, "Pick something on your Apple TV and it'll show up here.");
}

static void create_cover_view(lv_obj_t* tab) {
  const lv_coord_t textWidth = CONTENT_WIDTH - 2 * SIDE_PAD;
  coverView.root = create_plain_box(tab);
  lv_obj_set_size(coverView.root, CONTENT_WIDTH, LV_PCT(100));

  lv_obj_t* stage = create_plain_box(coverView.root);
  lv_obj_set_size(stage, CONTENT_WIDTH, STAGE_HEIGHT);
  lv_obj_align(stage, LV_ALIGN_TOP_MID, 0, STAGE_TOP);

  coverView.cover = create_plain_box(stage);
  lv_obj_center(coverView.cover);
  lv_obj_set_style_radius(coverView.cover, COVER_RADIUS, LV_PART_MAIN);
  lv_obj_set_style_clip_corner(coverView.cover, true, LV_PART_MAIN);
  lv_obj_set_style_bg_color(coverView.cover, lv_color_hex(COLOR_PLACEHOLDER_TILE), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(coverView.cover, LV_OPA_COVER, LV_PART_MAIN);

  coverView.glyph = create_label(coverView.cover, &lv_font_montserrat_24, COLOR_PLACEHOLDER_GLYPH);
  lv_obj_center(coverView.glyph);

  lv_obj_t* lines = create_plain_box(coverView.root);
  lv_obj_set_width(lines, textWidth);
  lv_obj_set_height(lines, LV_SIZE_CONTENT);
  lv_obj_align(lines, LV_ALIGN_TOP_MID, 0, STAGE_TOP + STAGE_HEIGHT + 6);
  lv_obj_set_flex_flow(lines, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(lines, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(lines, 2, LV_PART_MAIN);

  coverView.title = create_line_label(lines, &montserrat_latin_16, COLOR_TEXT, textWidth);
  coverView.subtitle = create_line_label(lines, &montserrat_latin_14, COLOR_TEXT_SECONDARY, textWidth);
  coverView.caption = create_line_label(lines, &montserrat_latin_12, COLOR_TEXT_TERTIARY, textWidth);
  lv_obj_set_style_text_align(coverView.title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_align(coverView.subtitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_align(coverView.caption, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

  coverView.progress = create_progress_row(coverView.root, textWidth);
}

static void create_text_forward_view(lv_obj_t* tab) {
  const lv_coord_t textWidth = CONTENT_WIDTH - 2 * TEXT_FORWARD_PAD;
  textForwardView.root = create_plain_box(tab);
  lv_obj_set_size(textForwardView.root, CONTENT_WIDTH, LV_PCT(100));

  lv_obj_t* top = create_plain_box(textForwardView.root);
  lv_obj_set_size(top, textWidth, 28);
  lv_obj_align(top, LV_ALIGN_TOP_MID, 0, TEXT_FORWARD_PAD);
  lv_obj_set_style_border_side(top, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
  lv_obj_set_style_border_width(top, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(top, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_border_opa(top, HAIRLINE_OPA, LV_PART_MAIN);
  textForwardView.type = create_label(top, &lv_font_montserrat_10, COLOR_TEXT_TERTIARY);
  lv_obj_align(textForwardView.type, LV_ALIGN_TOP_LEFT, 0, 0);
  textForwardView.state = create_label(top, &lv_font_montserrat_10, COLOR_TEXT_TERTIARY);
  lv_obj_align(textForwardView.state, LV_ALIGN_TOP_RIGHT, 0, 0);

  lv_obj_t* body = create_plain_box(textForwardView.root);
  lv_obj_set_size(body, textWidth, 170);
  lv_obj_align(body, LV_ALIGN_TOP_MID, 0, TEXT_FORWARD_PAD + 32);
  lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(body, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(body, 6, LV_PART_MAIN);

  textForwardView.title = create_label(body, &montserrat_latin_24, COLOR_TEXT);
  textForwardView.subtitle = create_label(body, &montserrat_latin_16, COLOR_TEXT_SECONDARY);
  textForwardView.caption = create_line_label(body, &montserrat_latin_12, COLOR_TEXT_TERTIARY, textWidth);
  const lv_coord_t titleLines = 3;
  const lv_coord_t subtitleLines = 2;
  lv_obj_set_size(textForwardView.title, textWidth, LV_SIZE_CONTENT);
  lv_obj_set_style_max_height(textForwardView.title,
                              titleLines * lv_font_get_line_height(&montserrat_latin_24), LV_PART_MAIN);
  lv_label_set_long_mode(textForwardView.title, LV_LABEL_LONG_DOT);
  lv_obj_set_size(textForwardView.subtitle, textWidth, LV_SIZE_CONTENT);
  lv_obj_set_style_max_height(textForwardView.subtitle,
                              subtitleLines * lv_font_get_line_height(&montserrat_latin_16), LV_PART_MAIN);
  lv_label_set_long_mode(textForwardView.subtitle, LV_LABEL_LONG_DOT);

  textForwardView.progress = create_progress_row(textForwardView.root, textWidth);
}

static void tick_cb(lv_timer_t*) {
  render_now_playing();
}

void create_tab_content_appleTV(lv_obj_t* tab) {
  lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(tab, 0, LV_PART_MAIN);

  create_idle_view(tab);
  create_cover_view(tab);
  create_text_forward_view(tab);
  tickTimer = lv_timer_create(tick_cb, TICK_MS, nullptr);
  tabIsShown = false;

  render_now_playing();
}

static const char* state_text() {
  switch (nowPlaying.state()) {
    case HubNowPlaying::State::PLAYING: return LV_SYMBOL_PLAY " PLAYING";
    case HubNowPlaying::State::PAUSED:  return LV_SYMBOL_PAUSE " PAUSED";
    case HubNowPlaying::State::STOPPED: return LV_SYMBOL_STOP " STOPPED";
    case HubNowPlaying::State::LOADING: return "LOADING";
    case HubNowPlaying::State::SEEKING: return "SEEKING";
    case HubNowPlaying::State::NONE:    return "";
  }
  return "";
}

static const char* state_symbol() {
  switch (nowPlaying.state()) {
    case HubNowPlaying::State::PLAYING: return LV_SYMBOL_PLAY;
    case HubNowPlaying::State::PAUSED:  return LV_SYMBOL_PAUSE;
    case HubNowPlaying::State::STOPPED: return LV_SYMBOL_STOP;
    case HubNowPlaying::State::LOADING:
    case HubNowPlaying::State::SEEKING:
    case HubNowPlaying::State::NONE:    return "";
  }
  return "";
}

static void set_text_if_changed(lv_obj_t* label, const std::string& text) {
  if (text != lv_label_get_text(label)) {
    lv_label_set_text(label, text.c_str());
  }
}

// Clearing LV_OBJ_FLAG_HIDDEN invalidates the object even when it is already shown, so skip no-ops.
static void set_hidden(lv_obj_t* obj, bool hidden) {
  if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN) == hidden) {
    return;
  }
  if (hidden) {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }
}

static void set_line(lv_obj_t* label, const std::string& text) {
  set_hidden(label, text.empty());
  set_text_if_changed(label, text);
}

static void render_progress(const ProgressRow& progress, time_t now) {
  set_line(progress.stateIcon, state_symbol());
  const bool hasProgress = nowPlaying.hasProgress();
  set_hidden(progress.bar, !hasProgress);
  set_hidden(progress.elapsed, !hasProgress);
  set_hidden(progress.duration, !hasProgress);
  if (!hasProgress) {
    return;
  }
  lv_bar_set_value(progress.bar, nowPlaying.progressPermille(now), LV_ANIM_OFF);
  set_text_if_changed(progress.elapsed, nowPlaying.elapsedText(now));
  set_text_if_changed(progress.duration, nowPlaying.durationText());
}

static void render_cover_view(time_t now) {
  const MediaTypeLook look = lookFor(nowPlaying.mediaType());
  lv_obj_set_size(coverView.cover, nowPlaying.coverWidth(), nowPlaying.coverHeight());
  set_text_if_changed(coverView.glyph, look.glyph);
  set_line(coverView.title, nowPlaying.title());
  set_line(coverView.subtitle, nowPlaying.subtitle());
  set_line(coverView.caption, nowPlaying.caption());
  render_progress(coverView.progress, now);
}

static void render_text_forward_view(time_t now) {
  const MediaTypeLook look = lookFor(nowPlaying.mediaType());
  set_text_if_changed(textForwardView.type, std::string(look.glyph) + "  " + look.label);
  lv_obj_set_style_text_color(textForwardView.type, lv_color_hex(look.accent), LV_PART_MAIN);
  set_text_if_changed(textForwardView.state, state_text());
  set_line(textForwardView.title, nowPlaying.title());
  set_line(textForwardView.subtitle, nowPlaying.subtitle());
  set_line(textForwardView.caption, nowPlaying.caption());
  render_progress(textForwardView.progress, now);
}

static void render_now_playing() {
  const bool tabIsBuilt = idleView != nullptr;
  if (!tabIsBuilt) {
    return;
  }

  const HubNowPlaying::View view = nowPlaying.view(FETCHES_ARTWORK);
  const time_t now = time(nullptr);
  set_hidden(idleView, view != HubNowPlaying::View::IDLE);
  set_hidden(coverView.root, view != HubNowPlaying::View::COVER);
  set_hidden(textForwardView.root, view != HubNowPlaying::View::TEXT_FORWARD);
  if (view == HubNowPlaying::View::COVER) {
    render_cover_view(now);
  } else if (view == HubNowPlaying::View::TEXT_FORWARD) {
    render_text_forward_view(now);
  }

  if (nowPlaying.ticks() && tabIsShown) {
    lv_timer_resume(tickTimer);
  } else {
    lv_timer_pause(tickTimer);
  }
}

void show_appleTV_now_playing(const omote_DeviceState& snapshot) {
  nowPlaying.observe(snapshot);
  render_now_playing();
}

static void on_shown_gui(const std::string& shownGuiName) {
  tabIsShown = shownGuiName == tabName_appleTV;
  render_now_playing();
}

void notify_tab_before_delete_appleTV(void) {
  tabIsShown = false;
  if (tickTimer != nullptr) {
    lv_timer_del(tickTimer);
    tickTimer = nullptr;
  }
  idleView = nullptr;
  coverView = {};
  textForwardView = {};
}

void register_gui_appleTV(void){
  register_gui(std::string(tabName_appleTV), & create_tab_content_appleTV, & notify_tab_before_delete_appleTV);
  Gui::registerShownGuiListener(on_shown_gui);
}

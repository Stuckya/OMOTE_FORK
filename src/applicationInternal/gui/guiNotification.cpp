#include "guiNotification.h"
#include "guiBase.h"
#include "applicationInternal/gui/guiTheme.h"
#include "applicationInternal/hub/sleepTimer.h"
#include "applicationInternal/omote_log.h"

namespace GuiNotification {

static lv_obj_t* notification_container = nullptr;
static lv_obj_t* notification_label = nullptr;
static lv_obj_t* volume_level_label = nullptr;
static lv_obj_t* volume_bar = nullptr;
static lv_obj_t* power_segments[HubPowerSession::CAPACITY] = {};
static lv_obj_t* action_row = nullptr;
// A warning that arrived mid-slide. The sleep timer's warning edge is consumed
// once, so dropping it here would lose the only notice before an automatic
// power-off.
static bool warning_pending = false;
static lv_timer_t* hide_timer = nullptr;
static lv_anim_t slide_anim;

static const int NOTIFICATION_HEIGHT_SINGLE = 48;
static const int NOTIFICATION_HEIGHT_DOUBLE = 80;
// Tall enough for a headline plus a full-size action pair: the banner's actions
// are the same buttons as the sleep sheet's, so they read the same everywhere.
static const int NOTIFICATION_HEIGHT_ACTIONS = 104;
static const int CONTENT_PADDING = 16;
static const int BAR_HEIGHT = 3;
static const int BAR_BOTTOM_INSET = 8;
static const int SEGMENT_GAP = 3;
static const int ACTION_HEIGHT = 40;
static const int ACTION_GAP = 8;
static const int SLIDE_DURATION = 300;
static const int AUTO_HIDE_DELAY = 3000;
static const int RAPID_UPDATE_DELAY = 1500;
static const int HOLD_UNTIL_UPDATED = 0;

static const uint32_t COLOR_SHELL = GuiTheme::kSurface1;
static const uint32_t COLOR_TEXT = 0xFFFFFF;
static const uint32_t COLOR_TRACK = 0x3A3A3C;
static const uint32_t COLOR_BUSY = 0x007AFF;
static const uint32_t COLOR_OK = 0x32D74B;
static const uint32_t COLOR_WARN = 0xFF9500;
static const uint32_t COLOR_MUTED = 0x8E8E93;
static const uint32_t COLOR_ERROR = 0xFF3B30;

static bool notification_visible = false;
static bool animation_in_progress = false;
static int current_height = NOTIFICATION_HEIGHT_SINGLE;
static int auto_hide_delay = AUTO_HIDE_DELAY;

static void slide_anim_cb(void* obj, int32_t value);
static void slide_down_complete_cb(lv_anim_t* anim);
static void flushPendingWarning();
static void slide_up_complete_cb(lv_anim_t* anim);
static void auto_hide_timer_cb(lv_timer_t* timer);
static void notification_gesture_cb(lv_event_t* e);

static lv_obj_t* createSegment(lv_obj_t* parent) {
    lv_obj_t* segment = lv_obj_create(parent);
    lv_obj_set_size(segment, SCR_WIDTH - 4 * CONTENT_PADDING, BAR_HEIGHT);
    lv_obj_set_style_bg_color(segment, lv_color_hex(COLOR_TRACK), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(segment, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_border_width(segment, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(segment, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(segment, 2, LV_PART_MAIN);
    lv_obj_clear_flag(segment, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(segment, LV_OBJ_FLAG_HIDDEN);
    return segment;
}

static void cancel_event_cb(lv_event_t*) {
    Hub::SleepTimer::cancel();
    hideNotification();
}

static void extend_event_cb(lv_event_t*) {
    Hub::SleepTimer::extendByDefault();
    hideNotification();
}

static void createActionRow() {
    action_row = GuiTheme::flexRow(notification_container, ACTION_GAP);
    lv_obj_set_width(action_row, SCR_WIDTH - 2 * CONTENT_PADDING);
    lv_obj_set_height(action_row, ACTION_HEIGHT);
    lv_obj_align(action_row, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Not "Dismiss": a swipe up already hides any banner, so the slot answers
    // the question the warning actually raises -- stop it, or push it back.
    lv_obj_t* cancel = GuiTheme::tintedButton(action_row, "Cancel", GuiTheme::kRed,
                                              LV_OPA_20, GuiTheme::kRed, cancel_event_cb);
    lv_obj_set_flex_grow(cancel, 1);
    lv_obj_set_height(cancel, ACTION_HEIGHT);
    lv_obj_t* extend = GuiTheme::tintedButton(action_row, "+15 min", GuiTheme::kSurface2,
                                              LV_OPA_COVER, GuiTheme::kBlue, extend_event_cb);
    lv_obj_set_flex_grow(extend, 1);
    lv_obj_set_height(extend, ACTION_HEIGHT);
    lv_obj_add_flag(action_row, LV_OBJ_FLAG_HIDDEN);
}

void init() {
    if (notification_container != nullptr) {
        return;
    }

    lv_obj_t* parent = lv_layer_top();
    if (!parent) {
        parent = lv_scr_act();
    }

    notification_container = lv_obj_create(parent);
    lv_obj_set_size(notification_container, SCR_WIDTH, NOTIFICATION_HEIGHT_SINGLE);

    lv_obj_move_to_index(notification_container, -1);

    int notification_y = statusbarTop + statusbarHeight - NOTIFICATION_HEIGHT_SINGLE;
    lv_obj_set_pos(notification_container, 0, notification_y);

    lv_obj_set_style_bg_color(notification_container, lv_color_hex(COLOR_SHELL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(notification_container, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_border_width(notification_container, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(notification_container, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(notification_container, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(notification_container, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(notification_container, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_pad_all(notification_container, CONTENT_PADDING, LV_PART_MAIN);

    notification_label = lv_label_create(notification_container);
    lv_label_set_text(notification_label, "");
    lv_obj_set_style_text_font(notification_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(notification_label, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    lv_obj_align(notification_label, LV_ALIGN_LEFT_MID, 0, 0);

    volume_level_label = lv_label_create(notification_container);
    lv_label_set_text(volume_level_label, "");
    lv_obj_set_style_text_font(volume_level_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(volume_level_label, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    lv_obj_align(volume_level_label, LV_ALIGN_TOP_RIGHT, -8, 8);
    lv_obj_add_flag(volume_level_label, LV_OBJ_FLAG_HIDDEN);

    volume_bar = lv_bar_create(notification_container);
    lv_obj_set_size(volume_bar, SCR_WIDTH - 4 * CONTENT_PADDING, BAR_HEIGHT);
    lv_obj_align(volume_bar, LV_ALIGN_BOTTOM_MID, 0, -BAR_BOTTOM_INSET);
    lv_obj_set_style_bg_color(volume_bar, lv_color_hex(COLOR_TRACK), LV_PART_MAIN);
    lv_obj_set_style_bg_color(volume_bar, lv_color_hex(COLOR_BUSY), LV_PART_INDICATOR);
    lv_obj_set_style_radius(volume_bar, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(volume_bar, 2, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(volume_bar, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(volume_bar, LV_OPA_100, LV_PART_INDICATOR);
    lv_obj_add_flag(volume_bar, LV_OBJ_FLAG_HIDDEN);

    for (size_t i = 0; i < HubPowerSession::CAPACITY; i++) {
        power_segments[i] = createSegment(notification_container);
    }

    createActionRow();

    lv_obj_add_event_cb(notification_container, notification_gesture_cb, LV_EVENT_GESTURE, nullptr);
    lv_obj_clear_flag(notification_container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_clear_flag(notification_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(notification_container, LV_OBJ_FLAG_FLOATING);

    lv_obj_add_flag(notification_container, LV_OBJ_FLAG_HIDDEN);

    omote_log_d("Notification system initialized\r\n");
}

static void slide_anim_cb(void* obj, int32_t value) {
    lv_obj_set_y((lv_obj_t*)obj, value);
}

static void cancelAutoHide() {
    if (!hide_timer) {
        return;
    }
    lv_timer_del(hide_timer);
    hide_timer = nullptr;
}

static void armAutoHide() {
    cancelAutoHide();
    if (auto_hide_delay == HOLD_UNTIL_UPDATED) {
        return;
    }
    hide_timer = lv_timer_create(auto_hide_timer_cb, auto_hide_delay, nullptr);
    lv_timer_set_repeat_count(hide_timer, 1);
}

static void slide_down_complete_cb(lv_anim_t* anim) {
    animation_in_progress = false;
    notification_visible = true;
    armAutoHide();
    flushPendingWarning();
}

static void slide_up_complete_cb(lv_anim_t* anim) {
    animation_in_progress = false;
    notification_visible = false;
    lv_obj_add_flag(notification_container, LV_OBJ_FLAG_HIDDEN);
    flushPendingWarning();
}

static void auto_hide_timer_cb(lv_timer_t* timer) {
    hide_timer = nullptr;
    if (notification_visible && !animation_in_progress) {
        hideNotification();
    }
}

static void notification_gesture_cb(lv_event_t* e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());

    if (dir == LV_DIR_TOP && notification_visible) {
        hideNotification();
    }
}

static void setHidden(lv_obj_t* obj, bool hidden) {
    if (hidden) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void showOnlyWidgetsFor(NotificationType type) {
    setHidden(volume_bar, type != NotificationType::VOLUME);
    setHidden(volume_level_label, type != NotificationType::VOLUME);
    setHidden(action_row, type != NotificationType::SLEEP_WARNING);
    for (size_t i = 0; i < HubPowerSession::CAPACITY; i++) {
        setHidden(power_segments[i], true);
    }
    lv_label_set_recolor(notification_label, false);
}

static void alignContent() {
    if (current_height == NOTIFICATION_HEIGHT_SINGLE) {
        lv_obj_align(notification_label, LV_ALIGN_LEFT_MID, 0, 0);
        return;
    }
    lv_obj_align(notification_label, LV_ALIGN_TOP_LEFT, 0, 8);
    lv_obj_align(volume_level_label, LV_ALIGN_TOP_RIGHT, -8, 8);
}

static void layoutContainer() {
    lv_obj_set_size(notification_container, SCR_WIDTH, current_height);
    lv_obj_set_pos(notification_container, 0, statusbarTop + statusbarHeight - current_height);
    alignContent();
    lv_obj_invalidate(notification_container);
}

// Resize a banner that is already on screen. Notification kinds differ in
// height, so replacing one in place has to resize it or the new content is laid
// out against the old box.
static void resizeShownContainer() {
    lv_obj_set_size(notification_container, SCR_WIDTH, current_height);
    lv_obj_set_pos(notification_container, 0, statusbarTop + statusbarHeight);
    alignContent();
    lv_obj_invalidate(notification_container);
}

static void showNotification() {
    if (notification_visible && !animation_in_progress) {
        resizeShownContainer();
        armAutoHide();
        return;
    }

    if (animation_in_progress) {
        return;
    }

    cancelAutoHide();

    lv_obj_clear_flag(notification_container, LV_OBJ_FLAG_HIDDEN);

    lv_obj_move_to_index(notification_container, -1);

    layoutContainer();

    int start_y = statusbarTop + statusbarHeight - current_height;
    int end_y = statusbarTop + statusbarHeight;

    lv_obj_set_pos(notification_container, 0, start_y);

    lv_anim_init(&slide_anim);
    lv_anim_set_var(&slide_anim, notification_container);
    lv_anim_set_values(&slide_anim, start_y, end_y);
    lv_anim_set_time(&slide_anim, SLIDE_DURATION);
    lv_anim_set_exec_cb(&slide_anim, slide_anim_cb);
    lv_anim_set_ready_cb(&slide_anim, slide_down_complete_cb);
    lv_anim_set_path_cb(&slide_anim, lv_anim_path_ease_out);

    animation_in_progress = true;
    lv_anim_start(&slide_anim);
}

void hideNotification() {
    if (!notification_visible || animation_in_progress) {
        return;
    }

    cancelAutoHide();

    int start_y = lv_obj_get_y(notification_container);
    int end_y = statusbarTop + statusbarHeight - current_height;

    lv_anim_init(&slide_anim);
    lv_anim_set_var(&slide_anim, notification_container);
    lv_anim_set_values(&slide_anim, start_y, end_y);
    lv_anim_set_time(&slide_anim, SLIDE_DURATION);
    lv_anim_set_exec_cb(&slide_anim, slide_anim_cb);
    lv_anim_set_ready_cb(&slide_anim, slide_up_complete_cb);
    lv_anim_set_path_cb(&slide_anim, lv_anim_path_ease_in);

    animation_in_progress = true;
    lv_anim_start(&slide_anim);
}

static void beginNotification(NotificationType type, int height, int hide_delay) {
    if (!notification_container) {
        init();
    }
    current_height = height;
    auto_hide_delay = hide_delay;
    showOnlyWidgetsFor(type);
    lv_obj_set_style_text_color(notification_label, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    // The volume row shares its line with the right-aligned level; every other
    // headline owns the full width and ellipsizes instead of running off the edge.
    if (type == NotificationType::VOLUME) {
        lv_obj_set_size(notification_label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        return;
    }
    lv_obj_set_size(notification_label, SCR_WIDTH - 2 * CONTENT_PADDING,
                    lv_font_get_line_height(&lv_font_montserrat_14));
    lv_label_set_long_mode(notification_label, LV_LABEL_LONG_DOT);
}

void showVolumeNotification(double level, bool is_muted) {
    beginNotification(NotificationType::VOLUME, NOTIFICATION_HEIGHT_DOUBLE, RAPID_UPDATE_DELAY);

    lv_label_set_text(notification_label, LV_SYMBOL_VOLUME_MID " Volume");

    char level_text[32];
    if (is_muted) {
        snprintf(level_text, sizeof(level_text), "MUTED");
    } else {
        snprintf(level_text, sizeof(level_text), "%.1f dB", level);
    }

    lv_label_set_text(volume_level_label, level_text);

    if (is_muted) {
        lv_bar_set_value(volume_bar, 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(volume_bar, lv_color_hex(COLOR_MUTED), LV_PART_INDICATOR);
    } else {
        const double dB_floor = -80.0;
        const double dB_ceiling = 23.0;
        const double dB_range = dB_ceiling - dB_floor;

        int percentage = (int)((level - dB_floor) * 100.0 / dB_range);
        if (percentage < 0) percentage = 0;
        if (percentage > 100) percentage = 100;

        lv_bar_set_value(volume_bar, percentage, LV_ANIM_ON);
        lv_obj_set_style_bg_color(volume_bar, lv_color_hex(COLOR_BUSY), LV_PART_INDICATOR);
    }

    showNotification();

    omote_log_d("Volume notification: %s\r\n", level_text);
}

void showPowerNotification(bool is_on) {
    beginNotification(NotificationType::POWER, NOTIFICATION_HEIGHT_SINGLE, AUTO_HIDE_DELAY);

    if (is_on) {
        lv_label_set_text(notification_label, LV_SYMBOL_POWER "  Device powered on");
    } else {
        lv_label_set_text(notification_label, LV_SYMBOL_POWER "  Device powered off");
    }

    showNotification();

    omote_log_d("Power notification: %s\r\n", is_on ? "ON" : "OFF");
}

static int holdFor(HubPowerSession::Phase phase) {
    switch (phase) {
        case HubPowerSession::Phase::RESOLVED:
            return RAPID_UPDATE_DELAY;
        case HubPowerSession::Phase::TIMED_OUT:
            return AUTO_HIDE_DELAY;
        default:
            return HOLD_UNTIL_UPDATED;
    }
}

static uint32_t colorFor(HubPowerSession::Tone tone) {
    switch (tone) {
        case HubPowerSession::Tone::OK:
            return COLOR_OK;
        case HubPowerSession::Tone::WARN:
            return COLOR_WARN;
        default:
            return COLOR_TEXT;
    }
}

static uint32_t colorFor(HubPowerSession::Outcome outcome) {
    switch (outcome) {
        case HubPowerSession::Outcome::CONFIRMED:
            return COLOR_OK;
        case HubPowerSession::Outcome::FAILED:
            return COLOR_WARN;
        default:
            return COLOR_TRACK;
    }
}

static void setHeadline(const HubPowerSession& session) {
    const std::string headline = session.headline();
    switch (session.tone()) {
        case HubPowerSession::Tone::OK:
            lv_label_set_text_fmt(notification_label, LV_SYMBOL_OK "  %s", headline.c_str());
            break;
        case HubPowerSession::Tone::WARN:
            lv_label_set_text_fmt(notification_label, LV_SYMBOL_WARNING "  %s", headline.c_str());
            break;
        default:
            // Recolor tints only the glyph; it stays off for the other tones so a
            // hub error message containing '#' cannot be misparsed.
            lv_label_set_recolor(notification_label, true);
            lv_label_set_text_fmt(notification_label, "#007AFF " LV_SYMBOL_POWER "#  %s", headline.c_str());
            break;
    }
    lv_obj_set_style_text_color(notification_label, lv_color_hex(colorFor(session.tone())), LV_PART_MAIN);
}

static void layoutSegments(const HubPowerSession& session) {
    const int count = (int)session.size();
    if (count == 0) {
        return;
    }
    const int usable = SCR_WIDTH - 2 * CONTENT_PADDING;
    const int width = (usable - SEGMENT_GAP * (count - 1)) / count;
    const int x0 = (usable - (width * count + SEGMENT_GAP * (count - 1))) / 2;

    for (int i = 0; i < count; i++) {
        lv_obj_t* segment = power_segments[i];
        lv_obj_set_size(segment, width, BAR_HEIGHT);
        lv_obj_align(segment, LV_ALIGN_BOTTOM_LEFT, x0 + i * (width + SEGMENT_GAP), -BAR_BOTTOM_INSET);
        lv_obj_set_style_bg_color(segment, lv_color_hex(colorFor(session.slotOutcome(i))), LV_PART_MAIN);
        setHidden(segment, false);
    }
}

void showPowerSession(const HubPowerSession& session) {
    if (session.phase() == HubPowerSession::Phase::IDLE) {
        return;
    }
    beginNotification(NotificationType::POWER_SESSION, NOTIFICATION_HEIGHT_DOUBLE, holdFor(session.phase()));

    setHeadline(session);
    layoutSegments(session);

    showNotification();

    omote_log_d("Power session: %s\r\n", session.headline().c_str());
}

void showSleepWarning() {
    if (animation_in_progress) {
        // Retried when the slide settles: showNotification would drop it, and
        // the caller has already spent its one-shot warning edge.
        warning_pending = true;
        return;
    }

    beginNotification(NotificationType::SLEEP_WARNING, NOTIFICATION_HEIGHT_ACTIONS,
                      HOLD_UNTIL_UPDATED);

    lv_label_set_text(notification_label, LV_SYMBOL_POWER "  Powering off in 1 min");
    lv_obj_set_style_text_color(notification_label, lv_color_hex(COLOR_WARN), LV_PART_MAIN);

    showNotification();

    omote_log_d("Sleep timer warning shown\r\n");
}

static void flushPendingWarning() {
    if (!warning_pending) {
        return;
    }
    warning_pending = false;
    showSleepWarning();
}

void showMessageNotification(const std::string& message) {
    beginNotification(NotificationType::MESSAGE, NOTIFICATION_HEIGHT_SINGLE, AUTO_HIDE_DELAY);

    lv_label_set_text(notification_label, message.c_str());

    showNotification();

    omote_log_d("Message notification: %s\r\n", message.c_str());
}

void showErrorNotification(const std::string& message) {
    beginNotification(NotificationType::ERROR, NOTIFICATION_HEIGHT_SINGLE, AUTO_HIDE_DELAY);

    lv_label_set_text(notification_label, message.c_str());
    lv_obj_set_style_text_color(notification_label, lv_color_hex(COLOR_ERROR), LV_PART_MAIN);

    showNotification();

    omote_log_d("Error notification: %s\r\n", message.c_str());
}

bool isNotificationVisible() {
    return notification_visible;
}

} // namespace GuiNotification

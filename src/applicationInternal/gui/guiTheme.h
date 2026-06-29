#pragma once

#include <lvgl.h>
#include <cstdint>

// Shared OMOTE GUI design system: color tokens (mirroring the design handoff)
// and small widget factories, so GUIs compose from one consistent vocabulary
// instead of inlining hex colors and per-widget styling.
namespace GuiTheme {

constexpr uint32_t kBlack = 0x000000;
constexpr uint32_t kSurface1 = 0x1c1c1e;
constexpr uint32_t kSurface2 = 0x2c2c2e;
constexpr uint32_t kSurface3 = 0x3a3a3c;
constexpr uint32_t kBlue = 0x007aff;
constexpr uint32_t kRed = 0xff3b30;
constexpr uint32_t kGreen = 0x34c759;
constexpr uint32_t kAmber = 0xff9500;
constexpr uint32_t kWhite = 0xffffff;
constexpr uint32_t kTextDim = 0xb9b9bb;
constexpr uint32_t kTextMute = 0x8a8a8d;
constexpr uint32_t kKeyTinted = 0x22262e;
constexpr uint32_t kKeyTintedTxt = 0x9fb4d8;
constexpr uint32_t kSegActive = 0x15151c;

inline lv_color_t color(uint32_t hex) { return lv_color_hex(hex); }

// Full-width filled primary action (Done, Try Again, Submit). Returns the button.
lv_obj_t *primaryButton(lv_obj_t *parent, const char *text, uint32_t bg,
                        lv_event_cb_t cb);

// Borderless text action (Cancel). Returns the button.
lv_obj_t *textButton(lv_obj_t *parent, const char *text, uint32_t txt_color,
                     lv_event_cb_t cb);

}  // namespace GuiTheme

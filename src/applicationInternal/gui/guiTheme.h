#pragma once

#include <lvgl.h>
#include <cstdint>

// Shared OMOTE GUI design system: color tokens (mirroring the design handoff)
// and small widget factories, so GUIs compose from one consistent vocabulary
// instead of inlining hex colors and per-widget styling.
namespace GuiTheme {

// Greys are chosen to survive RGB565: green carries 6 bits to red/blue's 5, so
// a grey with bit 2 set (0x1c, 0x2c) quantizes a step greener than its
// neighbours and reads as a green cast on the panel.
constexpr uint32_t kBlack = 0x000000;
constexpr uint32_t kSurface1 = 0x1a1a1a;
constexpr uint32_t kSurface2 = 0x2a2a2a;
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

// Filled action whose label takes the tint rather than white: the grammar for
// destructive ("Forget", red on red) and secondary ("+15 min", blue on grey)
// actions. Returns the button.
lv_obj_t *tintedButton(lv_obj_t *parent, const char *text, uint32_t bg,
                       lv_opa_t bg_opa, uint32_t txt_color, lv_event_cb_t cb);

// Transparent, non-interactive flex containers: the layout scaffolding sheets
// are built from, sized to their content. Callers override size and alignment
// where they need to. `gap` is the space between children.
lv_obj_t *flexRow(lv_obj_t *parent, lv_coord_t gap);
lv_obj_t *flexColumn(lv_obj_t *parent, lv_coord_t gap);

// Eats the leftover height of a flex column, pushing what follows to the
// bottom of the sheet -- where terminal actions belong.
lv_obj_t *spacer(lv_obj_t *parent);

// A row of equal-width actions divided by hairlines, above and between (the
// alert/banner grammar). Add actions with splitAction, which draws the divider
// for every action after the first.
lv_obj_t *splitActionRow(lv_obj_t *parent, lv_coord_t height);
lv_obj_t *splitAction(lv_obj_t *row, const char *text, uint32_t txt_color,
                      lv_event_cb_t cb, void *user_data = nullptr);

}  // namespace GuiTheme

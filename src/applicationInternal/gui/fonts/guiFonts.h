#pragma once

#include <lvgl.h>

// Montserrat glyphs for hub text that the built-in fonts lack: Latin-1, Latin Extended-A,
// typographic punctuation and the euro sign. Each falls back to lv_font_montserrat_N of the
// same size, which still draws ASCII and the LV_SYMBOL icons.
LV_FONT_DECLARE(montserrat_latin_12)
LV_FONT_DECLARE(montserrat_latin_14)
LV_FONT_DECLARE(montserrat_latin_16)
LV_FONT_DECLARE(montserrat_latin_24)

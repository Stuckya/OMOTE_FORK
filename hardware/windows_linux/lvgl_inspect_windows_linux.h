#pragma once

// Simulator-only LVGL inspector. Registers an lv_timer that watches a trigger
// file and, on demand, dumps the active screen as a PNG plus a DOM-like JSON
// tree (type, geometry, state, text) for headless UI inspection by tooling.
// Must be called after lv_init() and on the LVGL thread, since LVGL is single
// threaded and the dump walks live object state.
void lvgl_inspect_init();

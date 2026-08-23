#pragma once

#include <lvgl.h>

const char * const tabName_sceneSelection = "Scene selection";
void register_gui_sceneSelection(void);

// Repaints the sleep-timer row in place when the hub's timer changes, so the
// row on screen never disagrees with the status bar.
void gui_sceneSelection_refreshSleepTimerRow(void);

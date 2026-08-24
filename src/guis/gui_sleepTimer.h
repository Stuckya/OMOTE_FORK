#pragma once

#include <lvgl.h>

// The modal sheet: the ring picker when nothing is armed, the countdown with
// extend/cancel when one is.
void gui_sleepTimer_show(void);
void gui_sleepTimer_hide(void);
void gui_sleepTimer_dismiss_overlay(void);

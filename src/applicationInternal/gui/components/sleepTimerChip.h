#pragma once

#include <lvgl.h>
#include <string>

// The status-bar sleep chip: a moon and the remaining time beside the scene
// name, and the tap target that opens the timer sheet. Hidden when no timer is
// armed, so an unused feature costs no status-bar room.
void createSleepTimerChip(lv_obj_t *parent);
void setLabelSleepTimer(const std::string &remaining);

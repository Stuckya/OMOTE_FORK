#pragma once

#include <lvgl.h>
#include "remote_messages.pb.h"

const char * const tabName_appleTV = "Apple TV";
void register_gui_appleTV(void);
void show_appleTV_now_playing(const omote_DeviceState& snapshot);

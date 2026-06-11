#pragma once

#include <lvgl.h>

const char * const tabName_settings = "Settings";
void register_gui_settings(void);

// Redraws the Devices rows from DeviceManager's cache; no-op when the settings
// tab is not on screen. Called when a DEVICE_LIST response lands.
void gui_settings_refresh_devices(void);

// accessed by "guiStatusUpdate.cpp"
extern lv_obj_t* objBattSettingsVoltage;
extern lv_obj_t* objBattSettingsPercentage;
//extern lv_obj_t* objBattSettingsIscharging;


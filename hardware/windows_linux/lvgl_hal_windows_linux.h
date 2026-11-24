#pragma once

void init_lvgl_HAL();
// The SDL window is ready after init_lvgl_HAL(), so there is nothing left to start.
void start_display_panel_HAL();

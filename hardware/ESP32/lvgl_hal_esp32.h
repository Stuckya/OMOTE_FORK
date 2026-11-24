#pragma once

// Switches the display on and registers the LVGL drivers. Nothing is sent to the panel yet.
void init_lvgl_HAL();
// Initialises the panel and the touch controller and starts I2C. LVGL must not draw before this.
void start_display_panel_HAL();

# Omote LVGL Patterns

Use this as the local map before changing GUI behavior.

## PlatformIO Configuration

- `platformio.ini` defines `SCR_WIDTH=240` and `SCR_HEIGHT=320`.
- LVGL is configured via build flags with `LV_CONF_SKIP=1`, not a checked-in `lv_conf.h`.
- Enabled fonts are Montserrat 10, 12, 16, and 24. Avoid adding large fonts/glyph ranges casually.
- ESP32 Rev1-4 uses Arduino, LovyanGFX, `LV_TICK_CUSTOM=1`, and `LV_MEM_SIZE=(32U * 1024U)`.
- ESP32-S3 Rev5+ increases LVGL heap to `128U * 1024U` and configures `LV_MEM_POOL_ALLOC=ps_malloc`.
- Simulator builds use SDL/lv_drivers and `LV_MEM_SIZE=(64U * 1024U)` for 64-bit.

## Startup And Main Loop

- `src/main.cpp` registers devices, GUIs, scenes, then calls `init_gui()`.
- `init_gui()` calls `lv_init()` and `init_lvgl_hardware()`, creates global styles/widgets, then builds tabs through the memory optimizer.
- `gui_loop()` is called once during setup, then repeatedly from the main loop. It handles deferred tab recreation, calls `lv_timer_handler()`, and flushes cross-thread WiFi/BLE UI updates.

## Display And Touch HAL

- ESP32 display/touch glue lives in `hardware/ESP32/lvgl_hal_esp32.cpp`.
- It allocates one or two `SCR_WIDTH * SCR_HEIGHT / 10` draw buffers with `malloc`.
- `useTwoBuffersForlvgl` is enabled; flush uses LovyanGFX `pushPixelsDMA` and calls `lv_disp_flush_ready`.
- Touch input uses LovyanGFX `getTouch`, writes `LV_INDEV_STATE_PR/REL`, and updates the last activity timestamp.
- The desktop simulator has a separate SDL HAL in `hardware/windows_linux/lvgl_hal_windows_linux.cpp`.

## GUI Tab Pattern

- A GUI registers through `register_gui(name, create_tab_content, notify_tab_before_delete, ...)`.
- `create_tab_content_*` receives a tab parent and creates only children under that tab.
- `notify_tab_before_delete_*` is the hook to persist local state and null any external `lv_obj_t*` pointers before the tab is destroyed.
- Register new GUI headers in `src/main.cpp`, call `register_gui_*()`, and decide whether it belongs in `main_gui_list` or a scene-specific list.
- Existing examples: `src/guis/gui_numpad.cpp`, `src/guis/gui_sceneSelection.cpp`, `src/guis/gui_settings.cpp`, device-specific GUI files under `src/devices/**/gui_*.cpp`.

## Memory Optimizer

- `guiMemoryOptimizer.cpp` keeps at most three GUI tabs in memory: previous, active, next.
- After a tab slide, Omote deletes and recreates the tabview and page indicator rather than keeping all GUI pages alive.
- `notify_active_tabs_before_delete()` calls each GUI deletion hook before the tabview is deleted.
- `safe_delete_lv_obj()` checks null and `lv_obj_is_valid()` before deleting major objects.
- Reusable styles such as `panel_style` are initialized once in `init_gui()` to avoid leaks from repeated tab recreation.

## Cross-Thread UI Updates

- Do not call LVGL directly from BLE, WiFi, MQTT, IR, or hardware callbacks that may run outside the GUI loop.
- `showWiFiConnected()` stores a pending status under `mutex_guiBase`; `flushWiFiConnectedStatus()` applies it from `gui_loop()`.
- `addBLEmessage()` appends text under `mutex_gui_BLEpairing`; `flushBLEMessages()` applies it from `gui_loop()`.
- Use this same stash-and-flush model for new background updates.

## Scene/UI Deferral

- `gui_sceneSelection.cpp` sets the scene label to `changing...`, then uses a one-shot `lv_timer_create(..., 50, ...)` to run scene activation after the UI gets a chance to redraw.
- `guiBase.cpp` waits one `gui_loop()` cycle after tab slide animation completion before recreating tabs.
- Preserve these deferrals unless you have verified the repaint/lifecycle behavior in the simulator and on hardware.

---
name: omote-lvgl
description: LVGL 8.3 and Omote firmware GUI engineering guidance. Use when working in the Omote firmware on LVGL UI code, GUI tabs, scene navigation, status labels, touch/display HALs, LovyanGFX integration, PlatformIO LVGL configuration, memory optimization, simulator behavior, or when reviewing proposed LVGL performance/thread-safety changes. Tailored to Omote's Arduino/LovyanGFX LVGL 8.3.11 stack, not generic LVGL 9 or ESP-IDF esp_lvgl_port projects.
---

# Omote LVGL

## Core Rule

Treat Omote as a version-pinned LVGL 8.3 firmware with its own GUI lifecycle. Do not apply LVGL 9 APIs, ESP-IDF `esp_lvgl_port`, `esp_lvgl_adapter`, or BSP lock patterns unless the task is explicitly a migration.

## Start Here

1. Inspect `platformio.ini` first for the active environment, LVGL memory flags, screen size, fonts, and simulator/ESP32 differences.
2. For display or touch changes, read `hardware/ESP32/lvgl_hal_esp32.cpp`, `hardware/ESP32/tft_hal_esp32.*`, and `hardware/windows_linux/lvgl_hal_windows_linux.cpp`.
3. For GUI tabs or widgets, read `src/applicationInternal/gui/guiRegistry.*`, `src/applicationInternal/gui/guiBase.cpp`, `src/applicationInternal/gui/guiMemoryOptimizer.cpp`, and a nearby GUI such as `src/guis/gui_numpad.cpp`.
4. For status updates or callbacks from WiFi/BLE/hardware code, read `src/applicationInternal/gui/guiStatusUpdate.cpp`, `src/guis/gui_BLEpairing.cpp`, and `src/applicationInternal/hardware/hardwarePresenter.cpp`.

## References

- Read `references/omote-patterns.md` before implementing or reviewing Omote GUI changes.
- Read `references/lvgl-8-3-reference.md` when validating LVGL API behavior, lifecycle, timers, styles, threading, display buffers, or event semantics.
- Read `references/optimization-watchlist.md` when the task mentions performance, memory, display tearing, simulator CPU usage, future cleanup, or "should we optimize this?"

## Working Rules

- Keep LVGL calls on the main GUI path driven by `gui_loop()` unless the existing code already provides a safe handoff.
- For callbacks from other threads/tasks, store plain data under a C++ mutex and flush it from `gui_loop()`. Follow the WiFi and BLE message patterns instead of calling `lv_*` directly.
- When adding a GUI tab, implement `create_tab_content_*`, `notify_tab_before_delete_*`, `register_gui_*`, and register it in `src/main.cpp`. Null or invalidate any external `lv_obj_t*` references when the tab can be deleted.
- Keep reusable `lv_style_t` objects static/global and initialize them once. Avoid calling `lv_style_init` each time tabs are recreated.
- Avoid blocking work inside LVGL event callbacks or timers. Defer heavy scene/device work through Omote commands or short one-shot `lv_timer_t` callbacks where the UI must repaint first.
- Be careful around tabview deletion: Omote recreates the tabview after sliding animations and waits one `gui_loop()` cycle before doing the work.
- For HAL changes, preserve the LVGL 8.3 `lv_disp_drv_t`, `lv_disp_draw_buf_t`, and `lv_indev_drv_t` APIs unless deliberately migrating versions.

## Verification

Prefer the simulator for GUI iteration when available: `pio run -e linux_64bit` or the platform-specific simulator environment. For firmware-sensitive HAL or memory changes, also build the relevant ESP32 environment such as `esp32-Rev1toRev4` or `esp32-s3-Rev5andHigher`.

To see the rendered UI and inspect the live object tree (positions, state, text) instead of reasoning about it blind, use the companion `lvgl-sim-inspect` skill — it screenshots the simulator and dumps a DOM-like tree of the active screen.

# LVGL 8.3 Reference

Use these notes to check Omote LVGL assumptions against official LVGL 8.3 docs. Source links are included so version-sensitive claims can be audited.

## Sources

- LVGL 8.3 docs index and license: https://lvgl.io/docs/open/8.3/
- Display interface: https://lvgl.io/docs/open/8.3/porting/display.html
- Tick interface: https://lvgl.io/docs/open/8.3/porting/tick.html
- Timer handler: https://lvgl.io/docs/open/8.3/porting/timer-handler.html
- OS and interrupts: https://lvgl.io/docs/open/8.3/porting/os.html
- Styles: https://lvgl.io/docs/open/8.3/overview/style.html
- Objects: https://lvgl.io/docs/open/8.3/overview/object.html
- Timers: https://lvgl.io/docs/open/8.3/overview/timer.html
- Events: https://lvgl.io/docs/open/8.3/overview/event.html

## Version Guardrails

- Omote currently depends on `lvgl/lvgl@^8.3.11`.
- Keep API examples in LVGL 8 style: `lv_disp_drv_t`, `lv_disp_draw_buf_t`, `lv_indev_drv_t`, `lv_img_*`, `lv_scr_act()`.
- Do not replace with LVGL 9 names such as `lv_display_t`, `lv_image_*`, or changed display APIs unless doing a migration.

## Display Buffers

- LVGL draw buffers may be smaller than the screen; changed areas are redrawn in pieces.
- Official 8.3 guidance says buffers larger than about 1/10 of the screen usually do not add significant performance.
- With two draw buffers, LVGL can render into one while the other is transferred by DMA or display hardware.
- Always call `lv_disp_flush_ready(disp)` after the display driver has finished using the buffer. If a DMA API is asynchronous, signal ready from the transfer-complete path, not while DMA can still read the buffer.

## Tick And Timer Handler

- Call `lv_tick_inc(period_ms)` periodically with elapsed milliseconds. It should run at higher priority than `lv_timer_handler()` so elapsed time stays accurate.
- Call `lv_timer_handler()` periodically, roughly every 5 ms for responsive UI. Omote calls it from `gui_loop()`.
- LVGL timers run inside `lv_timer_handler()` and are non-preemptive, so LVGL API calls are allowed from LVGL timer callbacks. Keep callbacks short.

## Threads And Interrupts

- LVGL 8.3 is not safe for concurrent `lv_*` calls. In a true multitasking design, every LVGL call and `lv_timer_handler()` must share the same mutex.
- Avoid LVGL calls from interrupts except `lv_tick_inc()` and `lv_disp_flush_ready()`.
- Prefer setting flags or copying data in non-GUI contexts, then consume it from an LVGL timer or the main GUI loop.

## Styles And Object Lifetime

- `lv_style_t` variables must be static, global, or dynamically allocated; local stack styles become invalid after the function returns.
- Initialize styles with `lv_style_init(&style)` before use, and do it once for reusable styles.
- `lv_obj_del(obj)` deletes an object and all children immediately.
- Use `lv_obj_del_async(obj)` when deletion must wait until the next `lv_timer_handler()`, especially when deleting an object involved in an event callback.

## Events

- Prefer LVGL event helpers over struct-field access in new code: `lv_event_get_target(e)`, `lv_event_get_current_target(e)`, `lv_event_get_user_data(e)`, and `lv_event_get_param(e)`.
- If `LV_OBJ_FLAG_EVENT_BUBBLE` is enabled, events propagate to parents. In LVGL 8.3 docs, the target can differ from the current target; use the helper appropriate to the local pattern.

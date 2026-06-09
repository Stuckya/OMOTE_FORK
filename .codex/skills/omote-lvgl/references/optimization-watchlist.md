# Omote LVGL Optimization Watchlist

These are not automatic changes. Treat them as audit prompts when a task touches related code.

## Display Flush And DMA

- Audit whether LovyanGFX `pushPixelsDMA` is blocking in the current configuration. If it can return before DMA completes, `lv_disp_flush_ready()` should move to a transfer-complete path or the flush should explicitly wait.
- Ensure LVGL draw buffers used by DMA are allocated from DMA-capable memory on ESP32 targets. If changing allocation, preserve the ESP32/S3 memory differences in `platformio.ini`.
- Add null checks after draw-buffer allocation. A failed `malloc` currently has no graceful path.

## Memory And Object Count

- Page indicator creation creates many small objects every tab rebuild. Consider reducing object count, reusing objects, or rendering indicators as a compact image/canvas if memory pressure returns.
- Keep `LV_USE_ASSERT_STYLE=1` enabled for development. Consider temporarily enabling `LV_USE_ASSERT_MEM_INTEGRITY`, `LV_USE_ASSERT_OBJ`, or `LV_USE_MEM_MONITOR` when debugging corruption or leaks.
- Avoid adding new global `lv_obj_t*` references unless there is a clear update path and a deletion hook that nulls them.

## Simulator

- `hardware/windows_linux/lvgl_hal_windows_linux.cpp` has a tick thread that can spin continuously. If simulator CPU use matters, test adding a small `SDL_Delay(1)` or equivalent while preserving elapsed-time accuracy.
- Keep simulator and firmware HAL differences explicit; do not fix simulator behavior by changing ESP32 assumptions without hardware testing.

## Event And User Data Hygiene

- New code should prefer documented helpers such as `lv_event_get_user_data(e)` and `lv_obj_get_user_data(obj)` where available instead of direct `target->user_data` access.
- Keep event bubbling deliberate. It is useful in Omote button grids, but parent handlers must ignore container-originated clicks.

## Blocking Work

- Keep network, BLE, IR, scene start/end sequences, and storage work out of LVGL callbacks when possible.
- If UI needs to show immediate feedback before a heavy command, update the label and defer the command through a one-shot LVGL timer or an Omote command path.

## Larger Future Moves

- If migrating to LVGL 9, create a separate skill/reference. Do not blend LVGL 9 API names into the LVGL 8.3 Omote skill.
- If migrating away from Arduino/LovyanGFX toward ESP-IDF display components, revisit every HAL, lock, buffer, and tick rule from first principles.

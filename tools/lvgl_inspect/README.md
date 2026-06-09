# LVGL simulator inspector

DOM-like inspection of the OMOTE LVGL UI running in the desktop simulator. On
demand it dumps the active screen as a **PNG** plus a **JSON tree** of every
LVGL object (type, geometry, state, text) — the equivalent of a browser's
element inspector, for headless/tooling use.

## How it works

- Instrumentation lives in `hardware/windows_linux/lvgl_inspect_windows_linux.{h,cpp}`
  and is compiled into every desktop sim build (`linux_64bit`, `windows_*`,
  `macOS`). It is never built for ESP32 (that HAL dir isn't in the firmware
  source filter), so it adds nothing to device firmware.
- `lvgl_inspect_init()` (called once from `init_lvgl_HAL`) registers an
  `lv_timer` that polls a trigger file. The dump runs on the LVGL thread, which
  is required because LVGL 8.x is single-threaded.
- Requires `-D LV_USE_SNAPSHOT=1` (set on the `linux_64bit` base env). The
  full-screen snapshot buffer (~150 KB) is allocated from the **system heap**,
  not the small LVGL heap, via `lv_snapshot_take_to_buf`.

## Trigger protocol

All paths live under `/tmp/omote_inspect/`:

| File          | Meaning                                                        |
|---------------|----------------------------------------------------------------|
| `request`     | Touch to request a dump. Consumed (deleted) when handled.      |
| `screen.png`  | Snapshot of the active screen (240×320, RGB).                  |
| `tree.json`   | Object tree: `{type,x,y,w,h,state[],text,children[]}`.         |
| `done`        | Written after a dump completes: `{"png":bool,"tree":bool}`.    |

Coordinates are absolute screen pixels. Off-screen nodes (e.g. `x >= 240` or
negative) are adjacent tab pages parked off-screen by the tabview.

## Usage

Use the helper script — it guarantees the sim process is stopped, so iterations
never leak lingering simulators:

```sh
# One-shot: boot, dump, kill. Leaves no process behind. Best for a single look.
tools/lvgl_inspect/omote_sim_inspect.sh shot

# Iterative: keep a sim up across several dumps, then stop it.
tools/lvgl_inspect/omote_sim_inspect.sh start
tools/lvgl_inspect/omote_sim_inspect.sh dump      # repeat as needed
tools/lvgl_inspect/omote_sim_inspect.sh stop
```

Override the binary for non-macOS hosts:

```sh
SIM_BIN=.pio/build/linux_64bit/program tools/lvgl_inspect/omote_sim_inspect.sh shot
```

Manual trigger without the script (sim already running):

```sh
rm -f /tmp/omote_inspect/done
touch /tmp/omote_inspect/request
# wait for /tmp/omote_inspect/done, then read screen.png + tree.json
```

## Caveat: hub connection blocks startup

The desktop WebSocket client has no connect timeout. If `WEBSOCKET_HUB_URL`
(in `src/secrets.h`, override in `src/secrets_override.h`) points at an
unreachable host, the sim blocks during setup — before the UI loop runs — so no
dump can happen. Point it at a reachable hub, or at a host that refuses fast
(e.g. `ws://127.0.0.1:8765`) when you only need the UI. The helper script warns
and exits if `Setup finished` never appears.

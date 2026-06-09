---
name: lvgl-sim-inspect
description: Capture what the OMOTE LVGL UI actually looks like in the desktop simulator — a PNG screenshot plus a DOM-like JSON tree of every on-screen object (type, x/y/w/h, state, text). This is for the LVGL UI running on the OMOTE embedded simulator, NOT web/HTML/React DOM inspection. Use whenever you need to SEE or VERIFY the simulator UI: "screenshot the sim", "what's on the screen now", "did my GUI/LVGL change render", "is that switch/button in the right place or state", "show me the scene selection or settings screen", or getting element positions from the running sim. Also use it to close the build-and-verify loop: after generating or changing an LVGL screen — including building one from a mockup or design image — run the sim and confirm the render matches the intended layout. Prefer this over launching `.pio/build/.../program` by hand; it drives the sim through a runner that reliably tears the process down, so you don't leak lingering simulators.
---

# LVGL simulator inspector

This is your eyes on the running OMOTE simulator. On demand it produces two
artifacts that together act like a browser's element inspector for LVGL:

- `/tmp/omote_inspect/screen.png` — the active screen, rendered (240×320).
- `/tmp/omote_inspect/tree.json` — every LVGL object as
  `{type, x, y, w, h, state[], text, children[]}`.

The point of the skill is to get those two files and then **read both** — the
PNG tells you how it looks, the tree tells you exactly what's there and where.

## How to run it

Always go through the helper script. It exists for one reason that matters: the
simulator is a long-lived GUI process, and a backgrounded `program &` that you
forget about survives the shell call that started it. Do that a few times and
you've got a pile of orphaned simulators to force-quit. The script tracks a PID
and sweeps any stray of the same binary, so start/stop is deterministic.

**One-shot — the default.** Boots the sim, waits for its UI loop, dumps, and
kills it. Leaves nothing behind:

```sh
tools/lvgl_inspect/omote_sim_inspect.sh shot
```

It prints the two output paths. Read them:

```
/tmp/omote_inspect/screen.png   →  Read tool (renders the image)
/tmp/omote_inspect/tree.json    →  Read tool (the object tree)
```

**Iterative — several looks in a row.** When you want to inspect, change
something, and look again without paying the boot cost each time, keep one sim
up and finish with `stop` so it doesn't linger:

```sh
tools/lvgl_inspect/omote_sim_inspect.sh start
tools/lvgl_inspect/omote_sim_inspect.sh dump     # repeat between actions
tools/lvgl_inspect/omote_sim_inspect.sh stop     # don't skip this
```

`status` reports whether a sim is currently running.

## Prerequisite: a built sim

The inspector is compiled into every desktop sim build (it needs
`LV_USE_SNAPSHOT=1`, already set on the `linux_64bit` base env). If the binary
is missing, build it first:

```sh
pio run -e macOS          # or linux_64bit / windows_64bit
```

On non-macOS hosts, point the script at the right binary:

```sh
SIM_BIN=.pio/build/linux_64bit/program tools/lvgl_inspect/omote_sim_inspect.sh shot
```

## Reading the tree

Coordinates are absolute screen pixels, so they map straight onto the PNG.

- **Off-screen nodes are normal.** Entries with `x >= 240` or negative `x` are
  adjacent tab pages parked off-screen by the tabview (e.g. the Settings page
  sitting at `x:240+` while Scene selection is visible). They're real content,
  not bugs — useful when you want to inspect a page that isn't currently shown.
- **`state`** lists active LVGL states: `checked`, `pressed`, `focused`,
  `disabled`, `hovered`, `edited`, `scrolled`. This is how you answer "is the
  Lift-to-Wake switch on?" — look for `"state":["checked"]` on that `switch`.
- **`text`** is populated for `label`, `checkbox`, and `textarea` nodes. A
  button's caption lives in its child `label`, so look one level down.

**Example** — confirming a toggle is on and finding where it is:

```json
{"type":"switch","x":414,"y":104,"w":40,"h":22,"state":["checked"]}
```

That's a 40×22 switch at (414,104) in the on state — and the `x:414` tells you
it's on an off-screen page, so you'd need to navigate there to see it lit.

## If it hangs at startup

The desktop WebSocket client has no connect timeout. If `WEBSOCKET_HUB_URL`
(`src/secrets.h`, overridable in `src/secrets_override.h`) points at a host that
doesn't answer, the sim blocks during setup — before the UI loop runs — so no
dump can happen. The script detects this (no `Setup finished`), warns, and
exits. Fix it by pointing the URL at a reachable hub, or at a host that refuses
the connection instantly (`ws://127.0.0.1:8765`) when you only need the UI.

## Under the hood (for maintainers)

- Instrumentation: `hardware/windows_linux/lvgl_inspect_windows_linux.{h,cpp}`,
  built only for desktop sims (the ESP32 source filter excludes this dir, so it
  adds nothing to firmware). `lvgl_inspect_init()` is called once from
  `init_lvgl_HAL`.
- The dump runs inside an `lv_timer`, i.e. on the LVGL thread, because LVGL 8.x
  is single-threaded — walking the object tree or snapshotting from another
  thread would race the renderer.
- The snapshot buffer (~150 KB) is taken into the **system heap** via
  `lv_snapshot_take_to_buf`; `lv_snapshot_take` would try the ~64 KB LVGL heap,
  fail, and assert-hang.
- Trigger protocol (all under `/tmp/omote_inspect/`): touch `request` to ask for
  a dump; `done` (`{"png":bool,"tree":bool}`) is written when finished. Full
  detail in `tools/lvgl_inspect/README.md`.

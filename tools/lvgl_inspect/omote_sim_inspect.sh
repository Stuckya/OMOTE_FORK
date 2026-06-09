#!/usr/bin/env bash
#
# Lifecycle + inspection helper for the OMOTE LVGL desktop simulator.
#
# The simulator UI inspector (compiled into the windows_linux/macOS sim) watches
# a trigger file and dumps the active LVGL screen as a PNG plus a DOM-like JSON
# tree. This script drives that, and — importantly — guarantees the sim process
# is reliably stopped so iterations don't leak lingering simulators.
#
# Commands:
#   start     Stop any existing sim, launch a fresh one, wait until its UI loop
#             is running, then return (sim keeps running in the background).
#   dump      Trigger a snapshot and wait for it; prints the output paths.
#   stop      Stop the sim (tracked PID + sweep any stray of this binary).
#   restart   stop + start.
#   shot      One-shot: start, dump, stop. Leaves no process behind. Recommended
#             for single inspections.
#   status    Show whether a sim is running.
#
# Env overrides:
#   SIM_BIN          path to the built simulator (default: .pio/build/macOS/program)
#   SIM_SETUP_WAIT   seconds to wait for "Setup finished" (default: 20)
#
set -euo pipefail

SIM_BIN="${SIM_BIN:-.pio/build/macOS/program}"
SIM_SETUP_WAIT="${SIM_SETUP_WAIT:-20}"

INSPECT_DIR="/tmp/omote_inspect"
PIDFILE="$INSPECT_DIR/sim.pid"
LOGFILE="$INSPECT_DIR/sim.log"
REQUEST="$INSPECT_DIR/request"
DONE="$INSPECT_DIR/done"
SCREEN_PNG="$INSPECT_DIR/screen.png"
TREE_JSON="$INSPECT_DIR/tree.json"

log() { printf '[sim] %s\n' "$*" >&2; }

sweep() {
  # Belt-and-suspenders: kill any process running this exact binary, not just
  # the PID we tracked, so a leaked sim from a crashed run is also cleaned up.
  pkill -f "$SIM_BIN" 2>/dev/null || true
  sleep 0.3
  pkill -9 -f "$SIM_BIN" 2>/dev/null || true
}

cmd_stop() {
  if [[ -f "$PIDFILE" ]]; then
    local pid
    pid="$(cat "$PIDFILE" 2>/dev/null || true)"
    if [[ -n "${pid:-}" ]]; then
      kill "$pid" 2>/dev/null || true
      for _ in $(seq 1 10); do kill -0 "$pid" 2>/dev/null || break; sleep 0.1; done
      kill -9 "$pid" 2>/dev/null || true
    fi
    rm -f "$PIDFILE"
  fi
  sweep
  log "stopped"
}

cmd_start() {
  cmd_stop
  mkdir -p "$INSPECT_DIR"
  if [[ ! -x "$SIM_BIN" ]]; then
    log "ERROR: simulator binary not found/executable: $SIM_BIN"
    log "build it first, e.g.: pio run -e macOS"
    exit 1
  fi
  rm -f "$DONE"
  "$SIM_BIN" >"$LOGFILE" 2>&1 &
  echo $! >"$PIDFILE"
  log "launched pid=$(cat "$PIDFILE"), waiting for UI loop (up to ${SIM_SETUP_WAIT}s)"

  local deadline=$((SIM_SETUP_WAIT * 10))
  for _ in $(seq 1 "$deadline"); do
    if grep -q "Setup finished" "$LOGFILE" 2>/dev/null; then
      log "UI loop running"
      return 0
    fi
    if ! kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
      log "ERROR: simulator exited during startup; see $LOGFILE"
      exit 1
    fi
    sleep 0.1
  done
  log "WARNING: 'Setup finished' not seen in ${SIM_SETUP_WAIT}s."
  log "The sim likely blocked connecting to the hub: the desktop WebSocket client"
  log "has no connect timeout, so an unreachable WEBSOCKET_HUB_URL freezes startup"
  log "before the UI loop runs. Point it at a reachable/refusing host (see README)."
  exit 1
}

cmd_dump() {
  if [[ ! -f "$PIDFILE" ]] || ! kill -0 "$(cat "$PIDFILE" 2>/dev/null)" 2>/dev/null; then
    log "ERROR: no running sim (start one first, or use 'shot')"
    exit 1
  fi
  rm -f "$DONE"
  : >"$REQUEST"
  for _ in $(seq 1 50); do [[ -f "$DONE" ]] && break; sleep 0.1; done
  if [[ ! -f "$DONE" ]]; then
    log "ERROR: dump timed out (no done marker)"
    exit 1
  fi
  log "dump: $(cat "$DONE")"
  printf '%s\n%s\n' "$SCREEN_PNG" "$TREE_JSON"
}

cmd_status() {
  if [[ -f "$PIDFILE" ]] && kill -0 "$(cat "$PIDFILE" 2>/dev/null)" 2>/dev/null; then
    log "running pid=$(cat "$PIDFILE")"
  else
    log "not running"
  fi
}

cmd_shot() {
  cmd_start
  cmd_dump
  cmd_stop
}

case "${1:-}" in
  start)   cmd_start ;;
  dump)    cmd_dump ;;
  stop)    cmd_stop ;;
  restart) cmd_stop; cmd_start ;;
  shot)    cmd_shot ;;
  status)  cmd_status ;;
  *) echo "usage: $0 {start|dump|stop|restart|shot|status}" >&2; exit 2 ;;
esac

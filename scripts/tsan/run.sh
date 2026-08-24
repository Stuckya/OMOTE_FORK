#!/usr/bin/env bash
# Runs the HalCallback race check under ThreadSanitizer. Requires clang; not part
# of CI, because the native test environment also builds under MinGW where TSan
# is unavailable. Exits non-zero if a data race is reported.
set -uo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
root="$here/../.."
out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

clang++ -fsanitize=thread -std=c++11 -g -O1 \
  -I "$root/lib/HalCallback/src" \
  "$here/hal_callback_race.cpp" -o "$out/race" || exit 1

report="$("$out/race" 2>&1)"
echo "$report"
if grep -q "ThreadSanitizer: data race" <<<"$report"; then
  echo "FAIL: data race reported"
  exit 1
fi
echo "OK: no data race reported"

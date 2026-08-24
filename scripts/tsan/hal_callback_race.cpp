// Manual ThreadSanitizer check for HalCallback.
//
// Reproduces the production shape: one thread re-registers the callback --
// init_mqtt() runs from setup() and again when the MQTT transport initialises,
// re-registering the WiFi handler each time -- while another delivers events,
// as the arduino_events task does once WiFi is up.
//
// Against a plain function pointer TSan reports a data race here. It is the only
// tool that can show that deterministically, and it cannot run in CI because the
// native test environment also builds under MinGW.
#include "halCallback.h"

#include <atomic>
#include <cstdio>
#include <thread>

static HalCallback<bool> callback;
static std::atomic<bool> stop(false);
static std::atomic<int> seen(0);

static void handler(bool value) {
  if (value) {
    seen.fetch_add(1, std::memory_order_relaxed);
  }
}

int main() {
  std::thread registrar([] {
    for (int i = 0; i < 200000 && !stop.load(); i++) {
      callback.set(&handler);
    }
  });
  std::thread events([] {
    for (int i = 0; i < 200000 && !stop.load(); i++) {
      callback(true);
    }
  });
  registrar.join();
  stop.store(true);
  events.join();
  printf("completed, handler saw %d invocations\n", seen.load());
  return 0;
}

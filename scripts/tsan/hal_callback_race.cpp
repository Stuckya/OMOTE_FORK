// Exercises concurrent callback registration and delivery under ThreadSanitizer.
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

#pragma once

#include <cstddef>

// A HAL callback slot that is safe to invoke before anything registers.
//
// HAL events are not synchronised with application setup: WiFi association can
// complete, or a radio frame can land, before the application has installed its
// handler. Raw function pointers made that a null dereference, and the guard was
// left to each call site to remember -- which it did not, consistently.
//
// Invoking an unset slot drops the event rather than crashing. That is the right
// trade here: the events are notifications the application has not yet asked to
// hear about, and a missed one is recoverable where a panic is not.
template <typename... Args>
class HalCallback {
public:
  typedef void (*Target)(Args...);

  void set(Target target) { this->target = target; }
  void clear() { target = nullptr; }
  bool isSet() const { return target != nullptr; }

  void operator()(Args... args) const {
    if (target != nullptr) {
      target(args...);
    }
  }

private:
  Target target = nullptr;
};

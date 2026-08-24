#pragma once

#include <atomic>
#include <cstddef>
#include <utility>

// A HAL callback slot that is safe to invoke before anything registers, and
// safe to re-register while events are being delivered.
//
// HAL events are not synchronised with application setup: WiFi association can
// complete, or a radio frame can land, before the application has installed its
// handler. Raw function pointers made that a null dereference, and the guard was
// left to each call site to remember -- which it did not, consistently.
//
// Registration genuinely overlaps delivery: init_mqtt() runs once from setup()
// and again when the MQTT transport initialises, and it re-registers the WiFi
// handler each time while WiFi events are already arriving on their own task.
// The target is therefore atomic, and invocation reads it exactly once so the
// pointer it checked is the pointer it calls.
//
// Invoking an unset slot drops the event. These are notifications the
// application has not yet asked to hear about, and a missed one is recoverable
// where a panic is not.
template <typename... Args>
class HalCallback {
public:
  typedef void (*Target)(Args...);

  void set(Target target) { target_.store(target, std::memory_order_release); }
  void clear() { target_.store(nullptr, std::memory_order_release); }

  bool isSet() const { return target_.load(std::memory_order_acquire) != nullptr; }

  // Arguments are forwarded rather than taken by value: the HAL callbacks carry
  // std::string, and a by-value hop copies every one of them on the way through.
  template <typename... CallArgs>
  void operator()(CallArgs&&... args) const {
    static_assert(sizeof...(CallArgs) == sizeof...(Args),
                  "HalCallback invoked with the wrong number of arguments");

    const Target target = target_.load(std::memory_order_acquire);
    if (target != nullptr) {
      target(std::forward<CallArgs>(args)...);
    }
  }

private:
  std::atomic<Target> target_{nullptr};
};

#pragma once

#include <atomic>
#include <cstddef>
#include <utility>

// Registration may overlap HAL event delivery; invoking an unset slot drops the event.
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

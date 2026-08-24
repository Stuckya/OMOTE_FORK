#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

// Hands ESP-NOW frames from the radio thread to the main loop.
//
// The ESP-NOW driver invokes its receive callback on the WiFi task, so anything
// that callback reaches runs off the main thread. LVGL is not thread safe, and
// the hub message path ends in LVGL calls, so frames are parked here and
// replayed from espnow_loop() instead of being dispatched where they land.
//
// Single producer (radio thread), single consumer (main loop): the producer
// owns head, the consumer owns tail, and neither writes the other's index.
// That is what makes the lock-free handoff safe without a mutex, which the
// receive callback must not block on.
class EspNowRxQueue {
public:
  // ESP-NOW v1 caps a payload at 250 bytes; a longer frame cannot be genuine.
  static const size_t MAX_FRAME_BYTES = 250;

  // One slot is always left empty to keep the full and empty states distinct,
  // so this holds CAPACITY - 1 frames.
  static const size_t CAPACITY = 9;

  struct Frame {
    uint8_t bytes[MAX_FRAME_BYTES];
    size_t length;
  };

  // Radio thread. Never blocks or allocates. Returns false when the frame is
  // malformed or the queue is full, in which case the frame is dropped.
  bool push(const uint8_t* data, size_t length);

  // Main loop. Copies the oldest frame into out and releases its slot.
  bool pop(Frame& out);

  size_t count() const;
  bool isEmpty() const;

  // Frames lost because the main loop did not drain in time, or because they
  // exceeded MAX_FRAME_BYTES. Non-zero means the link is outrunning the loop.
  uint32_t droppedFrames() const;

private:
  Frame frames[CAPACITY];
  std::atomic<size_t> head{0};
  std::atomic<size_t> tail{0};
  std::atomic<uint32_t> dropped{0};
};

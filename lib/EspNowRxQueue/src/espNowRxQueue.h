#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

// SPSC queue: the WiFi task owns head; the main loop owns tail.
class EspNowRxQueue {
public:
  // ESP-NOW v1 caps a payload at 250 bytes; a longer frame cannot be genuine.
  static const size_t MAX_FRAME_BYTES = 250;

  // One slot stays empty to distinguish full from empty.
  static const size_t CAPACITY = 9;

  struct Frame {
    uint8_t bytes[MAX_FRAME_BYTES];
    size_t length;
  };

  // WiFi task only; rejects invalid frames and a full queue without blocking.
  bool push(const uint8_t* data, size_t length);

  bool pop(Frame& out);

  size_t count() const;
  bool isEmpty() const;

  // Counts malformed, oversized, and full-queue drops.
  uint32_t droppedFrames() const;

private:
  Frame frames[CAPACITY];
  std::atomic<size_t> head{0};
  std::atomic<size_t> tail{0};
  std::atomic<uint32_t> dropped{0};
};

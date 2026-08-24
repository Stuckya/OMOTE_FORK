#include "espNowRxQueue.h"

#include <cstring>

namespace {
size_t advance(size_t index) {
  return (index + 1) % EspNowRxQueue::CAPACITY;
}
}  // namespace

bool EspNowRxQueue::push(const uint8_t* data, size_t length) {
  if (data == nullptr || length == 0 || length > MAX_FRAME_BYTES) {
    dropped.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  const size_t writeAt = head.load(std::memory_order_relaxed);
  const size_t next = advance(writeAt);
  if (next == tail.load(std::memory_order_acquire)) {
    // Only the consumer may advance tail, so a full queue drops the newest frame.
    dropped.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  memcpy(frames[writeAt].bytes, data, length);
  frames[writeAt].length = length;

  // Publishes the frame contents written above to the consumer.
  head.store(next, std::memory_order_release);
  return true;
}

bool EspNowRxQueue::pop(Frame& out) {
  const size_t readAt = tail.load(std::memory_order_relaxed);
  if (readAt == head.load(std::memory_order_acquire)) {
    return false;
  }

  out.length = frames[readAt].length;
  memcpy(out.bytes, frames[readAt].bytes, out.length);

  // Prevent producer reuse until the frame copy completes.
  tail.store(advance(readAt), std::memory_order_release);
  return true;
}

size_t EspNowRxQueue::count() const {
  const size_t writeAt = head.load(std::memory_order_acquire);
  const size_t readAt = tail.load(std::memory_order_acquire);
  return (writeAt + CAPACITY - readAt) % CAPACITY;
}

bool EspNowRxQueue::isEmpty() const {
  return count() == 0;
}

uint32_t EspNowRxQueue::droppedFrames() const {
  return dropped.load(std::memory_order_relaxed);
}

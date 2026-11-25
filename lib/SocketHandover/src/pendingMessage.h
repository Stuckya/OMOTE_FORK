#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>

// While a background task connects the socket, the main loop leaves one message here, and that
// task sends it the moment the socket opens. Single use: the socket is lent once per boot.
class PendingMessage {
public:
  static const size_t MAX_BYTES = 1024;

  // Main loop. The caller keeps its own copy and asks wasSent() once it holds the socket again.
  bool set(const uint8_t* data, size_t len) {
    State empty = State::Empty;
    if (len > MAX_BYTES || !state_.compare_exchange_strong(empty, State::Filling)) {
      return false;
    }
    memcpy(bytes_, data, len);
    length_ = len;
    state_.store(State::Waiting);
    return true;
  }

  // Connecting task, before it gives the socket back. A message still being copied in stays unsent.
  template <typename Send>
  void sendIfWaiting(Send send) {
    State waiting = State::Waiting;
    if (!state_.compare_exchange_strong(waiting, State::Sending)) {
      return;
    }
    state_.store(send(bytes_, length_) ? State::Sent : State::NotSent);
  }

  bool wasSent() const { return state_.load() == State::Sent; }

private:
  enum class State { Empty, Filling, Waiting, Sending, Sent, NotSent };

  std::atomic<State> state_{State::Empty};
  uint8_t bytes_[MAX_BYTES];
  size_t length_ = 0;
};

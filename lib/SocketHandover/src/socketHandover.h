#pragma once

#include <atomic>

// A socket client is not safe to drive from two tasks. A background task may connect it during
// startup, then the main loop drives it. It changes hands once in each direction, never again.
class SocketHandover {
public:
  bool lendToBackground() {
    Holder neverLent = Holder::MainLoopBeforeLending;
    return holder_.compare_exchange_strong(neverLent, Holder::Background);
  }

  bool mainLoopHoldsSocket() const { return holder_.load() != Holder::Background; }

  void askForSocketBack() { askedBack_.store(true); }

  bool backgroundShouldKeepConnecting(bool connected) const {
    return !connected && !askedBack_.load();
  }

  void giveBack() { holder_.store(Holder::MainLoopAfterLending); }

private:
  enum class Holder { MainLoopBeforeLending, Background, MainLoopAfterLending };

  std::atomic<Holder> holder_{Holder::MainLoopBeforeLending};
  std::atomic<bool> askedBack_{false};
};

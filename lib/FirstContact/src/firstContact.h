#pragma once

// An access point can acknowledge and then drop what a station sends in the first moments after it
// joins. An address request is cheap to repeat, and its answer shows that frames now get through.
class FirstContact {
public:
  enum class Step { Wait, AskForAddress, Connect };

  // Poll from the moment WiFi is joined. Time counts from the first poll.
  Step next(unsigned long nowMs, bool addressKnown) {
    if (!joined_) {
      joined_ = true;
      joinedAtMs_ = nowMs;
      nextAskAtMs_ = nowMs;
    }
    const unsigned long sinceJoinMs = nowMs - joinedAtMs_;
    if (addressKnown || sinceJoinMs >= GIVE_UP_AFTER_MS) {
      return Step::Connect;
    }
    if (nowMs < nextAskAtMs_) {
      return Step::Wait;
    }
    if (sinceJoinMs >= STEADY_ASKS_FOR_MS) {
      gapMs_ += gapMs_ / 2;
    }
    nextAskAtMs_ = nowMs + gapMs_;
    return Step::AskForAddress;
  }

private:
  // Measured on one access point: the drop was over within 12 ms on 96 joins in 100, and lasted past 90 ms once in 245.
  static const unsigned long STEADY_ASKS_FOR_MS = 16;
  // The network stack repeats an unanswered request by itself about once a second.
  static const unsigned long GIVE_UP_AFTER_MS = 1000;

  bool joined_ = false;
  unsigned long joinedAtMs_ = 0;
  unsigned long nextAskAtMs_ = 0;
  unsigned long gapMs_ = 4;
};

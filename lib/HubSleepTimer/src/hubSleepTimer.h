#pragma once

#include <cstdint>
#include <string>

// The remote's view of the hub's sleep timer. The hub owns the countdown --
// the OMOTE deep-sleeps and cannot keep one -- so every value here comes from a
// pushed status; the local tick only keeps the display moving between them.
class HubSleepTimer {
public:
  // As the hub reports it. FIRED is momentary and leaves no timer behind, so
  // stage() never returns it.
  enum class Stage { IDLE, ARMED, WARNING, FIRED };

  static const uint16_t MIN_MINUTES = 15;
  static const uint16_t MAX_MINUTES = 180;
  static const uint16_t STEP_MINUTES = 15;
  static const uint16_t DEFAULT_MINUTES = 15;

  void observe(Stage pushed, uint32_t remainingSeconds, unsigned long now);

  Stage stage() const;
  bool isArmed() const;
  // Still inside the final minute. Asked again when a deferred warning is
  // replayed, because by then the one-shot edge is spent and the timer may
  // have been cancelled or extended out of the window.
  bool isWarning() const;
  uint32_t remainingSeconds(unsigned long now) const;

  // "42m" for the status bar, "41:32" for the sheet -- the same minute, because
  // the indicator rounds up.
  std::string indicatorText(unsigned long now) const;
  std::string countdownText(unsigned long now) const;

  // Consume-once edges: the banner shows on the crossing, not on every push,
  // and the power session opens once when the hub fires.
  bool takeWarning();
  bool takeFired();

  void reset();

  static uint16_t snapMinutes(int minutes);

private:
  Stage currentStage = Stage::IDLE;
  uint32_t observedRemaining = 0;
  unsigned long observedAt = 0;
  bool warningPending = false;
  bool firedPending = false;
};

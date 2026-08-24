#include "hubSleepTimer.h"

#include <cstdio>

void HubSleepTimer::observe(Stage pushed, uint32_t remainingSeconds, unsigned long now) {
  if (pushed == Stage::FIRED) {
    firedPending = true;
    reset();
    return;
  }
  // A wake loses the previous stage with the RAM, so a first sighting inside
  // the final minute is a crossing too.
  if (pushed == Stage::WARNING && currentStage != Stage::WARNING) {
    warningPending = true;
  }
  currentStage = pushed;
  observedRemaining = (pushed == Stage::IDLE) ? 0 : remainingSeconds;
  observedAt = now;
}

HubSleepTimer::Stage HubSleepTimer::stage() const { return currentStage; }

bool HubSleepTimer::isArmed() const {
  return currentStage == Stage::ARMED || currentStage == Stage::WARNING;
}

bool HubSleepTimer::isWarning() const { return currentStage == Stage::WARNING; }

uint32_t HubSleepTimer::remainingSeconds(unsigned long now) const {
  if (!isArmed()) return 0;
  const unsigned long elapsed = (now - observedAt) / 1000;
  if (elapsed >= observedRemaining) return 0;
  return observedRemaining - (uint32_t)elapsed;
}

std::string HubSleepTimer::indicatorText(unsigned long now) const {
  const uint32_t left = remainingSeconds(now);
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%um", (unsigned)((left + 59) / 60));
  return std::string(buffer);
}

std::string HubSleepTimer::countdownText(unsigned long now) const {
  const uint32_t left = remainingSeconds(now);
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%02u:%02u", (unsigned)(left / 60),
           (unsigned)(left % 60));
  return std::string(buffer);
}

bool HubSleepTimer::takeWarning() {
  const bool pending = warningPending;
  warningPending = false;
  return pending;
}

bool HubSleepTimer::takeFired() {
  const bool pending = firedPending;
  firedPending = false;
  return pending;
}

void HubSleepTimer::reset() {
  currentStage = Stage::IDLE;
  observedRemaining = 0;
  observedAt = 0;
  warningPending = false;
}

uint16_t HubSleepTimer::snapMinutes(int minutes) {
  if (minutes <= (int)MIN_MINUTES) return MIN_MINUTES;
  if (minutes >= (int)MAX_MINUTES) return MAX_MINUTES;
  const int steps = (minutes + STEP_MINUTES / 2) / STEP_MINUTES;
  const uint16_t snapped = (uint16_t)(steps * STEP_MINUTES);
  if (snapped < MIN_MINUTES) return MIN_MINUTES;
  if (snapped > MAX_MINUTES) return MAX_MINUTES;
  return snapped;
}

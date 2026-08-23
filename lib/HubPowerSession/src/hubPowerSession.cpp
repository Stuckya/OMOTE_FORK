#include "hubPowerSession.h"

void HubPowerSession::expect(const std::string& id, const std::string& name, bool targetOn,
                             bool alreadyThere, unsigned long now) {
  if (currentPhase != Phase::IN_PROGRESS) {
    reset();
    currentPhase = Phase::IN_PROGRESS;
    openedAt = now;
  }
  if (indexOf(id) >= 0 || count >= CAPACITY) {
    return;
  }

  Device& device = devices[count++];
  device.id = id;
  device.name = name;
  device.targetOn = targetOn;
  device.outcome = alreadyThere ? Outcome::CONFIRMED : Outcome::PENDING;
  settleIfComplete();
}

bool HubPowerSession::observe(const std::string& id, bool isOn) {
  if (currentPhase == Phase::IDLE) {
    return false;
  }
  const int index = indexOf(id);
  if (index < 0) {
    return false;
  }

  Device& device = devices[index];
  if (device.outcome == Outcome::CONFIRMED || isOn != device.targetOn) {
    return false;
  }
  device.outcome = Outcome::CONFIRMED;
  settleIfComplete();
  return true;
}

void HubPowerSession::noteError(const std::string& message) {
  errorMessage = message;
}

HubPowerSession::Phase HubPowerSession::tick(unsigned long now) {
  if (currentPhase != Phase::IN_PROGRESS || now - openedAt < TIMEOUT_MS) {
    return currentPhase;
  }
  for (size_t i = 0; i < count; i++) {
    if (devices[i].outcome == Outcome::PENDING) {
      devices[i].outcome = Outcome::FAILED;
    }
  }
  currentPhase = Phase::TIMED_OUT;
  return currentPhase;
}

HubPowerSession::Phase HubPowerSession::phase() const {
  return currentPhase;
}

HubPowerSession::Tone HubPowerSession::tone() const {
  switch (currentPhase) {
    case Phase::RESOLVED:
      return Tone::OK;
    case Phase::TIMED_OUT:
      return Tone::WARN;
    default:
      return errorMessage.empty() ? Tone::BUSY : Tone::WARN;
  }
}

std::string HubPowerSession::headline() const {
  const char* state = targetOn() ? "on" : "off";
  switch (currentPhase) {
    case Phase::IDLE:
      return "";
    case Phase::IN_PROGRESS:
      if (!errorMessage.empty()) {
        return errorMessage;
      }
      return std::string("Powering ") + state + "...";
    case Phase::RESOLVED:
      if (count == 1) {
        return devices[0].name + " " + state;
      }
      return std::string("All devices ") + state;
    case Phase::TIMED_OUT: {
      const size_t failed = countWith(Outcome::FAILED);
      if (failed == 1) {
        for (size_t i = 0; i < count; i++) {
          if (devices[i].outcome == Outcome::FAILED) {
            return devices[i].name + " no reply";
          }
        }
      }
      return std::to_string(failed) + " devices no reply";
    }
  }
  return "";
}

bool HubPowerSession::targetOn() const {
  return count > 0 && devices[0].targetOn;
}

size_t HubPowerSession::size() const {
  return count;
}

const HubPowerSession::Device& HubPowerSession::at(size_t index) const {
  return devices[index];
}

HubPowerSession::Outcome HubPowerSession::slotOutcome(size_t index) const {
  const size_t confirmed = countWith(Outcome::CONFIRMED);
  if (index < confirmed) {
    return Outcome::CONFIRMED;
  }
  if (index < confirmed + countWith(Outcome::FAILED)) {
    return Outcome::FAILED;
  }
  return Outcome::PENDING;
}

void HubPowerSession::reset() {
  count = 0;
  currentPhase = Phase::IDLE;
  openedAt = 0;
  errorMessage.clear();
}

int HubPowerSession::indexOf(const std::string& id) const {
  for (size_t i = 0; i < count; i++) {
    if (devices[i].id == id) {
      return (int)i;
    }
  }
  return -1;
}

size_t HubPowerSession::countWith(Outcome outcome) const {
  size_t matches = 0;
  for (size_t i = 0; i < count; i++) {
    if (devices[i].outcome == outcome) {
      matches++;
    }
  }
  return matches;
}

void HubPowerSession::settleIfComplete() {
  if (countWith(Outcome::CONFIRMED) == count) {
    currentPhase = Phase::RESOLVED;
  }
}

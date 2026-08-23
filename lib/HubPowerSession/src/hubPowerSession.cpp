#include "hubPowerSession.h"

void HubPowerSession::expect(const std::string&, const std::string&, bool, bool, unsigned long) {}

bool HubPowerSession::observe(const std::string&, bool) {
  return false;
}

void HubPowerSession::noteError(const std::string&) {}

HubPowerSession::Phase HubPowerSession::tick(unsigned long) {
  return currentPhase;
}

HubPowerSession::Phase HubPowerSession::phase() const {
  return currentPhase;
}

HubPowerSession::Tone HubPowerSession::tone() const {
  return Tone::BUSY;
}

std::string HubPowerSession::headline() const {
  return "";
}

bool HubPowerSession::targetOn() const {
  return false;
}

size_t HubPowerSession::size() const {
  return count;
}

const HubPowerSession::Device& HubPowerSession::at(size_t index) const {
  return devices[index];
}

void HubPowerSession::reset() {}

int HubPowerSession::indexOf(const std::string&) const {
  return -1;
}

size_t HubPowerSession::countWith(Outcome) const {
  return 0;
}

void HubPowerSession::settleIfComplete() {}

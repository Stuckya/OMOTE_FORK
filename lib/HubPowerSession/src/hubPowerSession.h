#pragma once

#include <cstddef>
#include <string>

// One scene power session: the devices a scene asked to change power, in the
// order asked, and what the hub has observed about each since. Confirmation
// comes from observed device state, never from the anonymous command reply.
class HubPowerSession {
public:
  static const size_t CAPACITY = 8;
  static const unsigned long TIMEOUT_MS = 5000;

  enum class Phase { IDLE, IN_PROGRESS, RESOLVED, TIMED_OUT };
  enum class Outcome { PENDING, CONFIRMED, FAILED };
  enum class Tone { BUSY, OK, WARN };

  struct Device {
    std::string id;
    std::string name;
    bool targetOn = false;
    Outcome outcome = Outcome::PENDING;
  };

  // Opens a session when none is in progress, extends one that is. A device
  // already in its target state is confirmed on arrival. Duplicate ids and
  // devices past CAPACITY are ignored.
  void expect(const std::string& id, const std::string& name, bool targetOn,
              bool alreadyThere, unsigned long now);

  // Observed power for a device; returns true when it changed the session.
  bool observe(const std::string& id, bool isOn);

  // A hub error that names no device; replaces the headline while in progress.
  void noteError(const std::string& message);

  // Applies the session timeout: pending devices become FAILED.
  Phase tick(unsigned long now);

  Phase phase() const;
  Tone tone() const;
  std::string headline() const;
  bool targetOn() const;
  size_t size() const;
  const Device& at(size_t index) const;
  void reset();

private:
  int indexOf(const std::string& id) const;
  size_t countWith(Outcome outcome) const;
  void settleIfComplete();

  Device devices[CAPACITY];
  size_t count = 0;
  Phase currentPhase = Phase::IDLE;
  unsigned long openedAt = 0;
  std::string errorMessage;
};

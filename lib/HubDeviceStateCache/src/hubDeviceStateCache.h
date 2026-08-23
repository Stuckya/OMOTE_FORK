#pragma once

#include "remote_messages.pb.h"
#include <cstddef>
#include <string>

// Bounded, allocation-free cache of the latest per-device snapshots from the hub.
// Sized to StateSync.devices[8]; a DeviceState is a full snapshot, so merging an
// existing device replaces the whole record (clearing stale optionals).
class HubDeviceStateCache {
public:
  static const size_t CAPACITY = 8;

  // Returns true if the snapshot was inserted or updated; false if ignored
  // (empty device_id) or dropped (cache full and device unknown).
  bool merge(const omote_DeviceState& state);

  // True when a device whose volume is already cached reports a different
  // level or mute. A first sighting is the silent wake fill, not a change.
  bool volumeChangedBy(const omote_DeviceState& incoming) const;

  // Record a level the remote already displayed from its own command result,
  // so the hub's pushed echo of it reads as unchanged. Seeds an unknown device.
  void noteVolume(const std::string& deviceId, float level, bool isMuted);

  const omote_DeviceState* find(const std::string& deviceId) const;

  // Merge-order walk for callers that must act on every known device, e.g.
  // naming what a hub-fired power-off is turning off. Null past the end.
  const omote_DeviceState* at(size_t index) const;
  size_t size() const;
  void clear();

private:
  int indexOf(const char* deviceId) const;

  omote_DeviceState entries[CAPACITY];
  size_t count = 0;
};

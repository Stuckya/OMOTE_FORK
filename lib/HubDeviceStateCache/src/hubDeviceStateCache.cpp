#include "hubDeviceStateCache.h"
#include <cstring>

bool HubDeviceStateCache::merge(const omote_DeviceState& state) {
  if (state.device_id[0] == '\0') {
    return false;
  }

  const int existing = indexOf(state.device_id);
  if (existing >= 0) {
    entries[existing] = state;
    return true;
  }

  if (count >= CAPACITY) {
    return false;
  }

  entries[count] = state;
  count++;
  return true;
}

bool HubDeviceStateCache::volumeChangedBy(const omote_DeviceState& incoming) const {
  if (!incoming.has_volume) {
    return false;
  }
  const omote_DeviceState* cached = find(incoming.device_id);
  if (cached == nullptr || !cached->has_volume) {
    return false;
  }
  return cached->volume.level != incoming.volume.level ||
         cached->volume.is_muted != incoming.volume.is_muted;
}

void HubDeviceStateCache::noteVolume(const std::string& deviceId, float level, bool isMuted) {
  int idx = indexOf(deviceId.c_str());
  if (idx < 0) {
    omote_DeviceState seed = omote_DeviceState_init_zero;
    strncpy(seed.device_id, deviceId.c_str(), sizeof(seed.device_id) - 1);
    if (!merge(seed)) {
      return;
    }
    idx = indexOf(seed.device_id);
  }
  entries[idx].has_volume = true;
  entries[idx].volume.level = level;
  entries[idx].volume.is_muted = isMuted;
}

const omote_DeviceState* HubDeviceStateCache::find(const std::string& deviceId) const {
  const int idx = indexOf(deviceId.c_str());
  return idx < 0 ? nullptr : &entries[idx];
}

size_t HubDeviceStateCache::size() const {
  return count;
}

void HubDeviceStateCache::clear() {
  count = 0;
}

int HubDeviceStateCache::indexOf(const char* deviceId) const {
  for (size_t i = 0; i < count; i++) {
    if (strncmp(entries[i].device_id, deviceId, sizeof(entries[i].device_id)) == 0) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

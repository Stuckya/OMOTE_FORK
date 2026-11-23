#include "hubDeviceStateListeners.h"
#include <map>

namespace Hub {

static std::map<std::string, DeviceStateListener>& listeners() {
  static std::map<std::string, DeviceStateListener> byDeviceId;
  return byDeviceId;
}

void registerDeviceStateListener(const std::string& deviceId, DeviceStateListener listener) {
  listeners()[deviceId] = listener;
}

void notifyDeviceStateListener(const omote_DeviceState& snapshot) {
  const auto registered = listeners().find(snapshot.device_id);
  if (registered == listeners().end()) {
    return;
  }
  registered->second(snapshot);
}

}  // namespace Hub

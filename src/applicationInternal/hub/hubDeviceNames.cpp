#include "hubDeviceNames.h"
#include <map>

namespace Hub {

static std::map<std::string, std::string>& deviceNames() {
  static std::map<std::string, std::string> names;
  return names;
}

void registerHubDeviceName(const std::string& deviceId, const std::string& name) {
  deviceNames()[deviceId] = name;
}

std::string hubDeviceDisplayName(const std::string& deviceId) {
  const auto named = deviceNames().find(deviceId);
  if (named != deviceNames().end()) {
    return named->second;
  }
  std::string fallback = deviceId;
  for (char& c : fallback) {
    if (c == '_') {
      c = ' ';
    }
  }
  return fallback;
}

}  // namespace Hub

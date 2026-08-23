#pragma once

#include <string>

namespace Hub {

// Human-readable name for a hub device id, declared by the device that owns the id.
void registerHubDeviceName(const std::string& deviceId, const std::string& name);

// Falls back to the id with underscores as spaces for devices that declared none.
std::string hubDeviceDisplayName(const std::string& deviceId);

// "Shield: reconnecting" for an attributed error; the bare message for a hub-level one.
std::string describeDeviceError(const std::string& deviceId, const std::string& message);

}  // namespace Hub

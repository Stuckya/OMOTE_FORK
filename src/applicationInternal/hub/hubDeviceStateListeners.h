#pragma once

#include "remote_messages.pb.h"
#include <string>

namespace Hub {

using DeviceStateListener = void (*)(const omote_DeviceState& snapshot);

// A device that draws its own hub state, such as a now-playing panel, declares a
// listener for its id. A device left out of the build declares none.
void registerDeviceStateListener(const std::string& deviceId, DeviceStateListener listener);

void notifyDeviceStateListener(const omote_DeviceState& snapshot);

}  // namespace Hub

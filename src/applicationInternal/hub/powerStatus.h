#pragma once

#include <string>
#include "remote_messages.pb.h"

// Tracks the power commands a scene sends as one session and drives the
// aggregate power banner from the hub's observed device state.
namespace Hub {
namespace PowerStatus {

// A command just left for the hub. `cached` is the remote's last snapshot of
// the device (or null); a device already in the target state is confirmed
// up front, since the hub publishes changes only and will never push it.
void commandSent(const std::string& deviceId, omote_OmoteCommand command,
                 const omote_DeviceState* cached);

void observed(const omote_DeviceState& state);

// Hub errors name no device; an open session shows the message in the banner
// instead of the generic error notification. Returns true when consumed.
bool noteError(const std::string& message);

// True from the first power command until the banner has had its final say.
bool isActive();

}  // namespace PowerStatus
}  // namespace Hub
